# SimpleChat P2P with DSDV Routing - Programming Assignment 3

**Student:** Rajni Pawar
**Course:** CS 550 - Advanced Operating Systems
**Semester:** Fall 2025

## Overview

This assignment enhances the SimpleChat P2P implementation from PA2 by adding:
1. **DSDV (Destination-Sequenced Distance-Vector) Routing** for point-to-point message forwarding
2. **Route Rumor Propagation** for automatic route discovery
3. **NAT Traversal** capabilities via LastIP/LastPort tracking
4. **Rendezvous Server Mode** (`--noforward`) for facilitating connections behind NATs

## Key Features

### Part 1: DSDV Routing

- **Routing Table**: Each node maintains routes to all known nodes with sequence numbers
- **Route Rumors**: Nodes broadcast their presence every 60 seconds
- **Private Messaging**: Direct node-to-node messages with automatic routing
- **Message Forwarding**: Multi-hop message delivery with hop limit (default: 10 hops)
- **Route Updates**: DSDV update rules - prefer higher sequence numbers, or direct routes with same sequence

### Part 2: NAT Traversal

- **LastIP/LastPort**: All rumors include the last hop's IP and port for NAT discovery
- **Rendezvous Server**: `--noforward` mode forwards route rumors but not chat messages
- **Direct Route Preference**: When same sequence number, prefer direct connections
- **Public Endpoint Discovery**: Nodes learn their public IP/port through rumor propagation

### Protocol Enhancements

- **Message Types**:
  - `CHAT_MESSAGE`: Regular chat messages with routing
  - `ROUTE_RUMOR`: Periodic route announcements
  - `ANTI_ENTROPY_REQUEST/RESPONSE`: PA2's synchronization (retained)
  - `ACK`: Acknowledgments for reliable delivery

- **Message Fields**:
  - `Origin`: Source node ID
  - `Dest`: Destination node ID (or "broadcast")
  - `SeqNo`: Sequence number for DSDV
  - `HopLimit`: Decremented at each hop (prevents loops)
  - `LastIP`: Last hop's IP address
  - `LastPort`: Last hop's port

## Project Structure

```
02_Pawar_Rajni_PA3/
├── CMakeLists.txt              # Build configuration
├── README.md                   # This file
├── src/                        # Source files
│   ├── main.cpp               # Entry point with CLI parsing
│   ├── simplechat.h/cpp       # Main controller
│   ├── chatwindow.h/cpp       # Qt GUI
│   ├── networkmanager.h/cpp   # Networking & routing logic
│   └── message.h/cpp          # Message data structure
├── scripts/                    # Helper scripts
│   ├── build.sh               # Build the project
│   ├── launch_3_nodes.sh      # Test local routing
│   ├── test_nat_traversal.sh  # Test NAT scenario
│   └── stop_all.sh            # Stop all instances
└── build/                      # Build output (generated)
```

## Prerequisites

### macOS
```bash
# Install Qt6 using Homebrew
brew install qt@6

# Install CMake
brew install cmake

# Add Qt to PATH
export PATH="/opt/homebrew/opt/qt@6/bin:$PATH"
```

### Linux (Ubuntu/Debian)
```bash
# Install Qt6
sudo apt-get update
sudo apt-get install qt6-base-dev qt6-tools-dev cmake build-essential

# Or for Qt5
sudo apt-get install qtbase5-dev qttools5-dev cmake build-essential
```

## Building the Project

### Using the Build Script (Recommended)
```bash
cd 02_Pawar_Rajni_PA3
./scripts/build.sh
```

### Manual Build
```bash
mkdir build
cd build
cmake ..
make -j$(nproc)
```

The executable will be: `build/SimpleChat_PA3`

## Running the Application

### Command Line Options

```bash
./build/SimpleChat_PA3 [OPTIONS]
```

**Options:**
- `-p, --port <port>`: Port number for this node (default: 9001)
- `--peers <ports>`: Comma-separated peer ports (e.g., `9001,9002,9003`)
- `--connect <port>`: Connect to a specific peer (e.g., rendezvous server)
- `--noforward`: Run as rendezvous server (forward route rumors only, not chat messages)
- `-h, --help`: Show help
- `-v, --version`: Show version

### Example Usage

**Single node:**
```bash
./build/SimpleChat_PA3 -p 9001
```

**Node with peer discovery:**
```bash
./build/SimpleChat_PA3 -p 9001 --peers 9001,9002,9003
```

