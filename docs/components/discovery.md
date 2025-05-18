# Peer Discovery

The Peer Discovery system in LinkNet enables automatic discovery of peers on local and wide-area networks, facilitating connections without manual configuration.

## Overview

The peer discovery module provides mechanisms to find other LinkNet instances on both local networks and across the internet. It handles the initial discovery phase of establishing connections, working with various network types and NAT configurations.

## Key Components

### Discovery Interface

Located in `include/linknet/discovery.h`, this interface defines the discovery operations:

```cpp
class PeerDiscovery {
 public:
  virtual ~PeerDiscovery() = default;
  
  // Start the discovery service
  virtual bool Start() = 0;
  
  // Stop the discovery service
  virtual void Stop() = 0;
  
  // Actively search for peers
  virtual void RefreshPeers() = 0;
  
  // Get discovered peers
  virtual std::vector<PeerInfo> GetDiscoveredPeers() const = 0;
  
  // Set callbacks
  virtual void SetPeerDiscoveredCallback(
      std::function<void(const PeerInfo&)> callback) = 0;
  
  // Announce presence on the network
  virtual void AnnouncePresence() = 0;
};
```

### Discovery Methods

LinkNet implements multiple discovery methods:

- **Local Network Discovery**: Uses multicast DNS (mDNS) to find peers on the local network
- **Known Peers Storage**: Remembers previously connected peers
- **STUN-based Discovery**: Uses STUN servers to facilitate connections through NAT
- **Manual Entry**: Allows users to manually enter peer addresses

## Discovery Process

### Local Network Discovery

1. **Service Announcement**: Broadcasts presence on the local network using mDNS
2. **Service Browsing**: Listens for other peers announcing their presence
3. **Information Exchange**: Gathers connection information from discovered peers
4. **Validation**: Verifies discovered peer information
5. **Callback Invocation**: Notifies the application of discovered peers

### Wide Area Network Discovery

1. **STUN Request**: Contacts STUN servers to determine external IP/port
2. **Information Exchange**: Shares connection details with known peers
3. **Hole Punching**: Attempts to establish direct connections through NATs
4. **Relay Fallback**: Uses relay servers if direct connection fails

## Discovery Flow

```
┌──────────────┐                       ┌──────────────┐
│   Peer A     │                       │   Peer B     │
└──────┬───────┘                       └───────┬──────┘
       │                                       │
       │ Broadcast mDNS announcement           │
       │───────────────────────────────────────▶
       │                                       │
       │ Broadcast mDNS announcement           │
       │◀───────────────────────────────────────
       │                                       │
       │ Resolve Peer B information            │
       │───────────────────────────────────────▶
       │                                       │
       │                Peer B information     │
       │◀───────────────────────────────────────
       │                                       │
       │ TCP Connection attempt                │
       │───────────────────────────────────────▶
       │                                       │
       │          Connection established       │
       │◀──────────────────────────────────────▶
```

## Features

### Protocol-based Service Identification

LinkNet uses standardized service identifiers for discovery:
- **Service Type**: `_linknet._tcp` for TCP-based communication
- **Service Name**: Includes peer name and unique identifier
- **TXT Records**: Additional information about capabilities and version

### Peer Information Management

The system maintains detailed information about discovered peers:
- **Connection Status**: Whether the peer is reachable, connected, etc.
- **Capabilities**: What features the peer supports
- **Network Location**: IP addresses and ports where the peer can be reached
- **Last Seen**: When the peer was last active

### Auto-connect Mode

When enabled, LinkNet can automatically:
- Connect to previously known trusted peers
- Establish connections with discovered peers on local networks
- Maintain a persistent list of peers across restarts

## Implementation Details

### Thread Safety

The discovery system is thread-safe:
- Service browsing happens on a dedicated thread
- Callbacks are synchronized with the main application

### Resource Management

The discovery service manages network resources efficiently:
- **Periodic Announcements**: Rather than constant broadcasting
- **Throttled Discovery**: Limits frequency of network scans
- **Resource Cleanup**: Properly shuts down services when not needed

## Advanced Features

### NAT Type Detection

The system can detect various NAT configurations:
- **Open Internet**: No NAT, directly addressable
- **Full Cone NAT**: Most permissive, easiest to traverse
- **Restricted Cone NAT**: Restricts by source IP
- **Port Restricted NAT**: Restricts by source IP and port
- **Symmetric NAT**: Most restrictive, hardest to traverse

### Network Topology Awareness

Discovery adapts to different network environments:
- **LAN-only Mode**: Only discovers peers on the local network
- **WAN Mode**: Attempts to discover peers across the internet
- **Mixed Mode**: Optimized for both local and internet peers

## Performance Considerations

Several optimizations ensure efficient discovery:
- **Caching**: Avoids redundant lookups for recently discovered peers
- **Exponential Backoff**: Reduces network traffic over time
- **Prioritization**: Focuses on high-probability connection methods first

## Code Examples

### Initializing the Discovery Service

```cpp
// Create the discovery service
auto discovery = std::make_shared<MulticastDiscovery>(network_manager);

// Set callback for discovered peers
discovery->SetPeerDiscoveredCallback([&](const PeerInfo& peer) {
  std::cout << "Discovered peer: " << peer.name 
            << " at " << peer.ip_address << ":" << peer.port << std::endl;
  
  // Optionally connect to the peer automatically
  network_manager->ConnectToPeer(peer.ip_address, peer.port);
});

// Start the discovery service
if (!discovery->Start()) {
  std::cerr << "Failed to start discovery service" << std::endl;
}
```

### Manual Peer Discovery Refresh

```cpp
// Trigger an active search for peers
discovery->RefreshPeers();

// Wait a moment for results
std::this_thread::sleep_for(std::chrono::seconds(2));

// Get the discovered peers
auto peers = discovery->GetDiscoveredPeers();
std::cout << "Found " << peers.size() << " peers:" << std::endl;

for (const auto& peer : peers) {
  std::cout << "- " << peer.name << " (" << peer.ip_address << ":" 
            << peer.port << ") - Status: " << static_cast<int>(peer.status) << std::endl;
}
```

### Stopping Discovery

```cpp
// Stop the discovery service when not needed
discovery->Stop();
```

## Related Documentation

- [Network Layer](network.md) - How discovery integrates with connection establishment
- [NAT Traversal](../advanced/nat_traversal.md) - Detailed information on NAT traversal techniques
- [Security Model](../advanced/security_model.md) - Security considerations for peer discovery
