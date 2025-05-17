#include "linknet/chat_manager.h"
#include "linknet/network.h"
#include "linknet/message.h"
#include "linknet/logger.h"
#include <algorithm>
#include <random>
#include <chrono>

namespace linknet {

ChatManager::ChatManager(std::shared_ptr<NetworkManager> network_manager)
    : _network_manager(network_manager) {
  
  // Generate a random user ID
  std::random_device rd;
  std::generate(_local_user_id.begin(), _local_user_id.end(), std::ref(rd));
  
  // Set default user name
  _local_user_name = "User-" + std::to_string(std::chrono::system_clock::now()
      .time_since_epoch().count() % 10000);
  
  // Register for network messages
  _network_manager->SetMessageCallback(
      [this](std::unique_ptr<Message> message) {
        HandleMessage(std::move(message));
      });
}

ChatManager::~ChatManager() = default;

bool ChatManager::SendMessage(const PeerId& peer_id, const std::string& message) {
  ChatMessage chat_msg(_local_user_id, message);
  
  bool result = _network_manager->SendMessage(peer_id, chat_msg);
  
  if (result) {
    // Store message in history
    ChatInfo info;
    info.sender_id = _local_user_id;
    info.sender_name = _local_user_name;
    info.content = message;
    info.timestamp = chat_msg.GetTimestamp();
    
    std::lock_guard<std::mutex> lock(_history_mutex);
    _chat_history[peer_id].push_back(info);
  }
  
  return result;
}

void ChatManager::BroadcastMessage(const std::string& message) {
  ChatMessage chat_msg(_local_user_id, message);
  
  _network_manager->BroadcastMessage(chat_msg);
  
  // Store message in history for all connected peers
  ChatInfo info;
  info.sender_id = _local_user_id;
  info.sender_name = _local_user_name;
  info.content = message;
  info.timestamp = chat_msg.GetTimestamp();
  
  std::lock_guard<std::mutex> lock(_history_mutex);
  
  auto peers = _network_manager->GetConnectedPeers();
  for (const auto& peer : peers) {
    _chat_history[peer.id].push_back(info);
  }
}

std::vector<ChatInfo> ChatManager::GetChatHistory(const PeerId& peer_id, size_t max_messages) {
  std::lock_guard<std::mutex> lock(_history_mutex);
  
  auto it = _chat_history.find(peer_id);
  if (it == _chat_history.end()) {
    return {};
  }
  
  const auto& history = it->second;
  
  if (history.size() <= max_messages) {
    return history;
  }
  
  // Return the most recent messages
  return std::vector<ChatInfo>(
      history.end() - max_messages,
      history.end());
}

std::vector<ChatInfo> ChatManager::GetAllChatHistory(size_t max_messages) {
  std::vector<ChatInfo> all_history;
  
  {
    std::lock_guard<std::mutex> lock(_history_mutex);
    
    // Count total messages
    size_t total_messages = 0;
    for (const auto& [peer_id, history] : _chat_history) {
      total_messages += history.size();
    }
    
    all_history.reserve(std::min(total_messages, max_messages));
    
    // Collect all messages
    for (const auto& [peer_id, history] : _chat_history) {
      all_history.insert(all_history.end(), history.begin(), history.end());
    }
  }
  
  // Sort by timestamp
  std::sort(all_history.begin(), all_history.end(),
      [](const ChatInfo& a, const ChatInfo& b) {
        return a.timestamp < b.timestamp;
      });
  
  // Limit to max_messages
  if (all_history.size() > max_messages) {
    all_history.erase(all_history.begin(), all_history.end() - max_messages);
  }
  
  return all_history;
}

void ChatManager::SetLocalUserId(const PeerId& user_id) {
  _local_user_id = user_id;
}

void ChatManager::SetLocalUserName(const std::string& name) {
  _local_user_name = name;
}

void ChatManager::SetMessageCallback(ChatMessageCallback callback) {
  _message_callback = std::move(callback);
}

void ChatManager::HandleMessage(std::unique_ptr<Message> message) {
  if (message->GetType() != MessageType::CHAT_MESSAGE) {
    return;
  }
  
  auto& chat_msg = static_cast<ChatMessage&>(*message);
  const PeerId& sender_id = chat_msg.GetSender();
  
  // Store message in history
  ChatInfo info;
  info.sender_id = sender_id;
  info.sender_name = "Unknown";  // We don't know the name yet
  info.content = chat_msg.GetContent();
  info.timestamp = chat_msg.GetTimestamp();
  
  {
    std::lock_guard<std::mutex> lock(_history_mutex);
    _chat_history[sender_id].push_back(info);
  }
  
  // Notify callback
  if (_message_callback) {
    _message_callback(info);
  }
}

}  // namespace linknet
