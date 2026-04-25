#include "../src/decoder.h"
#include <cassert>
#include <vector>
#include <cstdio>

int main() {
    std::vector<char> bits;
    Decoder dec([&](char bit){ bits.push_back(bit); });

    // Feed all-zero IQ samples (no signal) — decoder must not crash,
    // and must not emit any bits (still in STATUS_INIT with no sync)
    for (int i = 0; i < 10000; i++) dec.sample_in(0.0, 0.0);
    assert(bits.empty());
    printf("test_decoder PASS\n");
    return 0;
}
