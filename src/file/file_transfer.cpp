#include "linknet/file_transfer.h"
#include "linknet/network.h"
#include "linknet/message.h"
#include "linknet/logger.h"
#include <fstream>
#include <mutex>
#include <unordered_map>
#include <map>
#include <filesystem>
#include <algorithm>
#include <chrono>
#include <cstring>

namespace linknet {

// Message types for file transfer
class FileChunkMessage : public Message {
 public:
  FileChunkMessage(const PeerId& sender, 
                  const std::string& file_id,
                  uint32_t chunk_index,
                  const ByteBuffer& data)
      : Message(MessageType::FILE_CHUNK, sender),
        _file_id(file_id),
        _chunk_index(chunk_index),
        _data(data) {}
  
  FileChunkMessage(const PeerId& sender)
      : Message(MessageType::FILE_CHUNK, sender), _chunk_index(0) {}
  
  const std::string& GetFileId() const { return _file_id; }
  uint32_t GetChunkIndex() const { return _chunk_index; }
  const ByteBuffer& GetData() const { return _data; }
  
  ByteBuffer Serialize() const override {
    // Header format:
    // - 1 byte: MessageType
    // - 32 bytes: PeerId
    // - 16 bytes: MessageId
    // - 8 bytes: Timestamp
    // - 4 bytes: File ID length
    // - N bytes: File ID
    // - 4 bytes: Chunk index
    // - 4 bytes: Data length
    // - M bytes: Data
    constexpr size_t HEADER_SIZE_WITHOUT_FILE_ID = 1 + 32 + 16 + 8 + 4 + 4 + 4;
    
    // Allocate buffer with room for header, file_id, and data
    ByteBuffer buffer(HEADER_SIZE_WITHOUT_FILE_ID + _file_id.size() + _data.size());
    
    // Fill the header
    buffer[0] = static_cast<uint8_t>(_type);
    
    // Copy PeerId
    std::copy(_sender.begin(), _sender.end(), buffer.begin() + 1);
    
    // Copy MessageId
    std::copy(_id.begin(), _id.end(), buffer.begin() + 33);
    
    // Copy Timestamp (network byte order)
    uint64_t timestamp_network = htobe64(static_cast<uint64_t>(_timestamp));
    std::memcpy(buffer.data() + 49, &timestamp_network, 8);
    
    // Copy File ID length (network byte order)
    uint32_t file_id_len_network = htobe32(static_cast<uint32_t>(_file_id.size()));
    std::memcpy(buffer.data() + 57, &file_id_len_network, 4);
    
    // Copy File ID
    std::copy(_file_id.begin(), _file_id.end(), buffer.begin() + 61);
    
    // Copy Chunk index (network byte order)
    uint32_t chunk_index_network = htobe32(_chunk_index);
    std::memcpy(buffer.data() + 61 + _file_id.size(), &chunk_index_network, 4);
    
    // Copy Data length (network byte order)
    uint32_t data_len_network = htobe32(static_cast<uint32_t>(_data.size()));
    std::memcpy(buffer.data() + 65 + _file_id.size(), &data_len_network, 4);
    
    // Copy Data
    std::copy(_data.begin(), _data.end(), buffer.begin() + 69 + _file_id.size());
    
    return buffer;
  }
  
