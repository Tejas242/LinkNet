# Getting Started with LinkNet

This guide will help you get LinkNet up and running on your system, from installation to basic usage.

## Requirements

### System Requirements

LinkNet is designed to work on modern systems with the following specifications:
- x86-64 or ARM64 processor
- 2 GB RAM minimum (4 GB recommended)
- 100 MB disk space
- Network connectivity

### Dependencies

LinkNet depends on several libraries and tools:

| Dependency | Version | Purpose |
|------------|---------|---------|
| C++ Compiler | GCC 8+, Clang 6+, MSVC 2019+ | C++17 compatible compiler |
| [CMake](https://cmake.org/) | 3.15+ | Build system |
| [Boost](https://www.boost.org/) | 1.70+ | Networking (Asio), synchronization primitives |
| [OpenSSL](https://www.openssl.org/) | 1.1.1+ | Cryptographic operations |
| [Protocol Buffers](https://developers.google.com/protocol-buffers) | 3.0+ | Data serialization |
| [libsodium](https://libsodium.org/) | 1.0.18+ | Advanced cryptographic functions |

## Installation

### Installing Dependencies

#### On Ubuntu/Debian
```bash
sudo apt update
sudo apt install build-essential cmake libboost-all-dev libssl-dev protobuf-compiler libprotobuf-dev libsodium-dev
```

#### On Fedora/RHEL/CentOS
```bash
sudo dnf install gcc-c++ cmake boost-devel openssl-devel protobuf-compiler protobuf-devel libsodium-devel
```

#### On macOS
```bash
brew install cmake boost openssl protobuf libsodium
```

### Building from Source

Follow these steps to build LinkNet from source:

```bash
# Clone the repository
git clone https://github.com/tejas242/LinkNet.git
cd LinkNet

# Create build directory
mkdir -p build && cd build

# Configure and build
cmake ..
make -j$(nproc)

# Run tests (optional but recommended)
make test
```

### Using Docker

For a containerized setup, you can use Docker:

```bash
# Build the Docker image
docker build -t linknet .

# Run LinkNet in a container
docker run -it --rm -p 8080:8080 linknet
```

## Basic Usage

### Starting LinkNet

To start LinkNet with default settings (listening on port 8080):

```bash
./build/bin/linknet
```

With custom settings:

```bash
./build/bin/linknet --port=9090 --no-auto-connect
```

### Command-Line Options

| Option | Description | Default |
|--------|-------------|---------|
| `--port=<number>` | Set the listening port | 8080 |
| `--no-auto-connect` | Disable automatic peer discovery | Auto-connect enabled |
| `--username=<name>` | Set your display name | System username |
| `--config=<file>` | Specify a config file | None |
| `--debug` | Enable debug logging | Disabled |

## Configuration

LinkNet can be configured using a configuration file. Create a file named `linknet.conf` in the same directory as the executable:

```
# LinkNet Configuration File

# Network settings
port = 8080
auto_connect = true

# User settings
username = YourName
download_directory = ~/Downloads/LinkNet

# Security settings
encryption_level = high  # Options: low, medium, high
```

## Next Steps

Once LinkNet is running:

1. Use the console interface to connect to peers
2. Send messages and files to connected peers
3. Explore advanced features like group chats and encrypted communications

For more detailed information, check out the following guides:
- [Architecture Overview](./architecture.md)
- [Network Configuration](./components/network.md)
- [Security Best Practices](./advanced/security_model.md)
