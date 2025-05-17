#ifndef LINKNET_TYPES_H_
#define LINKNET_TYPES_H_

#include <cstdint>
#include <string>
#include <vector>
#include <array>

namespace linknet {

// ID types
using PeerId = std::array<uint8_t, 32>;
using MessageId = std::array<uint8_t, 16>;

// Buffer type for binary data
using ByteBuffer = std::vector<uint8_t>;

// Message type
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

// Connection status
enum class ConnectionStatus : uint8_t {
  DISCONNECTED = 0,
  CONNECTING = 1,
  CONNECTED = 2,
  ERROR = 3,
};

// File transfer status
enum class FileTransferStatus : uint8_t {
  PENDING = 0,
  IN_PROGRESS = 1,
  COMPLETED = 2,
  FAILED = 3,
  REJECTED = 4,
};

// Peer information
struct PeerInfo {
  PeerId id;
  std::string name;
  std::string ip_address;
  uint16_t port;
  ConnectionStatus status;
};

}  // namespace linknet

#endif  // LINKNET_TYPES_H_
