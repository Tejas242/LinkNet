#include "linknet/discovery.h"
#include "linknet/network.h"
#include "linknet/logger.h"

#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <cstring>
#include <iostream>

namespace linknet {

// Discovery constants
constexpr const char* MULTICAST_GROUP = "239.255.0.1";
constexpr uint16_t MULTICAST_PORT = 30001;
constexpr const char* DISCOVERY_PREFIX = "LINKNET_DISCOVERY";
constexpr int DISCOVERY_INTERVAL_SEC = 5;
constexpr int PEER_TIMEOUT_SEC = 30;

PeerDiscovery::PeerDiscovery(std::shared_ptr<NetworkManager> network_manager)
    : _network_manager(network_manager),
      _running(false),
      _port(0),
      _broadcast_socket(-1),
      _listen_socket(-1) {}

PeerDiscovery::~PeerDiscovery() {
  Stop();
}

bool PeerDiscovery::Start(uint16_t port) {
  if (_running) {
    LOG_WARNING("Peer discovery already running");
    return false;
  }
  
  _port = port;
  
  try {
    // Create broadcast socket
    _broadcast_socket = socket(AF_INET, SOCK_DGRAM, 0);
    if (_broadcast_socket < 0) {
      LOG_ERROR("Failed to create broadcast socket: ", strerror(errno));
      return false;
    }
    
    // Set socket options for broadcast
    int broadcast_enable = 1;
    if (setsockopt(_broadcast_socket, SOL_SOCKET, SO_BROADCAST, 
                  &broadcast_enable, sizeof(broadcast_enable)) < 0) {
      LOG_ERROR("Failed to set broadcast socket options: ", strerror(errno));
      close(_broadcast_socket);
      return false;
    }
    
    // Create listen socket
    _listen_socket = socket(AF_INET, SOCK_DGRAM, 0);
    if (_listen_socket < 0) {
      LOG_ERROR("Failed to create listen socket: ", strerror(errno));
      close(_broadcast_socket);
      return false;
    }
    
    // Set socket options for reuse
    int reuse = 1;
    if (setsockopt(_listen_socket, SOL_SOCKET, SO_REUSEADDR, 
                  &reuse, sizeof(reuse)) < 0) {
      LOG_ERROR("Failed to set listen socket options: ", strerror(errno));
      close(_broadcast_socket);
      close(_listen_socket);
      return false;
    }
    
    // Bind listen socket to the multicast port
    struct sockaddr_in addr;
    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = htonl(INADDR_ANY);
    addr.sin_port = htons(MULTICAST_PORT);
    
    if (bind(_listen_socket, (struct sockaddr*)&addr, sizeof(addr)) < 0) {
      LOG_ERROR("Failed to bind listen socket: ", strerror(errno));
      close(_broadcast_socket);
      close(_listen_socket);
      return false;
    }
    
    // Join multicast group
    struct ip_mreq mreq;
    mreq.imr_multiaddr.s_addr = inet_addr(MULTICAST_GROUP);
    mreq.imr_interface.s_addr = htonl(INADDR_ANY);
    
    if (setsockopt(_listen_socket, IPPROTO_IP, IP_ADD_MEMBERSHIP, 
                  &mreq, sizeof(mreq)) < 0) {
      LOG_ERROR("Failed to join multicast group: ", strerror(errno));
      close(_broadcast_socket);
      close(_listen_socket);
      return false;
    }
    
    _running = true;
    
    // Start broadcast thread
    _broadcast_thread = std::thread(&PeerDiscovery::BroadcastThreadFunc, this);
    
    // Start listen thread
    _listen_thread = std::thread(&PeerDiscovery::ListenThreadFunc, this);
    
    LOG_INFO("Peer discovery started on port ", _port);
    
    return true;
  } catch (const std::exception& e) {
    LOG_ERROR("Failed to start peer discovery: ", e.what());
    
    if (_broadcast_socket >= 0) {
      close(_broadcast_socket);
      _broadcast_socket = -1;
    }
    
    if (_listen_socket >= 0) {
      close(_listen_socket);
      _listen_socket = -1;
    }
    
    return false;
  }
}

void PeerDiscovery::Stop() {
  if (!_running.exchange(false)) {
    return;
  }
  
  // Close sockets to interrupt any blocking calls
  if (_broadcast_socket >= 0) {
    close(_broadcast_socket);
    _broadcast_socket = -1;
  }
  
  if (_listen_socket >= 0) {
    close(_listen_socket);
    _listen_socket = -1;
  }
  
  // Wait for threads to finish
  if (_broadcast_thread.joinable()) {
    _broadcast_thread.join();
  }
  
  if (_listen_thread.joinable()) {
    _listen_thread.join();
  }
  
  LOG_INFO("Peer discovery stopped");
}

void PeerDiscovery::SetDiscoveredCallback(PeerDiscoveredCallback callback) {
  _discovered_callback = std::move(callback);
}

void PeerDiscovery::BroadcastThreadFunc() {
  struct sockaddr_in addr;
  memset(&addr, 0, sizeof(addr));
  addr.sin_family = AF_INET;
  addr.sin_addr.s_addr = inet_addr(MULTICAST_GROUP);
  addr.sin_port = htons(MULTICAST_PORT);
  
  while (_running) {
    try {
      // Create discovery message
      std::string message = std::string(DISCOVERY_PREFIX) + ":" + std::to_string(_port);
      
      // Send the message
      sendto(_broadcast_socket, message.c_str(), message.size(), 0,
            (struct sockaddr*)&addr, sizeof(addr));
      
      // Sleep for the broadcast interval
      for (int i = 0; i < DISCOVERY_INTERVAL_SEC && _running; ++i) {
        std::this_thread::sleep_for(std::chrono::seconds(1));
      }
      
      // Clean up expired peers
      auto now = std::chrono::steady_clock::now();
      
      {
        std::lock_guard<std::mutex> lock(_discovered_peers_mutex);
        
        for (auto it = _discovered_peers.begin(); it != _discovered_peers.end();) {
          if (now - it->second > std::chrono::seconds(PEER_TIMEOUT_SEC)) {
            it = _discovered_peers.erase(it);
          } else {
            ++it;
          }
        }
      }
    } catch (const std::exception& e) {
      LOG_ERROR("Error in broadcast thread: ", e.what());
      std::this_thread::sleep_for(std::chrono::seconds(1));
    }
  }
}

void PeerDiscovery::ListenThreadFunc() {
  char buffer[256];
  struct sockaddr_in addr;
  socklen_t addr_len = sizeof(addr);
  
  while (_running) {
    try {
      // Receive message
      ssize_t received = recvfrom(_listen_socket, buffer, sizeof(buffer) - 1, 0,
                                 (struct sockaddr*)&addr, &addr_len);
      
      if (received < 0) {
        if (_running) {
          LOG_ERROR("Failed to receive discovery message: ", strerror(errno));
        }
        continue;
      }
      
      // Null-terminate the buffer
      buffer[received] = '\0';
      
      // Get sender IP address
      std::string sender_ip = inet_ntoa(addr.sin_addr);
      
      // Parse message
      std::string message(buffer);
      
      if (message.substr(0, strlen(DISCOVERY_PREFIX)) == DISCOVERY_PREFIX) {
        // Extract port number
        size_t colon_pos = message.find(':');
        if (colon_pos != std::string::npos && colon_pos < message.size() - 1) {
          std::string port_str = message.substr(colon_pos + 1);
          
          try {
            uint16_t port = std::stoi(port_str);
            
            // Skip self - check both localhost and port equality
            if (port == _port || (sender_ip == "127.0.0.1" && port == _port)) {
              // Same port means it's likely the same instance on this machine
              LOG_DEBUG("Skipping own discovery message from ", sender_ip, ":", port);
              continue;
            }
            
            // Update peer discovery time
            std::string peer_key = sender_ip + ":" + port_str;
            bool is_new = false;
            
            {
              std::lock_guard<std::mutex> lock(_discovered_peers_mutex);
              auto it = _discovered_peers.find(peer_key);
              
              if (it == _discovered_peers.end()) {
                _discovered_peers[peer_key] = std::chrono::steady_clock::now();
                is_new = true;
              } else {
                it->second = std::chrono::steady_clock::now();
              }
            }
            
            // Notify callback for new peers
            if (is_new && _discovered_callback) {
              _discovered_callback(sender_ip, port);
            }
          } catch (const std::exception& e) {
            LOG_ERROR("Failed to parse discovery port: ", e.what());
          }
        }
      }
    } catch (const std::exception& e) {
      LOG_ERROR("Error in listen thread: ", e.what());
      std::this_thread::sleep_for(std::chrono::seconds(1));
    }
  }
}

}  // namespace linknet
