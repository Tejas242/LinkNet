#ifndef LINKNET_NETWORK_H_
#define LINKNET_NETWORK_H_

#include "linknet/types.h"
#include <string>
#include <functional>
#include <memory>
#include <atomic>
#include <future>

namespace linknet {

// Forward declarations
class Message;

// Callback types
using MessageCallback = std::function<void(std::unique_ptr<Message>)>;
using ConnectionCallback = std::function<void(const PeerId&, ConnectionStatus)>;
using ErrorCallback = std::function<void(const std::string&)>;

// Interface for network operations
class NetworkManager {
 public:
  virtual ~NetworkManager() = default;

  // Start the network manager on the specified port
  virtual bool Start(uint16_t port) = 0;
  
  // Stop the network manager
  virtual void Stop() = 0;
  
  // Connect to a peer
  virtual bool ConnectToPeer(const std::string& address, uint16_t port) = 0;
  
  // Disconnect from a peer
  virtual void DisconnectFromPeer(const PeerId& peer_id) = 0;
  
  // Send a message to a peer
  virtual bool SendMessage(const PeerId& peer_id, const Message& message) = 0;
  
  // Broadcast a message to all connected peers
  virtual void BroadcastMessage(const Message& message) = 0;
  
  // Get connected peers
  virtual std::vector<PeerInfo> GetConnectedPeers() const = 0;
  
  // Set callbacks
  virtual void SetMessageCallback(MessageCallback callback) = 0;
  virtual void SetConnectionCallback(ConnectionCallback callback) = 0;
  virtual void SetErrorCallback(ErrorCallback callback) = 0;
};

// Factory to create a concrete implementation
class NetworkFactory {
 public:
  static std::unique_ptr<NetworkManager> Create();
};

}  // namespace linknet

#endif  // LINKNET_NETWORK_H_
