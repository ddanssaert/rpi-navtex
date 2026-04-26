#include "../src/fir2.h"
#include <cassert>
#include <cmath>
#include <cstdio>

int main() {
    // T1: 7 samples → exactly 1 callback per channel (7x decimation)
    {
        int count_518 = 0, count_490 = 0;
        Fir2 fir(
            [&](double, double) { count_518++; },
            [&](double, double) { count_490++; }
        );
        for (int i = 0; i < 7; i++) fir.sample_in(1.0, 0.0);
        assert(count_518 == 1);
        assert(count_490 == 1);
        printf("T1 PASS: 7 samples → 1 callback per channel\n");
    }

    // T2: 14 samples → exactly 2 callbacks per channel
    {
        int count_518 = 0, count_490 = 0;
        Fir2 fir(
            [&](double, double) { count_518++; },
            [&](double, double) { count_490++; }
        );
        for (int i = 0; i < 14; i++) fir.sample_in(1.0, 0.0);
        assert(count_518 == 2);
        assert(count_490 == 2);
        printf("T2 PASS: 14 samples → 2 callbacks per channel\n");
    }

    // T3: DC input produces finite, non-NaN outputs on both channels
    {
        bool ok = true;
        Fir2 fir(
            [&](double I, double Q) { ok = ok && std::isfinite(I) && std::isfinite(Q); },
            [&](double I, double Q) { ok = ok && std::isfinite(I) && std::isfinite(Q); }
        );
        for (int i = 0; i < 70; i++) fir.sample_in(1.0, 0.0);
        assert(ok);
        printf("T3 PASS: DC input → finite outputs on both channels\n");
    }

    printf("test_fir2 PASS\n");
    return 0;
}
