#ifndef LINKNET_MESSAGE_H_
#define LINKNET_MESSAGE_H_

#include "linknet/types.h"
#include <ctime>
#include <string>
#include <memory>

namespace linknet {

class Message {
 public:
  Message(MessageType type, const PeerId& sender);
  virtual ~Message() = default;

  MessageType GetType() const { return _type; }
  const PeerId& GetSender() const { return _sender; }
  const MessageId& GetId() const { return _id; }
  std::time_t GetTimestamp() const { return _timestamp; }
  
  // Serialize the message to a byte buffer
  virtual ByteBuffer Serialize() const = 0;
  
  // Deserialize data to populate this message
  virtual bool Deserialize(const ByteBuffer& data) = 0;

 protected:
  MessageType _type;
  PeerId _sender;
  MessageId _id;
  std::time_t _timestamp;

  // Generate a unique message ID
  static MessageId GenerateMessageId();
};

// Chat message for text communication
class ChatMessage : public Message {
 public:
  ChatMessage(const PeerId& sender, const std::string& content);
  ChatMessage(const PeerId& sender);  // For deserialization
  
  const std::string& GetContent() const { return _content; }
  void SetContent(const std::string& content) { _content = content; }
  
  ByteBuffer Serialize() const override;
  bool Deserialize(const ByteBuffer& data) override;
  
 private:
  std::string _content;
};

// File transfer request message
class FileTransferRequestMessage : public Message {
 public:
  FileTransferRequestMessage(const PeerId& sender, 
                            const std::string& filename, 
                            uint64_t file_size);
  FileTransferRequestMessage(const PeerId& sender);  // For deserialization
  
  const std::string& GetFilename() const { return _filename; }
  uint64_t GetFileSize() const { return _file_size; }
  
  ByteBuffer Serialize() const override;
  bool Deserialize(const ByteBuffer& data) override;
  
 private:
  std::string _filename;
  uint64_t _file_size;
};

// Message factory to create messages from raw data
class MessageFactory {
 public:
  static std::unique_ptr<Message> CreateFromBuffer(const ByteBuffer& data);
};

}  // namespace linknet

#endif  // LINKNET_MESSAGE_H_