**Rendezvous server:**
```bash
./build/SimpleChat_PA3 -p 45678 --noforward
```

**Node connecting to rendezvous:**
```bash
./build/SimpleChat_PA3 -p 11111 --connect 45678
```

## Testing Instructions

### Test 1: Local 3-Node Routing

**Objective:** Verify DSDV routing with 3 nodes

```bash
./scripts/launch_3_nodes.sh
```

**Procedure:**
1. Launch script starts 3 nodes (ports 9001, 9002, 9003)
2. Wait 60 seconds for initial route rumors
3. On Node 9001: Select "Node9003" from dropdown
4. Send a private message: "Hello Node 3!"
5. **Expected:** Message appears on Node 9003
6. Check routing table output in console for route via Node 9002

**Verification:**
- Route rumors logged every 60 seconds
- Routing table updates visible in console
- Messages successfully routed through intermediate nodes

### Test 2: NAT Traversal with Rendezvous Server

**Objective:** Test rendezvous server facilitating connections

```bash
./scripts/test_nat_traversal.sh
```

**Setup:**
- Rendezvous Server: port 45678 (noforward mode)
- Node N1: port 11111
- Node N2: port 22222

**Procedure:**
1. Script launches rendezvous with `--noforward`
2. N1 and N2 connect to rendezvous
3. Wait 60-120 seconds for route rumors to propagate
4. On N1: Check that "Node22222" appears in dropdown
5. Send message from N1 to N2
6. **Expected:** N2 receives message despite rendezvous's `--noforward`

**Verification:**
- Route rumors propagate through rendezvous
- N1 and N2 discover each other's routes
- Chat messages route directly (not through rendezvous)
- Rendezvous console shows route rumors but NOT chat messages

### Test 3: Route Rumor Propagation

**Objective:** Verify periodic route announcements

**Procedure:**
1. Launch any 2+ nodes
2. Observe console output
3. **Expected:** Route rumors every 60 seconds
4. Each rumor includes Origin, SeqNo, LastIP, LastPort
5. Sequence numbers increment with each rumor

**Sample Console Output:**
```
Sending route rumor: Origin=Node9001 SeqNo=1
Received route rumor: Origin=Node9002 SeqNo=1 from Node9002 (127.0.0.1:9002)
Updated route to Node9002 via Node9002 (127.0.0.1:9002) SeqNo=1 Direct=true
```

### Test 4: Message Forwarding with Hop Limit

**Objective:** Test hop limit prevents infinite loops

**Procedure:**
1. Launch 3+ nodes in a line topology
2. Send message with long route
3. Set hop limit to a low value (modify Message constructor if needed)
4. **Expected:** Message dropped when hop limit reaches 0

**Console Output:**
```
Forwarding message to Node9003 via next hop Node9002 (127.0.0.1:9002)
Message hop limit reached, dropping
```

### Test 5: Direct Route Preference

**Objective:** Verify direct routes preferred over indirect

**Procedure:**
1. Launch 3 nodes
2. Wait for route rumors (all direct initially)
3. Stop middle node temporarily
4. Observe route updates
5. Restart middle node
6. **Expected:** Routes update to prefer direct connections

## Implementation Details

### DSDV Routing Table

Each routing table entry contains:
```cpp
struct RouteInfo {
    QString nextHop;      // Next hop node ID
    QString nextHopIP;    // Next hop IP address
    quint16 nextHopPort;  // Next hop port
    int seqNo;            // Sequence number from origin
    bool isDirect;        // Is this a direct route?
    qint64 lastUpdated;   // Timestamp
};
```

**Update Logic:**
```cpp
if (newSeqNo > currentSeqNo) {
    updateRoute();  // Always prefer higher sequence number
} else if (newSeqNo == currentSeqNo && isDirect && !currentIsDirect) {
    updateRoute();  // Prefer direct route with same sequence
}
```

### Route Rumor Format

```json
{
  "Origin": "Node9001",
  "SeqNo": 23,
  "Type": 4,
  "LastIP": "127.0.0.1",
  "LastPort": 9001
}
```

### Private Message Format

```json
{
  "Origin": "NodeA",
  "Dest": "NodeB",
  "ChatText": "Hello",
  "SeqNo": 5,
  "HopLimit": 10,
  "LastIP": "127.0.0.1",
  "LastPort": 9001
}
```

### Message Forwarding Algorithm

