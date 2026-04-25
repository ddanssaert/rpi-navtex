#!/bin/sh

# Start the SDRPlay API Service in the background
echo "Starting sdrplay_apiService..."
sdrplay_apiService &

# Give it a moment to initialize the shared memory
sleep 2

# Execute the application
echo "Starting sdr-dsp..."
exec /usr/local/bin/sdr-dsp