  bool Deserialize(const ByteBuffer& data) override {
    constexpr size_t MIN_HEADER_SIZE = 1 + 32 + 16 + 8 + 4;  // Without file_id length
    
    if (data.size() < MIN_HEADER_SIZE) {
      LOG_ERROR("FileChunkMessage: Buffer too small to deserialize");
      return false;
    }
    
    // Verify message type
    MessageType type = static_cast<MessageType>(data[0]);
    if (type != MessageType::FILE_CHUNK) {
      LOG_ERROR("FileChunkMessage: Incorrect message type: ", static_cast<int>(type));
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
    
    // Get File ID length
    uint32_t file_id_len_network;
    std::memcpy(&file_id_len_network, data.data() + 57, 4);
    uint32_t file_id_len = be32toh(file_id_len_network);
    
    if (data.size() < MIN_HEADER_SIZE + file_id_len + 8) {  // + 8 for chunk index and data length
      LOG_ERROR("FileChunkMessage: Buffer too small for file_id and chunk info");
      return false;
    }
    
    // Copy File ID
    _file_id.assign(data.begin() + 61, data.begin() + 61 + file_id_len);
    
    // Copy Chunk index
    uint32_t chunk_index_network;
    std::memcpy(&chunk_index_network, data.data() + 61 + file_id_len, 4);
    _chunk_index = be32toh(chunk_index_network);
    
    // Get Data length
    uint32_t data_len_network;
    std::memcpy(&data_len_network, data.data() + 65 + file_id_len, 4);
    uint32_t data_len = be32toh(data_len_network);
    
    if (data.size() < MIN_HEADER_SIZE + file_id_len + 8 + data_len) {
      LOG_ERROR("FileChunkMessage: Buffer too small for data");
      return false;
    }
    
    // Copy Data
    _data.assign(data.begin() + 69 + file_id_len, data.begin() + 69 + file_id_len + data_len);
    
    return true;
  }
  
 private:
  std::string _file_id;
  uint32_t _chunk_index;
  ByteBuffer _data;
};

class FileTransferCompleteMessage : public Message {
 public:
  FileTransferCompleteMessage(const PeerId& sender, 
                             const std::string& file_id,
                             bool success,
                             const std::string& error_message = "")
      : Message(MessageType::FILE_TRANSFER_COMPLETE, sender),
        _file_id(file_id),
        _success(success),
        _error_message(error_message) {}
  
  FileTransferCompleteMessage(const PeerId& sender)
      : Message(MessageType::FILE_TRANSFER_COMPLETE, sender), _success(false) {}
  
  const std::string& GetFileId() const { return _file_id; }
  bool IsSuccess() const { return _success; }
  const std::string& GetErrorMessage() const { return _error_message; }
  
  ByteBuffer Serialize() const override {
    // Header format:
    // - 1 byte: MessageType
    // - 32 bytes: PeerId
    // - 16 bytes: MessageId
    // - 8 bytes: Timestamp
    // - 4 bytes: File ID length
    // - N bytes: File ID
    // - 1 byte: Success flag
    // - 4 bytes: Error message length
    // - M bytes: Error message
    constexpr size_t HEADER_SIZE_WITHOUT_FILE_ID_ERROR = 1 + 32 + 16 + 8 + 4 + 1 + 4;
    
    // Allocate buffer with room for header, file_id, and error message
    ByteBuffer buffer(HEADER_SIZE_WITHOUT_FILE_ID_ERROR + _file_id.size() + _error_message.size());
    
    // Fill the header
    buffer[0] = static_cast<uint8_t>(_type);
    
    // Copy PeerId
    std::copy(_sender.begin(), _sender.end(), buffer.begin() + 1);
    
    // Copy MessageId
    std::copy(_id.begin(), _id.end(), buffer.begin() + 33);
    
    // Copy Timestamp (network byte order)
    uint64_t timestamp_network = htobe64(static_cast<uint64_t>(_timestamp));
    std::memcpy(buffer.data() + 49, &timestamp_network, 8);
    
    // Copy File ID length (network byte order)
    uint32_t file_id_len_network = htobe32(static_cast<uint32_t>(_file_id.size()));
    std::memcpy(buffer.data() + 57, &file_id_len_network, 4);
    
    // Copy File ID
    std::copy(_file_id.begin(), _file_id.end(), buffer.begin() + 61);
    
    // Copy Success flag
    buffer[61 + _file_id.size()] = _success ? 1 : 0;
    
    // Copy Error message length (network byte order)
    uint32_t error_len_network = htobe32(static_cast<uint32_t>(_error_message.size()));
    std::memcpy(buffer.data() + 62 + _file_id.size(), &error_len_network, 4);
    
    // Copy Error message
    std::copy(_error_message.begin(), _error_message.end(), 
             buffer.begin() + 66 + _file_id.size());
    
    return buffer;
  }
  
