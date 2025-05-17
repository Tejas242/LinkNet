#ifndef LINKNET_FILE_TRANSFER_H_
#define LINKNET_FILE_TRANSFER_H_

#include "linknet/types.h"
#include <string>
#include <functional>
#include <memory>
#include <filesystem>

namespace linknet {

// Forward declaration
class NetworkManager;

// Callbacks for file transfer events
using FileTransferProgressCallback = std::function<void(const PeerId&, const std::string&, double)>;
using FileTransferCompletedCallback = std::function<void(const PeerId&, const std::string&, bool, const std::string&)>;
using FileTransferRequestCallback = std::function<bool(const PeerId&, const std::string&, uint64_t)>;

class FileTransferManager {
 public:
  virtual ~FileTransferManager() = default;

  // Send a file to a peer
  virtual bool SendFile(const PeerId& peer_id, const std::string& file_path) = 0;
  
  // Cancel an ongoing file transfer
  virtual void CancelTransfer(const PeerId& peer_id, const std::string& file_path) = 0;
  
  // Get the status of ongoing transfers
  virtual std::vector<std::tuple<PeerId, std::string, FileTransferStatus, double>> 
      GetOngoingTransfers() const = 0;
  
  // Set callbacks
  virtual void SetProgressCallback(FileTransferProgressCallback callback) = 0;
  virtual void SetCompletedCallback(FileTransferCompletedCallback callback) = 0;
  virtual void SetRequestCallback(FileTransferRequestCallback callback) = 0;
};

// Factory to create a concrete implementation
class FileTransferFactory {
 public:
  static std::unique_ptr<FileTransferManager> Create(std::shared_ptr<NetworkManager> network_manager);
};

}  // namespace linknet

#endif  // LINKNET_FILE_TRANSFER_H_
