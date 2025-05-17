#include "linknet/console_ui.h"
#include "linknet/network.h"
#include "linknet/file_transfer.h"
#include "linknet/message.h"
#include "linknet/logger.h"
#include <iostream>
#include <sstream>
#include <algorithm>
#include <functional>
#include <atomic>
#include <chrono>
#include <iomanip>
#include <random>

namespace linknet {

ConsoleUI::ConsoleUI(std::shared_ptr<NetworkManager> network_manager,
                   std::shared_ptr<FileTransferManager> file_transfer_manager)
    : _network_manager(network_manager),
      _file_transfer_manager(file_transfer_manager),
      _running(false) {
  
  // Register built-in commands
  RegisterCommand("connect", 
      [this](const std::vector<std::string>& args) {
        if (args.size() < 2) {
          DisplayMessage("Usage: /connect <ip:port>");
          return false;
        }
        
        std::string address = args[1];
        uint16_t port = 8080;  // Default port
        
        size_t colon_pos = address.find(':');
        if (colon_pos != std::string::npos) {
          try {
            port = std::stoi(address.substr(colon_pos + 1));
            address = address.substr(0, colon_pos);
          } catch (const std::exception& e) {
            DisplayMessage("Invalid port number");
            return false;
          }
        }
        
        DisplayMessage("Connecting to " + address + ":" + std::to_string(port) + "...");
        
        if (!_network_manager->ConnectToPeer(address, port)) {
          DisplayMessage("Failed to initiate connection");
          return false;
        }
        
        return true;
      }, 
      "Connect to a peer");
  
  RegisterCommand("chat", 
      [this](const std::vector<std::string>& args) {
        if (args.size() < 3) {
          DisplayMessage("Usage: /chat <peer_id> <message>");
          return false;
        }
        
        std::string peer_id_str = args[1];
        
        // Convert string to PeerId
        PeerId peer_id;
        try {
          if (peer_id_str.size() != 64) {
            DisplayMessage("Invalid peer ID length");
            return false;
          }
          
          for (size_t i = 0; i < 32; ++i) {
            std::string byte_str = peer_id_str.substr(i * 2, 2);
            peer_id[i] = std::stoi(byte_str, nullptr, 16);
          }
        } catch (const std::exception& e) {
          DisplayMessage("Invalid peer ID format");
          return false;
        }
        
        // Combine the rest of the arguments as the message
        std::string message;
        for (size_t i = 2; i < args.size(); ++i) {
          if (i > 2) {
            message += " ";
          }
          message += args[i];
        }
        
        // Create sender ID (for now, just a random ID)
        PeerId sender_id;
        std::random_device rd;
        std::generate(sender_id.begin(), sender_id.end(), std::ref(rd));
        
        // Send the message
        ChatMessage chat_msg(sender_id, message);
        if (!_network_manager->SendMessage(peer_id, chat_msg)) {
          DisplayMessage("Failed to send message");
          return false;
        }
        
        DisplayMessage("Message sent");
        return true;
      }, 
      "Send a chat message to a peer");
  
  RegisterCommand("send", 
      [this](const std::vector<std::string>& args) {
        if (args.size() < 3) {
          DisplayMessage("Usage: /send <peer_id> <file_path>");
          return false;
        }
        
        std::string peer_id_str = args[1];
        std::string file_path = args[2];
        
        // Convert string to PeerId
        PeerId peer_id;
        try {
          if (peer_id_str.size() != 64) {
            DisplayMessage("Invalid peer ID length");
            return false;
          }
          
          for (size_t i = 0; i < 32; ++i) {
            std::string byte_str = peer_id_str.substr(i * 2, 2);
            peer_id[i] = std::stoi(byte_str, nullptr, 16);
          }
        } catch (const std::exception& e) {
          DisplayMessage("Invalid peer ID format");
          return false;
        }
        
        DisplayMessage("Sending file " + file_path + " to peer...");
        
        if (!_file_transfer_manager->SendFile(peer_id, file_path)) {
          DisplayMessage("Failed to initiate file transfer");
          return false;
        }
        
        return true;
      }, 
      "Send a file to a peer");
  
  RegisterCommand("peers", 
      [this](const std::vector<std::string>&) {
        auto peers = _network_manager->GetConnectedPeers();
        
        if (peers.empty()) {
          DisplayMessage("No peers connected");
          return true;
        }
        
        DisplayMessage("Connected peers:");
        for (const auto& peer : peers) {
          std::stringstream ss;
          ss << "ID: ";
          for (const auto& byte : peer.id) {
            ss << std::hex << std::setw(2) << std::setfill('0') << static_cast<int>(byte);
          }
          ss << " | Name: " << peer.name 
             << " | IP: " << peer.ip_address << ":" << peer.port;
          
          DisplayMessage(ss.str());
        }
        
        return true;
      }, 
      "List connected peers");
  
  RegisterCommand("transfers", 
      [this](const std::vector<std::string>&) {
        auto transfers = _file_transfer_manager->GetOngoingTransfers();
        
        if (transfers.empty()) {
          DisplayMessage("No ongoing file transfers");
          return true;
        }
        
        DisplayMessage("Ongoing file transfers:");
        for (const auto& [peer_id, file_path, status, progress] : transfers) {
          std::stringstream ss;
          ss << "File: " << file_path
             << " | Status: " << static_cast<int>(status)
             << " | Progress: " << std::fixed << std::setprecision(1)
             << (progress * 100.0) << "%";
          
          DisplayMessage(ss.str());
        }
        
        return true;
      }, 
      "List ongoing file transfers");
  
  RegisterCommand("help", 
      [this](const std::vector<std::string>&) {
        DisplayHelp();
        return true;
      }, 
      "Display available commands");
  
  RegisterCommand("exit", 
      [this](const std::vector<std::string>&) {
        Stop();
        return true;
      }, 
      "Exit the application");
}

ConsoleUI::~ConsoleUI() {
  Stop();
}

void ConsoleUI::Start() {
  if (_running) {
    return;
  }
  
  _running = true;
  
  // Start the input thread
  _input_thread = std::thread(&ConsoleUI::InputThreadFunc, this);
  
  // Start the display thread
  _display_thread = std::thread(&ConsoleUI::DisplayThreadFunc, this);
  
  DisplayHelp();
}

void ConsoleUI::Stop() {
  if (!_running.exchange(false)) {
    return;
  }
  
  // Wake up the display thread
  {
    std::lock_guard<std::mutex> lock(_display_queue_mutex);
    _display_queue.push("Exiting...");
    _display_cv.notify_one();
  }
  
  // Wait for threads to finish
  if (_input_thread.joinable()) {
    _input_thread.join();
  }
  
  if (_display_thread.joinable()) {
    _display_thread.join();
  }
}

void ConsoleUI::DisplayMessage(const std::string& message) {
  std::lock_guard<std::mutex> lock(_display_queue_mutex);
  _display_queue.push(message);
  _display_cv.notify_one();
}

void ConsoleUI::RegisterCommand(const std::string& command, 
                              CommandHandler handler,
                              const std::string& description) {
  std::lock_guard<std::mutex> lock(_commands_mutex);
  _commands[command] = std::make_pair(handler, description);
}

void ConsoleUI::InputThreadFunc() {
  while (_running) {
    std::string input;
    std::getline(std::cin, input);
    
    if (!_running) {
      break;
    }
    
    if (input.empty()) {
      continue;
    }
    
    if (input[0] == '/') {
      // This is a command
      ProcessCommand(input.substr(1));  // Remove the leading '/'
    } else {
      // This is a broadcast message
      DisplayMessage("Broadcasting message: " + input);
      
      // Create sender ID (for now, just a random ID)
      PeerId sender_id;
      std::random_device rd;
      std::generate(sender_id.begin(), sender_id.end(), std::ref(rd));
      
      // Create and broadcast the message
      ChatMessage message(sender_id, input);
      _network_manager->BroadcastMessage(message);
    }
  }
}

void ConsoleUI::DisplayThreadFunc() {
  while (_running) {
    std::string message;
    
    {
      std::unique_lock<std::mutex> lock(_display_queue_mutex);
      _display_cv.wait(lock, [this] { return !_display_queue.empty() || !_running; });
      
      if (!_running && _display_queue.empty()) {
        break;
      }
      
      message = _display_queue.front();
      _display_queue.pop();
    }
    
    // Display the message
    std::cout << message << std::endl;
  }
}

void ConsoleUI::ProcessCommand(const std::string& input) {
  std::istringstream iss(input);
  std::vector<std::string> args;
  std::string arg;
  
  while (iss >> arg) {
    args.push_back(arg);
  }
  
  if (args.empty()) {
    return;
  }
  
  std::string command = args[0];
  
  std::lock_guard<std::mutex> lock(_commands_mutex);
  auto it = _commands.find(command);
  
  if (it == _commands.end()) {
    DisplayMessage("Unknown command: " + command);
    DisplayHelp();
    return;
  }
  
  try {
    it->second.first(args);
  } catch (const std::exception& e) {
    DisplayMessage("Error executing command: " + std::string(e.what()));
  }
}

void ConsoleUI::DisplayHelp() {
  DisplayMessage("Available commands:");
  
  std::lock_guard<std::mutex> lock(_commands_mutex);
  for (const auto& [command, handler_desc] : _commands) {
    DisplayMessage("  /" + command + " - " + handler_desc.second);
  }
}

}  // namespace linknet
