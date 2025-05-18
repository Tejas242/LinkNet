# API Reference

This API reference provides detailed information on LinkNet's public interfaces, allowing developers to extend the system with custom functionality or integrate it into other applications.

## Core Interfaces

LinkNet is designed with modularity in mind, exposing well-defined interfaces for various system components. This document covers the main interfaces and how to implement and use them.

## NetworkManager Interface

Located in `include/linknet/network.h`, this interface handles all network communication.

### Key Methods

```cpp
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
  
  // Get local listening port
  virtual uint16_t GetLocalPort() const = 0;
  
  // Set callbacks
  virtual void SetMessageCallback(MessageCallback callback) = 0;
  virtual void SetConnectionCallback(ConnectionCallback callback) = 0;
  virtual void SetErrorCallback(ErrorCallback callback) = 0;
};
```

### Usage Example

```cpp
class CustomApplication {
public:
  CustomApplication() {
    // Create network manager
    _network = std::make_shared<AsioNetworkManager>();
    
    // Set up callbacks
    _network->SetMessageCallback(
        [this](std::unique_ptr<Message> msg) { HandleMessage(std::move(msg)); });
    
    _network->SetConnectionCallback(
        [this](const PeerId& peer_id, ConnectionStatus status) { 
          HandleConnectionChange(peer_id, status); 
        });
    
    // Start listening
    _network->Start(8080);
  }
  
private:
  void HandleMessage(std::unique_ptr<Message> msg) {
    // Process received message
  }
  
  void HandleConnectionChange(const PeerId& peer_id, ConnectionStatus status) {
    // Handle peer connection/disconnection
  }
  
  std::shared_ptr<NetworkManager> _network;
};
```

### Custom Implementation

```cpp
class CustomNetworkManager : public NetworkManager {
public:
  bool Start(uint16_t port) override {
    // Initialize your custom network layer
    return true;
  }
  
  bool ConnectToPeer(const std::string& address, uint16_t port) override {
    // Implement connection logic
    return true;
  }
  
  bool SendMessage(const PeerId& peer_id, const Message& message) override {
    // Implement message sending
    ByteBuffer data = message.Serialize();
    // Send data to the peer
    return true;
  }
  
  // Implement other required methods...
};
```

## CryptoProvider Interface

Located in `include/linknet/crypto.h`, this interface handles cryptographic operations.

### Key Methods

```cpp
class CryptoProvider {
 public:
  virtual ~CryptoProvider() = default;
  
  // Generate a new random key
  virtual Key GenerateKey() const = 0;
  
  // Generate a keypair for asymmetric encryption
  virtual KeyPair GenerateKeyPair() const = 0;
  
  // Generate a keypair for digital signatures
  virtual SignatureKeyPair GenerateSignatureKeyPair() const = 0;
  
  // Generate a random nonce
  virtual Nonce GenerateNonce() const = 0;
  
  // Encrypt data
  virtual ByteBuffer Encrypt(const ByteBuffer& plaintext, 
                            const Key& key, 
                            const Nonce& nonce) const = 0;
  
  // Decrypt data
  virtual ByteBuffer Decrypt(const ByteBuffer& ciphertext, 
                            const Key& key, 
                            const Nonce& nonce) const = 0;
  
  // Sign data
  virtual ByteBuffer Sign(const ByteBuffer& data, 
                         const SignPrivateKey& private_key) const = 0;
  
  // Verify signature
  virtual bool Verify(const ByteBuffer& data, 
                     const ByteBuffer& signature,
                     const SignPublicKey& public_key) const = 0;
  
  // Hash data
  virtual ByteBuffer Hash(const ByteBuffer& data) const = 0;
  
  // Derive a key from a password
  virtual ByteBuffer DeriveKey(const ByteBuffer& password, 
                              const ByteBuffer& salt,
                              size_t iterations) const = 0;
};
```

### Usage Example

