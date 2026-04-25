# SDR-DSP Pipeline Implementation Plan

> **For Antigravity:** REQUIRED WORKFLOW: Use `.agent/workflows/execute-plan.md` to execute this plan in single-flow mode.

**Goal:** Build and containerize a refactored C++ signal-processing component that receives IQ samples from the SDRPlay RSPDX, runs the full DSP pipeline (FIR decimation → FSK demodulation → SITOR-B frame decoding), and publishes decoded Navtex messages to the `api-broker` via HTTP POST.

**Architecture:** The reference code in `reference/backend/receiver/` is ported into `sdr-dsp/src/`. The busy-wait consumer loop (`usleep`) is replaced with a producer→consumer design using a condition variable. All magic constants (frequencies, decimation factors, gain, antenna) are moved into a `config.h` populated at runtime from environment variables, making them injectable from `docker-compose.yml`. Completed messages are delivered as JSON via HTTP POST to the `api-broker`'s `POST /messages` endpoint over the Docker network. The SDRPlay API 3.x is baked into the Docker image for a fully plug-and-play experience.

**Tech Stack:** C++17, CMake, pthreads, SDRPlay API 3.x (baked into Docker image), libcurl, nlohmann/json (header-only), Docker multi-stage build.

**Reference code location:** `reference/backend/receiver/` — study these files but do NOT modify them.

---

### Task 1: Scaffold the `sdr-dsp` container directory and CMake build

**Files:**
- Create: `sdr-dsp/CMakeLists.txt`
- Create: `sdr-dsp/Dockerfile`
- Create: `sdr-dsp/src/config.h`
- Create: `sdr-dsp/src/main.cpp` (stub)

**Step 1: Write the failing build test**

```bash
cd sdr-dsp && docker build -t navtex-sdr-dsp .
```
Expected: FAIL — no Dockerfile yet.

**Step 2: Run test to verify it fails**

Run: `docker build -t navtex-sdr-dsp ./sdr-dsp`
Expected: `unable to prepare context: unable to evaluate symlinks in Dockerfile path`

**Step 3: Write minimal implementation**

Create `sdr-dsp/CMakeLists.txt`:

```cmake
cmake_minimum_required(VERSION 3.16)
project(navtex_sdr_dsp CXX)
set(CMAKE_CXX_STANDARD 17)

find_library(SDRPLAY_LIB sdrplay_api REQUIRED)
find_package(CURL REQUIRED)

add_executable(sdr-dsp
    src/main.cpp
    src/fir1.cpp
    src/fir2.cpp
    src/decoder.cpp
    src/nav_b_sm.cpp
    src/message_publisher.cpp
)

target_include_directories(sdr-dsp PRIVATE src /usr/local/include third_party)
target_link_libraries(sdr-dsp PRIVATE ${SDRPLAY_LIB} CURL::libcurl pthread)
```

Create `sdr-dsp/src/config.h` — all SDR constants as `inline constexpr` values (or overridden by env at runtime):

```cpp
#pragma once
#include <cstdlib>
#include <string>

// --- SDR hardware ---
inline int    CFG_FREQ_TUNER()  { return 504000; }   // (490+518)/2 kHz
inline int    CFG_FREQ_UPPER()  { return 518000; }
inline int    CFG_FREQ_LOWER()  { return 490000; }
inline double CFG_IN_SAMPLE_RATE() { return 252000.0; }
inline int    CFG_H_DECIMATION()   { return 8; }     // hardware decimation
inline int    CFG_SW_DECIMATION()  { return 4; }     // software FIR1 decimation

// --- SDR tuner ---
// Overridable via env var SDR_ANTENNA (A/B/C)
inline char CFG_ANTENNA() {
    const char* v = std::getenv("SDR_ANTENNA");
    if (v && v[0] >= 'A' && v[0] <= 'C') return v[0];
    return 'A';
}
// Overridable via env var SDR_LNA_STATE (0-8)
inline int CFG_LNA_STATE() {
    const char* v = std::getenv("SDR_LNA_STATE");
    return v ? std::atoi(v) : 0;
}

// --- API Broker ---
// Overridable via env var BROKER_URL
inline std::string CFG_BROKER_URL() {
    const char* v = std::getenv("BROKER_URL");
    return v ? std::string(v) : "http://api-broker:8000/messages";
}
```

