#!/bin/bash
# Simple HTTP server for testing PSP Web File Browser locally
# This simulates the server at softa.site for development/testing

PORT=8080

echo "================================================"
echo "PSP Web File Browser - Local Test Server"
echo "================================================"
echo ""
echo "This server simulates http://softa.site for testing"
echo "Server will run on: http://localhost:$PORT"
echo ""
echo "Test URLs:"
echo "  http://localhost:$PORT/psp"
echo "  http://localhost:$PORT/pspfiles"
echo "  http://localhost:$PORT/pspfiles/[filename]"
echo ""
echo "Press Ctrl+C to stop the server"
echo "================================================"
echo ""

cd server

# Check if Python 3 is available
if command -v python3 &> /dev/null; then
    echo "Starting Python 3 HTTP server..."
    python3 -m http.server $PORT
elif command -v python &> /dev/null; then
    echo "Starting Python 2 HTTP server..."
    python -m SimpleHTTPServer $PORT
else
    echo "Error: Python is not installed"
    echo "Please install Python 3 to run this test server"
    exit 1
fi
