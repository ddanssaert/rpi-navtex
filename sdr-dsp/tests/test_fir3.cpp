#include "../src/fir3.h"
#include <cassert>
#include <cmath>
#include <cstdio>

int main() {
    // T1: 10 samples → exactly 1 callback (10x decimation)
    {
        int count = 0;
        Fir3 fir([&](double I, double Q) { count++; });
        for (int i = 0; i < 10; i++) fir.sample_in(1.0, 0.0);
        assert(count == 1);
        printf("T1 PASS: 10 samples → 1 callback\n");
    }

    // T2: 70 samples → exactly 7 callbacks
    {
        int count = 0;
        Fir3 fir([&](double I, double Q) { count++; });
        for (int i = 0; i < 70; i++) fir.sample_in(1.0, 0.0);
        assert(count == 7);
        printf("T2 PASS: 70 samples → 7 callbacks\n");
    }

    // T3: DC input produces finite, non-NaN output
    {
        bool ok = true;
        Fir3 fir([&](double I, double Q) {
            ok = ok && std::isfinite(I) && std::isfinite(Q);
        });
        for (int i = 0; i < 100; i++) fir.sample_in(1.0, 0.0);
        assert(ok);
        printf("T3 PASS: DC input → finite outputs\n");
    }

    printf("test_fir3 PASS\n");
    return 0;
}