  bool Deserialize(const ByteBuffer& data) override {
    constexpr size_t MIN_HEADER_SIZE = 1 + 32 + 16 + 8 + 4;  // Without file_id length
    
    if (data.size() < MIN_HEADER_SIZE) {
      LOG_ERROR("FileTransferCompleteMessage: Buffer too small to deserialize");
      return false;
    }
    
    // Verify message type
    MessageType type = static_cast<MessageType>(data[0]);
    if (type != MessageType::FILE_TRANSFER_COMPLETE) {
      LOG_ERROR("FileTransferCompleteMessage: Incorrect message type: ", static_cast<int>(type));
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
    
    // Get File ID length
    uint32_t file_id_len_network;
    std::memcpy(&file_id_len_network, data.data() + 57, 4);
    uint32_t file_id_len = be32toh(file_id_len_network);
    
    if (data.size() < MIN_HEADER_SIZE + file_id_len + 5) {  // + 5 for success flag and error length
      LOG_ERROR("FileTransferCompleteMessage: Buffer too small for file_id and success info");
      return false;
    }
    
    // Copy File ID
    _file_id.assign(data.begin() + 61, data.begin() + 61 + file_id_len);
    
    // Copy Success flag
    _success = data[61 + file_id_len] != 0;
    
    // Get Error message length
    uint32_t error_len_network;
    std::memcpy(&error_len_network, data.data() + 62 + file_id_len, 4);
    uint32_t error_len = be32toh(error_len_network);
    
    if (data.size() < MIN_HEADER_SIZE + file_id_len + 5 + error_len) {
      LOG_ERROR("FileTransferCompleteMessage: Buffer too small for error message");
      return false;
    }
    
    // Copy Error message
    _error_message.assign(data.begin() + 66 + file_id_len, 
                        data.begin() + 66 + file_id_len + error_len);
    
    return true;
  }
  
 private:
  std::string _file_id;
  bool _success;
  std::string _error_message;
};

// Implementation of FileTransferManager
class BasicFileTransferManager : public FileTransferManager {
 public:
  static constexpr size_t DEFAULT_CHUNK_SIZE = 16 * 1024;  // 16 KB chunks
  
  explicit BasicFileTransferManager(std::shared_ptr<NetworkManager> network_manager)
      : _network_manager(network_manager), _chunk_size(DEFAULT_CHUNK_SIZE) {
    
    // Register for network messages
    _network_manager->SetMessageCallback(
        [this](std::unique_ptr<Message> message) {
          HandleMessage(std::move(message));
        });
  }

  ~BasicFileTransferManager() override = default;

  bool SendFile(const PeerId& peer_id, const std::string& file_path) override {
    // Check if file exists
    if (!std::filesystem::exists(file_path)) {
      LOG_ERROR("File not found: ", file_path);
      return false;
    }
    
    // Get file size
    uint64_t file_size = std::filesystem::file_size(file_path);
    
    // Generate a unique file ID (using the file path for simplicity)
    std::string file_id = file_path;
    std::string filename = std::filesystem::path(file_path).filename().string();
    
    // Send file transfer request
    FileTransferRequestMessage request(peer_id, filename, file_size);
    bool sent = _network_manager->SendMessage(peer_id, request);
    
    if (!sent) {
      LOG_ERROR("Failed to send file transfer request");
      return false;
    }
    
    // Store the transfer info
    TransferInfo transfer_info;
    transfer_info.file_path = file_path;
    transfer_info.file_id = file_id;
    transfer_info.file_size = file_size;
    transfer_info.peer_id = peer_id;
    transfer_info.status = FileTransferStatus::PENDING;
    transfer_info.bytes_transferred = 0;
    transfer_info.start_time = std::chrono::steady_clock::now();
    
    {
      std::lock_guard<std::mutex> lock(_transfers_mutex);
      _outgoing_transfers.emplace(std::make_pair(peer_id, file_id), std::move(transfer_info));
    }
    
    LOG_INFO("File transfer request sent for ", filename);
    return true;
  }
  
