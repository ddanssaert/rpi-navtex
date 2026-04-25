#pragma once
#include <cstdlib>
#include <string>

// --- SDR hardware ---
inline int    CFG_FREQ_TUNER()  { return 504000; }   // (490+518)/2 kHz
inline int    CFG_FREQ_UPPER()  { return 518000; }
inline int    CFG_FREQ_LOWER()  { return 490000; }
inline double CFG_IN_SAMPLE_RATE() { return 256000.0; }
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
