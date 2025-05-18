# NAT Traversal

NAT (Network Address Translation) traversal in LinkNet enables peers to establish direct connections even when they are behind NATs or firewalls, allowing for true peer-to-peer communication.

## Overview

NAT traversal is a critical component of LinkNet's networking capabilities. It allows peers to establish direct connections with each other even when one or both peers are behind NAT devices, which is common in home and business networks.

## NAT Challenges

NAT devices present several challenges for peer-to-peer applications:

### Types of NAT

Different NAT types vary in restrictiveness:

| NAT Type | Description | Difficulty |
|----------|-------------|------------|
| **Full Cone** | After an internal host sends a packet, any external host can send to the mapped port | Easy |
| **Address-Restricted Cone** | External hosts can send to the mapped port only if internal host previously sent to that external IP | Moderate |
| **Port-Restricted Cone** | External hosts can send to the mapped port only if internal host previously sent to that specific IP:port | Difficult |
| **Symmetric** | For each destination, a different external port is used; only the specific external host can send back | Very Difficult |

### Double NAT Scenario

When both peers are behind NAT, establishing a connection is even more challenging:

```
Peer A                           Peer B
   │                               │
   ▼                               ▼
┌──────┐        Internet        ┌──────┐
│ NAT A │◄───────────────────────▶NAT B │
└──────┘                        └──────┘
```

## NAT Traversal Techniques

LinkNet implements several techniques to overcome these challenges:

### STUN (Session Traversal Utilities for NAT)

STUN allows peers to:
- Determine if they are behind a NAT
- Discover their public IP address and port mappings
- Determine the type of NAT they are behind

LinkNet uses STUN to:
1. Identify the external address (IP:port) for each peer
2. Share this information during connection establishment
3. Establish direct connections when NAT types allow

### UDP Hole Punching

The primary technique used for establishing direct peer-to-peer connections:

1. **Mapping Creation**: Both peers send outbound UDP packets to establish mappings in their NATs
2. **Address Exchange**: Peers exchange their public endpoints via a signaling channel
3. **Simultaneous Connection**: Both peers send packets to each other's public endpoints
4. **Hole Punching**: These outbound packets create "holes" in the NAT that allow incoming packets

```
┌─────────────┐                       ┌─────────────┐
│   Peer A    │                       │   Peer B    │
│ 10.0.1.2:1234│                       │ 10.0.2.3:5678│
└──────┬──────┘                       └──────┬──────┘
       │                                     │
       │                                     │
┌──────▼──────┐                       ┌──────▼──────┐
│    NAT A    │                       │    NAT B    │
│203.0.113.1:5000                     │198.51.100.1:6000
└──────┬──────┘                       └──────┬──────┘
       │                                     │
       │                Internet             │
       │                                     │
       │  1. A sends to B's public endpoint  │
       │────────────────────────────────────▶│
       │                                     │
       │  2. B sends to A's public endpoint  │
       │◀────────────────────────────────────│
       │                                     │
       │  3. Connection established          │
       │◀───────────────────────────────────▶│
```

### ICE (Interactive Connectivity Establishment)

A framework that combines various NAT traversal techniques:
- STUN for direct connectivity
- TURN for relay when direct connection is impossible
- Candidate collection, prioritization, and checking

LinkNet's ICE implementation:
1. Gathers address candidates (local, STUN-derived, TURN relays)
2. Exchanges candidates with the remote peer
3. Systematically checks pairs of candidates
4. Selects the best working pair for communication

### TURN (Traversal Using Relays around NAT)

When direct connection is impossible (e.g., symmetric NATs on both sides), TURN provides a fallback:
- Acts as a relay server
- Forwards traffic between peers
- Used only when necessary due to performance impact

## Connection Establishment Process

LinkNet establishes peer connections using the following process:

### 1. Local Information Gathering

Each peer collects information about itself:
- Local IP addresses and ports
- STUN-derived public IP address and port
- NAT type determination

### 2. Connection Initiation