```cpp
void SecureDataHandler(std::shared_ptr<CryptoProvider> crypto) {
  // Generate encryption key
  auto key = crypto->GenerateKey();
  auto nonce = crypto->GenerateNonce();
  
  // Encrypt data
  ByteBuffer plaintext = {'S', 'e', 'c', 'r', 'e', 't', ' ', 'd', 'a', 't', 'a'};
  ByteBuffer ciphertext = crypto->Encrypt(plaintext, key, nonce);
  
  // Later decrypt it
  ByteBuffer decrypted = crypto->Decrypt(ciphertext, key, nonce);
}
```

## Message Class

Located in `include/linknet/message.h`, this is the base class for all messages.

### Key Methods

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
  // Helper method for serializing the common header
  ByteBuffer SerializeHeader() const;
  
  // Data members
  MessageType _type;
  PeerId _sender_id;
  MessageId _id;
  std::time_t _timestamp;
};
```

### Creating Custom Message Types

```cpp
class StatusMessage : public Message {
 public:
  StatusMessage(const PeerId& sender_id, 
                const std::string& status_text,
                bool is_online)
      : Message(MessageType::CUSTOM_STATUS, sender_id),
        _status_text(status_text),
        _is_online(is_online) {}
  
  ByteBuffer Serialize() const override {
    // Start with standard header
    ByteBuffer buffer = SerializeHeader();
    
    // Calculate required size
    size_t status_length = _status_text.size();
    buffer.resize(buffer.size() + 2 + status_length + 1);
    
    // Write status text length (2 bytes)
    uint16_t len = static_cast<uint16_t>(status_length);
    size_t offset = buffer.size() - 2 - status_length - 1;
    std::memcpy(buffer.data() + offset, &len, sizeof(len));
    offset += sizeof(len);
    
    // Write status text
    std::memcpy(buffer.data() + offset, _status_text.data(), status_length);
    offset += status_length;
    
    // Write online status (1 byte)
    buffer[offset] = _is_online ? 1 : 0;
    
    return buffer;
  }
  
  static std::unique_ptr<StatusMessage> Deserialize(
      const ByteBuffer& data, size_t offset) {
    // Read sender ID (already handled by base class deserialization)
    
    // Read status text length
    uint16_t status_length;
    std::memcpy(&status_length, data.data() + offset, sizeof(status_length));
    offset += sizeof(status_length);
    
    // Read status text
    std::string status_text(reinterpret_cast<const char*>(data.data() + offset), status_length);
    offset += status_length;
    
    // Read online status
    bool is_online = data[offset] != 0;
    
    // Create message object
    PeerId sender_id; // Extracted by the caller
    return std::make_unique<StatusMessage>(sender_id, status_text, is_online);
  }
  
  const std::string& GetStatusText() const { return _status_text; }
  bool IsOnline() const { return _is_online; }
  
 private:
  std::string _status_text;
  bool _is_online;
};
```

### Registering Custom Message Types

You need to extend the message factory to handle your custom message types:

```cpp
// Add this to your Message::Deserialize implementation
std::unique_ptr<Message> Message::Deserialize(const ByteBuffer& data) {
  if (data.size() < HEADER_SIZE) {
    return nullptr; // Invalid message
  }
  
  // Extract message type
  MessageType type = static_cast<MessageType>(data[0]);
  
  // Extract sender ID
  PeerId sender_id;
  std::memcpy(sender_id.data(), data.data() + 1, sender_id.size());
  
  // Create appropriate message type
  switch (type) {
    case MessageType::CHAT_MESSAGE:
      return ChatMessage::Deserialize(data, HEADER_SIZE);
    case MessageType::FILE_TRANSFER_REQUEST:
      return FileTransferRequestMessage::Deserialize(data, HEADER_SIZE);
    // Add your custom message type
    case MessageType::CUSTOM_STATUS:
      return StatusMessage::Deserialize(data, HEADER_SIZE);
    // ... other message types
    default:
      return nullptr; // Unknown message type
  }
}
```

## FileTransferManager Interface

Located in `include/linknet/file_transfer.h`, this interface handles file transfers.

### Key Methods

```cpp
class FileTransferManager {
 public:
  virtual ~FileTransferManager() = default;
  
