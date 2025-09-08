#!/bin/bash

# Simple script to run GUI tests without Xvfb
# This works if you're running in a terminal with access to the display

echo "Running GUI tests directly..."
cd build

# Check if GUI test executable exists
if [ ! -f "./src/applications/gui_test/gui_test" ]; then
    echo "❌ GUI test executable not found. Make sure to build first: make -j6"
    exit 1
fi

# Run the test
./src/applications/gui_test/gui_test

EXIT_CODE=$?

if [ $EXIT_CODE -eq 0 ]; then
    echo "✅ All GUI tests passed!"
else
    echo "❌ GUI tests failed with exit code $EXIT_CODE"
fi

exit $EXIT_CODE