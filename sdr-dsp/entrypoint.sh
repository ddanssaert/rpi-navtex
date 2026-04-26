#!/bin/sh

# Start the SDRPlay API Service in the background and log to both stdout and file
echo "Starting sdrplay_apiService..."
sdrplay_apiService 2>&1 | tee /tmp/sdrplay_service.log &
SERVICE_PID=$!

# Give it a moment to initialize
sleep 3

# Check if service is still running
if kill -0 $SERVICE_PID 2>/dev/null; then
    echo "sdrplay_apiService is running (PID: $SERVICE_PID)"
else
    echo "ERROR: sdrplay_apiService failed to start or crashed!"
    cat /tmp/sdrplay_service.log
    exit 1
fi

# Execute the application
echo "Starting sdr-dsp..."
exec /usr/local/bin/sdr-dsp
