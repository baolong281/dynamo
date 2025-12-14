#!/bin/bash

# Kill background processes on exit
trap "exit" INT TERM
trap "kill 0" EXIT

# Build first
echo "Building project..."
mkdir -p build
cd build
cmake ..
make -j
cd ..

EXECUTABLE="./build/app"

if [ ! -f "$EXECUTABLE" ]; then
    echo "Error: $EXECUTABLE not found. Build failed?"
    exit 1
fi

echo "=================================================="
echo "Starting bootstrap node on port 8080..."
echo "=================================================="
$EXECUTABLE --port 8080 --address localhost 2>&1 | sed "s/^/[Node 8080] /" &
sleep 2 # Give it a moment to start

for i in {1..9}; do
    PORT=$((8080 + i))
    echo "Starting node on port $PORT..."
    $EXECUTABLE --port $PORT --address localhost --bootstrap-servers localhost:8080 2>&1 | sed "s/^/[Node $PORT] /" &
done

echo "Cluster running. Press Ctrl+C to stop all nodes."
wait
