#pragma once
#include <functional>
#include <string>
#include <regex.h>

class NavBSm {
public:
    using MessageCallback = std::function<void(
        const std::string& bbbb,
        const std::string& message,
        int freq)>;

    NavBSm(unsigned int frequency, MessageCallback on_message);
    ~NavBSm();
    void receive_bit(char bit);

private:
    MessageCallback on_message_;
    unsigned int freq_;

    // Character lookup tables from reference nav_b_sm.h
    unsigned char code_to_ltrs_[128] = {
        //0    1    2    3    4    5    6    7    8    9    a    b    c    d    e    f
        '_', '_', '_', '_', '_', '_', '_', 'p', '_', '_', '_', 'J', '_', 'W', 'A', '_', // 0
        '_', '_', '_', 'F', '_', 'Y', 'S', '_', '_', '-', 'D', '_', 'Z', '_', '_', '_', // 1
        '_', '_', '_', 'C', '_', 'P', 'I', '_', '_', 'G', 'R', '_', 'L', '_', '_', '_', // 2
        '_', 'M', 'N', '_', 'H', '_', '_', '_', 'O', '_', '_', '_', '_', '_', '_', '_', // 3
        '_', '_', '_', 'K', '_', 'Q', 'U', '_', '_', 'f', 'E', '_', 'q', '_', '_', '_', // 4
        '_', 'X', 'l', '_', '_', '_', '_', '_', 'B', '_', '_', '_', ' ', '_', '_', '_', // 5
        '_', 'V', ' ', '_', 'n', '_', '_', '_', 'T', '_', '_', '_', '_', '_', '_', '_', // 6
        'r', '_', '_', '_', '_', '_', '_', '_', '_', '_', '_', '_', '_', '_', '_', '_'  // 7
    };
    unsigned char code_to_figs_[128] = {
        //0    1    2    3    4    5    6    7    8    9    a    b    c    d    e    f
        '_', '_', '_', '_', '_', '_', '_', 'p', '_', '_', '_', 'b', '_', '2', '-', '_', // 0
        '_', '_', '_', '*', '_', '6','\'', '_', '_', '-', '%', '_', '+', ' ', '_', '_', // 1
        '_', '_', '_', ':', '_', '0', '8', '_', '_', '*', '4', '_', ')', '_', '_', '_', // 2
        '_', '.', ',', '_', '*', '_', '_', '_', '9', '_', '_', '_', '_', '_', '_', '_', // 3
        '_', '_', '_', '(', '_', '1', '7', '_', '_', 'f', '3', '_', 'q', '_', '_', '_', // 4
        '_', '/', 'l', '_', '_', '_', '_', '_', '?', '_', '_', '_', ' ', '_', '_', '_', // 5
        '_', '=', ' ', '_', 'n', '_', '_', '_', '5', '_', '_', '_', '_', '_', '_', '_', // 6
        'r', '_', '_', '_', '_', '_', '_', '_', '_', '_', '_', '_', '_', '_', '_', '_'  // 7
    };

    // Phase detection constants (from reference)
    static constexpr int PH1 = 0x07;
    static constexpr int PH2 = 0x4c;
    static constexpr int E_BUFFER_SIZE = 20;
    static constexpr int ERROR_THRESHOLD = 12;
    static constexpr int PHASE_DETECTION_DIS_TIMER_VALUE = 100 * 11; // 11 seconds

    // Phasing state machine states (0-29 corresponding to STAT_INIT to STAT_...BBYYB)
    int status_ = 0;

    // Byte state machine
    static constexpr int S_BYTE_WAIT = 1;
    static constexpr int S_BYTE_RECEIVED_DX = 2;
    static constexpr int S_BYTE_RECEIVED_RX = 3;
    static constexpr int BYTE_MODE_LETTERS = 3;
    static constexpr int BYTE_MODE_FIGURES = 4;

    char DX_byte_ = 0;
    char dx_buffer_[3]{};
    char error_buffer_[E_BUFFER_SIZE]{};
    char temp_byte_ = 0;
    char line_buffer_[5000]{};
    char message_buffer_[5000]{};
    char message_bbbb_[10]{};

    int byte_status_ = S_BYTE_WAIT;
    int byte_mode_   = BYTE_MODE_LETTERS;
    int bits_received_ = 0;
    int dx_buf_ptr_ = 0;
    int dx_buf_filled_ = 0;
    int error_count_ = 0;
    int error_buffer_ptr_ = 0;
    int error_buffer_filled_ = 0;
    int end_of_emission_counter_ = 0;
    int previous_DX_was_alpha_ = 0;
    int phase_det_disable_timer_ = 0;
    int byte_reception_enabled_ = 0;
    int message_reception_ongoing_ = 0;

    // Pre-compiled regexes (compiled once in constructor, freed in destructor)
    regex_t regex_som_{};
    regex_t regex_eom_{};

    // CCIR-476 validity: in Y=1/B=0 encoding, a valid character has exactly 3 Y-bits
    // (= 4 Mark/B-bits). Uses GCC/Clang built-in — no C++20 required.
    static bool ccir476_valid(unsigned char b) {
        return __builtin_popcount(b & 0x7F) == 3;
    }

    void init();
    void message_abort();
    void message_byte_out(unsigned char byte_in);
    void receive_rxdx_byte(unsigned char byte_received);
    void message_line_out();
};
