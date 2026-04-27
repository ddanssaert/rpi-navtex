#pragma once
#include <sdrplay_api.h>
#include <atomic>
#include <mutex>
#include <thread>
#include <memory>

// Forward decl to avoid pulling httplib.h into the header
namespace httplib { class Server; }

class ControlServer {
public:
    ControlServer(sdrplay_api_DeviceT* device,
                  sdrplay_api_DeviceParamsT* params,
                  std::mutex& sdr_mutex);
    ~ControlServer();

    // Starts a background thread that listens on host:port. Non-blocking.
    void start(const char* host, int port);

    // Stops the server and joins the thread.
    void stop();

    // Set true once sdrplay_api_Init has returned successfully; control endpoint
    // returns 503 until this is true.
    std::atomic<bool> streaming_active{false};

private:
    sdrplay_api_DeviceT* device_;
    sdrplay_api_DeviceParamsT* params_;
    std::mutex& sdr_mutex_;
    std::unique_ptr<httplib::Server> svr_;
    std::thread thr_;
};
