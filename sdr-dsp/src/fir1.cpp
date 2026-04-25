#include "fir1.h"

// 37-tap low-pass FIR filter coefficients
// Source: https://fiiir.com  s=252000, c=23000, Tb=26000, Kaiser SA=60dB
const double Fir1::filter_h_[Fir1::FILTER_SIZE] = {
    -0.000281792569572607,
    -0.000251360578620161,
     0.000352714845005618,
     0.001713054514664199,
     0.003521302844075157,
     0.004809321525020601,
     0.004142355129614629,
     0.000251359504199421,
    -0.007058416160837216,
    -0.016118579814418200,
    -0.023209712806215206,
    -0.023300868039639527,
    -0.011669350602513945,
     0.014091475032761831,
     0.052470033032779466,
     0.097612533914950089,
     0.140463679436186456,
     0.171242462423331770,
     0.182439576738455317,
     0.171242462423331770,
     0.140463679436186456,
     0.097612533914950089,
     0.052470033032779466,
     0.014091475032761831,
    -0.011669350602513945,
    -0.023300868039639527,
    -0.023209712806215206,
    -0.016118579814418200,
    -0.007058416160837216,
     0.000251359504199421,
     0.004142355129614629,
     0.004809321525020597,
     0.003521302844075161,
     0.001713054514664200,
     0.000352714845005618,
    -0.000251360578620161,
    -0.000281792569572607
};

Fir1::Fir1(Callback on_sample)
    : on_sample_(std::move(on_sample))
{}

void Fir1::sample_in(double I, double Q) {
    buf_[buf_ptr_].I = I;
    buf_[buf_ptr_].Q = Q;

    decim_count_++;
    if ((decim_count_ % DECIMATION_FACTOR) == 0) {
        double sum_I = 0.0, sum_Q = 0.0;
        int conv_ptr = buf_ptr_;

        if (buf_ptr_ > FILTER_SIZE) {
            for (int i = 0; i < FILTER_SIZE; i++) {
                double coef = filter_h_[i];
                sum_I += coef * buf_[conv_ptr].I;
                sum_Q += coef * buf_[conv_ptr].Q;
                conv_ptr--;
            }
        } else {
            for (int i = 0; i < FILTER_SIZE; i++) {
                double coef = filter_h_[i];
                sum_I += coef * buf_[conv_ptr].I;
                sum_Q += coef * buf_[conv_ptr].Q;
                if (conv_ptr == 0)
                    conv_ptr = BUF_SIZE - 1;
                else
                    conv_ptr--;
            }
        }

        on_sample_(sum_I, sum_Q);
        decim_count_ = 0;
    }

    buf_ptr_++;
    buf_ptr_ %= BUF_SIZE;
}
