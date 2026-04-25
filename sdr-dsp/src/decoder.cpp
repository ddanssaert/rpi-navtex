#include "decoder.h"
#include <cmath>

Decoder::Decoder(BitCallback on_bit)
    : on_bit_(std::move(on_bit))
{
    // Pre-compute bit correlation filter coefficients
    for (int i = 0; i < SAMPLES_TO_USE; i++) {
        float angle = (float)(i * 2 * 3.1415f * NAVTEX_FSK_SHIFT) / SAMPLE_RATE;
        bit_filterR_[i] = std::cos(angle);
        bit_filterI_[i] = std::sin(angle);
    }

    // Generate square-wave correlation mask for bit synchronization
    // Standard SITOR-B sync pattern (Mark/Space transitions)
    for (int i = 0; i < SAMPLES_PER_BIT; i++) {
        if (i < SAMPLES_PER_BIT / 10) corr_mask_[i] = 0;
        else if (i < SAMPLES_PER_BIT / 2) corr_mask_[i] = 1.0;
        else if (i < 9 * SAMPLES_PER_BIT / 10) corr_mask_[i] = -1.0;
        else corr_mask_[i] = 0;
    }
}

void Decoder::sample_in(double I, double Q) {
    double prodReal = I * prevI_ + Q * prevQ_;
    double prodImg  = Q * prevI_ - I * prevQ_;
    double result   = std::atan2(prodImg, prodReal);

    prevI_ = I;
    prevQ_ = Q;

    bs_decoded_sample_in(result);
    bd_decoded_sample_in(result, I, Q);
}

void Decoder::bd_in_bit_sync(int offset) {
    if (status_ == INIT) {
        status_ = SYNCED_WAIT;
        bit_sync_offset_ = offset;
    }
    next_bit_sync_offset_ = offset;
}

void Decoder::bd_decoded_sample_in(double /*ds*/, double sampleR, double sampleI) {
    bd_seq_nbr_++;

    if (status_ == INIT) return;

    if (status_ == SYNCED_WAIT) {
        if ((bd_seq_nbr_ % SAMPLES_PER_BIT) == bit_sync_offset_) {
            status_ = SYNCED_BIT_START;
            burn_count_ = 0;
        }
    }

    if (status_ == SYNCED_BIT_START) {
        if (burn_count_ == SAMPLES_TO_BURN) {
            status_ = SYNCED_RECEIVING;
            samplecount_ = 0;
            Brot_sumR_ = Brot_sumI_ = Yrot_sumR_ = Yrot_sumI_ = 0.0f;
            return;
        } else {
            burn_count_++;
            return;
        }
    }

    if (status_ == SYNCED_RECEIVING) {
        Yrot_sumR_ += (float)(sampleR * bit_filterR_[samplecount_] - sampleI * bit_filterI_[samplecount_]);
        Yrot_sumI_ += (float)(sampleR * bit_filterI_[samplecount_] + sampleI * bit_filterR_[samplecount_]);
        Brot_sumR_ += (float)(sampleR * bit_filterR_[samplecount_] + sampleI * bit_filterI_[samplecount_]);
        Brot_sumI_ += (float)(-sampleR * bit_filterI_[samplecount_] + sampleI * bit_filterR_[samplecount_]);

        samplecount_++;
        if (samplecount_ == SAMPLES_TO_USE) {
            float Brot = Brot_sumR_ * Brot_sumR_ + Brot_sumI_ * Brot_sumI_;
            float Yrot = Yrot_sumR_ * Yrot_sumR_ + Yrot_sumI_ * Yrot_sumI_;
            on_bit_(Brot > Yrot ? 'B' : 'Y');
            status_ = SYNCED_WAIT;
            bit_sync_offset_ = next_bit_sync_offset_;
        }
    }
}

void Decoder::bs_decoded_sample_in(double ds) {
    // Store last SAMPLES_PER_BIT delta-angle samples
    delta_angle_buf_[dab_idx_] = ds;
    dab_idx_++;
    if (dab_idx_ == SAMPLES_PER_BIT) {
        dab_idx_ = 0;
        dab_primed_ = true;
    }

    // Correlate with mask, store abs result in correlation buffer
    if (dab_primed_) {
        double temp = 0.0;
        int dab_j = dab_idx_;
        for (int i = 0; i < SAMPLES_PER_BIT; i++) {
            temp += corr_mask_[i] * delta_angle_buf_[dab_j];
            dab_j = (dab_j + 1) % SAMPLES_PER_BIT;
        }
        corr_buf_[cb_idx_] = std::fabs(temp);
        cb_idx_++;
        if (cb_idx_ == CORR_BUF_SIZE) {
            cb_idx_ = 0;
            cb_primed_ = true;
        }
    }

    // Update one correlation sum per sample
    if (cb_primed_) {
        double temp = 0.0;
        for (int i = csa_idx_; i < CORR_BUF_SIZE; i += SAMPLES_PER_BIT) {
            temp += corr_buf_[i];
        }
        corr_sum_[csa_idx_] = temp;
        csa_idx_++;
        if (csa_idx_ == SAMPLES_PER_BIT) {
            csa_idx_ = 0;
            csa_primed_ = true;
        }
    }

    // Find best offset 1 out of 9 times
    if (csa_primed_) {
        if ((bs_seq_nbr_ % SAMPLES_PER_BIT) == 0) {
            double temp_max = -1.0;
            int max_index = 0;
            for (int i = 0; i < SAMPLES_PER_BIT; i++) {
                if (corr_sum_[i] > temp_max) {
                    temp_max = corr_sum_[i];
                    max_index = i;
                }
            }

            if (prev_offset_ != -1 && max_index != prev_offset_) {
                if (max_index > prev_offset_) {
                    if (max_index - prev_offset_ > 4)
                        max_index = (prev_offset_ - 1 + SAMPLES_PER_BIT) % SAMPLES_PER_BIT;
                    else
                        max_index = (prev_offset_ + 1) % SAMPLES_PER_BIT;
                } else {
                    if (prev_offset_ - max_index > 4)
                        max_index = (prev_offset_ + 1) % SAMPLES_PER_BIT;
                    else
                        max_index = (prev_offset_ - 1 + SAMPLES_PER_BIT) % SAMPLES_PER_BIT;
                }
            }
            prev_offset_ = max_index;
            // Only signal bit sync if there's actual correlation energy (not just noise/zero input)
            if (temp_max > 0.0) {
                bd_in_bit_sync((max_index + 5) % SAMPLES_PER_BIT);
            }
        }
        bs_seq_nbr_ = (bs_seq_nbr_ + 1) % SAMPLES_PER_BIT;
    }
}
