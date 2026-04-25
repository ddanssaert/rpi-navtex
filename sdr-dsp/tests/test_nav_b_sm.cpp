#include "../src/nav_b_sm.h"
#include <cassert>
#include <string>
#include <vector>
#include <cstdio>

// Helper: feed bits from a string of 'B'/'Y' characters
void feed_bits(NavBSm& sm, const std::string& bits) {
    for (char b : bits) sm.receive_bit(b);
}

int main() {
    std::vector<std::string> messages;
    NavBSm sm(518000, [&](const std::string& bbbb, const std::string& msg, int freq) {
        messages.push_back(bbbb + "|" + msg);
    });

    // Feed the 29-bit phasing sequence: BBBBBBYYYYBBYYBBBBBBYYYYBBYYB + BB to trigger enable
    // (from nav_b_sm.h STAT_* state machine)
    std::string phasing = "BBBBBBYYYYBBYYBBBBBBYYYYBBYYBB";
    feed_bits(sm, phasing);

    // No message yet (need ZCZC marker) — just verify no crash
    assert(messages.empty());

    printf("test_nav_b_sm PASS\n");
    return 0;
}