  void CancelTransfer(const PeerId& peer_id, const std::string& file_path) override {
    std::string file_id = file_path;  // Same as in SendFile
    
    {
      std::lock_guard<std::mutex> lock(_transfers_mutex);
      
      // Check outgoing transfers
      auto out_it = _outgoing_transfers.find(std::make_pair(peer_id, file_id));
      if (out_it != _outgoing_transfers.end()) {
        out_it->second.status = FileTransferStatus::FAILED;
        
        // Notify the peer
        FileTransferCompleteMessage complete(peer_id, file_id, false, "Transfer cancelled by sender");
        _network_manager->SendMessage(peer_id, complete);
        
        _outgoing_transfers.erase(out_it);
        LOG_INFO("Outgoing file transfer cancelled: ", file_path);
        return;
      }
      
      // Check incoming transfers
      auto in_it = _incoming_transfers.find(std::make_pair(peer_id, file_id));
      if (in_it != _incoming_transfers.end()) {
        in_it->second.status = FileTransferStatus::FAILED;
        
        if (in_it->second.output_stream.is_open()) {
          in_it->second.output_stream.close();
        }
        
        // Notify the peer
        FileTransferCompleteMessage complete(peer_id, file_id, false, "Transfer cancelled by receiver");
        _network_manager->SendMessage(peer_id, complete);
        
        _incoming_transfers.erase(in_it);
        LOG_INFO("Incoming file transfer cancelled: ", file_path);
        return;
      }
    }
    
    LOG_WARNING("No active transfer found for cancellation: ", file_path);
  }
  
  std::vector<std::tuple<PeerId, std::string, FileTransferStatus, double>> 
      GetOngoingTransfers() const override {
    std::vector<std::tuple<PeerId, std::string, FileTransferStatus, double>> result;
    
    {
      std::lock_guard<std::mutex> lock(_transfers_mutex);
      
      // Add outgoing transfers
      for (const auto& [key, transfer] : _outgoing_transfers) {
        double progress = 0.0;
        if (transfer.file_size > 0) {
          progress = static_cast<double>(transfer.bytes_transferred) / transfer.file_size;
        }
        
        result.emplace_back(transfer.peer_id, transfer.file_path, 
                            transfer.status, progress);
      }
      
      // Add incoming transfers
      for (const auto& [key, transfer] : _incoming_transfers) {
        double progress = 0.0;
        if (transfer.file_size > 0) {
          progress = static_cast<double>(transfer.bytes_transferred) / transfer.file_size;
        }
        
        result.emplace_back(transfer.peer_id, transfer.file_path, 
                            transfer.status, progress);
      }
    }
    
    return result;
  }
  
  void SetProgressCallback(FileTransferProgressCallback callback) override {
    _progress_callback = std::move(callback);
  }
  
  void SetCompletedCallback(FileTransferCompletedCallback callback) override {
    _completed_callback = std::move(callback);
  }
  
  void SetRequestCallback(FileTransferRequestCallback callback) override {
    _request_callback = std::move(callback);
  }

 private:
  mutable std::mutex _transfers_mutex;

  struct TransferInfo {
    std::string file_path;
    std::string file_id;
    uint64_t file_size;
    PeerId peer_id;
    FileTransferStatus status;
    uint64_t bytes_transferred;
    std::chrono::steady_clock::time_point start_time;
    std::ifstream input_stream;
    std::ofstream output_stream;
    uint32_t next_chunk_index;
    std::unordered_map<uint32_t, bool> received_chunks;
  };
  
  void HandleMessage(std::unique_ptr<Message> message) {
    switch (message->GetType()) {
      case MessageType::FILE_TRANSFER_REQUEST:
        HandleFileTransferRequest(static_cast<FileTransferRequestMessage&>(*message));
        break;
        
      case MessageType::FILE_CHUNK:
        HandleFileChunk(static_cast<FileChunkMessage&>(*message));
        break;
        
      case MessageType::FILE_TRANSFER_COMPLETE:
        HandleFileTransferComplete(static_cast<FileTransferCompleteMessage&>(*message));
        break;
        
      default:
        // Not a file transfer message
        break;
    }
  }
  
