#pragma once
#include <functional>
#include <cmath>

// Fir2 handles both the 518 kHz (upper) and 490 kHz (lower) channel splits.
// The reference fir2cpp.C applies a 14 kHz frequency shift, then routes
// shifted+conjugate inputs into two independent FIR filter paths.
// Each path calls its respective callback when a decimated output is ready.
class Fir2 {
public:
    using Callback = std::function<void(double I, double Q)>;
    // on_518: called for the upper (518kHz) channel output
    // on_490: called for the lower (490kHz) channel output
    Fir2(Callback on_518, Callback on_490);
    void sample_in(double I, double Q);

private:
    Callback on_518_;
    Callback on_490_;

    // Filter parameters (from reference fir2cpp.C)
    static constexpr int FILTER_SIZE = 47;
    static constexpr int DECIMATION_FACTOR = 7;
    static constexpr int BUF_SIZE = 1024;
    static constexpr int FS_IN = 63000;
    static constexpr int FREQ_SHIFT = 14000;
    static constexpr int FS_FILTER_SIZE = 9;  // FS_IN/FREQ_SHIFT = 4.5 → 9 elements

    static const double filter_h_[FILTER_SIZE];

    // Frequency shift coefficients
    double fs_real_[FS_FILTER_SIZE];
    double fs_img_[FS_FILTER_SIZE];
    int fs_idx_ = 0;

    // Upper channel (518) buffer
    double buf_I_[BUF_SIZE]{};
    double buf_Q_[BUF_SIZE]{};
    int buf_ptr_ = 0;
    int decim_count_ = 0;

    // Lower channel (490) buffer
    double buf_I_490_[BUF_SIZE]{};
    double buf_Q_490_[BUF_SIZE]{};
    int buf_ptr_490_ = 0;
    int decim_count_490_ = 0;

    void fir_in_518(double I, double Q);
    void fir_in_490(double I, double Q);
};
