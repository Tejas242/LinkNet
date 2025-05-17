#include "linknet/network.h"
#include "linknet/file_transfer.h"
#include "linknet/console_ui.h"
#include "linknet/logger.h"
#include "linknet/crypto.h"
#include "linknet/chat_manager.h"
#include "linknet/discovery.h"
#include "linknet/message.h"
#include <memory>
#include <iostream>
#include <cstdlib>
#include <csignal>
#include <thread>
#include <chrono>

std::shared_ptr<linknet::ConsoleUI> g_ui;

void SignalHandler(int signal) {
  if (g_ui) {
    g_ui->Stop();
  }
  
  std::exit(signal);
}

void SetupSignalHandlers() {
  std::signal(SIGINT, SignalHandler);
  std::signal(SIGTERM, SignalHandler);
}

int main(int argc, char* argv[]) {
  // Parse command line arguments
  uint16_t port = 8080;  // Default port
  bool auto_connect = true;  // Default to auto-connect
  
  for (int i = 1; i < argc; ++i) {
    std::string arg = argv[i];
    if (arg.find("--port=") == 0) {
      std::string port_str = arg.substr(7);
      try {
        port = std::stoi(port_str);
      } catch (const std::exception& e) {
        std::cerr << "Invalid port number: " << port_str << std::endl;
        return 1;
      }
    } else if (arg.find("--auto-connect=") == 0) {
      std::string auto_connect_str = arg.substr(14);
      if (auto_connect_str == "false" || auto_connect_str == "0") {
        auto_connect = false;
      }
    } else if (arg == "--no-auto-connect") {
      auto_connect = false;
    } else if (arg == "--help" || arg == "-h") {
      std::cout << "LinkNet - P2P Chat and File Sharing System" << std::endl;
      std::cout << "Usage: " << argv[0] << " [OPTIONS]" << std::endl;
      std::cout << "Options:" << std::endl;
      std::cout << "  --port=PORT                Port to listen on (default: 8080)" << std::endl;
      std::cout << "  --auto-connect=true|false  Auto-connect to discovered peers (default: true)" << std::endl;
      std::cout << "  --no-auto-connect          Disable auto-connect to discovered peers" << std::endl;
      std::cout << "  --help, -h                 Show this help message" << std::endl;
      return 0;
    }
  }
  
  // Initialize logger
  linknet::Logger::GetInstance().SetLogLevel(linknet::LogLevel::INFO);
  linknet::Logger::GetInstance().SetLogFile("linknet.log");
  
  LOG_INFO("LinkNet starting on port ", port);
  
  try {
    // Initialize crypto
    auto crypto_provider = linknet::crypto::CryptoFactory::Create();
    
    // Set up network manager
    // Convert unique_ptr to shared_ptr since our other components require shared_ptr
    std::shared_ptr<linknet::NetworkManager> network_manager = 
        std::shared_ptr<linknet::NetworkManager>(linknet::NetworkFactory::Create().release());
    
    if (!network_manager->Start(port)) {
      LOG_FATAL("Failed to start network manager on port ", port);
      return 1;
    }
    
    // Set up chat manager
    auto chat_manager = std::make_shared<linknet::ChatManager>(network_manager);
    
    // Set up peer discovery
    auto peer_discovery = std::make_shared<linknet::PeerDiscovery>(network_manager);
    
    // Set up chat message callback
    chat_manager->SetMessageCallback([](const linknet::ChatInfo& chat_info) {
      std::stringstream peer_id_ss;
      for (const auto& byte : chat_info.sender_id) {
        peer_id_ss << std::hex << std::setw(2) << std::setfill('0') << static_cast<int>(byte);
      }
      
      LOG_INFO("Chat message from ", peer_id_ss.str(), ": ", chat_info.content);
      
      if (g_ui) {
        g_ui->DisplayColoredMessage("Message from peer: " + chat_info.content, linknet::TextColor::CYAN);
      }
    });
    
    // Set up message handling chain
    // First, create a handler for non-chat messages
    auto non_chat_handler = [network_manager](std::unique_ptr<linknet::Message> message) {
      switch (message->GetType()) {        
        case linknet::MessageType::CONNECTION_NOTIFICATION: {
          auto conn_msg = static_cast<linknet::ConnectionMessage&>(*message);
          const linknet::PeerId& sender_id = conn_msg.GetSender();
          
          std::stringstream peer_id_ss;
          for (const auto& byte : sender_id) {
            peer_id_ss << std::hex << std::setw(2) << std::setfill('0') << static_cast<int>(byte);
          }
          
          LOG_INFO("Connection notification from ", peer_id_ss.str(), 
                   ", status: ", static_cast<int>(conn_msg.GetStatus()));
          
          if (g_ui) {
            if (conn_msg.GetStatus() == linknet::ConnectionStatus::CONNECTED) {
              g_ui->DisplayColoredMessage("Peer connected: " + peer_id_ss.str(), linknet::TextColor::GREEN);
            } else {
              g_ui->DisplayColoredMessage("Peer disconnected: " + peer_id_ss.str(), linknet::TextColor::RED);
            }
          }
          break;
        }
        
        default:
          LOG_DEBUG("Received unhandled message type: ", static_cast<int>(message->GetType()));
          break;
      }
    };
    
    // Set the chat manager's next handler
    chat_manager->SetNextHandler(non_chat_handler);
    
    // Set the network manager's message callback to send everything to the chat manager
    network_manager->SetMessageCallback([chat_manager](std::unique_ptr<linknet::Message> message) {
      chat_manager->HandleMessage(std::move(message));
    });
    
    // Handle connection status changes
    network_manager->SetConnectionCallback([](const linknet::PeerId& peer_id, linknet::ConnectionStatus status) {
      std::stringstream peer_id_ss;
      for (const auto& byte : peer_id) {
        peer_id_ss << std::hex << std::setw(2) << std::setfill('0') << static_cast<int>(byte);
      }
      
      switch (status) {
        case linknet::ConnectionStatus::CONNECTED:
          LOG_INFO("Peer connected: ", peer_id_ss.str());
          if (g_ui) {
            g_ui->DisplayColoredMessage("Peer connected: " + peer_id_ss.str(), linknet::TextColor::GREEN);
          }
          break;
          
        case linknet::ConnectionStatus::DISCONNECTED:
          LOG_INFO("Peer disconnected: ", peer_id_ss.str());
          if (g_ui) {
            g_ui->DisplayColoredMessage("Peer disconnected: " + peer_id_ss.str(), linknet::TextColor::RED);
          }
          break;
          
        default:
          break;
      }
    });
    
    // Handle network errors
    network_manager->SetErrorCallback([](const std::string& error) {
      LOG_ERROR("Network error: ", error);
      if (g_ui) {
        g_ui->DisplayColoredMessage("Network error: " + error, linknet::TextColor::RED);
      }
    });
    
    // Set up file transfer manager
    // Convert unique_ptr to shared_ptr since our ConsoleUI requires shared_ptr
    std::shared_ptr<linknet::FileTransferManager> file_transfer_manager = 
        std::shared_ptr<linknet::FileTransferManager>(linknet::FileTransferFactory::Create(network_manager).release());
    
    // Handle file transfer progress
    file_transfer_manager->SetProgressCallback([](const linknet::PeerId& peer_id, 
                                                const std::string& file_path, 
                                                double progress) {
      std::stringstream peer_id_ss;
      for (const auto& byte : peer_id) {
        peer_id_ss << std::hex << std::setw(2) << std::setfill('0') << static_cast<int>(byte);
      }
      
      LOG_INFO("File transfer progress for ", file_path, ": ", 
               std::fixed, std::setprecision(1), progress * 100.0, "%");
      
      if (g_ui) {
        std::stringstream msg;
        msg << "File transfer progress for " << file_path << ": "
            << std::fixed << std::setprecision(1) << (progress * 100.0) << "%";
        g_ui->DisplayMessage(msg.str());
      }
    });
    
    // Handle file transfer completion
    file_transfer_manager->SetCompletedCallback([](const linknet::PeerId& peer_id, 
                                                 const std::string& file_path, 
                                                 bool success, 
                                                 const std::string& error) {
      std::stringstream peer_id_ss;
      for (const auto& byte : peer_id) {
        peer_id_ss << std::hex << std::setw(2) << std::setfill('0') << static_cast<int>(byte);
      }
      
      if (success) {
        LOG_INFO("File transfer completed for ", file_path);
        if (g_ui) {
          g_ui->DisplayColoredMessage("File transfer completed for " + file_path, linknet::TextColor::GREEN);
        }
      } else {
        LOG_ERROR("File transfer failed for ", file_path, ": ", error);
        if (g_ui) {
          g_ui->DisplayColoredMessage("File transfer failed for " + file_path + ": " + error, linknet::TextColor::RED);
        }
      }
    });
    
    // Handle file transfer requests
    file_transfer_manager->SetRequestCallback([](const linknet::PeerId& peer_id, 
                                               const std::string& filename, 
                                               uint64_t file_size) {
      std::stringstream peer_id_ss;
      for (const auto& byte : peer_id) {
        peer_id_ss << std::hex << std::setw(2) << std::setfill('0') << static_cast<int>(byte);
      }
      
      std::stringstream size_ss;
      if (file_size < 1024) {
        size_ss << file_size << " B";
      } else if (file_size < 1024 * 1024) {
        size_ss << (file_size / 1024.0) << " KB";
      } else if (file_size < 1024 * 1024 * 1024) {
        size_ss << (file_size / (1024.0 * 1024.0)) << " MB";
      } else {
        size_ss << (file_size / (1024.0 * 1024.0 * 1024.0)) << " GB";
      }
      
      LOG_INFO("File transfer request from ", peer_id_ss.str(), 
               ": ", filename, " (", size_ss.str(), ")");
      
      if (g_ui) {
        g_ui->DisplayColoredMessage("File transfer request from " + peer_id_ss.str() + 
                           ": " + filename + " (" + size_ss.str() + ")", linknet::TextColor::MAGENTA);
        g_ui->DisplayColoredMessage("Automatically accepting file transfer", linknet::TextColor::YELLOW);
      }
      
      return true;  // Always accept for now
    });
    
    // Set up console UI
    g_ui = std::make_shared<linknet::ConsoleUI>(network_manager, file_transfer_manager, chat_manager);
    
    // Set up signal handlers
    SetupSignalHandlers();
    
    // Start peer discovery
    if (!peer_discovery->Start(port)) {
      LOG_WARNING("Failed to start peer discovery; automatic peer finding disabled");
    } else {
      // Handle discovered peers
      peer_discovery->SetDiscoveredCallback([&](const std::string& ip, uint16_t peer_port) {
        LOG_INFO("Discovered peer at ", ip, ":", peer_port);
        if (g_ui) {
          g_ui->DisplayColoredMessage("Discovered peer at " + ip + ":" + std::to_string(peer_port), linknet::TextColor::CYAN);
          
          // Only auto-connect if the option is enabled
          if (auto_connect) {
            g_ui->DisplayColoredMessage("Automatically connecting to peer...", linknet::TextColor::YELLOW);
            // Attempt to connect to the peer
            network_manager->ConnectToPeer(ip, peer_port);
          } else {
            g_ui->DisplayColoredMessage("Auto-connect disabled. Use /connect " + ip + ":" + 
                                      std::to_string(peer_port) + " to connect manually", linknet::TextColor::GRAY);
          }
        }
        
        // Only attempt connection if auto-connect is enabled
        if (auto_connect) {
          network_manager->ConnectToPeer(ip, peer_port);
        }
      });
    }
    
    // Start the UI
    g_ui->Start();
    
    // Wait for the UI to exit
    while (g_ui->IsRunning()) {
      std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }
    
    // Clean up
    if (peer_discovery) {
      peer_discovery->Stop();
    }
    
    network_manager->Stop();
    
    LOG_INFO("LinkNet exiting");
    
    return 0;
  } catch (const std::exception& e) {
    LOG_FATAL("Unhandled exception: ", e.what());
    return 1;
  }
}
