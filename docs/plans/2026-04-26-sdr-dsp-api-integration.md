# SDR-DSP API Integration Implementation Plan

> **For Antigravity:** REQUIRED WORKFLOW: Use `.agent/workflows/execute-plan.md` to execute this plan in single-flow mode.

**Goal:** Integrate the SDR-DSP C++ pipeline to call the `POST /messages` endpoint with the correct schema when a Navtex message is decoded.

**Architecture:** Update the `MessagePublisher` class in the C++ DSP pipeline to parse the 4-character Navtex designator (B1B2B3B4) into `station_id`, `message_type`, and `serial_id`, and send these along with the message content as a JSON payload to the API broker.

**Tech Stack:** C++, nlohmann/json, POSIX sockets, FastAPI (API side).

---

### Task 1: Update Test for New Schema

**Files:**
- Modify: [test_publisher.cpp](file:///home/david/git/rpi-navtex/sdr-dsp/tests/test_publisher.cpp)

**Step 1: Write the failing test**

Update `sdr-dsp/tests/test_publisher.cpp` to expect the new JSON fields.

```cpp
// Change assertions in sdr-dsp/tests/test_publisher.cpp:52-53
    assert(received.find("\"station_id\":\"O\"") != std::string::npos);
    assert(received.find("\"message_type\":\"B\"") != std::string::npos);
    assert(received.find("\"serial_id\":1") != std::string::npos);
    assert(received.find("\"content\":") != std::string::npos);
    assert(received.find("\"raw_data\":") != std::string::npos);
```

**Step 2: Run test to verify it fails**

Run: `make -C sdr-dsp/tests test_publisher && ./sdr-dsp/tests/test_publisher`
Expected: FAIL with assertion error.

---

### Task 2: Implement New Schema in MessagePublisher

**Files:**
- Modify: [message_publisher.cpp](file:///home/david/git/rpi-navtex/sdr-dsp/src/message_publisher.cpp)

**Step 1: Update `publish` implementation**

Modify `sdr-dsp/src/message_publisher.cpp:37-50` to parse `bbbb` and use the new schema.

```cpp
void MessagePublisher::publish(const std::string& bbbb,
                               const std::string& message,
                               int freq) {
    // ... timestamp logic ...

    // Parse Navtex designator (B1B2B3B4)
    std::string station_id = "";
    std::string message_type = "";
    int serial_id = 0;
    if (bbbb.size() >= 2) {
        station_id += bbbb[0];
        message_type += bbbb[1];
    }
    if (bbbb.size() >= 4) {
        try {
            serial_id = std::stoi(bbbb.substr(2, 2));
        } catch (...) {}
    }

    nlohmann::json j;
    j["station_id"]   = station_id;
    j["message_type"] = message_type;
    j["serial_id"]    = serial_id;
    j["content"]      = message; // API expects content
    j["raw_data"]     = "ZCZC " + bbbb + "\n" + message + "\nNNNN";
    std::string payload = j.dump();

    // ... socket logic ...
}
```

**Step 2: Run test to verify it passes**

Run: `make -C sdr-dsp/tests test_publisher && ./sdr-dsp/tests/test_publisher`
Expected: PASS

**Step 3: Commit**

```bash
git add sdr-dsp/src/message_publisher.cpp sdr-dsp/tests/test_publisher.cpp
git commit -m "feat: update message publisher to match API schema"
```

---

### Task 3: Final Verification with Docker Compose

**Step 1: Restart services and check logs**

Run: `docker compose up -d --build`
Expected: Services start.

**Step 2: Verify logs**

Run: `docker compose logs -f sdr-dsp api-broker`
Expected: No errors in logs. If a message is decoded (or simulated), it should hit the API.

**Step 3: Commit any final tweaks**

```bash
git commit -m "chore: final integration verification"
```
