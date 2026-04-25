#pragma once
#include <functional>

class Fir1 {
public:
    using Callback = std::function<void(double I, double Q)>;
    explicit Fir1(Callback on_sample);
    void sample_in(double I, double Q);
private:
    Callback on_sample_;
    static constexpr int FILTER_SIZE = 37;
    static constexpr int DECIMATION_FACTOR = 4;
    static constexpr int BUF_SIZE = 4096;
    static const double filter_h_[FILTER_SIZE];
    struct IQ { double I, Q; };
    IQ buf_[BUF_SIZE]{};
    int buf_ptr_ = 0;
    int decim_count_ = 0;
};
