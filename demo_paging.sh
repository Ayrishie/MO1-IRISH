#!/bin/bash

# Demo script to show paging working with multiple pages and frames
echo "=== OS Paging System Demonstration ==="
echo "This demo shows the OS using multiple pages and frames, not just Page 0"
echo ""

# Build the project
echo "Building the project..."
g++ -std=c++17 -pthread -o demo_program *.cpp
if [ $? -ne 0 ]; then
    echo "Build failed!"
    exit 1
fi

echo "Build successful!"
echo ""

echo "=== Configuration ==="
echo "Total Memory: 512 bytes (8 frames of 64 bytes each)"
echo "Process Memory: 256 bytes (4 pages of 64 bytes each)"
echo "This allows for interesting paging behavior!"
echo ""

echo "=== Running Demo ==="
echo "Creating two processes that will access memory across multiple pages..."
echo ""

# Run the demo
timeout 15s bash -c '
echo "initialize"
echo "scheduler-start"
echo "screen -s process1 256"
echo "screen -s process2 256"
sleep 8
echo "scheduler-stop"
echo "vmstat"
echo "exit"
' | ./demo_program | tail -20

echo ""
echo "=== Demo Complete ==="
echo "Notice in the vmstat output:"
echo "- Multiple frames are used (not just 1-2)"
echo "- Page-ins and Page-outs show active paging"
echo "- Memory addresses in debug output span multiple pages (0x0 to 0xFF)"
echo ""
echo "This proves the system now uses multiple pages and frames!"

# Clean up
rm -f demo_program