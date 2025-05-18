# User Interface

The User Interface in LinkNet provides a terminal-based interface for interacting with the application, offering a clean and intuitive way to manage connections, send messages, and transfer files.

## Overview

LinkNet's `ConsoleUI` class handles all user interaction through a text-based terminal interface. It processes commands, displays status information, and visualizes chat and file transfer activities in real-time.

## Key Components

### ConsoleUI Class

Located in `include/linknet/console_ui.h`, this class provides the core UI functionality:

```cpp
class ConsoleUI {
 public:
  ConsoleUI(std::shared_ptr<NetworkManager> network_manager,
            std::shared_ptr<ChatManager> chat_manager,
            std::shared_ptr<FileTransferManager> file_manager);
  ~ConsoleUI();
  
  void Start();
  void Stop();
  
  void SetLocalUserName(const std::string& name);
  
 private:
  // Command processing
  void ProcessCommand(const std::string& command);
  
  // Display functions
  void DisplayHelp();
  void DisplayStatus();
  void DisplayPeerList();
  void DisplayChatHistory(const PeerId& peer_id);
  void DisplayTransferStatus();
  
  // Command handling functions
  void HandleConnectCommand(const std::vector<std::string>& args);
  void HandleMessageCommand(const std::vector<std::string>& args);
  void HandleBroadcastCommand(const std::vector<std::string>& args);
  void HandleSendFileCommand(const std::vector<std::string>& args);
  void HandleDisconnectCommand(const std::vector<std::string>& args);
  // ...
};
```

## User Interface Design

### Main Layout

The console UI is divided into logical sections:

```
┌─────────────────────────────────────────────────────────┐
│                      LinkNet v1.0                        │
├─────────────────────────────────────────────────────────┤
│ STATUS: Connected (5 peers) | User: Alice               │
├─────────────────────────────────────────────────────────┤
│                                                         │
│                                                         │
│ [10:15] Bob: Hello, how are you?                        │
│ [10:16] You: I'm good, thanks!                          │
│ [10:17] Bob: Would you like to see the project files?   │
│ [10:17] You: Yes, please send them                      │
│                                                         │
│ ** Bob is sending project.zip (2.5 MB) - 45% complete   │
│                                                         │
│                                                         │
├─────────────────────────────────────────────────────────┤
│ > _                                                     │
└─────────────────────────────────────────────────────────┘
```

### User Input

The UI captures user input through a command-line interface at the bottom of the screen. The input line supports:
- Command history navigation (up/down arrows)
- Tab completion for commands and peers
- Command editing (left/right arrows, backspace, delete)

## Command System

The UI processes various commands for different operations:

### Connection Management

- `/connect <host> <port>` - Connect to a peer
- `/disconnect <peer-id>` - Disconnect from a peer
- `/list` - List connected peers
- `/status` - Show connection status

### Messaging

- `/msg <peer-id> <message>` - Send a message to a peer
- `/broadcast <message>` - Send a message to all peers
- `/history <peer-id>` - Show chat history with a peer

### File Transfer

- `/send <peer-id> <file-path>` - Send a file to a peer
- `/transfers` - List active file transfers
- `/cancel <transfer-id>` - Cancel a file transfer

### System Control

- `/help` - Display available commands
- `/quit` - Exit the application
- `/clear` - Clear the screen
- `/name <new-name>` - Change display name

## Display Features

### Chat Visualization

- Message formatting with timestamps
- Color coding for different peers
- Indication of message status (sent, delivered, read)
- Line wrapping for long messages

### Transfer Progress

- Real-time progress bars for file transfers
- Transfer speed calculation
- Time remaining estimation
- Transfer completion notifications

### Status Indicators

- Connection status (connected, disconnected, connecting)
- Peer availability indicators
- Notification for new messages
- Error and warning highlighting

## Implementation Details

### Threading Model

The UI runs on multiple threads:
- **Main Thread**: Handles user input and command processing
- **Render Thread**: Updates the display periodically
- **Event Thread**: Processes incoming events (messages, connections)

### Terminal Handling

The UI adapts to different terminal environments:
- Uses ANSI escape sequences for cursor movement and colors
- Handles terminal resizing
- Falls back to simpler display for limited terminals

### Input Buffering

The input system processes keystrokes efficiently:
- Non-blocking input capture
- Special key handling (control keys, escape sequences)
- Unicode support for international characters

## Advanced Features

### Notifications

The UI can provide various notifications:
- Visual alerts for new messages
- Sound notifications (if supported)
- Status bar indicators for events

### Customization

Users can customize various aspects of the UI:
- Color schemes
- Display density
- Timestamp formats
- Alert preferences

### Logging

All UI activity can be logged:
- Chat logs saved to files
- Command history tracking
- Error logging for troubleshooting

## Code Examples

### Starting the UI

```cpp
// Create managers
auto network_manager = std::make_shared<AsioNetworkManager>();
auto chat_manager = std::make_shared<ChatManager>(network_manager);
auto file_manager = FileTransferFactory::Create(network_manager);

// Create and start the UI
auto ui = std::make_shared<ConsoleUI>(network_manager, chat_manager, file_manager);
ui->SetLocalUserName("Alice");
ui->Start();

// The UI runs until Stop() is called
```

### Custom Command Processing

You can extend the ConsoleUI to add custom commands:

```cpp
void CustomConsoleUI::ProcessCommand(const std::string& command) {
  if (command.empty()) return;
  
  std::vector<std::string> tokens = SplitString(command, ' ');
  std::string cmd = tokens[0];
  
  if (cmd == "/custom") {
    HandleCustomCommand(tokens);
  } else {
    // Pass to parent implementation for standard commands
    ConsoleUI::ProcessCommand(command);
  }
}

void CustomConsoleUI::HandleCustomCommand(const std::vector<std::string>& args) {
  // Custom command implementation
  DisplayMessage("Custom command executed");
}
```

### Displaying Custom Information

```cpp
void DisplayNetworkStats() {
  auto stats = network_manager->GetStatistics();
  
  std::cout << "Network Statistics:" << std::endl;
  std::cout << "  Bytes sent: " << stats.bytes_sent << std::endl;
  std::cout << "  Bytes received: " << stats.bytes_received << std::endl;
  std::cout << "  Messages sent: " << stats.messages_sent << std::endl;
  std::cout << "  Messages received: " << stats.messages_received << std::endl;
  std::cout << "  Active connections: " << stats.active_connections << std::endl;
}
```

## User Guide

### Basic Usage

1. Start LinkNet with `./linknet`
2. Connect to a peer with `/connect <host> <port>`
3. Send a message with `/msg <peer-id> Hello!`
4. Send a file with `/send <peer-id> /path/to/file`
5. View connected peers with `/list`
6. Exit with `/quit`

### Advanced Usage

1. Change your display name with `/name <new-name>`
2. View chat history with `/history <peer-id>`
3. Broadcast a message to all peers with `/broadcast <message>`
4. Check file transfer status with `/transfers`
5. Cancel a file transfer with `/cancel <transfer-id>`

## Related Documentation

- [Chat System](chat.md) - Integration with the chat functionality
- [File Transfer](file_transfer.md) - How file transfers are displayed in the UI
- [Network Layer](network.md) - How the UI shows connection status
