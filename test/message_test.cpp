#include <gtest/gtest.h>
#include "linknet/message.h"
#include <algorithm>

namespace linknet {
namespace test {

TEST(MessageTest, ChatMessageSerialization) {
  // Create a random PeerId
  PeerId sender_id;
  std::generate(sender_id.begin(), sender_id.end(), []() { return rand() % 256; });
  
  // Create a chat message
  std::string content = "Hello, world!";
  ChatMessage original(sender_id, content);
  
  // Serialize the message
  ByteBuffer serialized = original.Serialize();
  
  // Create a new message and deserialize
  ChatMessage deserialized(sender_id);
  bool result = deserialized.Deserialize(serialized);
  
  // Verify deserialization was successful
  EXPECT_TRUE(result);
  
  // Verify the content matches
  EXPECT_EQ(content, deserialized.GetContent());
  
  // Verify the sender ID matches
  EXPECT_EQ(sender_id, deserialized.GetSender());
  
  // Verify the message ID matches
  EXPECT_EQ(original.GetId(), deserialized.GetId());
  
  // Verify the timestamp matches
  EXPECT_EQ(original.GetTimestamp(), deserialized.GetTimestamp());
}

TEST(MessageTest, FileTransferRequestMessageSerialization) {
  // Create a random PeerId
  PeerId sender_id;
  std::generate(sender_id.begin(), sender_id.end(), []() { return rand() % 256; });
  
  // Create a file transfer request message
  std::string filename = "test.txt";
  uint64_t file_size = 12345;
  FileTransferRequestMessage original(sender_id, filename, file_size);
  
  // Serialize the message
  ByteBuffer serialized = original.Serialize();
  
  // Create a new message and deserialize
  FileTransferRequestMessage deserialized(sender_id);
  bool result = deserialized.Deserialize(serialized);
  
  // Verify deserialization was successful
  EXPECT_TRUE(result);
  
  // Verify the filename matches
  EXPECT_EQ(filename, deserialized.GetFilename());
  
  // Verify the file size matches
  EXPECT_EQ(file_size, deserialized.GetFileSize());
  
  // Verify the sender ID matches
  EXPECT_EQ(sender_id, deserialized.GetSender());
  
  // Verify the message ID matches
  EXPECT_EQ(original.GetId(), deserialized.GetId());
  
  // Verify the timestamp matches
  EXPECT_EQ(original.GetTimestamp(), deserialized.GetTimestamp());
}

TEST(MessageTest, MessageFactory) {
  // Create a random PeerId
  PeerId sender_id;
  std::generate(sender_id.begin(), sender_id.end(), []() { return rand() % 256; });
  
  // Create a chat message
  std::string content = "Hello, world!";
  ChatMessage original(sender_id, content);
  
  // Serialize the message
  ByteBuffer serialized = original.Serialize();
  
  // Use the MessageFactory to create a new message
  auto deserialized = MessageFactory::CreateFromBuffer(serialized);
  
  // Verify the message was created
  ASSERT_NE(nullptr, deserialized);
  
  // Verify the message type
  EXPECT_EQ(MessageType::CHAT_MESSAGE, deserialized->GetType());
  
  // Verify the sender ID matches
  EXPECT_EQ(sender_id, deserialized->GetSender());
  
  // Cast to ChatMessage and verify content
  auto chat_msg = dynamic_cast<ChatMessage*>(deserialized.get());
  ASSERT_NE(nullptr, chat_msg);
  EXPECT_EQ(content, chat_msg->GetContent());
}

}  // namespace test
}  // namespace linknet
