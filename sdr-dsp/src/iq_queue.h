#pragma once
#include <mutex>
#include <condition_variable>
#include <vector>

class IqQueue {
public:
    explicit IqQueue(size_t capacity)
        : buf_(capacity * 2), capacity_(capacity) {}

    void push(short xi, short xq) {
        std::unique_lock<std::mutex> lk(mu_);
        // Block if full (back-pressure)
        not_full_.wait(lk, [&]{ return count_ < capacity_ || closed_; });
        if (closed_) return;
        buf_[write_pos_ * 2]     = xi;
        buf_[write_pos_ * 2 + 1] = xq;
        write_pos_ = (write_pos_ + 1) % capacity_;
        count_++;
        lk.unlock();
        not_empty_.notify_one();
    }

    // Returns false when queue is closed and empty
    bool pop(short& xi, short& xq) {
        std::unique_lock<std::mutex> lk(mu_);
        not_empty_.wait(lk, [&]{ return count_ > 0 || closed_; });
        if (count_ == 0) return false;
        xi = buf_[read_pos_ * 2];
        xq = buf_[read_pos_ * 2 + 1];
        read_pos_ = (read_pos_ + 1) % capacity_;
        count_--;
        lk.unlock();
        not_full_.notify_one();
        return true;
    }

    void close() {
        std::lock_guard<std::mutex> lk(mu_);
        closed_ = true;
        not_empty_.notify_all();
        not_full_.notify_all();
    }

private:
    std::vector<short> buf_;
    size_t capacity_;
    size_t write_pos_ = 0, read_pos_ = 0, count_ = 0;
    bool closed_ = false;
    std::mutex mu_;
    std::condition_variable not_empty_, not_full_;
};
