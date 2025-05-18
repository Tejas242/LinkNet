# Chat System

The Chat System in LinkNet provides a structured way to handle text-based communication between peers, making it easy to send, receive, and manage messages.

## Overview

The `ChatManager` class manages all aspects of text messaging functionality, including sending messages, receiving messages, and maintaining chat history. It abstracts the network layer complexity, providing a clean API for chat operations.

## Key Components

### ChatManager Class

Located in `include/linknet/chat_manager.h`, this class provides the core functionality:

```cpp
class ChatManager {
 public:
  explicit ChatManager(std::shared_ptr<NetworkManager> network_manager);
  ~ChatManager();

  bool SendMessage(const PeerId& peer_id, const std::string& message);
  void BroadcastMessage(const std::string& message);
  std::vector<ChatInfo> GetChatHistory(const PeerId& peer_id, size_t max_messages = 50);
  std::vector<ChatInfo> GetAllChatHistory(size_t max_messages = 100);
  void SetLocalUserId(const PeerId& user_id);
  void SetLocalUserName(const std::string& name);
  const std::string& GetLocalUserName() const;
  void SetChatMessageCallback(ChatMessageCallback callback);
};
```

### ChatInfo Structure

Represents a chat message with all relevant metadata:

```cpp
struct ChatInfo {
  PeerId sender_id;
  std::string sender_name;
  std::string content;
  std::time_t timestamp;
};
```

### Chat Message Callback

Allows the application to react when new messages are received:

```cpp
using ChatMessageCallback = std::function<void(const ChatInfo&)>;
```

## Message Flow

The chat system processes messages in the following way:

### Sending Messages

1. **Message Creation**: User creates a chat message
2. **Formatting**: ChatManager adds metadata (sender, timestamp)
3. **Message Object**: Creates a Message object with type CHAT_MESSAGE
4. **Network Dispatch**: Sends the message via the NetworkManager
5. **Delivery Status**: (Optional) Tracks message delivery status

### Receiving Messages

1. **Network Reception**: NetworkManager receives a message
2. **Type Checking**: Identifies message as CHAT_MESSAGE
3. **Callback Invocation**: ChatManager is notified of new message
4. **Message Processing**: Extracts message content and metadata
5. **History Storage**: Stores message in chat history
6. **UI Notification**: Invokes callback to update UI

## Chat History Management

The ChatManager maintains a database of message history:

- **Per-Peer History**: Separate chat history for each peer
- **Global History**: Combined view of all chats
- **Persistence**: (Optional) Messages can be stored to disk
- **Retrieval**: Efficient query methods with pagination support

## Security Features

Messages in the chat system benefit from LinkNet's security infrastructure:

- **End-to-End Encryption**: All messages are encrypted
- **Message Authentication**: Prevents tampering and forgery
- **Forward Secrecy**: Past messages remain secure even if keys are compromised
- **Metadata Protection**: Limits exposure of communication metadata

## Implementation Details

### Thread Safety

The ChatManager is designed to be thread-safe:

- Accesses to chat history are synchronized
- Thread-safe callbacks for message reception
- Atomic operations where appropriate

### Error Handling

The chat system handles various error conditions:

- **Message Delivery Failures**: Reports and logs delivery issues
- **Invalid Messages**: Detects and rejects malformed messages
- **Resource Exhaustion**: Manages memory usage for large chat histories

## Advanced Features

### Message Types

Beyond basic text messages, the chat system supports:

- **System Messages**: Notifications about peer status
- **Formatted Messages**: Basic text formatting (bold, italic, etc.)
- **Read Receipts**: Acknowledgment when messages are read
- **Typing Indicators**: Show when peers are composing messages

### Group Chats

For multi-peer conversations:

- **Group Creation**: Create chat groups with multiple peers
- **Member Management**: Add/remove peers from groups
- **Message Distribution**: Send messages to all group members

## Performance Considerations

Several optimizations ensure efficient chat operations:

- **Message Batching**: Combines small messages when appropriate
- **Lazy Loading**: Only loads necessary portions of chat history
- **Efficient Storage**: Compact representation of messages

## Code Examples

### Initializing the Chat Manager

```cpp
// Assuming network_manager is already initialized
auto chat_manager = std::make_shared<ChatManager>(network_manager);

// Set local user information
chat_manager->SetLocalUserId(my_peer_id);
chat_manager->SetLocalUserName("Alice");

// Set up message reception callback
chat_manager->SetChatMessageCallback([](const ChatInfo& message) {
  std::cout << message.sender_name << ": " << message.content << std::endl;
});
```

### Sending Messages

```cpp
// Send a direct message
PeerId bob_id = /* ... */;
chat_manager->SendMessage(bob_id, "Hello Bob, how are you?");

// Broadcast to all connected peers
chat_manager->BroadcastMessage("Hello everyone!");
```

### Retrieving Chat History

```cpp
// Get chat history with a specific peer
PeerId bob_id = /* ... */;
auto chat_with_bob = chat_manager->GetChatHistory(bob_id, 20); // Last 20 messages

// Display messages
for (const auto& msg : chat_with_bob) {
  std::cout << msg.sender_name << " (" 
            << std::put_time(std::localtime(&msg.timestamp), "%H:%M:%S") 
            << "): " << msg.content << std::endl;
}

// Get all chat history
auto all_chats = chat_manager->GetAllChatHistory(100); // Last 100 messages from all chats
```

## Related Documentation

- [Message System](messaging.md) - Details on the message format used by the chat system
- [Network Layer](network.md) - How messages are transmitted between peers
- [Console UI](ui.md) - User interface integration with the chat system
- [Security Model](../advanced/security_model.md) - Security aspects of chat communications
