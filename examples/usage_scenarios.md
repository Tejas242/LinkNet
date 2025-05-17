# LinkNet Usage Scenarios

This document outlines various usage scenarios for the LinkNet P2P application, demonstrating how to use different features for secure communication and file sharing.

## Basic Setup

### Scenario 1: Starting Two LinkNet Nodes

#### Node A (Alice)
```bash
# Start a LinkNet node on port 8080
./linknet --port=8080
```

#### Node B (Bob)
```bash
# Start a LinkNet node on port 8081
./linknet --port=8081
```

### Connecting Nodes Manually

#### On Node B (Bob)
```
# Connect to Node A
/connect 192.168.1.100:8080
```

The console will display:
```
Connecting to 192.168.1.100:8080...
Peer connected: 7a8b9c0d1e2f3g4h5i6j7k8l9m0n1o2p3q4r5s6t7u8v9w0x1y2z3a4b5c6d
```

#### On Node A (Alice)
```
Peer connected: 1a2b3c4d5e6f7g8h9i0j1k2l3m4n5o6p7q8r9s0t1u2v3w4x5y6z7a8b9c0d
```

## Chat Communication

### Scenario 2: Sending Messages Between Peers

#### On Node A (Alice)
```
# List connected peers
/peers

# Output:
Connected peers:
ID: 1a2b3c4d5e6f7g8h9i0j1k2l3m4n5o6p7q8r9s0t1u2v3w4x5y6z7a8b9c0d | Name: Bob | IP: 192.168.1.101:8081

# Send a message to Bob
/chat 1a2b3c4d5e6f7g8h9i0j1k2l3m4n5o6p7q8r9s0t1u2v3w4x5y6z7a8b9c0d Hello Bob, how are you today?

# Output:
Message sent
```

#### On Node B (Bob)
```
# Message received:
Message from peer: Hello Bob, how are you today?

# Reply to Alice
/chat 7a8b9c0d1e2f3g4h5i6j7k8l9m0n1o2p3q4r5s6t7u8v9w0x1y2z3a4b5c6d I'm doing great, thanks for asking!

# Output:
Message sent
```

#### On Node A (Alice)
```
# Message received:
Message from peer: I'm doing great, thanks for asking!
```

### Scenario 3: Broadcasting Messages to All Connected Peers

#### On Node A (Alice)
```
# Simply type a message without a command to broadcast it
Important announcement for everyone!

# Output:
Broadcasting message: Important announcement for everyone!
```

#### On All Connected Nodes (including Bob)
```
# Message received:
Message from peer: Important announcement for everyone!
```

## File Transfer

### Scenario 4: Sending a File to a Peer

#### On Node A (Alice)
```
# Send a file to Bob
/send 1a2b3c4d5e6f7g8h9i0j1k2l3m4n5o6p7q8r9s0t1u2v3w4x5y6z7a8b9c0d /home/alice/documents/project_proposal.pdf

# Output:
Sending file /home/alice/documents/project_proposal.pdf to peer...
File transfer progress for /home/alice/documents/project_proposal.pdf: 10.0%
File transfer progress for /home/alice/documents/project_proposal.pdf: 25.0%
File transfer progress for /home/alice/documents/project_proposal.pdf: 50.0%
File transfer progress for /home/alice/documents/project_proposal.pdf: 75.0%
File transfer progress for /home/alice/documents/project_proposal.pdf: 100.0%
File transfer completed for /home/alice/documents/project_proposal.pdf
```

#### On Node B (Bob)
```
# Automatic display of file transfer request and progress
File transfer request from 7a8b9c0d1e2f3g4h5i6j7k8l9m0n1o2p3q4r5s6t7u8v9w0x1y2z3a4b5c6d: project_proposal.pdf (2.5 MB)
Automatically accepting file transfer
File transfer progress for project_proposal.pdf: 10.0%
File transfer progress for project_proposal.pdf: 25.0%
File transfer progress for project_proposal.pdf: 50.0%
File transfer progress for project_proposal.pdf: 75.0%
File transfer progress for project_proposal.pdf: 100.0%
File transfer completed for project_proposal.pdf
```

### Scenario 5: Checking Ongoing Transfers

