#pragma once
#include <string>

class MessagePublisher {
public:
    explicit MessagePublisher(const std::string& broker_url);
    ~MessagePublisher();
    void publish(const std::string& bbbb,
                 const std::string& message,
                 int freq);
private:
    std::string broker_url_;
};
