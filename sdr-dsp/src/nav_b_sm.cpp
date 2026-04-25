#include "nav_b_sm.h"
#include <cstdio>
#include <cstring>
#include <regex.h>

NavBSm::NavBSm(unsigned int frequency, MessageCallback on_message)
    : on_message_(std::move(on_message)), freq_(frequency)
{
    init();
}

void NavBSm::init() {
    status_      = 0; // STAT_INIT
    byte_status_ = S_BYTE_WAIT;
    byte_mode_   = BYTE_MODE_LETTERS;

    bits_received_  = 0;
    dx_buf_ptr_     = 0;
    dx_buf_filled_  = 0;

    error_count_          = 0;
    error_buffer_ptr_     = 0;
    error_buffer_filled_  = 0;

    end_of_emission_counter_ = 0;
    previous_DX_was_alpha_   = 0;

    line_buffer_[0]    = '\0';
    message_buffer_[0] = '\0';
    message_bbbb_[0]   = '\0';

    phase_det_disable_timer_  = 0;
    byte_reception_enabled_   = 0;
    message_reception_ongoing_= 0;
}

void NavBSm::message_abort() {
    printf("message abort\n");
    if (message_reception_ongoing_) {
        on_message_(std::string(message_bbbb_), std::string(message_buffer_), (int)freq_);
    }
    init();
}

void NavBSm::message_line_out() {
    regex_t    regex_som;
    regex_t    regex_eom;
    regmatch_t pmatch[4];

    if (message_reception_ongoing_) {
        strcat(message_buffer_, line_buffer_);
        strcat(message_buffer_, "\n");
        printf("line added: %s\n", line_buffer_);
    }

    regcomp(&regex_som, "(CZC|Z.ZC|ZC.C|ZCZ.) +([A-Z][A-Z])([0-9][0-9])", REG_EXTENDED);
    if (regexec(&regex_som, line_buffer_, 4, pmatch, 0) == 0) {
        strcpy(message_buffer_, line_buffer_);
        strcat(message_buffer_, "\n");
        strncat(message_bbbb_, line_buffer_ + pmatch[2].rm_so, pmatch[2].rm_eo - pmatch[2].rm_so);
        strncat(message_bbbb_, line_buffer_ + pmatch[3].rm_so, pmatch[3].rm_eo - pmatch[3].rm_so);
        message_bbbb_[4] = '\0';
        printf("============START OF MESSAGE============ \n");
        message_reception_ongoing_ = 1;
    } else {
        regcomp(&regex_eom, "NNN.*|N.NN.*|NN.N.*", REG_EXTENDED);
        if (regexec(&regex_eom, line_buffer_, 1, pmatch, 0) == 0) {
            if (message_reception_ongoing_) {
                on_message_(std::string(message_bbbb_), std::string(message_buffer_), (int)freq_);
            }
            message_buffer_[0] = '\0';
            message_bbbb_[0]   = '\0';
            printf("============ END OF MESSAGE ============\n");
            message_reception_ongoing_ = 0;
        }
    }
    line_buffer_[0] = '\0';
}

void NavBSm::message_byte_out(unsigned char byte_in) {
    if (byte_in == '\0') {
        printf("*");
        strcat(line_buffer_, "*");
    } else if (code_to_ltrs_[byte_in] == 'l') {
        byte_mode_ = BYTE_MODE_LETTERS;
    } else if (code_to_ltrs_[byte_in] == 'f') {
        byte_mode_ = BYTE_MODE_FIGURES;
    } else if (code_to_ltrs_[byte_in] == 'n') {
        message_line_out();
    } else if (code_to_ltrs_[byte_in] == 'r') {
        // carriage return - ignored
    } else if (code_to_ltrs_[byte_in] == 'p') {
        // padding
    } else if (code_to_ltrs_[byte_in] == 'q') {
        // padding2
    } else {
        printf(".");
        if (byte_mode_ == BYTE_MODE_LETTERS)
            strncat(line_buffer_, (char*)&code_to_ltrs_[byte_in], 1);
        else
            strncat(line_buffer_, (char*)&code_to_figs_[byte_in], 1);
        printf(";");
    }
}

