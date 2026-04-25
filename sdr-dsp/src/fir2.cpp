#include "fir2.h"
#include <cmath>

// 47-tap low-pass FIR filter coefficients
// Source: https://fiiir.com  s=63000, c=2300, Tb=5500, Kaiser SA=65dB
const double Fir2::filter_h_[Fir2::FILTER_SIZE] = {
    -0.000147108163274380,
    -0.000344080458697122,
    -0.000638246787779785,
    -0.001020551993014626,
    -0.001453156218493991,
    -0.001862463815197339,
    -0.002135768595997864,
    -0.002123044308901244,
    -0.001644885141859342,
    -0.000506805672445239,
     0.001480845849197846,
     0.004479099821734249,
     0.008595303819273255,
     0.013859966338180792,
     0.020208854764962980,
     0.027473387873362013,
     0.035381576745767467,
     0.043570581210619269,
     0.051610508057461923,
     0.059037578414322979,
     0.065393451564912761,
     0.070266515161527154,
     0.073330495501089402,
     0.074375892066497792,
     0.073330495501089402,
     0.070266515161527154,
     0.065393451564912802,
     0.059037578414322937,
     0.051610508057461923,
     0.043570581210619269,
     0.035381576745767467,
     0.027473387873362013,
     0.020208854764962980,
     0.013859966338180792,
     0.008595303819273255,
     0.004479099821734245,
     0.001480845849197846,
    -0.000506805672445239,
    -0.001644885141859342,
    -0.002123044308901244,
    -0.002135768595997864,
    -0.001862463815197341,
    -0.001453156218493992,
    -0.001020551993014626,
    -0.000638246787779785,
    -0.000344080458697122,
    -0.000147108163274380
};

Fir2::Fir2(Callback on_518, Callback on_490)
    : on_518_(std::move(on_518)), on_490_(std::move(on_490))
{
    // Pre-compute frequency shift coefficients (14kHz shift at 63kHz input)
    for (int i = 0; i < FS_FILTER_SIZE; i++) {
        fs_real_[i] =  std::cos((2.0 * M_PI * i * FREQ_SHIFT) / FS_IN);
        fs_img_[i]  = -std::sin((2.0 * M_PI * i * FREQ_SHIFT) / FS_IN);
    }
}

void Fir2::sample_in(double I, double Q) {
    // Upper (518): multiply by e^{+j*2pi*FREQ_SHIFT/FS_IN * n}
    fir_in_518(
        I * fs_real_[fs_idx_] - Q * fs_img_[fs_idx_],
        I * fs_img_[fs_idx_] + Q * fs_real_[fs_idx_]
    );
    // Lower (490): multiply by conjugate e^{-j*2pi*FREQ_SHIFT/FS_IN * n}
    fir_in_490(
        I * fs_real_[fs_idx_] + Q * fs_img_[fs_idx_],
       -I * fs_img_[fs_idx_] + Q * fs_real_[fs_idx_]
    );

    fs_idx_++;
    fs_idx_ %= FS_FILTER_SIZE;
}

void Fir2::fir_in_518(double I, double Q) {
    buf_I_[buf_ptr_] = I;
    buf_Q_[buf_ptr_] = Q;

    decim_count_++;
    if ((decim_count_ % DECIMATION_FACTOR) == 0) {
        double sum_I = 0.0, sum_Q = 0.0;
        int conv_ptr = buf_ptr_;

        for (int i = 0; i < FILTER_SIZE; i++) {
            double coef = filter_h_[i];
            sum_I += coef * buf_I_[conv_ptr];
            sum_Q += coef * buf_Q_[conv_ptr];
            if (conv_ptr == 0)
                conv_ptr = BUF_SIZE - 1;
            else
                conv_ptr--;
        }
        on_518_(sum_I, sum_Q);
        decim_count_ = 0;
    }

    buf_ptr_++;
    buf_ptr_ %= BUF_SIZE;
}

void Fir2::fir_in_490(double I, double Q) {
    buf_I_490_[buf_ptr_490_] = I;
    buf_Q_490_[buf_ptr_490_] = Q;

    decim_count_490_++;
    if ((decim_count_490_ % DECIMATION_FACTOR) == 0) {
        double sum_I = 0.0, sum_Q = 0.0;
        int conv_ptr = buf_ptr_490_;

        for (int i = 0; i < FILTER_SIZE; i++) {
            double coef = filter_h_[i];
            sum_I += coef * buf_I_490_[conv_ptr];
            sum_Q += coef * buf_Q_490_[conv_ptr];
            if (conv_ptr == 0)
                conv_ptr = BUF_SIZE - 1;
            else
                conv_ptr--;
        }
        on_490_(sum_I, sum_Q);
        decim_count_490_ = 0;
    }

    buf_ptr_490_++;
    buf_ptr_490_ %= BUF_SIZE;
}
