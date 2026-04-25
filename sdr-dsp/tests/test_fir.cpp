#include "../src/fir1.h"
#include <cassert>
#include <cmath>
#include <cstdio>

int main() {
    int callback_count = 0;
    // Fir1 decimates by 4, so 8 samples in → exactly 2 callback calls
    Fir1 fir([&](double I, double Q) {
        callback_count++;
        // DC input: I=1.0, Q=0.0 → filtered output should be near 0.182 * 1.0 (centre tap amplitude)
        // Just verify it's called the right number of times and output is finite
        assert(std::isfinite(I));
        assert(std::isfinite(Q));
    });
    for (int i = 0; i < 8; i++) fir.sample_in(1.0, 0.0);
    assert(callback_count == 2);
    printf("test_fir PASS\n");
    return 0;
}
