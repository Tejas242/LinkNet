# Messaging System

The Messaging System in LinkNet provides a structured framework for communication between peers, defining message formats, serialization, and routing.

## Overview

The message system is the backbone of LinkNet's communication protocol. It defines how data is structured, serialized, and transmitted between peers. All higher-level services like chat and file transfer build on top of this foundational layer.

## Key Components

### Message Base Class

Located in `include/linknet/message.h`, this abstract class defines the common interface for all messages:

```cpp
class Message {
 public:
  Message(MessageType type, const PeerId& sender_id);
  virtual ~Message() = default;
  
  // Serialize message to binary format
  virtual ByteBuffer Serialize() const = 0;
  
  // Deserialize from binary format (static factory method)
  static std::unique_ptr<Message> Deserialize(const ByteBuffer& data);
  
  // Getters
  MessageType GetType() const;
  const PeerId& GetSenderId() const;
  const MessageId& GetId() const;
  std::time_t GetTimestamp() const;
  
 protected:
  MessageType _type;
  PeerId _sender_id;
  MessageId _id;
  std::time_t _timestamp;
};
```

### Message Types

LinkNet defines several message types for different purposes:

```cpp
enum class MessageType : uint8_t {
  CHAT_MESSAGE = 0,
  FILE_TRANSFER_REQUEST = 1,
  FILE_TRANSFER_RESPONSE = 2,
  FILE_CHUNK = 3,
  FILE_TRANSFER_COMPLETE = 4,
  PEER_DISCOVERY = 5,
  PING = 6,
  PONG = 7,
  CONNECTION_NOTIFICATION = 8,
};
```

### Concrete Message Classes

Each message type has a specialized implementation:

- **ChatMessage**: Contains text messages between users
- **FileTransferRequestMessage**: Initiates a file transfer
- **FileTransferResponseMessage**: Accepts or rejects a file transfer
- **FileChunkMessage**: Contains a chunk of file data
- **FileTransferCompleteMessage**: Signals the completion of a file transfer
- **PeerDiscoveryMessage**: Used for peer discovery
- **PingMessage**: Used to check connectivity
- **PongMessage**: Response to a ping message
- **ConnectionNotificationMessage**: Notifies about connection changes

## Message Format

Each message follows a common binary format:

### Header

The standard message header contains:

```
+-----+----------+-----------+----------+
| Byte|  Field   |  Length   |  Notes   |
+-----+----------+-----------+----------+
|  0  | Type     | 1 byte    | Message type enum value
|  1  | Sender   | 32 bytes  | Sender's PeerId 
| 33  | Message  | 16 bytes  | Unique message ID
|     | ID       |           |
| 49  | Timestamp| 8 bytes   | Unix timestamp (64-bit)
+-----+----------+-----------+----------+
```

### Body

The message body is specific to each message type:

#### Chat Message

```
+-----+----------+-----------+----------+
| Byte|  Field   |  Length   |  Notes   |
+-----+----------+-----------+----------+
| 57  | Name Len | 2 bytes   | Length of sender name
| 59  | Name     | N bytes   | Sender's display name
| 59+N| Msg Len  | 4 bytes   | Length of message content
| 63+N| Content  | M bytes   | Message content
+-----+----------+-----------+----------+
```

#### File Transfer Request

```
+-----+----------+-----------+----------+
| Byte|  Field   |  Length   |  Notes   |
+-----+----------+-----------+----------+
| 57  | ID Len   | 2 bytes   | Length of file ID
| 59  | File ID  | N bytes   | Unique file identifier
| 59+N| Name Len | 2 bytes   | Length of filename
| 61+N| Filename | M bytes   | Name of the file
| 61+N+M| Size   | 8 bytes   | File size in bytes
| 69+N+M| Hash   | 32 bytes  | File hash for verification
+-----+----------+-----------+----------+
```

## Message Flow

### Message Creation

1. Application code creates a message object of the appropriate type
2. Message is populated with relevant data
3. Message ID and timestamp are automatically generated
4. Sender ID is set based on local peer identity

### Serialization

1. Message object calls its `Serialize()` method
2. Header fields are written to a byte buffer
3. Type-specific fields are appended to the buffer
4. Binary data is optionally encrypted

### Transmission

1. Serialized message is passed to the network layer
2. Network layer adds any transport headers and sends the data
3. (Optional) Acknowledgment is awaited

### Reception

1. Network layer receives data and strips transport headers
2. Binary data is passed to `Message::Deserialize()`
3. Header is parsed to determine message type
4. Appropriate concrete message subclass is instantiated
5. Message-specific deserialization completes the object
6. Message is passed to the appropriate handler