Create `sdr-dsp/Dockerfile` (multi-stage build with SDRPlay API baked in):

```dockerfile
FROM debian:bookworm-slim AS build

RUN apt-get update && apt-get install -y \
    cmake g++ libpthread-stubs0-dev libcurl4-openssl-dev \
    && rm -rf /var/lib/apt/lists/*

# Bake SDRPlay API into the image.
# The installer .run file must be placed in sdr-dsp/vendor/ before building.
# Download from: https://www.sdrplay.com/downloads/
COPY vendor/SDRplay_RSP_API-Linux-*.run /tmp/sdrplay_install.run
RUN chmod +x /tmp/sdrplay_install.run \
    && /tmp/sdrplay_install.run --noexec --target /tmp/sdrplay \
    && cp /tmp/sdrplay/inc/* /usr/local/include/ \
    && cp /tmp/sdrplay/*/libsdrplay_api.so* /usr/local/lib/ \
    && ldconfig \
    && rm -rf /tmp/sdrplay*

WORKDIR /build
COPY . .
RUN cmake -B build -DCMAKE_BUILD_TYPE=Release && cmake --build build -j$(nproc)

FROM debian:bookworm-slim
RUN apt-get update && apt-get install -y libcurl4 && rm -rf /var/lib/apt/lists/*
COPY --from=build /usr/local/lib/libsdrplay_api.so* /usr/local/lib/
RUN ldconfig
COPY --from=build /build/build/sdr-dsp /usr/local/bin/sdr-dsp
ENTRYPOINT ["/usr/local/bin/sdr-dsp"]
```

