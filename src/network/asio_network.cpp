#include "linknet/network.h"
#include "linknet/message.h"
#include "linknet/logger.h"
#include <boost/asio.hpp>
#include <thread>
#include <mutex>
#include <unordered_map>
#include <queue>
#include <algorithm>
#include <random>

namespace std {
template <>
struct hash<std::array<unsigned char, 32>> {
  std::size_t operator()(const std::array<unsigned char, 32>& arr) const noexcept {
    std::size_t h = 0;
    for (auto b : arr) {
      h ^= std::hash<unsigned char>{}(b) + 0x9e3779b9 + (h << 6) + (h >> 2);
    }
    return h;
  }
};
}

namespace asio = boost::asio;

namespace linknet {

// A session represents a connected peer
class PeerSession : public std::enable_shared_from_this<PeerSession> {
 public:
  using tcp = boost::asio::ip::tcp;
  
  PeerSession(tcp::socket socket, PeerId peer_id, MessageCallback message_callback)
      : _socket(std::move(socket)), 
        _peer_id(peer_id),
        _message_callback(message_callback),
        _is_connected(true) {
    
    _peer_info.id = peer_id;
    _peer_info.ip_address = _socket.remote_endpoint().address().to_string();
    _peer_info.port = _socket.remote_endpoint().port();
    _peer_info.status = ConnectionStatus::CONNECTED;
  }
  
  void Start() {
    ReadMessage();
  }
  
  bool IsConnected() const {
    return _is_connected;
  }
  
  void Close() {
    if (_is_connected) {
      boost::system::error_code ec;
      _socket.close(ec);
      _is_connected = false;
      
      if (ec) {
        LOG_ERROR("Error closing socket: ", ec.message());
      }
    }
  }
  
  const PeerId& GetPeerId() const {
    return _peer_id;
  }
  
  const PeerInfo& GetPeerInfo() const {
    return _peer_info;
  }
  
  bool SendMessage(const Message& message) {
    if (!_is_connected) {
      return false;
    }
    
    try {
      ByteBuffer data = message.Serialize();
      
      // Write the size of the message first (4 bytes)
      uint32_t size_network = htobe32(static_cast<uint32_t>(data.size()));
      asio::write(_socket, asio::buffer(&size_network, 4));
      
      // Then write the message
      asio::write(_socket, asio::buffer(data));
      
      return true;
    } catch (const std::exception& e) {
      LOG_ERROR("Error sending message: ", e.what());
      Close();
      return false;
    }
  }
  
 private:
  void ReadMessage() {
    auto self = shared_from_this();
    
    // Read the size of the message first (4 bytes)
    asio::async_read(
        _socket,
        asio::buffer(&_read_size_buffer, 4),
        [this, self](const boost::system::error_code& ec, std::size_t length) {
          if (!ec && length == 4) {
            uint32_t size_network;
            std::memcpy(&size_network, _read_size_buffer, 4);
            uint32_t message_size = be32toh(size_network);
            
            _read_buffer.resize(message_size);
            
            // Then read the message
            asio::async_read(
                _socket,
                asio::buffer(_read_buffer),
                [this, self](const boost::system::error_code& ec, std::size_t /*length*/) {
                  if (!ec) {
                    try {
                      auto message = MessageFactory::CreateFromBuffer(_read_buffer);
                      if (message) {
                        _message_callback(std::move(message));
                      }
                      
                      // Continue reading
                      ReadMessage();
                    } catch (const std::exception& e) {
                      LOG_ERROR("Error processing message: ", e.what());
                      Close();
                    }
                  } else {
                    LOG_ERROR("Error reading message: ", ec.message());
                    Close();
                  }
                });
          } else {
            LOG_ERROR("Error reading message size: ", ec.message());
            Close();
          }
        });
  }
  
  tcp::socket _socket;
  PeerId _peer_id;
  PeerInfo _peer_info;
  MessageCallback _message_callback;
  std::atomic<bool> _is_connected;
  