## Features

### Message Identification

Each message has a unique identifier:
- Generated using a cryptographically secure random generator
- 128-bit (16-byte) value
- Used for message tracking, acknowledgment, and deduplication

### Timestamps

Messages include creation timestamps:
- Unix timestamp format (seconds since epoch)
- Used for message ordering and history
- Helps detect clock skew between peers

### Error Detection

The messaging system includes several error detection mechanisms:
- **Length Checking**: Verifies message sizes match expected values
- **Type Validation**: Ensures message types are valid
- **Integrity Checking**: Verifies message hasn't been corrupted

## Implementation Details

### Thread Safety

Message objects are designed for thread safety:
- Immutable after creation
- Thread-safe deserialization
- No shared state between messages

### Memory Management

Efficient memory handling:
- Uses move semantics for large data transfers
- Avoids unnecessary copies during serialization
- Preallocates buffers of appropriate size

### Versioning

The message format supports versioning:
- Type field can include version information
- Backward compatibility with older message formats
- Forward compatibility with newer messages (when possible)

## Advanced Usage

### Custom Message Types

You can extend the messaging system with custom types:

```cpp
class CustomMessage : public Message {
 public:
  CustomMessage(const PeerId& sender_id, const std::string& custom_data)
      : Message(MessageType::CUSTOM_TYPE, sender_id),
        _custom_data(custom_data) {}
  
  ByteBuffer Serialize() const override {
    // Serialize standard header
    ByteBuffer buffer = SerializeHeader();
    
    // Add custom fields
    uint16_t data_length = _custom_data.size();
    buffer.resize(buffer.size() + 2 + data_length);
    
    // Write data length (2 bytes)
    std::memcpy(buffer.data() + buffer.size() - 2 - data_length, 
                &data_length, sizeof(data_length));
    
    // Write data content
    std::memcpy(buffer.data() + buffer.size() - data_length,
                _custom_data.data(), data_length);
    
    return buffer;
  }
  
  static std::unique_ptr<CustomMessage> Deserialize(
      const ByteBuffer& data, size_t offset) {
    // Implementation of deserialization logic
  }
  
  const std::string& GetCustomData() const { return _custom_data; }
  
 private:
  std::string _custom_data;
};
```

## Performance Considerations

Several optimizations ensure efficient message handling:
- **Pooled Allocations**: Reuses message objects and buffers
- **Zero-copy Design**: Minimizes unnecessary copying
- **Lazy Parsing**: Only deserializes fields when needed
- **Batching**: Combines small messages when appropriate

## Code Examples

### Creating and Sending a Chat Message

```cpp
// Create a chat message
auto chat_msg = std::make_unique<ChatMessage>(
    local_peer_id,    // Sender ID
    "Alice",          // Sender name
    "Hello, world!"   // Message content
);

// Send the message
network_manager->SendMessage(recipient_id, *chat_msg);
```

### Handling Received Messages

```cpp
// Set up message callback
network_manager->SetMessageCallback([](std::unique_ptr<Message> msg) {
  switch (msg->GetType()) {
    case MessageType::CHAT_MESSAGE: {
      auto chat_msg = static_cast<ChatMessage*>(msg.get());
      std::cout << "Chat from " << chat_msg->GetSenderName() 
                << ": " << chat_msg->GetContent() << std::endl;
      break;
    }
    case MessageType::FILE_TRANSFER_REQUEST: {
      auto request_msg = static_cast<FileTransferRequestMessage*>(msg.get());
      std::cout << "File transfer request: " 
                << request_msg->GetFileName() << " (" 
                << request_msg->GetFileSize() << " bytes)" << std::endl;
      // Process file transfer request
      break;
    }
    // Handle other message types...
    default:
      std::cout << "Unknown message type: " 
                << static_cast<int>(msg->GetType()) << std::endl;
  }
});
```

### Broadcasting a System Notification

```cpp
// Create a notification message
auto notification = std::make_unique<ConnectionNotificationMessage>(
    local_peer_id,
    ConnectionStatus::CONNECTED,
    "User has joined the network"
);

// Broadcast to all connected peers
network_manager->BroadcastMessage(*notification);
```

## Related Documentation

- [Network Layer](network.md) - How messages are transmitted between peers
- [Chat System](chat.md) - How chat uses the message system
- [File Transfer](file_transfer.md) - How file transfers use the message system
- [Cryptography](cryptography.md) - How messages are secured
