#!/bin/bash

# Script to run GUI tests with virtual framebuffer

# Find an available display number
DISPLAY_NUM=99
while [ $DISPLAY_NUM -lt 200 ]; do
    if ! pgrep -f "Xvfb :$DISPLAY_NUM" > /dev/null; then
        break
    fi
    DISPLAY_NUM=$((DISPLAY_NUM + 1))
done

echo "Starting Xvfb on display :$DISPLAY_NUM"

# Create a temporary directory for X11 sockets if needed
TEMP_X11_DIR="/tmp/.X11-unix-$$"
mkdir -p "$TEMP_X11_DIR" 2>/dev/null || true

# Start virtual framebuffer with better options
Xvfb :$DISPLAY_NUM -screen 0 1024x768x24 -ac +extension GLX +render -noreset > /tmp/xvfb.log 2>&1 &
XVFB_PID=$!

# Set display environment variable
export DISPLAY=:$DISPLAY_NUM

# Wait for Xvfb to start and verify it's running
echo "Waiting for Xvfb to start..."
for i in {1..20}; do
    if kill -0 $XVFB_PID 2>/dev/null && xdpyinfo -display :$DISPLAY_NUM >/dev/null 2>&1; then
        echo "Xvfb started successfully on display :$DISPLAY_NUM"
        break
    fi
    if [ $i -eq 20 ]; then
        echo "ERROR: Xvfb failed to start after 10 seconds"
        echo "Xvfb log:"
        cat /tmp/xvfb.log 2>/dev/null || echo "No Xvfb log available"
        kill $XVFB_PID 2>/dev/null || true
        exit 1
    fi
    sleep 0.5
done

# Run the GUI test
echo "Running GUI tests..."
cd build
./src/applications/gui_test/gui_test

# Store the exit code
EXIT_CODE=$?

# Clean up
echo "Cleaning up..."
kill $XVFB_PID 2>/dev/null || true
rm -f /tmp/xvfb.log
rm -rf "$TEMP_X11_DIR" 2>/dev/null || true

if [ $EXIT_CODE -eq 0 ]; then
    echo "✅ All GUI tests passed!"
else
    echo "❌ GUI tests failed with exit code $EXIT_CODE"
fi

exit $EXIT_CODE