When Peer A wants to connect to Peer B:
1. A sends a connection request with its information
2. B receives the request and collects its own information
3. B sends a response with its information
4. Both peers now have the necessary information to attempt direct connection

### 3. Connection Attempt Strategies

LinkNet tries multiple strategies in parallel:

```
┌───────────────────┐    ┌───────────────────┐    ┌───────────────────┐
│ Direct Connection │    │   UDP Hole Punch  │    │   TURN Relay      │
│                   │    │                   │    │                   │
│ A ────────────► B │    │ A ◄─────────────▶ B │    │ A ───▶ TURN ───▶ B │
└───────────────────┘    └───────────────────┘    └───────────────────┘
        1st try                  2nd try                  3rd try
```

### 4. Connection Convergence

As soon as one strategy succeeds:
1. The successful connection is established
2. Other attempts are terminated
3. The connection is tested and verified

## NAT Type Detection

LinkNet determines the NAT type to optimize traversal strategies:

### Detection Algorithm

1. Send a binding request to STUN server 1
2. Send a binding request to STUN server 2
3. Send a binding request to a different port on STUN server 1
4. Compare the mapped addresses from all responses
5. Apply logic to determine NAT type:
   - Same external address for all tests = no NAT
   - Same port for different servers, different port for different destination = address-restricted cone
   - Different ports for different destinations = symmetric NAT
   - Etc.

### Behavior Adaptation

Once the NAT type is known, LinkNet adapts its strategy:
- For full cone NATs: Direct connection works
- For address-restricted: Hole punching with extra packets
- For symmetric NATs: Try multiple mappings or fall back to TURN

## Implementation Details

### Concurrency Model

NAT traversal operations run in parallel for efficiency:
- Multiple connection attempts proceed concurrently
- Prioritization of promising candidates
- Quick abandonment of failed strategies

### Timeouts and Fallbacks

The system handles failed connection attempts gracefully:
- Reasonable timeouts for each strategy
- Automatic progression to next strategy
- Fallback to less optimal but more reliable methods

### Keep-Alive Mechanisms

Once established, connections are maintained through:
- Periodic keep-alive packets
- Detecting connection failures
- Automatic reconnection attempts

## Performance Considerations

Several optimizations ensure efficient NAT traversal:

- **Caching**: Remember successful strategies for peer combinations
- **Prediction**: Start with strategies most likely to work based on history
- **Parallelization**: Attempt multiple strategies simultaneously
- **Prioritization**: Try faster methods first, more reliable methods later

## Advanced Configuration

LinkNet allows customization of NAT traversal behavior:

### STUN Server Configuration

```
# Set primary and backup STUN servers
stun_servers:
  - stun.l.google.com:19302
  - stun1.l.google.com:19302
  - stun2.l.google.com:19302
```

### TURN Server Configuration

```
# Configure TURN server with credentials
turn_servers:
  - url: turn:turn.example.com:3478
    username: linknet-user
    credential: password123
  - url: turns:turn-secure.example.com:5349
    username: linknet-secure
    credential: password456
```

### Connection Strategy Parameters

```
# Adjust connection attempt parameters
connection:
  direct_timeout_ms: 500
  hole_punch_attempts: 5
  simultaneous_candidates: 3
  ice_gathering_timeout_ms: 1000
```

## Troubleshooting NAT Issues

Common issues and solutions:

| Issue | Symptoms | Solutions |
|-------|----------|-----------|
| **Symmetric NAT** | Can't establish direct connections | Use TURN relay, or try port prediction |
| **Firewall Blocking** | Connection attempts time out | Configure firewall to allow LinkNet traffic |
| **Port Exhaustion** | Connection fails after many peers | Implement port reuse, close unused connections |
| **NAT Timeouts** | Connections drop after periods of inactivity | Configure appropriate keep-alive intervals |

## Related Documentation

- [Network Layer](../components/network.md) - How NAT traversal fits into the network stack
- [Peer Discovery](../components/discovery.md) - Finding and connecting to peers
- [Configuration Options](../getting_started.md#configuration) - Setting up NAT traversal options