void NavBSm::receive_rxdx_byte(unsigned char byte_received) {
    switch (byte_status_) {
        case S_BYTE_WAIT:
            if (byte_received == PH1) byte_status_ = S_BYTE_RECEIVED_RX;
            if (byte_received == PH2) byte_status_ = S_BYTE_RECEIVED_DX;
            break;

        case S_BYTE_RECEIVED_RX:
            dx_buffer_[dx_buf_ptr_] = (char)byte_received;
            dx_buf_ptr_++;
            if (dx_buf_ptr_ == 3) {
                dx_buf_ptr_    = 0;
                dx_buf_filled_ = 1;
            }

            if (byte_received == PH1) {
                printf("\n alpha received in DX position\n");
                if (previous_DX_was_alpha_) {
                    end_of_emission_counter_++;
                    if (end_of_emission_counter_ == 2) {
                        printf("\nend of emission detected\n");
                        printf("\nstopping reception\n");
                        message_abort();
                        break;
                    }
                }
                previous_DX_was_alpha_ = 1;
            } else {
                previous_DX_was_alpha_ = 0;
            }
            byte_status_ = S_BYTE_RECEIVED_DX;
            break;

        case S_BYTE_RECEIVED_DX:
            if (dx_buf_filled_) {
                DX_byte_ = dx_buffer_[dx_buf_ptr_];
                if (code_to_ltrs_[(unsigned char)byte_received] != '_') {
                    message_byte_out(byte_received);
                } else {
                    if (code_to_ltrs_[(unsigned char)DX_byte_] != '_') {
                        message_byte_out((unsigned char)DX_byte_);
                    } else {
                        message_byte_out('\0');
                    }
                }
            }
            byte_status_ = S_BYTE_RECEIVED_RX;
            break;
    }

    // Error tracking (sliding window)
    if (error_buffer_filled_) {
        if (error_buffer_[error_buffer_ptr_] == '_')
            error_count_--;
    }
    error_buffer_[error_buffer_ptr_] = code_to_ltrs_[(unsigned char)byte_received];
    if (error_buffer_[error_buffer_ptr_] == '_')
        error_count_++;
    error_buffer_ptr_++;
    if (error_buffer_ptr_ == E_BUFFER_SIZE) {
        error_buffer_ptr_    = 0;
        error_buffer_filled_ = 1;
    }

    if (error_count_ > ERROR_THRESHOLD) {
        message_byte_out('\0');
        printf("\n error th exceeded \n");
        message_abort();
    }
}

void NavBSm::receive_bit(char bit_received) {
    if (byte_reception_enabled_) {
        temp_byte_ <<= 1;
        if (bit_received == 'Y')
            temp_byte_ |= 0x01;
        bits_received_++;
        if (bits_received_ == 7) {
            receive_rxdx_byte((unsigned char)temp_byte_);
            bits_received_ = 0;
            temp_byte_     = 0x00;
        }
    }

    if (phase_det_disable_timer_ != 0) {
        phase_det_disable_timer_--;
        if (phase_det_disable_timer_ == 0)
            printf("phase det disable timer expired\n");
        return;
    }

    // 29-state phasing sequence detection FSM
    // States 0-29 map to STAT_INIT through STAT_...BBYYB (the complete 29-bit sequence)
    switch (status_) {
        case 0:  status_ = (bit_received == 'B') ? 1  : 0;  break;
        case 1:  status_ = (bit_received == 'B') ? 2  : 0;  break;
        case 2:  status_ = (bit_received == 'B') ? 3  : 0;  break;
        case 3:  status_ = (bit_received == 'B') ? 4  : 0;  break;
        case 4:  status_ = (bit_received == 'B') ? 5  : 0;  break;
        case 5:  status_ = (bit_received == 'B') ? 6  : 0;  break;
        case 6:  status_ = (bit_received == 'Y') ? 7  : 6;  break; // BBBBBB — extra B allowed
        case 7:  status_ = (bit_received == 'Y') ? 8  : 0;  break;
        case 8:  status_ = (bit_received == 'Y') ? 9  : 0;  break;
        case 9:  status_ = (bit_received == 'Y') ? 10 : 0;  break;
        case 10: status_ = (bit_received == 'B') ? 11 : 0;  break;
        case 11: status_ = (bit_received == 'B') ? 12 : 0;  break;
        case 12: status_ = (bit_received == 'Y') ? 13 : 0;  break;
        case 13: status_ = (bit_received == 'Y') ? 14 : 0;  break;
        case 14: status_ = (bit_received == 'B') ? 15 : 0;  break;
        case 15: status_ = (bit_received == 'B') ? 16 : 0;  break;
        case 16: status_ = (bit_received == 'B') ? 17 : 0;  break;
        case 17: status_ = (bit_received == 'B') ? 18 : 0;  break;
        case 18: status_ = (bit_received == 'B') ? 19 : 0;  break;
        case 19: status_ = (bit_received == 'B') ? 20 : 0;  break;
        case 20: status_ = (bit_received == 'Y') ? 21 : 0;  break;
        case 21: status_ = (bit_received == 'Y') ? 22 : 0;  break;
        case 22: status_ = (bit_received == 'Y') ? 23 : 0;  break;
        case 23: status_ = (bit_received == 'Y') ? 24 : 0;  break;
        case 24: status_ = (bit_received == 'B') ? 25 : 0;  break;
        case 25: status_ = (bit_received == 'B') ? 26 : 0;  break;
        case 26: status_ = (bit_received == 'Y') ? 27 : 0;  break;
        case 27: status_ = (bit_received == 'Y') ? 28 : 0;  break;
        case 28: status_ = (bit_received == 'B') ? 29 : 0;  break;
        case 29:
            if (bit_received == 'B') {
                byte_reception_enabled_ = 1;
                bits_received_          = 0;
                temp_byte_              = 0x00;
                printf("phasing detected\n");
                phase_det_disable_timer_ = PHASE_DETECTION_DIS_TIMER_VALUE;
            }
            status_ = 0;
            break;
    }
}