  // Send a file to a peer
  virtual bool SendFile(const PeerId& peer_id, const std::string& file_path) = 0;
  
  // Cancel an ongoing file transfer
  virtual void CancelTransfer(const PeerId& peer_id, const std::string& file_path) = 0;
  
  // Get the status of ongoing transfers
  virtual std::vector<std::tuple<PeerId, std::string, FileTransferStatus, double>> 
      GetOngoingTransfers() const = 0;
  
  // Set callbacks
  virtual void SetProgressCallback(FileTransferProgressCallback callback) = 0;
  virtual void SetCompletedCallback(FileTransferCompletedCallback callback) = 0;
  virtual void SetRequestCallback(FileTransferRequestCallback callback) = 0;
};
```

### Usage Example

```cpp
void SetupFileTransfer(std::shared_ptr<NetworkManager> network) {
  auto file_transfer = FileTransferFactory::Create(network);
  
  // Set progress callback
  file_transfer->SetProgressCallback(
      [](const PeerId& peer_id, const std::string& filename, double progress) {
        std::cout << "Transfer progress: " << (progress * 100) << "%" << std::endl;
      });
  
  // Set completion callback
  file_transfer->SetCompletedCallback(
      [](const PeerId& peer_id, const std::string& filename, 
         bool success, const std::string& error) {
        if (success) {
          std::cout << "Transfer completed" << std::endl;
        } else {
          std::cout << "Transfer failed: " << error << std::endl;
        }
      });
  
  // Send a file
  PeerId peer_id; // Get this from somewhere
  file_transfer->SendFile(peer_id, "/path/to/file.txt");
}
```

## ChatManager Class

Located in `include/linknet/chat_manager.h`, this class manages chat functionality.

### Key Methods

```cpp
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
  
  // Set the local user ID and name
  void SetLocalUserId(const PeerId& user_id);
  void SetLocalUserName(const std::string& name);
  
  // Get the local user name
  const std::string& GetLocalUserName() const;
  
  // Set callback for incoming messages
  void SetChatMessageCallback(ChatMessageCallback callback);
};
```

### Usage Example

```cpp
void SetupChat(std::shared_ptr<NetworkManager> network) {
  auto chat = std::make_shared<ChatManager>(network);
  
  // Set local user information
  PeerId my_id; // Get from elsewhere
  chat->SetLocalUserId(my_id);
  chat->SetLocalUserName("Alice");
  
  // Set message callback
  chat->SetChatMessageCallback([](const ChatInfo& info) {
    std::cout << info.sender_name << ": " << info.content << std::endl;
  });
  
  // Send messages
  PeerId bob_id; // Get from elsewhere
  chat->SendMessage(bob_id, "Hello Bob!");
}
```

## PeerDiscovery Interface

Located in `include/linknet/discovery.h`, this interface handles peer discovery.

### Key Methods

```cpp
class PeerDiscovery {
 public:
  virtual ~PeerDiscovery() = default;
  
  // Start the discovery service
  virtual bool Start() = 0;
  
  // Stop the discovery service
  virtual void Stop() = 0;
  
  // Actively search for peers
  virtual void RefreshPeers() = 0;
  
  // Get discovered peers
  virtual std::vector<PeerInfo> GetDiscoveredPeers() const = 0;
  
  // Set callbacks
  virtual void SetPeerDiscoveredCallback(
      std::function<void(const PeerInfo&)> callback) = 0;
  
