#include "linknet/message.h"
#include "linknet/logger.h"
#include <random>
#include <cstring>
#include <algorithm>

namespace linknet {

Message::Message(MessageType type, const PeerId& sender)
    : _type(type), _sender(sender), _id(GenerateMessageId()), _timestamp(std::time(nullptr)) {}

MessageId Message::GenerateMessageId() {
  static std::random_device rd;
  static std::mt19937 gen(rd());
  static std::uniform_int_distribution<uint8_t> dis(0, 255);

  MessageId id;
  for (auto& byte : id) {
    byte = dis(gen);
  }
  return id;
}

// ChatMessage implementation
ChatMessage::ChatMessage(const PeerId& sender, const std::string& content)
    : Message(MessageType::CHAT_MESSAGE, sender), _content(content) {}

ChatMessage::ChatMessage(const PeerId& sender)
    : Message(MessageType::CHAT_MESSAGE, sender) {}

ByteBuffer ChatMessage::Serialize() const {
  // Header format:
  // - 1 byte: MessageType
  // - 32 bytes: PeerId
  // - 16 bytes: MessageId
  // - 8 bytes: Timestamp
  // - 4 bytes: Content length
  // - N bytes: Content
  constexpr size_t HEADER_SIZE = 1 + 32 + 16 + 8 + 4;
  
  // Allocate buffer with room for header and content
  ByteBuffer buffer(HEADER_SIZE + _content.size());
  
  // Fill the header
  buffer[0] = static_cast<uint8_t>(_type);
  
  // Copy PeerId
  std::copy(_sender.begin(), _sender.end(), buffer.begin() + 1);
  
  // Copy MessageId
  std::copy(_id.begin(), _id.end(), buffer.begin() + 33);
  
  // Copy Timestamp (network byte order)
  uint64_t timestamp_network = htobe64(static_cast<uint64_t>(_timestamp));
  std::memcpy(buffer.data() + 49, &timestamp_network, 8);
  
  // Copy Content length (network byte order)
  uint32_t content_len_network = htobe32(static_cast<uint32_t>(_content.size()));
  std::memcpy(buffer.data() + 57, &content_len_network, 4);
  
  // Copy Content
  std::copy(_content.begin(), _content.end(), buffer.begin() + HEADER_SIZE);
  
  return buffer;
}

bool ChatMessage::Deserialize(const ByteBuffer& data) {
  constexpr size_t HEADER_SIZE = 1 + 32 + 16 + 8 + 4;
  
  if (data.size() < HEADER_SIZE) {
    LOG_ERROR("ChatMessage: Buffer too small to deserialize");
    return false;
  }
  
  // Verify message type
  MessageType type = static_cast<MessageType>(data[0]);
  if (type != MessageType::CHAT_MESSAGE) {
    LOG_ERROR("ChatMessage: Incorrect message type: ", static_cast<int>(type));
    return false;
  }
  
  // Copy PeerId
  std::copy(data.begin() + 1, data.begin() + 33, _sender.begin());
  
  // Copy MessageId
  std::copy(data.begin() + 33, data.begin() + 49, _id.begin());
  
  // Copy Timestamp
  uint64_t timestamp_network;
  std::memcpy(&timestamp_network, data.data() + 49, 8);
  _timestamp = static_cast<std::time_t>(be64toh(timestamp_network));
  
  // Get Content length
  uint32_t content_len_network;
  std::memcpy(&content_len_network, data.data() + 57, 4);
  uint32_t content_len = be32toh(content_len_network);
  
  // Verify buffer is large enough
  if (data.size() < HEADER_SIZE + content_len) {
    LOG_ERROR("ChatMessage: Buffer too small for content");
    return false;
  }
  
  // Copy Content
  _content.assign(data.begin() + HEADER_SIZE, data.begin() + HEADER_SIZE + content_len);
  
  return true;
}

// FileTransferRequestMessage implementation
FileTransferRequestMessage::FileTransferRequestMessage(
    const PeerId& sender, const std::string& filename, uint64_t file_size)
    : Message(MessageType::FILE_TRANSFER_REQUEST, sender),
      _filename(filename),
      _file_size(file_size) {}

FileTransferRequestMessage::FileTransferRequestMessage(const PeerId& sender)
    : Message(MessageType::FILE_TRANSFER_REQUEST, sender), _file_size(0) {}

ByteBuffer FileTransferRequestMessage::Serialize() const {
  // Header format:
  // - 1 byte: MessageType
  // - 32 bytes: PeerId
  // - 16 bytes: MessageId
  // - 8 bytes: Timestamp
  // - 8 bytes: File size
  // - 4 bytes: Filename length
  // - N bytes: Filename
  constexpr size_t HEADER_SIZE = 1 + 32 + 16 + 8 + 8 + 4;
  
  // Allocate buffer with room for header and filename
  ByteBuffer buffer(HEADER_SIZE + _filename.size());
  
  // Fill the header
  buffer[0] = static_cast<uint8_t>(_type);
  
  // Copy PeerId
  std::copy(_sender.begin(), _sender.end(), buffer.begin() + 1);
  
  // Copy MessageId
  std::copy(_id.begin(), _id.end(), buffer.begin() + 33);
  
  // Copy Timestamp (network byte order)
  uint64_t timestamp_network = htobe64(static_cast<uint64_t>(_timestamp));
  std::memcpy(buffer.data() + 49, &timestamp_network, 8);
  
  // Copy File size (network byte order)
  uint64_t file_size_network = htobe64(_file_size);
  std::memcpy(buffer.data() + 57, &file_size_network, 8);
  
  // Copy Filename length (network byte order)
  uint32_t filename_len_network = htobe32(static_cast<uint32_t>(_filename.size()));
  std::memcpy(buffer.data() + 65, &filename_len_network, 4);
  
  // Copy Filename
  std::copy(_filename.begin(), _filename.end(), buffer.begin() + HEADER_SIZE);
  
  return buffer;
}

bool FileTransferRequestMessage::Deserialize(const ByteBuffer& data) {
  constexpr size_t HEADER_SIZE = 1 + 32 + 16 + 8 + 8 + 4;
  
  if (data.size() < HEADER_SIZE) {
    LOG_ERROR("FileTransferRequestMessage: Buffer too small to deserialize");
    return false;
  }
  
  // Verify message type
  MessageType type = static_cast<MessageType>(data[0]);
  if (type != MessageType::FILE_TRANSFER_REQUEST) {
    LOG_ERROR("FileTransferRequestMessage: Incorrect message type: ", static_cast<int>(type));
    return false;
  }
  
  // Copy PeerId
  std::copy(data.begin() + 1, data.begin() + 33, _sender.begin());
  
  // Copy MessageId
  std::copy(data.begin() + 33, data.begin() + 49, _id.begin());
  
  // Copy Timestamp
  uint64_t timestamp_network;
  std::memcpy(&timestamp_network, data.data() + 49, 8);
  _timestamp = static_cast<std::time_t>(be64toh(timestamp_network));
  
  // Get File size
  uint64_t file_size_network;
  std::memcpy(&file_size_network, data.data() + 57, 8);
  _file_size = be64toh(file_size_network);
  
  // Get Filename length
  uint32_t filename_len_network;
  std::memcpy(&filename_len_network, data.data() + 65, 4);
  uint32_t filename_len = be32toh(filename_len_network);
  
  // Verify buffer is large enough
  if (data.size() < HEADER_SIZE + filename_len) {
    LOG_ERROR("FileTransferRequestMessage: Buffer too small for filename");
    return false;
  }
  
  // Copy Filename
  _filename.assign(data.begin() + HEADER_SIZE, data.begin() + HEADER_SIZE + filename_len);
  
  return true;
}

// MessageFactory implementation
std::unique_ptr<Message> MessageFactory::CreateFromBuffer(const ByteBuffer& data) {
  if (data.empty()) {
    LOG_ERROR("MessageFactory: Empty buffer");
    return nullptr;
  }
  
  // Extract the message type from the first byte
  MessageType type = static_cast<MessageType>(data[0]);
  
  // We need at least the PeerId (32 bytes) plus the type byte
  if (data.size() < 33) {
    LOG_ERROR("MessageFactory: Buffer too small for PeerId");
    return nullptr;
  }
  
  // Extract the sender PeerId
  PeerId sender;
  std::copy(data.begin() + 1, data.begin() + 33, sender.begin());
  
  std::unique_ptr<Message> message;
  
  switch (type) {
    case MessageType::CHAT_MESSAGE: {
      auto chat_msg = std::make_unique<ChatMessage>(sender);
      if (chat_msg->Deserialize(data)) {
        message = std::move(chat_msg);
      }
      break;
    }
    
    case MessageType::FILE_TRANSFER_REQUEST: {
      auto file_req_msg = std::make_unique<FileTransferRequestMessage>(sender);
      if (file_req_msg->Deserialize(data)) {
        message = std::move(file_req_msg);
      }
      break;
    }
    
    default:
      LOG_ERROR("MessageFactory: Unsupported message type: ", static_cast<int>(type));
      break;
  }
  
  return message;
}

}  // namespace linknet