  uint8_t _read_size_buffer[4];
  ByteBuffer _read_buffer;
};

// Implementation of NetworkManager using ASIO
class AsioNetworkManager : public NetworkManager {
 public:
  AsioNetworkManager()
      : _io_context(), 
        _work_guard(_io_context.get_executor()),
        _acceptor(_io_context),
        _is_running(false) {}
  
  ~AsioNetworkManager() override {
    Stop();
  }
  
  bool Start(uint16_t port) override {
    if (_is_running) {
      LOG_WARNING("Network manager already running");
      return false;
    }
    
    try {
      asio::ip::tcp::endpoint endpoint(asio::ip::tcp::v4(), port);
      _acceptor.open(endpoint.protocol());
      _acceptor.set_option(asio::ip::tcp::acceptor::reuse_address(true));
      _acceptor.bind(endpoint);
      _acceptor.listen();
      
      LOG_INFO("Network manager started on port ", port);
      
      StartAccept();
      
      // Start the ASIO io_context in a separate thread
      _io_thread = std::thread([this]() {
        try {
          _io_context.run();
        } catch (const std::exception& e) {
          LOG_ERROR("ASIO io_context error: ", e.what());
        }
      });
      
      _is_running = true;
      return true;
    } catch (const std::exception& e) {
      LOG_ERROR("Error starting network manager: ", e.what());
      return false;
    }
  }
  
  void Stop() override {
    if (!_is_running) {
      return;
    }
    
    _is_running = false;
    
    {
      std::lock_guard<std::mutex> lock(_peers_mutex);
      for (auto& [peer_id, session] : _peer_sessions) {
        session->Close();
      }
      _peer_sessions.clear();
    }
    
    _acceptor.close();
    _io_context.stop();
    
    if (_io_thread.joinable()) {
      _io_thread.join();
    }
    
    LOG_INFO("Network manager stopped");
  }
  
  bool ConnectToPeer(const std::string& address, uint16_t port) override {
    if (!_is_running) {
      LOG_ERROR("Network manager not running");
      return false;
    }
    
    try {
      asio::ip::tcp::resolver resolver(_io_context);
      auto endpoints = resolver.resolve(address, std::to_string(port));
      
      auto socket = std::make_shared<asio::ip::tcp::socket>(_io_context);
      asio::async_connect(
          *socket, endpoints,
          [this, address, port, socket](
              const boost::system::error_code& ec, const asio::ip::tcp::endpoint& /*endpoint*/) {
            if (!ec) {
              LOG_INFO("Connected to peer at ", address, ":", port);
              
              // Generate a stable peer ID for this connection
              PeerId peer_id;
              std::random_device rd;
              std::generate(peer_id.begin(), peer_id.end(), std::ref(rd));
              
              auto session = std::make_shared<PeerSession>(std::move(*socket), peer_id, _message_callback);
              
              {
                std::lock_guard<std::mutex> lock(_peers_mutex);
                _peer_sessions[peer_id] = session;
              }
              
              session->Start();
              
              // Send a connection notification message to the peer
              ConnectionMessage conn_msg(peer_id, ConnectionStatus::CONNECTED);
              session->SendMessage(conn_msg);
              
              // Notify connection callback
              if (_connection_callback) {
                _connection_callback(peer_id, ConnectionStatus::CONNECTED);
              }
            } else {
              LOG_ERROR("Failed to connect to peer at ", address, ":", port, ": ", ec.message());
              
              if (_error_callback) {
                _error_callback("Failed to connect to peer at " + address + ":" + 
                               std::to_string(port) + ": " + ec.message());
              }
            }
          });
      
      return true;
    } catch (const std::exception& e) {
      LOG_ERROR("Error connecting to peer: ", e.what());
      return false;
    }
  }
  
  void DisconnectFromPeer(const PeerId& peer_id) override {
    std::lock_guard<std::mutex> lock(_peers_mutex);
    auto it = _peer_sessions.find(peer_id);
    
    if (it != _peer_sessions.end()) {
      it->second->Close();
      _peer_sessions.erase(it);
      
      LOG_INFO("Disconnected from peer");
      
      // Notify disconnection
      if (_connection_callback) {
        _connection_callback(peer_id, ConnectionStatus::DISCONNECTED);
      }
    }
  }
  