```
# Check the status of all ongoing transfers
/transfers

# Output if transfers are in progress:
Ongoing file transfers:
File: /home/alice/photos/vacation.jpg | Status: 1 | Progress: 45.2%

# Output if no transfers:
No ongoing file transfers
```

## Automatic Peer Discovery

### Scenario 6: Discovering Peers on LAN

#### Node C (Charlie) - New Node
```bash
# Start a LinkNet node on port 8082, peer discovery will start automatically
./linknet --port=8082

# After a moment, discovered peers will appear
Discovered peer at 192.168.1.100:8080
Automatically connecting to peer...
Peer connected: 7a8b9c0d1e2f3g4h5i6j7k8l9m0n1o2p3q4r5s6t7u8v9w0x1y2z3a4b5c6d

Discovered peer at 192.168.1.101:8081
Automatically connecting to peer...
Peer connected: 1a2b3c4d5e6f7g8h9i0j1k2l3m4n5o6p7q8r9s0t1u2v3w4x5y6z7a8b9c0d
```

#### On Existing Nodes (A and B)
```
Discovered peer at 192.168.1.102:8082
Automatically connecting to peer...
Peer connected: 2a3b4c5d6e7f8g9h0i1j2k3l4m5n6o7p8q9r0s1t2u3v4w5x6y7z8a9b0c1d
```

## Security Features

### Scenario 7: Encryption in Action

All communications in LinkNet are automatically encrypted. When messages and files are transferred:

1. Keys are exchanged securely using asymmetric encryption
2. Messages and file chunks are encrypted with symmetric encryption
3. Digital signatures verify the authenticity of the sender

This happens transparently to users - you don't need to perform any manual steps to secure your communications.

## Error Handling

### Scenario 8: Handling Connection Errors

#### Attempting to Connect to an Unavailable Peer
```
/connect 192.168.1.200:8080

# Output:
Connecting to 192.168.1.200:8080...
Network error: Connection refused
```

#### When a Peer Disconnects
```
Peer disconnected: 1a2b3c4d5e6f7g8h9i0j1k2l3m4n5o6p7q8r9s0t1u2v3w4x5y6z7a8b9c0d
```

### Scenario 9: File Transfer Errors

```
/send 1a2b3c4d5e6f7g8h9i0j1k2l3m4n5o6p7q8r9s0t1u2v3w4x5y6z7a8b9c0d /home/alice/nonexistent.txt

# Output:
Sending file /home/alice/nonexistent.txt to peer...
File transfer failed for /home/alice/nonexistent.txt: File not found
```

## Advanced Usage

### Scenario 10: Multiple File Transfers

LinkNet supports multiple simultaneous file transfers. You can send different files to different peers at the same time:

```
/send 1a2b3c4d5e6f7g8h9i0j1k2l3m4n5o6p7q8r9s0t1u2v3w4x5y6z7a8b9c0d /home/alice/document1.pdf
/send 2a3b4c5d6e7f8g9h0i1j2k3l4m5n6o7p8q9r0s1t2u3v4w5x6y7z8a9b0c1d /home/alice/document2.pdf

# Output:
Sending file /home/alice/document1.pdf to peer...
Sending file /home/alice/document2.pdf to peer...

# Use /transfers to monitor both transfers:
/transfers

Ongoing file transfers:
File: /home/alice/document1.pdf | Status: 1 | Progress: 32.5%
File: /home/alice/document2.pdf | Status: 1 | Progress: 18.7%
```

### Scenario 11: Exiting the Application

```
/exit

# Output:
Exiting...
```

## Performance Considerations

- **Large Files**: When transferring very large files (>1GB), expect the transfer to take time proportional to your network speed
- **Many Connections**: LinkNet can handle multiple peer connections, but performance may degrade with very large numbers of simultaneous connections (>50)
- **Network Conditions**: NAT traversal techniques help establish connections through firewalls, but in very restrictive networks, manual port forwarding may be necessary

## Troubleshooting Tips

1. **Connection Issues**: Ensure the port is not blocked by firewalls
2. **Peer Discovery Not Working**: Check that UDP multicast is supported on your network
3. **Transfer Speeds**: Slow transfers may indicate network congestion or throttling
4. **Application Crashes**: Check the `linknet.log` file for detailed error information
