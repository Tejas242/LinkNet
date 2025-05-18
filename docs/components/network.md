# Network Layer

The network layer in LinkNet handles all communication between peers, providing a robust and efficient foundation for the entire system.

## Overview

The NetworkManager interface defines the contract for network operations in LinkNet. It abstracts the underlying network implementation, allowing for different transport protocols while providing a consistent API for the rest of the system.

## Key Components

### NetworkManager Interface

Located in `include/linknet/network.h`, this interface defines the core network operations:

```cpp
class NetworkManager {
 public:
  virtual ~NetworkManager() = default;
  virtual bool Start(uint16_t port) = 0;
  virtual void Stop() = 0;
  virtual bool ConnectToPeer(const std::string& address, uint16_t port) = 0;
  virtual void DisconnectFromPeer(const PeerId& peer_id) = 0;
  virtual bool SendMessage(const PeerId& peer_id, const Message& message) = 0;
  virtual void BroadcastMessage(const Message& message) = 0;
  virtual std::vector<PeerInfo> GetConnectedPeers() const = 0;
  virtual uint16_t GetLocalPort() const = 0;
  virtual void SetMessageCallback(MessageCallback callback) = 0;
  virtual void SetConnectionCallback(ConnectionCallback callback) = 0;
  virtual void SetErrorCallback(ErrorCallback callback) = 0;
};
```

### Asio-based Implementation

The primary implementation uses Boost.Asio for asynchronous I/O operations:

- **PeerSession**: Represents a connection to a remote peer
- **AsioNetworkManager**: Concrete implementation of the NetworkManager interface

## Connection Establishment

The connection process involves several steps:

1. **Listener Setup**: When NetworkManager starts, it creates a TCP listener on the specified port
2. **Connection Initiation**: 
   - Outbound: `ConnectToPeer()` establishes a TCP connection to the remote peer
   - Inbound: The listener accepts incoming connections
3. **Handshake**: Peers exchange identity information and establish encryption
4. **Session Creation**: A PeerSession object is created to manage the connection

```
┌────────────┐                       ┌────────────┐
│   Peer A   │                       │   Peer B   │
└─────┬──────┘                       └──────┬─────┘
      │                                     │
      │ TCP Connect                         │
      │────────────────────────────────────▶│
      │                                     │
      │ Handshake (Identity + Crypto Setup) │
      │◀───────────────────────────────────▶│
      │                                     │
      │ Connection Established              │
      │◀───────────────────────────────────▶│
```

## Message Handling

Messages are processed asynchronously:

1. **Serialization**: Messages are converted to binary format
2. **Encryption**: Message content is encrypted (if secure connection)
3. **Transmission**: Data is sent over TCP/UDP
4. **Reception**: Recipient receives data asynchronously
5. **Decryption**: Message is decrypted (if secure connection)
6. **Deserialization**: Binary data is converted back to Message object
7. **Callback**: The message callback is invoked with the received message

## NAT Traversal

LinkNet implements several techniques to establish connections through NAT:

1. **UDP Hole Punching**: Synchronizes connection attempts to create NAT mappings
2. **STUN Protocol Support**: Determines the public IP address and port mapping
3. **Connection Reversal**: Allows the peer with the more permissive NAT to initiate

## Error Handling

The network layer handles various failure scenarios:

- **Connection Failures**: Attempts reconnection with exponential backoff
- **Message Delivery Failures**: Reports errors to upper layers
- **Unexpected Disconnections**: Notifies services and cleans up resources

## Performance Considerations

Several optimizations ensure efficient network operations:

- **Connection Pooling**: Reuses connections to avoid setup/teardown overhead
- **Buffer Management**: Uses pre-allocated buffers to minimize memory allocations
- **Asynchronous Operations**: Non-blocking I/O for high concurrency
- **Batching**: Combines small messages when appropriate to reduce overhead

## Advanced Usage

### Custom Transport Implementation

You can implement custom transport mechanisms by creating a new class that implements the NetworkManager interface. This allows LinkNet to work with various transport mechanisms (WebRTC, custom protocols, etc.).

Example skeleton:

```cpp
class CustomNetworkManager : public NetworkManager {
 public:
  bool Start(uint16_t port) override {
    // Initialize your custom transport
  }
  
  bool ConnectToPeer(const std::string& address, uint16_t port) override {
    // Implement custom connection logic
  }
  
  // Implement other required methods...
};
```

### Configuration Options

The network layer can be configured using the following settings:

- **Connection Timeout**: Max time to wait for connection establishment
- **Keep-alive Interval**: Frequency of keep-alive messages
- **Buffer Sizes**: Size of send/receive buffers
- **Max Connections**: Maximum number of simultaneous connections

## Code Examples

### Initializing the Network Layer

```cpp
auto network_manager = std::make_shared<AsioNetworkManager>();
network_manager->Start(8080);

// Set up callbacks
network_manager->SetMessageCallback([](std::unique_ptr<Message> msg) {
  std::cout << "Message received: " << msg->GetType() << std::endl;
});

network_manager->SetConnectionCallback([](const PeerId& peer_id, ConnectionStatus status) {
  std::cout << "Connection status changed: " << static_cast<int>(status) << std::endl;
});
```

### Connecting to a Peer

```cpp
// Connect to a peer
bool success = network_manager->ConnectToPeer("192.168.1.5", 8080);
if (success) {
  std::cout << "Connection initiated" << std::endl;
} else {
  std::cerr << "Failed to initiate connection" << std::endl;
}
```

### Sending a Message

```cpp
// Create a message
auto message = std::make_unique<TextMessage>(local_peer_id, "Hello, world!");

// Send it to a specific peer
network_manager->SendMessage(remote_peer_id, *message);

// Or broadcast to all connected peers
network_manager->BroadcastMessage(*message);
```

## Related Documentation

- [Message System](messaging.md) - Details on the message format and handling
- [NAT Traversal](../advanced/nat_traversal.md) - In-depth guide to NAT traversal techniques
- [Security Model](../advanced/security_model.md) - How security integrates with networking
