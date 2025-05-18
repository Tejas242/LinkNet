# LinkNet Architecture

This document provides an overview of the LinkNet architecture, explaining the system's design principles, components, and how they interact.

## Design Philosophy

LinkNet is built on several core principles:

1. **Security by Design**: End-to-end encryption for all communications with no compromises
2. **Decentralization**: No central servers or authorities, true peer-to-peer architecture
3. **Modularity**: Clean separation of concerns with well-defined interfaces
4. **Performance**: Optimized for low latency and efficient resource usage
5. **Extensibility**: Easy to add new features while maintaining backward compatibility

## High-Level Architecture

LinkNet follows a layered architecture pattern:

```
┌─────────────────────────────────────────────────────────────┐
│                     Application Layer                        │
│                     (Console UI, CLI)                        │
└───────────────────────────────┬─────────────────────────────┘
                                │
┌───────────────────────────────▼─────────────────────────────┐
│                      Service Layer                           │
│            (Chat Manager, File Transfer Manager)             │
└───────────────────────────────┬─────────────────────────────┘
                                │
┌───────────────────────────────▼─────────────────────────────┐
│                     Message Layer                            │
│                (Message Format, Serialization)               │
└───────────────────────────────┬─────────────────────────────┘
                                │
┌───────────────┬───────────────▼───────────────┬─────────────┐
│  Network Layer │      Security Layer          │ Discovery    │
│  (Asio-based)  │      (Crypto Provider)       │ (mDNS)       │
└───────────────┴───────────────────────────────┴─────────────┘
```

## Core Components

### Network Layer (NetworkManager)

Handles all communication between peers using Boost.Asio for asynchronous I/O operations. It manages:
- TCP/UDP socket connections
- Connection establishment and maintenance
- Message transmission and reception
- NAT traversal

### Cryptography (CryptoProvider)

Provides all cryptographic operations required for secure communication:
- Symmetric encryption/decryption (AES-256, ChaCha20-Poly1305)
- Asymmetric encryption (X25519)
- Digital signatures (Ed25519)
- Key generation and management
- Secure random number generation

### Message System

Defines a common format for all messages exchanged in the system:
- Binary message format with type-length-value (TLV) encoding
- Support for different message types (chat, file transfer, etc.)
- Message serialization/deserialization
- Message ID generation and tracking

### Chat Manager

Manages text-based communication between peers:
- Message composition and sending
- Message reception and display
- Chat history storage and retrieval
- User status management

### File Transfer Manager

Handles file sharing capabilities:
- Breaking files into chunks for efficient transfer
- Progress tracking and reporting
- Transfer resumption after interruption
- File integrity verification

### Peer Discovery

Enables automatic discovery of peers on local and wide networks:
- Local network discovery using multicast DNS (mDNS)
- Peer information exchange
- Connectivity status tracking

### Console UI

Provides a text-based user interface for interacting with the system:
- Command processing
- Status display
- Chat visualization
- File transfer progress reporting

## Data Flow

A typical message flow in LinkNet:

1. User inputs a message in the UI
2. ChatManager formats the message and adds metadata
3. Message is serialized into a binary format
4. CryptoProvider encrypts the message
5. NetworkManager sends the encrypted message to the recipient
6. Recipient's NetworkManager receives the encrypted message
7. Recipient's CryptoProvider decrypts the message
8. Message is deserialized and passed to the ChatManager
9. ChatManager processes the message and displays it in the UI

## Project Structure

```
├── include/linknet/  # Public API headers
│   ├── network.h     # Network interface
│   ├── crypto.h      # Cryptography interface
│   ├── message.h     # Message definitions
│   ├── chat_manager.h # Chat system
│   ├── file_transfer.h # File transfer system
│   ├── discovery.h   # Peer discovery
│   ├── console_ui.h  # User interface
│   ├── logger.h      # Logging system
│   └── types.h       # Common type definitions
│
├── src/             # Implementation files
│   ├── main.cpp     # Application entry point
│   ├── chat/        # Chat implementation
│   ├── common/      # Shared utilities
│   ├── crypto/      # Cryptography implementation
│   ├── discovery/   # Peer discovery implementation
│   ├── file/        # File transfer implementation
│   ├── network/     # Network implementation
│   └── ui/          # User interface implementation
│
├── test/            # Test files
│   └── ...
│
└── third_party/     # External dependencies
```

## Dependency Graph

```
ConsoleUI ──► ChatManager ──┬──► NetworkManager ──► Socket I/O
                            │
                            └──► Message ◄─┬── CryptoProvider
                                           │
FileTransferManager ───────────────────────┘
         │
         ▼
     FileSystem
```

## Threading Model

LinkNet uses a multi-threaded architecture:
- Main thread for UI and user interaction
- Network thread for asynchronous I/O operations
- Worker thread pool for CPU-intensive tasks (encryption, file processing)
- Background thread for peer discovery

Thread safety is ensured through careful synchronization using mutexes, atomic variables, and lock-free data structures where possible.