  bool SendMessage(const PeerId& peer_id, const Message& message) override {
    std::shared_ptr<PeerSession> session;
    
    {
      std::lock_guard<std::mutex> lock(_peers_mutex);
      auto it = _peer_sessions.find(peer_id);
      
      if (it == _peer_sessions.end() || !it->second->IsConnected()) {
        return false;
      }
      
      session = it->second;
    }
    
    return session->SendMessage(message);
  }
  
  void BroadcastMessage(const Message& message) override {
    std::vector<std::shared_ptr<PeerSession>> sessions;
    
    {
      std::lock_guard<std::mutex> lock(_peers_mutex);
      for (auto& [peer_id, session] : _peer_sessions) {
        if (session->IsConnected()) {
          sessions.push_back(session);
        }
      }
    }
    
    for (auto& session : sessions) {
      session->SendMessage(message);
    }
  }
  
  std::vector<PeerInfo> GetConnectedPeers() const override {
    std::vector<PeerInfo> peers;
    
    {
      std::lock_guard<std::mutex> lock(_peers_mutex);
      peers.reserve(_peer_sessions.size());
      
      for (const auto& [peer_id, session] : _peer_sessions) {
        if (session->IsConnected()) {
          peers.push_back(session->GetPeerInfo());
        }
      }
    }
    
    return peers;
  }
  
  void SetMessageCallback(MessageCallback callback) override {
    _message_callback = std::move(callback);
  }
  
  void SetConnectionCallback(ConnectionCallback callback) override {
    _connection_callback = std::move(callback);
  }
  
  void SetErrorCallback(ErrorCallback callback) override {
    _error_callback = std::move(callback);
  }
  
  uint16_t GetLocalPort() const override {
    try {
      return _acceptor.local_endpoint().port();
    } catch (const std::exception& e) {
      LOG_ERROR("Error getting local port: ", e.what());
      return 0;
    }
  }
  
 private:
  void StartAccept() {
    _acceptor.async_accept(
        [this](const boost::system::error_code& ec, asio::ip::tcp::socket socket) {
          if (!ec) {
            LOG_INFO("Accepted connection from ", 
                    socket.remote_endpoint().address().to_string(), ":",
                    socket.remote_endpoint().port());
            
            // Generate a stable peer ID for this connection
            PeerId peer_id;
            std::random_device rd;
            std::generate(peer_id.begin(), peer_id.end(), std::ref(rd));
            
            auto session = std::make_shared<PeerSession>(std::move(socket), peer_id, _message_callback);
            
            {
              std::lock_guard<std::mutex> lock(_peers_mutex);
              _peer_sessions[peer_id] = session;
            }
            
            session->Start();
            
            // Send a connection notification message to the peer
            ConnectionMessage conn_msg(peer_id, ConnectionStatus::CONNECTED);
            session->SendMessage(conn_msg);
            
            // Notify connection callback
            if (_connection_callback) {
              _connection_callback(peer_id, ConnectionStatus::CONNECTED);
            }
          } else {
            LOG_ERROR("Error accepting connection: ", ec.message());
          }
          
          // Continue accepting connections
          if (_is_running) {
            StartAccept();
          }
        });
  }
  
  asio::io_context _io_context;
  asio::executor_work_guard<asio::io_context::executor_type> _work_guard;
  asio::ip::tcp::acceptor _acceptor;
  std::thread _io_thread;
  std::atomic<bool> _is_running;
  
  mutable std::mutex _peers_mutex;
  std::unordered_map<PeerId, std::shared_ptr<PeerSession>, 
                      std::hash<PeerId>> _peer_sessions;
  
  MessageCallback _message_callback;
  ConnectionCallback _connection_callback;
  ErrorCallback _error_callback;
};

std::unique_ptr<NetworkManager> NetworkFactory::Create() {
  return std::make_unique<AsioNetworkManager>();
}

}  // namespace linknet
