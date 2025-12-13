#!/bin/bash
# Simple script to serve the Dynamo Web UI
# This avoids CORS issues that occur when opening index.html directly

cd "$(dirname "$0")"
echo "Starting web server for Dynamo UI..."
echo "Open your browser to: http://localhost:3001"
echo "Press Ctrl+C to stop the server"
python3 -m http.server 3000
