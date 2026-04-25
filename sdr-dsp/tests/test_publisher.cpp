#include "../src/message_publisher.h"
#include <cassert>
#include <string>
#include <thread>
#include <chrono>
#include <cstring>
#include <cstdio>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>

// Minimal HTTP server to verify the POST request
int main() {
    // Start a tiny TCP server on a random port
    int server_fd = ::socket(AF_INET, SOCK_STREAM, 0);
    int opt = 1;
    setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
    struct sockaddr_in addr{};
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = INADDR_ANY;
    addr.sin_port = 0;  // let OS pick a port
    ::bind(server_fd, (struct sockaddr*)&addr, sizeof(addr));
    socklen_t len = sizeof(addr);
    ::getsockname(server_fd, (struct sockaddr*)&addr, &len);
    int port = ntohs(addr.sin_port);
    ::listen(server_fd, 1);

    std::string url = "http://127.0.0.1:" + std::to_string(port) + "/messages";

    // POST in a background thread
    std::thread t([&] {
        std::this_thread::sleep_for(std::chrono::milliseconds(50));
        MessagePublisher pub(url);
        pub.publish("OB01", "ZCZC OB01\nTEST\nNNNN", 518000);
    });

    int client_fd = ::accept(server_fd, nullptr, nullptr);
    char buf[2048]{};
    ::read(client_fd, buf, sizeof(buf) - 1);

    // Send a minimal 200 OK response so curl doesn't error
    const char* response = "HTTP/1.1 200 OK\r\nContent-Length: 0\r\n\r\n";
    ::write(client_fd, response, strlen(response));

    t.join();
    ::close(client_fd);
    ::close(server_fd);

    std::string received(buf);
    // Verify it's a POST with JSON content
    assert(received.find("POST /messages") != std::string::npos);
    assert(received.find("\"station_id\":\"O\"") != std::string::npos);
    assert(received.find("\"message_type\":\"B\"") != std::string::npos);
    assert(received.find("\"serial_id\":1") != std::string::npos);
    assert(received.find("\"content\":") != std::string::npos);
    assert(received.find("\"raw_data\":") != std::string::npos);
    printf("test_publisher PASS\n");
    return 0;
}