```cpp
bool forwardMessage(Message& message) {
    // Check hop limit
    if (message.getHopLimit() == 0) {
        drop();
        return false;
    }

    // Decrement hop limit
    message.setHopLimit(message.getHopLimit() - 1);

    // Look up route
    if (!routingTable.contains(dest)) {
        return false;
    }

    // Forward to next hop
    RouteInfo route = routingTable[dest];
    send(route.nextHopIP, route.nextHopPort, message);
    return true;
}
```

### Rendezvous Server (--noforward mode)

```cpp
void handleChatMessage(const Message& message) {
    if (noForwardMode && !message.getChatText().isEmpty()) {
        // Drop chat messages in rendezvous mode
        return;
    }
    // Route rumors always forwarded
    processMessage(message);
}
```

## Network Namespace Testing (Optional - Linux Only)

For more realistic NAT simulation on Linux:

```bash
# Create NAT environments
sudo ip netns add nat1
sudo ip netns add nat2

# Configure NAT
sudo ip netns exec nat1 iptables -t nat -A POSTROUTING -j MASQUERADE
sudo ip netns exec nat2 iptables -t nat -A POSTROUTING -j MASQUERADE --random

# Run rendezvous server normally
./build/SimpleChat_PA3 -p 45678 --noforward &

# Run nodes in NAT namespaces
sudo ip netns exec nat1 ./build/SimpleChat_PA3 -p 11111 --connect 45678 &
sudo ip netns exec nat2 ./build/SimpleChat_PA3 -p 22222 --connect 45678 &
```

**Note:** This is advanced testing. The regular test scripts demonstrate the core functionality without requiring network namespaces.

## Troubleshooting

### Port Already in Use
```bash
# Find and kill process
lsof -i :9001
kill -9 <PID>

# Or use stop script
./scripts/stop_all.sh
```

### Qt Not Found
```bash
# Set Qt path for CMake
export CMAKE_PREFIX_PATH="/opt/homebrew/opt/qt@6"
# or
export CMAKE_PREFIX_PATH="/usr/lib/x86_64-linux-gnu/cmake/Qt6"
```

### Routes Not Updating
- Wait at least 60 seconds for route rumors
- Check console for "Sending route rumor" messages
- Verify nodes are connected (peer discovery)
- Ensure no firewall blocking UDP

### Messages Not Being Forwarded
- Check hop limit hasn't reached 0
- Verify routing table has entry for destination
- Look for "No route to destination" in console
- Ensure intermediate nodes are running

## Known Limitations

1. **Local network testing only**: Currently configured for localhost
2. **No persistent storage**: Routing tables reset on restart
3. **Fixed route rumor interval**: 60 seconds (not configurable at runtime)
4. **UDP only**: No TCP fallback
5. **No encryption**: Messages sent in plaintext
6. **No authentication**: No verification of node identity

## Key Differences from PA2

| Feature | PA2 | PA3 |
|---------|-----|-----|
| Messaging | Broadcast + Direct | DSDV Routed Private Messages |
| Route Discovery | Manual peer addition | Automatic via route rumors |
| Multi-hop | Anti-entropy only | Full message forwarding |
| NAT Support | None | LastIP/LastPort + Rendezvous |
| Server Mode | None | --noforward for rendezvous |
| Hop Limit | None | 10 hops with decrement |

## Submission Checklist

- [x] DSDV routing table implementation
- [x] Route rumor generation (60s intervals)
- [x] Route rumor processing and forwarding
- [x] Private messaging with Dest, Origin, HopLimit
- [x] Message forwarding with hop limit decrement
- [x] --noforward mode for rendezvous server
- [x] LastIP and LastPort in all rumors
- [x] Direct route preference logic
- [x] Build scripts
- [x] Testing scripts for local and NAT scenarios
- [x] Comprehensive README with examples
- [x] Working implementation

## References

- DSDV Protocol: C.E. Perkins and P. Bhagwat, "Highly Dynamic Destination-Sequenced Distance-Vector Routing (DSDV) for Mobile Computers"
- NAT Traversal: RFC 5128 - "State of Peer-to-Peer Communication across NATs"
- Qt Documentation: https://doc.qt.io/qt-6/
- Programming Assignment 2: Previous implementation (anti-entropy, broadcast)

## Contact

For questions or issues:
- **Student:** Rajni Pawar
- **Course:** CS 550 - Advanced Operating Systems
- **Semester:** Fall 2025

---

**Note:** This implementation is for educational purposes as part of CS 550 coursework.
