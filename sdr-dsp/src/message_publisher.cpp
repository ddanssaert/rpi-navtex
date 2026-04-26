#include "message_publisher.h"
#include <json.hpp>
#include <ctime>
#include <cstdio>
#include <string>
#include <cstring>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <unistd.h>

MessagePublisher::MessagePublisher(const std::string& broker_url)
    : broker_url_(broker_url) {}

MessagePublisher::~MessagePublisher() {}

// Parse http://host:port/path from a URL string.
static bool parse_url(const std::string& url,
                      std::string& host, int& port, std::string& path) {
    // Expect: http://host[:port]/path
    if (url.compare(0, 7, "http://") != 0) return false;
    size_t start = 7;
    size_t slash = url.find('/', start);
    path = (slash != std::string::npos) ? url.substr(slash) : "/";
    std::string hostport = url.substr(start, (slash != std::string::npos) ? slash - start : std::string::npos);
    size_t colon = hostport.find(':');
    if (colon != std::string::npos) {
        host = hostport.substr(0, colon);
        port = std::stoi(hostport.substr(colon + 1));
    } else {
        host = hostport;
        port = 80;
    }
    return true;
}

void MessagePublisher::publish(const std::string& bbbb,
                               const std::string& message,
                               int freq) {
    // ISO-8601 timestamp
    std::time_t now = std::time(nullptr);
    char ts[25];
    std::strftime(ts, sizeof(ts), "%Y-%m-%dT%H:%M:%SZ", std::gmtime(&now));

    // Parse Navtex designator (B1B2B3B4)
    std::string station_id = "";
    std::string message_type = "";
    int serial_id = 0;
    if (bbbb.size() >= 2) {
        station_id += bbbb[0];
        message_type += bbbb[1];
    }
    if (bbbb.size() >= 4) {
        try {
            serial_id = std::stoi(bbbb.substr(2, 2));
        } catch (...) {
            // keep 0
        }
    }

    nlohmann::json j;
    j["station_id"]   = station_id;
    j["message_type"] = message_type;
    j["serial_id"]    = serial_id;
    j["content"]      = message;
    j["raw_data"]     = "ZCZC " + bbbb + "\n" + message + "\nNNNN";
    std::string payload = j.dump();

    std::string host, path;
    int port = 80;
    if (!parse_url(broker_url_, host, port, path)) {
        fprintf(stderr, "MessagePublisher: invalid broker URL: %s\n", broker_url_.c_str());
        return;
    }

    struct hostent* he = gethostbyname(host.c_str());
    if (!he) {
        fprintf(stderr, "MessagePublisher: cannot resolve %s\n", host.c_str());
        return;
    }

    int sock = ::socket(AF_INET, SOCK_STREAM, 0);
    if (sock < 0) { perror("socket"); return; }

    // 5-second timeout on send and receive — prevents consumer thread from blocking
    struct timeval tv{};
    tv.tv_sec = 5;
    ::setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv));
    ::setsockopt(sock, SOL_SOCKET, SO_SNDTIMEO, &tv, sizeof(tv));

    struct sockaddr_in serv{};
    serv.sin_family = AF_INET;
    serv.sin_port   = htons((uint16_t)port);
    memcpy(&serv.sin_addr, he->h_addr_list[0], (size_t)he->h_length);

    if (::connect(sock, (struct sockaddr*)&serv, sizeof(serv)) < 0) {
        fprintf(stderr, "MessagePublisher: cannot connect to %s:%d\n", host.c_str(), port);
        ::close(sock);
        return;
    }

    // Build HTTP/1.0 POST request (no chunked encoding needed)
    std::string req =
        "POST " + path + " HTTP/1.0\r\n"
        "Host: " + host + "\r\n"
        "Content-Type: application/json\r\n"
        "Content-Length: " + std::to_string(payload.size()) + "\r\n"
        "Connection: close\r\n"
        "\r\n" + payload;

    ::write(sock, req.c_str(), req.size());

    char resp[256]{};
    ::read(sock, resp, sizeof(resp) - 1);
    ::close(sock);

    // Check for 2xx status
    if (strncmp(resp, "HTTP/", 5) != 0) {
        fprintf(stderr, "MessagePublisher: unexpected response from broker\n");
    }
}
