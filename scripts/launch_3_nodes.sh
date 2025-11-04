#!/bin/bash

# Launch 3 nodes for testing PA3 routing

echo "Launching 3 SimpleChat PA3 nodes for local testing..."

# Clean up any existing processes first
echo "Cleaning up existing processes..."
./scripts/stop_all.sh > /dev/null 2>&1
sleep 1

echo "Press Ctrl+C to stop all nodes"

# Trap Ctrl+C to kill all child processes
trap 'echo ""; echo "Stopping all nodes..."; pkill -P $$; exit 0' INT

# Launch nodes in background
echo "Starting Node 9001..."
./build/SimpleChat_PA3 -p 9001 --peers 9001,9002,9003 &

sleep 1

echo "Starting Node 9002..."
./build/SimpleChat_PA3 -p 9002 --peers 9001,9002,9003 &

sleep 1

echo "Starting Node 9003..."
./build/SimpleChat_PA3 -p 9003 --peers 9001,9002,9003 &

echo ""
echo "All nodes started!"
echo ""
echo "Testing scenario:"
echo "- Wait 60 seconds for route rumors to propagate"
echo "- Try sending a private message from Node1 to Node3"
echo "- The message should be routed through the network"
echo ""
echo "Press Ctrl+C to stop all nodes"

# Wait for all background processes
wait
