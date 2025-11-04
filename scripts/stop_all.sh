#!/bin/bash

# Stop all running SimpleChat instances
# This can be called manually or by Ctrl+C in launch scripts

echo "Stopping all SimpleChat PA3 instances..."

# Kill by executable name
pkill -9 -f "SimpleChat_PA3" 2>/dev/null
pkill -9 -f "SimpleChat_P2P" 2>/dev/null

# Wait a moment for processes to terminate
sleep 0.5

# Check if any are still running
REMAINING=$(ps aux | grep -E "[S]impleChat_PA3|[S]impleChat_P2P" | wc -l | tr -d ' ')

if [ "$REMAINING" -eq "0" ]; then
    echo "✓ All SimpleChat instances stopped successfully."
else
    echo "⚠ Warning: $REMAINING SimpleChat process(es) may still be running."
    echo "You can manually kill them with: killall -9 SimpleChat_PA3"
fi

exit 0
