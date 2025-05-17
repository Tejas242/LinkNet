#!/bin/bash

# Stop and remove existing containers
echo "Stopping any existing LinkNet containers..."
docker-compose down

# Clean up any dangling images
echo "Cleaning up Docker environment..."
docker system prune -f

# Build and start the Docker containers
echo "Building and starting LinkNet containers..."
docker-compose build --no-cache
docker-compose up -d

# Check which terminal emulator is available
TERMINAL=""
if command -v gnome-terminal &> /dev/null; then
    TERMINAL="gnome-terminal"
    echo "Using gnome-terminal"
    
    echo "Opening terminal for node1..."
    gnome-terminal --tab --title="LinkNet Node 1" -- docker attach linknet-node1 &
    sleep 1
    
    echo "Opening terminal for node2..."
    gnome-terminal --tab --title="LinkNet Node 2" -- docker attach linknet-node2 &
    
elif command -v xterm &> /dev/null; then
    TERMINAL="xterm"
    echo "Using xterm"
    
    echo "Opening terminal for node1..."
    xterm -T "LinkNet Node 1" -e "docker attach linknet-node1" &
    sleep 1
    
    echo "Opening terminal for node2..."
    xterm -T "LinkNet Node 2" -e "docker attach linknet-node2" &
    
elif command -v konsole &> /dev/null; then
    TERMINAL="konsole"
    echo "Using konsole"
    
    echo "Opening terminal for node1..."
    konsole --new-tab -p tabtitle="LinkNet Node 1" -e docker attach linknet-node1 &
    sleep 1
    
    echo "Opening terminal for node2..."
    konsole --new-tab -p tabtitle="LinkNet Node 2" -e docker attach linknet-node2 &
    
else
    echo "No supported terminal emulator found. Please manually attach to containers with:"
    echo "docker attach linknet-node1"
    echo "docker attach linknet-node2"
fi

echo "Both nodes are running. You can interact with them in the opened terminals."
echo "To connect from node1 to node2, use: /connect 172.28.0.3:8081"
echo "To connect from node2 to node1, use: /connect 172.28.0.2:8080"
