#include "../src/iq_queue.h"
#include <cassert>
#include <thread>
#include <vector>
#include <cstdio>

int main() {
    IqQueue q(256);
    std::vector<std::pair<short,short>> consumed;

    std::thread producer([&] {
        for (int i = 0; i < 10; i++) q.push(i, i*2);
        q.close(); // signals consumer to stop
    });

    std::thread consumer([&] {
        short xi, xq;
        while (q.pop(xi, xq)) consumed.emplace_back(xi, xq);
    });

    producer.join();
    consumer.join();

    assert(consumed.size() == 10);
    assert(consumed[0].first == 0 && consumed[9].first == 9);
    printf("test_iq_queue PASS\n");
    return 0;
}
