# File Transfer

The File Transfer system in LinkNet enables secure and efficient peer-to-peer file sharing with robust error handling and progress tracking.

## Overview

The `FileTransferManager` interface and its implementation provide a complete solution for sending and receiving files in a peer-to-peer environment. It handles file chunking, transmission, reassembly, progress tracking, and integrity verification.

## Key Components

### FileTransferManager Interface

Located in `include/linknet/file_transfer.h`, this interface defines the file transfer operations:

```cpp
class FileTransferManager {
 public:
  virtual ~FileTransferManager() = default;
  virtual bool SendFile(const PeerId& peer_id, const std::string& file_path) = 0;
  virtual void CancelTransfer(const PeerId& peer_id, const std::string& file_path) = 0;
  virtual std::vector<std::tuple<PeerId, std::string, FileTransferStatus, double>> 
      GetOngoingTransfers() const = 0;
  virtual void SetProgressCallback(FileTransferProgressCallback callback) = 0;
  virtual void SetCompletedCallback(FileTransferCompletedCallback callback) = 0;
  virtual void SetRequestCallback(FileTransferRequestCallback callback) = 0;
};
```

### File Transfer Callbacks

The system uses callbacks to notify the application of events:

```cpp
// Progress updates during transfer
using FileTransferProgressCallback = std::function<void(
    const PeerId&, const std::string&, double)>;

// Transfer completion (success or failure)
using FileTransferCompletedCallback = std::function<void(
    const PeerId&, const std::string&, bool, const std::string&)>;

// Permission request for incoming transfers
using FileTransferRequestCallback = std::function<bool(
    const PeerId&, const std::string&, uint64_t)>;
```

### Message Types

Specific message types defined for file transfer operations:

- **FILE_TRANSFER_REQUEST**: Initiates a file transfer session
- **FILE_TRANSFER_RESPONSE**: Accepts or rejects a transfer request
- **FILE_CHUNK**: Contains a piece of the file being transferred
- **FILE_TRANSFER_COMPLETE**: Signals transfer completion

## File Transfer Process

### Sending a File

1. **File Analysis**: The file is analyzed to determine size and generate a unique ID
2. **Request Initiation**: A FILE_TRANSFER_REQUEST message is sent to the recipient
3. **Permission**: System waits for approval from the recipient
4. **Chunking**: File is divided into manageable chunks (typically 64KB-1MB each)
5. **Transmission**: Chunks are sent sequentially, with progress tracking
6. **Completion**: A FILE_TRANSFER_COMPLETE message is sent with integrity verification info

### Receiving a File

1. **Request Reception**: A FILE_TRANSFER_REQUEST message is received
2. **Authorization**: The request callback is invoked to get user permission
3. **Response**: A FILE_TRANSFER_RESPONSE message is sent (accept/reject)
4. **File Creation**: A destination file is created (typically in a downloads folder)
5. **Chunk Reception**: FILE_CHUNK messages are received and written to the file
6. **Verification**: After all chunks are received, the file integrity is verified
7. **Completion**: The completion callback is invoked with the result

## File Transfer Flow

```
   Sender                             Receiver
     │                                   │
     │  ┌──────────────────────────────┐│
     │  │FILE_TRANSFER_REQUEST         ││
     │  │- File ID                     ││
     │  │- File name                   ││
     │  │- File size                   ││
     │  │- Hash algorithm              ││
     │  └──────────────────────────────┘│
     │─────────────────────────────────▶│
     │                                   │◄─── User prompt
     │                                   │      Accept/Reject
     │  ┌──────────────────────────────┐│
     │  │FILE_TRANSFER_RESPONSE        ││
     │  │- File ID                     ││
     │  │- Accepted/Rejected           ││
     │  └──────────────────────────────┘│
     │◀─────────────────────────────────│
     │                                   │
     │  ┌──────────────────────────────┐│
     │  │FILE_CHUNK                    ││
     │  │- File ID                     ││
     │  │- Chunk index                 ││
     │  │- Chunk data                  ││
     │  └──────────────────────────────┘│
     │─────────────────────────────────▶│
     │                                   │
     │  ┌──────────────────────────────┐│
     │  │FILE_CHUNK                    ││
     │  │- File ID                     ││
     │  │- Chunk index                 ││
     │  │- Chunk data                  ││
     │  └──────────────────────────────┘│
     │─────────────────────────────────▶│
     │            ...                    │
     │  ┌──────────────────────────────┐│
     │  │FILE_TRANSFER_COMPLETE        ││
     │  │- File ID                     ││
     │  │- File hash                   ││
     │  └──────────────────────────────┘│
     │─────────────────────────────────▶│
     │                                   │◄─── Hash verification
     │                                   │      Report to user
     │                                   │
```

