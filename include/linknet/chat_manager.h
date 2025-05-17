#ifndef LINKNET_CHAT_MANAGER_H_
#define LINKNET_CHAT_MANAGER_H_

#include "linknet/types.h"
#include <string>
#include <map>
#include <functional>
#include <memory>
#include <vector>
#include <mutex>
#include <ctime>

namespace linknet {

// Forward declarations
class NetworkManager;
class Message;

// Chat message structure
struct ChatInfo {
  PeerId sender_id;
  std::string sender_name;
  std::string content;
  std::time_t timestamp;
};

// Callback types
using ChatMessageCallback = std::function<void(const ChatInfo&)>;

class ChatManager {
 public:
  explicit ChatManager(std::shared_ptr<NetworkManager> network_manager);
  ~ChatManager();

  // Send a chat message to a specific peer
  bool SendMessage(const PeerId& peer_id, const std::string& message);
  
  // Broadcast a chat message to all connected peers
  void BroadcastMessage(const std::string& message);
  
  // Get chat history with a specific peer
  std::vector<ChatInfo> GetChatHistory(const PeerId& peer_id, size_t max_messages = 50);
  
  // Get all chat history
  std::vector<ChatInfo> GetAllChatHistory(size_t max_messages = 100);
  
  // Set the local user ID
  void SetLocalUserId(const PeerId& user_id);
  
  // Set the local user name
  void SetLocalUserName(const std::string& name);
  
  // Set callback for incoming chat messages
  void SetMessageCallback(ChatMessageCallback callback);
  
  // Handle incoming messages
  void HandleMessage(std::unique_ptr<Message> message);
  
  // Set the next handler in the chain for non-chat messages
  void SetNextHandler(std::function<void(std::unique_ptr<Message>)> handler);

 private:
  // Next handler in the chain for messages we don't handle
  std::function<void(std::unique_ptr<Message>)> _next_handler;
  
  std::shared_ptr<NetworkManager> _network_manager;
  PeerId _local_user_id;
  std::string _local_user_name;
  ChatMessageCallback _message_callback;
  
  // Message history (peer_id -> vector of messages)
  std::map<PeerId, std::vector<ChatInfo>> _chat_history;
  std::mutex _history_mutex;
};

}  // namespace linknet

#endif  // LINKNET_CHAT_MANAGER_H_