> **Note:** The SDRPlay API installer (`.run` file) must be placed in `sdr-dsp/vendor/` before building. Download it from the [SDRPlay downloads page](https://www.sdrplay.com/downloads/). The resulting Docker image is fully self-contained — no host SDK installation needed.

Create stub `sdr-dsp/src/main.cpp`:

```cpp
#include <cstdio>
int main() {
    printf("sdr-dsp stub\n");
    return 0;
}
```

**Step 4: Run test to verify it passes**

Run: `docker build -t navtex-sdr-dsp ./sdr-dsp`
Expected: PASS (image builds from the stub `main.cpp`)

> Skip if Docker is unavailable on dev machine — the CMake build test below is the primary gate.

**Step 5: Commit**

```bash
git add sdr-dsp/
git commit -m "chore: scaffold sdr-dsp container with cmake build system"
```

---

### Task 2: Port the FIR filter chain (`fir1`, `fir2`)

**Files:**
- Create: `sdr-dsp/src/fir1.h`
- Create: `sdr-dsp/src/fir1.cpp`
- Create: `sdr-dsp/src/fir2.h`
- Create: `sdr-dsp/src/fir2.cpp`
- Create: `sdr-dsp/tests/test_fir.cpp`

**Background:** `reference/backend/receiver/fir1cpp.C` is a 37-tap FIR low-pass filter with decimation factor 4 (252kHz → 63kHz). Its output calls `sample_in_2` (fir2), which further decimates to 900 samples/sec fed into the decoder. We port this verbatim but:
- Replace `extern "C"` linkage with a clean C++ class `Fir1` / `Fir2`
- Keep the same filter coefficients exactly
- Replace the global function call chain with a `std::function<void(double,double)>` callback injected at construction

**Step 1: Write the failing test**

Create `sdr-dsp/tests/test_fir.cpp`:

```cpp
#include "../src/fir1.h"
#include <cassert>
#include <cmath>

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
```

**Step 2: Run test to verify it fails**

```bash
cd sdr-dsp && g++ -std=c++17 -o /tmp/test_fir tests/test_fir.cpp src/fir1.cpp && /tmp/test_fir
```
Expected: FAIL — `fir1.h` does not exist yet.

**Step 3: Write minimal implementation**

Create `sdr-dsp/src/fir1.h`:

```cpp
#pragma once
#include <functional>

class Fir1 {
public:
    using Callback = std::function<void(double I, double Q)>;
    explicit Fir1(Callback on_sample);
    void sample_in(double I, double Q);
private:
    Callback on_sample_;
    static constexpr int FILTER_SIZE = 37;
    static constexpr int DECIMATION_FACTOR = 4;
    static constexpr int BUF_SIZE = 4096;
    static const double filter_h_[FILTER_SIZE];
    struct IQ { double I, Q; };
    IQ buf_[BUF_SIZE]{};
    int buf_ptr_ = 0;
    int decim_count_ = 0;
};
```

Create `sdr-dsp/src/fir1.cpp` — port the coefficients and convolution loop from `reference/backend/receiver/fir1cpp.C` verbatim, adapting to the class structure and injecting the callback instead of calling `sample_in_2` directly. (The `fir2.cpp` file is wired as the callback.)

Create `sdr-dsp/src/fir2.h` and `sdr-dsp/src/fir2.cpp` identically, porting from `reference/backend/receiver/fir2cpp.C`, with its own callback for the decoder.

**Step 4: Run test to verify it passes**

```bash
cd sdr-dsp && g++ -std=c++17 -o /tmp/test_fir tests/test_fir.cpp src/fir1.cpp && /tmp/test_fir
```
Expected: `test_fir PASS`

**Step 5: Commit**

```bash
git add sdr-dsp/src/fir1.h sdr-dsp/src/fir1.cpp sdr-dsp/src/fir2.h sdr-dsp/src/fir2.cpp sdr-dsp/tests/test_fir.cpp
git commit -m "feat(sdr-dsp): port FIR filter chain as injectable callback classes"
```

---

### Task 3: Port the FSK Decoder

**Files:**
- Create: `sdr-dsp/src/decoder.h`
- Create: `sdr-dsp/src/decoder.cpp`
- Create: `sdr-dsp/tests/test_decoder.cpp`

**Background:** `reference/backend/receiver/decoder.C` implements differential phase FSK demodulation and bit-sync using correlations. The only global magic number we remove: `SAMPLE_RATE 900` and `NAVTEX_FSK_SHIFT 85` are moved to `config.h`. The `byte_state_machine*` output pointer is replaced with a `std::function<void(char)>` bit callback injected at construction — no raw pointers.

**Step 1: Write the failing test**

Create `sdr-dsp/tests/test_decoder.cpp`:

```cpp
#include "../src/decoder.h"
#include <cassert>
#include <vector>

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
```

**Step 2: Run test to verify it fails**

```bash
cd sdr-dsp && g++ -std=c++17 -o /tmp/test_decoder tests/test_decoder.cpp src/decoder.cpp && /tmp/test_decoder
```
Expected: FAIL — `decoder.h` does not exist.

**Step 3: Write minimal implementation**

Create `sdr-dsp/src/decoder.h`:

```cpp
#pragma once
#include <functional>
#include <cmath>

class Decoder {
public:
    using BitCallback = std::function<void(char bit)>;
    explicit Decoder(BitCallback on_bit);
    void sample_in(double I, double Q);

private:
    BitCallback on_bit_;

    // Constants (formerly magic numbers in decoder.h)
    static constexpr int SAMPLES_TO_BURN = 2;
    static constexpr int SAMPLES_TO_USE  = 5;
    static constexpr int SAMPLE_RATE     = 900;
    static constexpr int NAVTEX_FSK_SHIFT= 85;
    static constexpr int SAMPLES_PER_BIT = 9;
    static constexpr int CORR_BUF_NUMBITS = 63;
    static constexpr int CORR_BUF_SIZE   = CORR_BUF_NUMBITS * SAMPLES_PER_BIT;

    // Status
    enum Status { INIT, SYNCED_WAIT, SYNCED_BIT_START, SYNCED_RECEIVING };
    Status status_ = INIT;

    // State (ported from decoder.h member vars)
    int bs_seq_nbr_ = 0, bd_seq_nbr_ = 0;
    int bit_sync_offset_ = 0, next_bit_sync_offset_ = 0;
    int burn_count_ = 0, samplecount_ = 0;
    double prevI_ = 0, prevQ_ = 0;
    float bit_filterR_[SAMPLES_TO_USE]{}, bit_filterI_[SAMPLES_TO_USE]{};
    float Brot_sumR_ = 0, Brot_sumI_ = 0, Yrot_sumR_ = 0, Yrot_sumI_ = 0;
    double delta_angle_buf_[SAMPLES_PER_BIT]{};
    double corr_buf_[CORR_BUF_SIZE]{};
    double corr_sum_[SAMPLES_PER_BIT]{};
    int dab_idx_ = 0, cb_idx_ = 0, csa_idx_ = 0;
    bool dab_primed_ = false, cb_primed_ = false, csa_primed_ = false;
    int prev_offset_ = -1;
    const int corr_mask_[SAMPLES_PER_BIT] = {0,1,1,1,0,-1,-1,-1,0};

    void bs_decoded_sample_in(double ds);
    void bd_decoded_sample_in(double ds, double I, double Q);
    void bd_in_bit_sync(int offset);
};
```

Create `sdr-dsp/src/decoder.cpp` — port `decoder.C` verbatim, adapting variable names to match the header above, and replacing `output_bsm->receive_bit(...)` with `on_bit_(...)`.

**Step 4: Run test to verify it passes**

```bash
cd sdr-dsp && g++ -std=c++17 -o /tmp/test_decoder tests/test_decoder.cpp src/decoder.cpp && /tmp/test_decoder
```
Expected: `test_decoder PASS`

**Step 5: Commit**

```bash
git add sdr-dsp/src/decoder.h sdr-dsp/src/decoder.cpp sdr-dsp/tests/test_decoder.cpp
git commit -m "feat(sdr-dsp): port FSK decoder with injected bit callback"
```

---

### Task 4: Port the SITOR-B Byte State Machine (`nav_b_sm`)

**Files:**
- Create: `sdr-dsp/src/nav_b_sm.h`
- Create: `sdr-dsp/src/nav_b_sm.cpp`
- Create: `sdr-dsp/tests/test_nav_b_sm.cpp`

**Background:** `reference/backend/receiver/nav_b_sm.C` implements phasing detection (29-state FSM), SITOR-B FEC error correction (RX/DX dual-copy), and message assembly (ZCZC → NNNN). We replace the direct call to `add_message(bbbb, message, freq)` (which writes to SQLite via `message_store.c`) with an injected `std::function<void(std::string bbbb, std::string message, int freq)>` callback. This decouples the DSP from persistence.

**Step 1: Write the failing test**

Create `sdr-dsp/tests/test_nav_b_sm.cpp`:

```cpp
#include "../src/nav_b_sm.h"
#include <cassert>
#include <string>
#include <vector>

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
```

**Step 2: Run test to verify it fails**

```bash
cd sdr-dsp && g++ -std=c++17 -o /tmp/test_nav tests/test_nav_b_sm.cpp src/nav_b_sm.cpp && /tmp/test_nav
```
Expected: FAIL — `nav_b_sm.h` does not exist.

**Step 3: Write minimal implementation**

Create `sdr-dsp/src/nav_b_sm.h`:

```cpp
#pragma once
#include <functional>
#include <string>

class NavBSm {
public:
    using MessageCallback = std::function<void(
        const std::string& bbbb,
        const std::string& message,
        int freq)>;

    NavBSm(unsigned int frequency, MessageCallback on_message);
    void receive_bit(char bit);

private:
    MessageCallback on_message_;
    unsigned int freq_;

    // Port all private members from nav_b_sm.h verbatim
    // (code_to_ltrs[], code_to_figs[], status, byte_mode, etc.)
    // ... (see reference/backend/receiver/nav_b_sm.h)

    void init();
    void message_abort();
    void message_byte_out(unsigned char byte_in);
    void receive_rxdx_byte(unsigned char byte_received);
    void message_line_out();
};
```

Create `sdr-dsp/src/nav_b_sm.cpp` — port `nav_b_sm.C` verbatim. Replace the `add_message(bbbb, message_buffer, freq)` call with `on_message_(message_bbbb_, message_buffer_, freq_)`.

**Step 4: Run test to verify it passes**

```bash
cd sdr-dsp && g++ -std=c++17 -o /tmp/test_nav tests/test_nav_b_sm.cpp src/nav_b_sm.cpp && /tmp/test_nav
```
Expected: `test_nav_b_sm PASS`

**Step 5: Commit**

```bash
git add sdr-dsp/src/nav_b_sm.h sdr-dsp/src/nav_b_sm.cpp sdr-dsp/tests/test_nav_b_sm.cpp
git commit -m "feat(sdr-dsp): port SITOR-B state machine with injected message callback"
```

---

### Task 5: Implement the Message Publisher (HTTP POST to api-broker)

**Files:**
- Create: `sdr-dsp/src/message_publisher.h`
- Create: `sdr-dsp/src/message_publisher.cpp`
- Create: `sdr-dsp/tests/test_publisher.cpp`
- Fetch: `sdr-dsp/third_party/json.hpp` (nlohmann/json single-header)

**Background:** When `NavBSm` fires its callback, a `MessagePublisher` serializes the message to JSON and HTTP POSTs it to the `api-broker`'s `POST /messages` endpoint. This is simpler than Unix sockets — the broker already runs FastAPI, and message volume is tiny (a few per hour). Format:

```json
{"bbbb":"OB01","message":"ZCZC OB01\n...\nNNNN","freq":518000,"ts":"2026-04-25T22:00:00Z"}
```

**Step 1: Write the failing test**

Create `sdr-dsp/tests/test_publisher.cpp`:

```cpp
#include "../src/message_publisher.h"
#include <cassert>
#include <string>
#include <thread>
#include <chrono>
#include <cstring>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>

// Minimal HTTP server to verify the POST request
int main() {
    // Start a tiny TCP server on a random port
    int server_fd = ::socket(AF_INET, SOCK_STREAM, 0);
    struct sockaddr_in addr{};
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = INADDR_ANY;
    addr.sin_port = 0;  // let OS pick a port
    ::bind(server_fd, (struct sockaddr*)&addr, sizeof(addr));
    socklen_t len = sizeof(addr);
    ::getsockname(server_fd, (struct sockaddr*)&addr, &len);
    int port = ntohs(addr.sin_port);
    ::listen(server_fd, 1);

    std::string url = "http://127.0.0.1:" + std::to_string(port) + "/messages";

    // POST in a background thread
    std::thread t([&] {
        std::this_thread::sleep_for(std::chrono::milliseconds(50));
        MessagePublisher pub(url);
        pub.publish("OB01", "ZCZC OB01\nTEST\nNNNN", 518000);
    });

    int client_fd = ::accept(server_fd, nullptr, nullptr);
    char buf[2048]{};
    ::read(client_fd, buf, sizeof(buf) - 1);

    // Send a minimal 200 OK response so curl doesn't error
    const char* response = "HTTP/1.1 200 OK\r\nContent-Length: 0\r\n\r\n";
    ::write(client_fd, response, strlen(response));

    t.join();
    ::close(client_fd);
    ::close(server_fd);

    std::string received(buf);
    // Verify it's a POST with JSON content
    assert(received.find("POST /messages") != std::string::npos);
    assert(received.find("\"bbbb\":\"OB01\"") != std::string::npos);
    assert(received.find("\"freq\":518000") != std::string::npos);
    printf("test_publisher PASS\n");
    return 0;
}
```

**Step 2: Run test to verify it fails**

```bash
cd sdr-dsp && g++ -std=c++17 -pthread -o /tmp/test_pub tests/test_publisher.cpp src/message_publisher.cpp -lcurl -I third_party && /tmp/test_pub
```
Expected: FAIL — `message_publisher.h` does not exist.

**Step 3: Write minimal implementation**

Download nlohmann/json single header:

```bash
mkdir -p sdr-dsp/third_party
curl -L https://github.com/nlohmann/json/releases/download/v3.11.3/json.hpp \
     -o sdr-dsp/third_party/json.hpp
```

Create `sdr-dsp/src/message_publisher.h`:

```cpp
#pragma once
#include <string>

class MessagePublisher {
public:
    explicit MessagePublisher(const std::string& broker_url);
    ~MessagePublisher();
    void publish(const std::string& bbbb,
                 const std::string& message,
                 int freq);
private:
    std::string broker_url_;
};
```

Create `sdr-dsp/src/message_publisher.cpp`:

```cpp
#include "message_publisher.h"
#include <json.hpp>
#include <curl/curl.h>
#include <ctime>
#include <cstdio>

MessagePublisher::MessagePublisher(const std::string& broker_url)
    : broker_url_(broker_url) {
    curl_global_init(CURL_GLOBAL_DEFAULT);
}

MessagePublisher::~MessagePublisher() {
    curl_global_cleanup();
}

void MessagePublisher::publish(const std::string& bbbb,
                               const std::string& message,
                               int freq) {
    // ISO-8601 timestamp
    std::time_t now = std::time(nullptr);
    char ts[25];
    std::strftime(ts, sizeof(ts), "%Y-%m-%dT%H:%M:%SZ", std::gmtime(&now));

    nlohmann::json j;
    j["bbbb"]    = bbbb;
    j["message"] = message;
    j["freq"]    = freq;
    j["ts"]      = ts;
    std::string payload = j.dump();

    CURL* curl = curl_easy_init();
    if (!curl) {
        fprintf(stderr, "curl_easy_init() failed\n");
        return;
    }

    struct curl_slist* headers = nullptr;
    headers = curl_slist_append(headers, "Content-Type: application/json");

    curl_easy_setopt(curl, CURLOPT_URL, broker_url_.c_str());
    curl_easy_setopt(curl, CURLOPT_POSTFIELDS, payload.c_str());
    curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
    curl_easy_setopt(curl, CURLOPT_TIMEOUT, 5L);

    CURLcode res = curl_easy_perform(curl);
    if (res != CURLE_OK) {
        fprintf(stderr, "POST to %s failed: %s\n",
                broker_url_.c_str(), curl_easy_strerror(res));
    }

    curl_slist_free_all(headers);
    curl_easy_cleanup(curl);
}
```

**Step 4: Run test to verify it passes**

```bash
cd sdr-dsp && g++ -std=c++17 -pthread -o /tmp/test_pub tests/test_publisher.cpp src/message_publisher.cpp -lcurl -I third_party && /tmp/test_pub
```
Expected: `test_publisher PASS`

**Step 5: Commit**

```bash
git add sdr-dsp/src/message_publisher.h sdr-dsp/src/message_publisher.cpp \
        sdr-dsp/third_party/json.hpp sdr-dsp/tests/test_publisher.cpp
git commit -m "feat(sdr-dsp): implement HTTP POST message publisher to api-broker"
```

---

### Task 6: Wire the full pipeline in `main.cpp` with condition-variable IQ queue

**Files:**
- Modify: `sdr-dsp/src/main.cpp`
- Create: `sdr-dsp/src/iq_queue.h`

**Background:** The reference `capt_sched.c` uses a busy-wait loop (`usleep(50000)` + polling `in_idx != out_idx`). We replace this with a proper producer→consumer design:

- **Producer:** the SDRPlay `StreamACallback` pushes `{short I, short Q}` pairs into a thread-safe `IqQueue` (circular buffer + `std::condition_variable`)
- **Consumer:** a dedicated thread blocks on the condition variable, drains the queue, and feeds samples into the FIR1 → FIR2 → Decoder → NavBSm → MessagePublisher pipeline

**Step 1: Write the failing test**

Create `sdr-dsp/tests/test_iq_queue.cpp`:

```cpp
#include "../src/iq_queue.h"
#include <cassert>
#include <thread>
#include <vector>

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
```

**Step 2: Run test to verify it fails**

```bash
cd sdr-dsp && g++ -std=c++17 -pthread -o /tmp/test_iq tests/test_iq_queue.cpp && /tmp/test_iq
```
Expected: FAIL — `iq_queue.h` does not exist.

**Step 3: Write minimal implementation**

Create `sdr-dsp/src/iq_queue.h`:

```cpp
#pragma once
#include <mutex>
#include <condition_variable>
#include <vector>

class IqQueue {
public:
    explicit IqQueue(size_t capacity)
        : buf_(capacity * 2), capacity_(capacity) {}

    void push(short xi, short xq) {
        std::unique_lock<std::mutex> lk(mu_);
        // Block if full (back-pressure)
        not_full_.wait(lk, [&]{ return count_ < capacity_ || closed_; });
        if (closed_) return;
        buf_[write_pos_ * 2]     = xi;
        buf_[write_pos_ * 2 + 1] = xq;
        write_pos_ = (write_pos_ + 1) % capacity_;
        count_++;
        lk.unlock();
        not_empty_.notify_one();
    }

    // Returns false when queue is closed and empty
    bool pop(short& xi, short& xq) {
        std::unique_lock<std::mutex> lk(mu_);
        not_empty_.wait(lk, [&]{ return count_ > 0 || closed_; });
        if (count_ == 0) return false;
        xi = buf_[read_pos_ * 2];
        xq = buf_[read_pos_ * 2 + 1];
        read_pos_ = (read_pos_ + 1) % capacity_;
        count_--;
        lk.unlock();
        not_full_.notify_one();
        return true;
    }

    void close() {
        std::lock_guard<std::mutex> lk(mu_);
        closed_ = true;
        not_empty_.notify_all();
        not_full_.notify_all();
    }

private:
    std::vector<short> buf_;
    size_t capacity_;
    size_t write_pos_ = 0, read_pos_ = 0, count_ = 0;
    bool closed_ = false;
    std::mutex mu_;
    std::condition_variable not_empty_, not_full_;
};
```

Modify `sdr-dsp/src/main.cpp` to wire the full pipeline:

```cpp
#include "config.h"
#include "iq_queue.h"
#include "fir1.h"
#include "fir2.h"
#include "decoder.h"
#include "nav_b_sm.h"
#include "message_publisher.h"
#include <sdrplay_api.h>
#include <pthread.h>
#include <cstdio>
#include <thread>
#include <atomic>
#include <csignal>

static IqQueue* g_queue = nullptr;
static std::atomic<bool> g_running{true};

void stream_callback(short* xi, short* xq,
                     sdrplay_api_StreamCbParamsT* params,
                     unsigned int num_samples, unsigned int /*reset*/,
                     void* /*ctx*/) {
    for (unsigned int i = 0; i < num_samples; i++)
        g_queue->push(xi[i], xq[i]);
}

void consumer_thread() {
    MessagePublisher pub(CFG_BROKER_URL());

    NavBSm nav518(CFG_FREQ_UPPER(), [&](const std::string& b, const std::string& m, int f){
        pub.publish(b, m, f);
    });
    NavBSm nav490(CFG_FREQ_LOWER(), [&](const std::string& b, const std::string& m, int f){
        pub.publish(b, m, f);
    });

    Decoder dec518([&](char bit){ nav518.receive_bit(bit); });
    Decoder dec490([&](char bit){ nav490.receive_bit(bit); });

    // fir2 feeds two decoders (upper & lower channel split)
    Fir2 fir2_518([&](double I, double Q){ dec518.sample_in(I, Q); });
    Fir2 fir2_490([&](double I, double Q){ dec490.sample_in(I, Q); });

    Fir1 fir1([&](double I, double Q){
        fir2_518.sample_in(I, Q);
        fir2_490.sample_in(I, Q);
    });

    short xi, xq;
    while (g_queue->pop(xi, xq))
        fir1.sample_in((double)xi, (double)xq);
}

int main() {
    IqQueue queue(CFG_IN_SAMPLE_RATE() * 8); // 8 seconds buffer
    g_queue = &queue;

    std::thread consumer(consumer_thread);

    // SDRPlay API init (abbreviated — full error handling omitted for brevity)
    // See reference/backend/receiver/capt_sched.c for full init sequence
    sdrplay_api_Open();
    // ... device selection, param config using CFG_* functions from config.h ...
    sdrplay_api_CallbackFnsT cbs{ stream_callback, nullptr, nullptr };
    sdrplay_api_Init(/* dev */{}, &cbs, nullptr);

    // Run until interrupted
    std::signal(SIGINT, [](int){ g_running = false; });
    while (g_running) std::this_thread::sleep_for(std::chrono::seconds(1));

    queue.close();
    consumer.join();
    sdrplay_api_Uninit(/* dev */{});
    sdrplay_api_Close();
    return 0;
}
```

**Step 4: Run test to verify it passes**

```bash
cd sdr-dsp && g++ -std=c++17 -pthread -o /tmp/test_iq tests/test_iq_queue.cpp && /tmp/test_iq
```
Expected: `test_iq_queue PASS`

**Step 5: Commit**

```bash
git add sdr-dsp/src/iq_queue.h sdr-dsp/src/main.cpp
git commit -m "feat(sdr-dsp): wire full DSP pipeline with condition-variable IQ queue"
```

---

### Task 7: Update `docker-compose.yml` to wire the `sdr-dsp` container

**Files:**
- Modify: `docker-compose.yml`

**Background:** Replace the `image: alpine:latest` placeholder for `sdr-dsp` with a proper `build:` directive, add USB device passthrough, and set the config env vars.

**Step 1: Write the failing test**

```bash
docker-compose config
```
Expected: currently passes (alpine image). After change, must still pass with the local build context for `sdr-dsp`.

**Step 2: Run test to verify the baseline state**

Run: `docker-compose config`
Expected: PASS (shows `image: alpine:latest` for sdr-dsp service)

**Step 3: Write minimal implementation**

Replace the `sdr-dsp` service block in `docker-compose.yml`:

```yaml
  sdr-dsp:
    build:
      context: ./sdr-dsp
      dockerfile: Dockerfile
    container_name: sdr-dsp
    restart: on-failure
    privileged: true          # Required for USB SDR device access
    devices:
      - /dev/bus/usb:/dev/bus/usb
    networks:
      - navtex_net
    volumes:
      - sqlite_data:/data
    environment:
      - SDR_ANTENNA=A
      - SDR_LNA_STATE=0
      - BROKER_URL=http://api-broker:8000/messages
    depends_on:
      - api-broker
```

**Step 4: Run test to verify it passes**

Run: `docker-compose config`
Expected: PASS — valid config shown with `build.context: ./sdr-dsp`

**Step 5: Commit**

```bash
git add docker-compose.yml
git commit -m "chore: wire sdr-dsp container with USB passthrough and config env vars"
```

---

## Verification Plan

### Automated Tests

These tests run without SDR hardware and verify the DSP math and plumbing:

```bash
# From sdr-dsp/ directory:

# 1. FIR filter chain
g++ -std=c++17 -o /tmp/test_fir tests/test_fir.cpp src/fir1.cpp && /tmp/test_fir

# 2. FSK Decoder (no-signal / no-crash test)
g++ -std=c++17 -o /tmp/test_dec tests/test_decoder.cpp src/decoder.cpp && /tmp/test_dec

# 3. SITOR-B state machine (phasing detection)
g++ -std=c++17 -o /tmp/test_nav tests/test_nav_b_sm.cpp src/nav_b_sm.cpp && /tmp/test_nav

# 4. Message Publisher (HTTP POST round-trip)
g++ -std=c++17 -pthread -o /tmp/test_pub tests/test_publisher.cpp src/message_publisher.cpp -lcurl -I third_party && /tmp/test_pub

# 5. IQ Queue (producer/consumer with condition variable)
g++ -std=c++17 -pthread -o /tmp/test_iq tests/test_iq_queue.cpp && /tmp/test_iq

# 6. docker-compose config validation
docker-compose config
```

All 6 must print `PASS` or exit 0.

### Manual Verification (requires hardware)

> These steps require a connected SDRPlay RSPDX, to be performed after deploying to the Raspberry Pi.

1. Run `docker-compose up sdr-dsp api-broker`
2. Observe `sdr-dsp` container logs — should print `phasing detected` within the first Navtex transmission slot (occurs every 4 minutes on 518kHz)
3. Within ~10 minutes, a full Navtex message (`ZCZC ... NNNN`) should appear in the logs
4. Verify the message was received by the api-broker: `curl http://localhost:8000/messages` (once the messages endpoint is wired)
