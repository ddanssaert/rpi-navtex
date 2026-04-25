#pragma once
#include <functional>
#include <cmath>

class Decoder {
public:
    using BitCallback = std::function<void(char bit)>;
    explicit Decoder(BitCallback on_bit);
    void sample_in(double I, double Q);

private:
    BitCallback on_bit_;

    // Constants (formerly magic numbers in decoder.h)
    static constexpr int SAMPLES_TO_BURN = 20;
    static constexpr int SAMPLES_TO_USE  = 40;
    static constexpr int SAMPLE_RATE     = 8000;
    static constexpr int NAVTEX_FSK_SHIFT= 85;
    static constexpr int SAMPLES_PER_BIT = 80;
    static constexpr int CORR_BUF_NUMBITS = 63;
    static constexpr int CORR_BUF_SIZE   = CORR_BUF_NUMBITS * SAMPLES_PER_BIT;

    // Status
    enum Status { INIT, SYNCED_WAIT, SYNCED_BIT_START, SYNCED_RECEIVING };
    Status status_ = INIT;

    // State (ported from decoder.h member vars)
    int bs_seq_nbr_ = 0, bd_seq_nbr_ = 0;
    int bit_sync_offset_ = 0, next_bit_sync_offset_ = 0;
    int burn_count_ = 0, samplecount_ = 0;
    double prevI_ = 0, prevQ_ = 0;
    float bit_filterR_[SAMPLES_TO_USE]{}, bit_filterI_[SAMPLES_TO_USE]{};
    float Brot_sumR_ = 0, Brot_sumI_ = 0, Yrot_sumR_ = 0, Yrot_sumI_ = 0;
    double delta_angle_buf_[SAMPLES_PER_BIT]{};
    double corr_buf_[CORR_BUF_SIZE]{};
    double corr_sum_[SAMPLES_PER_BIT]{};
    int dab_idx_ = 0, cb_idx_ = 0, csa_idx_ = 0;
    bool dab_primed_ = false, cb_primed_ = false, csa_primed_ = false;
    int prev_offset_ = -1;
    double corr_mask_[SAMPLES_PER_BIT]{};

    void bs_decoded_sample_in(double ds);
    void bd_decoded_sample_in(double ds, double I, double Q);
    void bd_in_bit_sync(int offset);
};