  // Announce presence on the network
  virtual void AnnouncePresence() = 0;
};
```

### Usage Example

```cpp
void SetupDiscovery(std::shared_ptr<NetworkManager> network) {
  auto discovery = std::make_shared<MulticastDiscovery>(network);
  
  // Set discovery callback
  discovery->SetPeerDiscoveredCallback([&network](const PeerInfo& peer) {
    std::cout << "Discovered peer: " << peer.name << std::endl;
    
    // Optionally connect automatically
    network->ConnectToPeer(peer.ip_address, peer.port);
  });
  
  // Start discovery
  discovery->Start();
  
  // Trigger a manual refresh
  discovery->RefreshPeers();
}
```

## Extending LinkNet

### Creating a Plugin

You can extend LinkNet with additional functionality by creating plugins:

```cpp
class CustomPlugin {
public:
  CustomPlugin(std::shared_ptr<NetworkManager> network,
               std::shared_ptr<ChatManager> chat,
               std::shared_ptr<FileTransferManager> file_transfer) 
      : _network(network), _chat(chat), _file_transfer(file_transfer) {
    
    // Register for events
    _network->SetMessageCallback(
        [this](std::unique_ptr<Message> msg) { 
          // Handle standard messages first
          if (_original_callback) {
            _original_callback(std::move(msg));
          }
          
          // Then handle custom functionality
          // ...
        });
  }
  
  void Initialize() {
    // Custom initialization
  }

private:
  std::shared_ptr<NetworkManager> _network;
  std::shared_ptr<ChatManager> _chat;
  std::shared_ptr<FileTransferManager> _file_transfer;
  MessageCallback _original_callback;
};
```

### Integration Example

Here's how to integrate all the components:

```cpp
int main() {
  // Create core components
  auto network = std::make_shared<AsioNetworkManager>();
  auto crypto = std::make_shared<SodiumCrypto>();
  auto chat = std::make_shared<ChatManager>(network);
  auto file_transfer = FileTransferFactory::Create(network);
  auto discovery = std::make_shared<MulticastDiscovery>(network);
  
  // Initialize components
  network->Start(8080);
  discovery->Start();
  
  // Set up event handling
  chat->SetChatMessageCallback([](const ChatInfo& info) {
    // Handle chat messages
  });
  
  file_transfer->SetProgressCallback([](const PeerId& peer_id, 
                                     const std::string& filename, 
                                     double progress) {
    // Update progress display
  });
  
  discovery->SetPeerDiscoveredCallback([&](const PeerInfo& peer) {
    // Handle discovered peers
  });
  
  // Create and initialize your extension
  auto custom_plugin = std::make_shared<CustomPlugin>(network, chat, file_transfer);
  custom_plugin->Initialize();
  
  // Run the application
  RunEventLoop();
  
  return 0;
}
```

## Common Patterns and Best Practices

### Error Handling

```cpp
try {
  // Attempt an operation
  bool success = network_manager->ConnectToPeer("192.168.1.5", 8080);
  if (!success) {
    // Handle failure
  }
} catch (const std::exception& e) {
  // Handle exceptions
  std::cerr << "Error: " << e.what() << std::endl;
}
```

### Resource Management

```cpp
class MyComponent {
public:
  MyComponent(std::shared_ptr<NetworkManager> network)
      : _network(network) {
    // Register callbacks
  }
  
  ~MyComponent() {
    // Clean up resources
  }

private:
  std::shared_ptr<NetworkManager> _network;
};
```

### Thread Safety

```cpp
class ThreadSafeComponent {
public:
  void AddItem(const std::string& item) {
    std::lock_guard<std::mutex> lock(_mutex);
    _items.push_back(item);
  }
  
  std::vector<std::string> GetItems() {
    std::lock_guard<std::mutex> lock(_mutex);
    return _items;
  }

private:
  std::vector<std::string> _items;
  std::mutex _mutex;
};
```

## Related Documentation

- [Network Layer](../components/network.md)
- [Cryptography](../components/cryptography.md)
- [Message System](../components/messaging.md)
- [Chat System](../components/chat.md)
- [File Transfer](../components/file_transfer.md)
