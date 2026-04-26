#pragma once
#include <functional>

// Narrow 250 Hz low-pass FIR, 10x decimation: 9 kHz → 900 Hz per channel.
// Coefficients from reference: https://fiiir.com s=9000, c=250, Tb=380, Kaiser SA=50dB
class Fir3 {
public:
    using Callback = std::function<void(double I, double Q)>;
    explicit Fir3(Callback on_out);
    void sample_in(double I, double Q);

private:
    Callback on_out_;

    static constexpr int FILTER_SIZE       = 71;
    static constexpr int DECIMATION_FACTOR = 10;
    static constexpr int BUF_SIZE          = 1024;

    static const double filter_h_[FILTER_SIZE];

    double buf_I_[BUF_SIZE]{};
    double buf_Q_[BUF_SIZE]{};
    int buf_ptr_     = 0;
    int decim_count_ = 0;
};