  void HandleFileTransferRequest(const FileTransferRequestMessage& message) {
    const PeerId& sender = message.GetSender();
    const std::string& filename = message.GetFilename();
    uint64_t file_size = message.GetFileSize();
    
    LOG_INFO("Received file transfer request from peer: ", filename, " (", file_size, " bytes)");
    
    bool accept = true;
    if (_request_callback) {
      accept = _request_callback(sender, filename, file_size);
    }
    
    if (!accept) {
      LOG_INFO("File transfer request rejected by user");
      FileTransferCompleteMessage response(sender, filename, false, "Transfer rejected by receiver");
      _network_manager->SendMessage(sender, response);
      return;
    }
    
    // Create the output directory if it doesn't exist
    std::filesystem::path output_dir = std::filesystem::current_path() / "downloads";
    std::filesystem::create_directories(output_dir);
    
    // Prepare the output file
    std::string output_path = (output_dir / filename).string();
    std::ofstream output_stream(output_path, std::ios::binary | std::ios::trunc);
    
    if (!output_stream) {
      LOG_ERROR("Failed to create output file: ", output_path);
      FileTransferCompleteMessage response(sender, filename, false, "Failed to create output file");
      _network_manager->SendMessage(sender, response);
      return;
    }
    
    // Store the transfer info
    TransferInfo transfer_info;
    transfer_info.file_path = output_path;
    transfer_info.file_id = filename;  // Using the filename as file_id for simplicity
    transfer_info.file_size = file_size;
    transfer_info.peer_id = sender;
    transfer_info.status = FileTransferStatus::IN_PROGRESS;
    transfer_info.bytes_transferred = 0;
    transfer_info.start_time = std::chrono::steady_clock::now();
    transfer_info.output_stream = std::move(output_stream);
    transfer_info.next_chunk_index = 0;
    
    {
      std::lock_guard<std::mutex> lock(_transfers_mutex);
      _incoming_transfers[std::make_pair(sender, filename)] = std::move(transfer_info);
    }
    
    LOG_INFO("File transfer accepted: ", output_path);
  }
  
  void HandleFileChunk(const FileChunkMessage& message) {
    const PeerId& sender = message.GetSender();
    const std::string& file_id = message.GetFileId();
    uint32_t chunk_index = message.GetChunkIndex();
    const ByteBuffer& data = message.GetData();
    
    std::lock_guard<std::mutex> lock(_transfers_mutex);
    auto it = _incoming_transfers.find(std::make_pair(sender, file_id));
    
    if (it == _incoming_transfers.end()) {
      LOG_ERROR("Received chunk for unknown file transfer: ", file_id);
      return;
    }
    
    TransferInfo& transfer = it->second;
    
    // Skip if already received this chunk
    if (transfer.received_chunks.find(chunk_index) != transfer.received_chunks.end()) {
      LOG_WARNING("Received duplicate chunk: ", chunk_index);
      return;
    }
    
    // Write the chunk to the file at the right position
    transfer.output_stream.seekp(chunk_index * _chunk_size);
    transfer.output_stream.write(reinterpret_cast<const char*>(data.data()), data.size());
    
    if (!transfer.output_stream) {
      LOG_ERROR("Failed to write chunk to file: ", transfer.file_path);
      FileTransferCompleteMessage response(sender, file_id, false, "Failed to write to output file");
      _network_manager->SendMessage(sender, response);
      transfer.status = FileTransferStatus::FAILED;
      transfer.output_stream.close();
      _incoming_transfers.erase(it);
      
      if (_completed_callback) {
        _completed_callback(sender, transfer.file_path, false, "Failed to write to output file");
      }
      
      return;
    }
    
    // Mark this chunk as received
    transfer.received_chunks[chunk_index] = true;
    transfer.bytes_transferred += data.size();
    
    // Update progress
    if (_progress_callback) {
      double progress = static_cast<double>(transfer.bytes_transferred) / transfer.file_size;
      _progress_callback(sender, transfer.file_path, progress);
    }
    
    // Check if transfer is complete
    if (transfer.bytes_transferred >= transfer.file_size) {
      LOG_INFO("File transfer complete: ", transfer.file_path);
      transfer.status = FileTransferStatus::COMPLETED;
      transfer.output_stream.close();
      
      FileTransferCompleteMessage response(sender, file_id, true);
      _network_manager->SendMessage(sender, response);
      
      if (_completed_callback) {
        _completed_callback(sender, transfer.file_path, true, "");
      }
      
      _incoming_transfers.erase(it);
    }
  }
  
