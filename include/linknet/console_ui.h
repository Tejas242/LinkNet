#ifndef LINKNET_CONSOLE_UI_H_
#define LINKNET_CONSOLE_UI_H_

#include "linknet/types.h"
#include <string>
#include <map>
#include <functional>
#include <memory>
#include <mutex>
#include <thread>
#include <queue>
#include <atomic>
#include <condition_variable>

namespace linknet {

// Forward declarations
class NetworkManager;
class FileTransferManager;
class ChatManager;

// ANSI color codes
enum class TextColor {
  RESET,
  RED,
  GREEN,
  YELLOW,
  BLUE,
  MAGENTA,
  CYAN,
  GRAY,
  BOLD_WHITE
};

// Command handlers
using CommandHandler = std::function<bool(const std::vector<std::string>&)>;

class ConsoleUI {
 public:
  ConsoleUI(std::shared_ptr<NetworkManager> network_manager,
           std::shared_ptr<FileTransferManager> file_transfer_manager,
           std::shared_ptr<ChatManager> chat_manager);
  ~ConsoleUI();

  // Start the UI
  void Start();
  
  // Stop the UI
  void Stop();
  
  // Display a message
  void DisplayMessage(const std::string& message);
  
  // Display a colored message
  void DisplayColoredMessage(const std::string& message, TextColor color);
  
  // Register a custom command
  void RegisterCommand(const std::string& command, CommandHandler handler,
                      const std::string& description);
                      
  // Check if UI is running
  bool IsRunning() const { return _running; }
  
 private:
  void InputThreadFunc();
  void DisplayThreadFunc();
  
  void ProcessCommand(const std::string& input);
  void DisplayHelp();
  
  // Apply color to text
  std::string ColorText(const std::string& text, TextColor color) const;
  
  std::shared_ptr<NetworkManager> _network_manager;
  std::shared_ptr<FileTransferManager> _file_transfer_manager;
  std::shared_ptr<ChatManager> _chat_manager;
  
  std::atomic<bool> _running;
  std::thread _input_thread;
  std::thread _display_thread;
  
  std::mutex _display_queue_mutex;
  std::queue<std::string> _display_queue;
  std::condition_variable _display_cv;
  
  std::mutex _commands_mutex;
  std::map<std::string, std::pair<CommandHandler, std::string>> _commands;
};

}  // namespace linknet

#endif  // LINKNET_CONSOLE_UI_H_