## Features

### Chunked Transfer

Files are broken into manageable chunks, which provides several benefits:
- **Memory Efficiency**: No need to load the entire file into memory
- **Resume Support**: Transfers can be resumed after interruptions
- **Parallel Transfer**: Multiple chunks can be transferred simultaneously
- **Progress Reporting**: Fine-grained progress updates

### Integrity Verification

File integrity is verified using cryptographic hashes:
- **Whole-file Hashing**: The complete file is hashed (typically using BLAKE2b)
- **Chunk-level Checksums**: Each chunk includes a checksum for early corruption detection
- **Automatic Retransmission**: Corrupted chunks are requested again

### Transfer Control

The system provides multiple ways to control active transfers:
- **Cancellation**: Either sender or receiver can cancel an ongoing transfer
- **Pause/Resume**: Transfers can be paused and resumed (depends on implementation)
- **Rate Limiting**: Optional bandwidth throttling

## Security Features

File transfers benefit from LinkNet's security infrastructure:
- **Encryption**: All file chunks are encrypted during transmission
- **Authentication**: File origins are verified to prevent spoofing
- **Malware Protection**: Optional scanning before files are fully received

## Implementation Details

### Thread Safety

The file transfer system is thread-safe:
- Multiple transfers can run concurrently
- Progress callbacks may be invoked from different threads
- File I/O operations are synchronized appropriately

### Error Handling

The system handles various error conditions:
- **Network Failures**: Retries or reports failures depending on the error
- **Disk Space Issues**: Checks available space before accepting transfers
- **File Access Errors**: Handles permission problems and locked files

## Advanced Features

### Adaptive Chunk Sizing

The implementation can adjust chunk size based on:
- Network conditions (bandwidth, latency)
- File type and size
- Available memory

### Parallel Transfers

The system can handle multiple concurrent transfers:
- Between the same pair of peers
- Between different peers
- Balancing resources appropriately

## Performance Considerations

Several optimizations ensure efficient file transfers:
- **Direct Disk-to-Network**: Minimizes memory copies
- **Zero-copy Techniques**: When supported by the platform
- **Streaming Processing**: Start processing data as it arrives
- **Compression**: Optional compression for compatible file types

## Code Examples

### Initializing the File Transfer Manager

```cpp
// Create the file transfer manager (assuming network_manager is already initialized)
auto file_transfer = FileTransferFactory::Create(network_manager);

// Set up callbacks
file_transfer->SetProgressCallback([](const PeerId& peer_id, const std::string& filename, double progress) {
  std::cout << "Transfer progress for " << filename << ": " << (progress * 100) << "%" << std::endl;
});

file_transfer->SetCompletedCallback([](const PeerId& peer_id, const std::string& filename, 
                                    bool success, const std::string& error) {
  if (success) {
    std::cout << "Transfer completed: " << filename << std::endl;
  } else {
    std::cerr << "Transfer failed: " << filename << " - " << error << std::endl;
  }
});

file_transfer->SetRequestCallback([](const PeerId& peer_id, const std::string& filename, uint64_t size) {
  std::cout << "Incoming file: " << filename << " (" 
            << (size / 1024.0 / 1024.0) << " MB)" << std::endl;
  // Return true to accept, false to reject
  return true;
});
```

### Sending a File

```cpp
// Send a file to a peer
PeerId bob_id = /* ... */;
bool success = file_transfer->SendFile(bob_id, "/path/to/document.pdf");

if (!success) {
  std::cerr << "Failed to initiate file transfer" << std::endl;
}
```

### Monitoring Transfers

```cpp
// Get all ongoing transfers
auto transfers = file_transfer->GetOngoingTransfers();

// Display status
for (const auto& [peer_id, filename, status, progress] : transfers) {
  std::cout << "Transfer to peer "
            << " - File: " << filename
            << " - Status: " << static_cast<int>(status)
            << " - Progress: " << (progress * 100) << "%" << std::endl;
}
```

### Cancelling a Transfer

```cpp
// Cancel an ongoing transfer
PeerId peer_id = /* ... */;
std::string filename = "document.pdf";
file_transfer->CancelTransfer(peer_id, filename);
```

## Related Documentation

- [Network Layer](network.md) - How file chunks are transmitted between peers
- [Message System](messaging.md) - Message format used by the file transfer system
- [Security Model](../advanced/security_model.md) - Security aspects of file transfers
