# LinkNet

A peer-to-peer chat and file-sharing system with secure encryption.

## Overview

LinkNet is a C++ application that enables direct peer-to-peer communication between users for both text messaging and file sharing. All communications are encrypted to ensure privacy and security.

### Features

- **Peer-to-peer Architecture**: Direct communication without centralized servers
- **Text Chat**: Real-time messaging between peers
- **File Sharing**: Transfer files directly between peers
- **End-to-End Encryption**: Secure communications using modern cryptography
- **Peer Discovery**: Automatic discovery of peers on local networks
- **NAT Traversal**: Techniques to establish connections through firewalls/NATs

## Requirements

- C++17 compatible compiler (GCC 8+, Clang 6+, MSVC 2019+)
- CMake 3.15+
- OpenSSL 1.1.1+
- Boost 1.70+ (for Asio)
- Protocol Buffers 3.0+
- libsodium 1.0.18+ (for additional cryptographic functions)

## Building

```bash
# Clone the repository
git clone https://github.com/yourusername/LinkNet.git
cd LinkNet

# Create build directory
mkdir build && cd build

# Configure and build
cmake ..
make

# Run tests
make test
```

## Usage

### Starting a LinkNet node

```bash
./linknet --port=8080
```

### Command-line Interface

Once running, the following commands are available:

- `/connect <ip:port>` - Connect to a specific peer
- `/chat <peer_id>` - Start a chat session with a peer
- `/send <peer_id> <file_path>` - Send a file to a peer
- `/peers` - List all connected peers
- `/help` - Display available commands
- `/exit` - Exit the application

## Project Structure

```
LinkNet/
├── include/            # Public header files
│   └── linknet/
├── src/                # Source files
│   ├── network/        # Networking components
│   ├── chat/           # Chat functionality
│   ├── file/           # File sharing functionality
│   ├── crypto/         # Encryption/decryption
│   ├── discovery/      # Peer discovery
│   ├── common/         # Shared utilities
│   └── ui/             # User interface
├── test/               # Test files
├── docs/               # Documentation
├── examples/           # Example code
└── third_party/        # Third-party dependencies
```

## Architecture

LinkNet follows a layered architecture:

1. **Transport Layer**: Handles low-level network communication
2. **Session Layer**: Manages peer connections and sessions
3. **Security Layer**: Provides encryption and authentication
4. **Application Layer**: Implements chat and file sharing
5. **User Interface Layer**: Provides interaction with the user

## Contributing

Contributions are welcome! Please feel free to submit a Pull Request.

## License

This project is licensed under the MIT License - see the LICENSE file for details.
