#include "../src/decoder.h"
#include <cassert>
#include <vector>
#include <cstdio>
#include <cmath>

int main() {
    // T1: zero IQ input → no bits emitted, no crash
    {
        std::vector<char> bits;
        Decoder dec([&](char bit){ bits.push_back(bit); });
        for (int i = 0; i < 10000; i++) dec.sample_in(0.0, 0.0);
        assert(bits.empty());
        printf("T1 PASS: zero input → no bits\n");
    }

    // T2: alternating +85/-85 Hz FSK tones at 900 Hz → eventually produces bits,
    //     all of which must be 'B' or 'Y'.
    //
    //     The decoder needs CORR_BUF_SIZE = 63*9 = 567 samples to prime, then
    //     another 9 to populate corr_sum_.  We send 3000 samples — well past that.
    {
        std::vector<char> bits;
        Decoder dec([&](char bit){ bits.push_back(bit); });

        const double Fs    = 900.0;
        const double shift = 85.0;
        const int    SPB   = 9;
        double phase = 0.0;

        for (int n = 0; n < 3000; n++) {
            // Alternate sign every SPB samples to create clear FSK transitions
            int bit_num = n / SPB;
            double freq = (bit_num % 2 == 0)
                ? ( 2.0 * M_PI * shift / Fs)
                : (-2.0 * M_PI * shift / Fs);
            phase += freq;
            dec.sample_in(std::cos(phase), std::sin(phase));
        }

        assert(!bits.empty());
        for (char b : bits) assert(b == 'B' || b == 'Y');
        printf("T2 PASS: FSK signal → %d bits, all B/Y\n", (int)bits.size());
    }

    printf("test_decoder PASS\n");
    return 0;
}