  void HandleFileTransferComplete(const FileTransferCompleteMessage& message) {
    const PeerId& sender = message.GetSender();
    const std::string& file_id = message.GetFileId();
    bool success = message.IsSuccess();
    const std::string& error_message = message.GetErrorMessage();
    
    std::lock_guard<std::mutex> lock(_transfers_mutex);
    auto it = _outgoing_transfers.find(std::make_pair(sender, file_id));
    
    if (it == _outgoing_transfers.end()) {
      LOG_ERROR("Received completion for unknown file transfer: ", file_id);
      return;
    }
    
    TransferInfo& transfer = it->second;
    
    if (success) {
      LOG_INFO("File transfer confirmed complete by receiver: ", transfer.file_path);
      transfer.status = FileTransferStatus::COMPLETED;
      
      if (_completed_callback) {
        _completed_callback(sender, transfer.file_path, true, "");
      }
    } else {
      LOG_ERROR("File transfer failed: ", transfer.file_path, ": ", error_message);
      transfer.status = FileTransferStatus::FAILED;
      
      if (_completed_callback) {
        _completed_callback(sender, transfer.file_path, false, error_message);
      }
    }
    
    // Close any open streams
    if (transfer.input_stream.is_open()) {
      transfer.input_stream.close();
    }
    
    _outgoing_transfers.erase(it);
  }
  
  void StartSendingFile(const PeerId& peer_id, const std::string& file_id) {
    std::lock_guard<std::mutex> lock(_transfers_mutex);
    auto it = _outgoing_transfers.find(std::make_pair(peer_id, file_id));
    
    if (it == _outgoing_transfers.end()) {
      LOG_ERROR("Cannot start sending unknown file: ", file_id);
      return;
    }
    
    TransferInfo& transfer = it->second;
    
    if (!transfer.input_stream.is_open()) {
      transfer.input_stream.open(transfer.file_path, std::ios::binary);
      if (!transfer.input_stream) {
        LOG_ERROR("Failed to open file for reading: ", transfer.file_path);
        FileTransferCompleteMessage complete(peer_id, file_id, false, "Failed to open file for reading");
        _network_manager->SendMessage(peer_id, complete);
        transfer.status = FileTransferStatus::FAILED;
        _outgoing_transfers.erase(it);
        
        if (_completed_callback) {
          _completed_callback(peer_id, transfer.file_path, false, "Failed to open file for reading");
        }
        
        return;
      }
    }
    
    transfer.status = FileTransferStatus::IN_PROGRESS;
    transfer.next_chunk_index = 0;
    
    // Start sending the file in chunks
    SendNextChunk(peer_id, file_id);
  }
  
