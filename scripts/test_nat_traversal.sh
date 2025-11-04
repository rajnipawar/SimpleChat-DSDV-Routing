#!/bin/bash

# Test NAT traversal with rendezvous server

echo "=== SimpleChat PA3 - NAT Traversal Test ==="
echo ""

# Clean up any existing processes first
echo "Cleaning up existing processes..."
./scripts/stop_all.sh > /dev/null 2>&1
sleep 1

echo "This script demonstrates:"
echo "1. Rendezvous server in --noforward mode"
echo "2. Two regular nodes discovering each other"
echo "3. Route rumors propagating through rendezvous"
echo ""
echo "Press Ctrl+C to stop all nodes"
echo ""

# Trap Ctrl+C to kill all child processes
trap 'echo ""; echo "Stopping all nodes..."; pkill -P $$; exit 0' INT

# Launch rendezvous server with --noforward mode
echo "Starting Rendezvous Server (port 45678) with --noforward..."
./build/SimpleChat_PA3 -p 45678 --noforward &
RENDEZVOUS_PID=$!

sleep 2

# Launch Node N1 connecting to rendezvous
echo "Starting Node N1 (port 11111) connecting to rendezvous..."
./build/SimpleChat_PA3 -p 11111 --connect 45678 &
NODE1_PID=$!

sleep 2

# Launch Node N2 connecting to rendezvous
echo "Starting Node N2 (port 22222) connecting to rendezvous..."
./build/SimpleChat_PA3 -p 22222 --connect 45678 &
NODE2_PID=$!

echo ""
echo "=== Setup Complete ==="
echo ""
echo "Rendezvous Server: port 45678 (noforward mode)"
echo "Node N1: port 11111"
echo "Node N2: port 22222"
echo ""
echo "Testing procedure:"
echo "1. Wait 60-120 seconds for route rumors to propagate"
echo "2. Check that Node N1 discovers Node N2 (and vice versa)"
echo "3. Send a private message from Node N1 to Node N2"
echo "4. The message should be routed despite rendezvous's -noforward"
echo "5. Route rumors continue to propagate through rendezvous"
echo ""
echo "Press Ctrl+C to stop all nodes"

# Wait for all background processes
wait
