#ifndef LINKNET_DISCOVERY_H_
#define LINKNET_DISCOVERY_H_

#include "linknet/types.h"
#include <string>
#include <functional>
#include <memory>
#include <thread>
#include <atomic>
#include <mutex>
#include <map>
#include <chrono>

namespace linknet {

// Forward declarations
class NetworkManager;

// Callback type for discovered peers
using PeerDiscoveredCallback = std::function<void(const std::string&, uint16_t)>;

class PeerDiscovery {
 public:
  explicit PeerDiscovery(std::shared_ptr<NetworkManager> network_manager);
  ~PeerDiscovery();

  // Start peer discovery
  bool Start(uint16_t port);
  
  // Stop peer discovery
  void Stop();
  
  // Set callback for discovered peers
  void SetDiscoveredCallback(PeerDiscoveredCallback callback);
  
  // Is discovery running
  bool IsRunning() const { return _running; }

 private:
  // Broadcast thread function
  void BroadcastThreadFunc();
  
  // Listen thread function
  void ListenThreadFunc();
  
  // Handle multicast message
  void HandleMessage(const std::string& sender_ip, const ByteBuffer& data);

  std::shared_ptr<NetworkManager> _network_manager;
  PeerDiscoveredCallback _discovered_callback;
  
  std::atomic<bool> _running;
  uint16_t _port;
  
  int _broadcast_socket;
  int _listen_socket;
  std::thread _broadcast_thread;
  std::thread _listen_thread;
  
  std::mutex _discovered_peers_mutex;
  std::map<std::string, std::chrono::steady_clock::time_point> _discovered_peers;
};

}  // namespace linknet

#endif  // LINKNET_DISCOVERY_H_