  void SendNextChunk(const PeerId& peer_id, const std::string& file_id) {
    std::lock_guard<std::mutex> lock(_transfers_mutex);
    auto it = _outgoing_transfers.find(std::make_pair(peer_id, file_id));
    
    if (it == _outgoing_transfers.end()) {
      return;  // Transfer was cancelled
    }
    
    TransferInfo& transfer = it->second;
    
    if (transfer.status != FileTransferStatus::IN_PROGRESS) {
      return;  // Transfer is not in progress
    }
    
    if (!transfer.input_stream.is_open()) {
      LOG_ERROR("File stream not open for sending chunks");
      return;
    }
    
    // Seek to the right position
    uint64_t pos = transfer.next_chunk_index * _chunk_size;
    transfer.input_stream.seekg(pos);
    
    if (!transfer.input_stream) {
      LOG_ERROR("Failed to seek in file: ", transfer.file_path);
      FileTransferCompleteMessage complete(peer_id, file_id, false, "Failed to read from file");
      _network_manager->SendMessage(peer_id, complete);
      transfer.status = FileTransferStatus::FAILED;
      transfer.input_stream.close();
      _outgoing_transfers.erase(it);
      
      if (_completed_callback) {
        _completed_callback(peer_id, transfer.file_path, false, "Failed to read from file");
      }
      
      return;
    }
    
    // Read a chunk
    ByteBuffer chunk(_chunk_size);
    transfer.input_stream.read(reinterpret_cast<char*>(chunk.data()), _chunk_size);
    std::streamsize bytes_read = transfer.input_stream.gcount();
    
    if (bytes_read == 0) {
      // End of file reached
      if (transfer.bytes_transferred >= transfer.file_size) {
        LOG_INFO("File sending complete: ", transfer.file_path);
        transfer.status = FileTransferStatus::COMPLETED;
        transfer.input_stream.close();
        
        if (_completed_callback) {
          _completed_callback(peer_id, transfer.file_path, true, "");
        }
        
        _outgoing_transfers.erase(it);
      } else {
        LOG_ERROR("Unexpected end of file: ", transfer.file_path);
        FileTransferCompleteMessage complete(peer_id, file_id, false, "Unexpected end of file");
        _network_manager->SendMessage(peer_id, complete);
        transfer.status = FileTransferStatus::FAILED;
        transfer.input_stream.close();
        _outgoing_transfers.erase(it);
        
        if (_completed_callback) {
          _completed_callback(peer_id, transfer.file_path, false, "Unexpected end of file");
        }
      }
      
      return;
    }
    
    // Resize chunk to actual bytes read
    chunk.resize(bytes_read);
    
    // Send the chunk
    FileChunkMessage chunk_msg(peer_id, file_id, transfer.next_chunk_index, chunk);
    bool sent = _network_manager->SendMessage(peer_id, chunk_msg);
    
    if (!sent) {
      LOG_ERROR("Failed to send file chunk: ", transfer.file_path);
      FileTransferCompleteMessage complete(peer_id, file_id, false, "Failed to send file chunk");
      _network_manager->SendMessage(peer_id, complete);
      transfer.status = FileTransferStatus::FAILED;
      transfer.input_stream.close();
      _outgoing_transfers.erase(it);
      
      if (_completed_callback) {
        _completed_callback(peer_id, transfer.file_path, false, "Failed to send file chunk");
      }
      
      return;
    }
    
    // Update progress
    transfer.bytes_transferred += bytes_read;
    transfer.next_chunk_index++;
    
    if (_progress_callback) {
      double progress = static_cast<double>(transfer.bytes_transferred) / transfer.file_size;
      _progress_callback(peer_id, transfer.file_path, progress);
    }
    
    // Check if we're done
    if (transfer.bytes_transferred >= transfer.file_size) {
      LOG_INFO("File sending complete: ", transfer.file_path);
      transfer.status = FileTransferStatus::COMPLETED;
      transfer.input_stream.close();
      
      if (_completed_callback) {
        _completed_callback(peer_id, transfer.file_path, true, "");
      }
      
      _outgoing_transfers.erase(it);
    } else {
      // Send next chunk
      SendNextChunk(peer_id, file_id);
    }
  }
  std::shared_ptr<NetworkManager> _network_manager;

  std::map<std::pair<PeerId, std::string>, TransferInfo> _outgoing_transfers;
  std::map<std::pair<PeerId, std::string>, TransferInfo> _incoming_transfers;

  size_t _chunk_size;

  FileTransferProgressCallback _progress_callback;
  FileTransferCompletedCallback _completed_callback;
  FileTransferRequestCallback _request_callback;
};

std::unique_ptr<FileTransferManager> FileTransferFactory::Create(
    std::shared_ptr<NetworkManager> network_manager) {
  return std::make_unique<BasicFileTransferManager>(network_manager);
}

}  // namespace linknet
