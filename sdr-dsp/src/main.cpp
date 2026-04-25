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
static std::atomic<double> g_energy_sum{0.0};
static std::atomic<long> g_energy_count{0};
static std::atomic<short> g_last_xi{0}, g_last_xq{0};

static void stream_callback(short* xi, short* xq,
                            sdrplay_api_StreamCbParamsT* /*params*/,
                            unsigned int num_samples, unsigned int /*reset*/,
                            void* /*ctx*/) {
    for (unsigned int i = 0; i < num_samples; i++) {
        g_queue->push(xi[i], xq[i]);
        // Simple energy estimation: I^2 + Q^2
        g_energy_sum = g_energy_sum + (double)xi[i]*xi[i] + (double)xq[i]*xq[i];
        g_energy_count++;
        if (i == 0) {
            g_last_xi = xi[0];
            g_last_xq = xq[0];
        }
    }
}

static void consumer_thread() {
    MessagePublisher pub(CFG_BROKER_URL());

    NavBSm nav518(CFG_FREQ_UPPER(), [&](const std::string& b, const std::string& m, int f){
        pub.publish(b, m, f);
    });
    NavBSm nav490(CFG_FREQ_LOWER(), [&](const std::string& b, const std::string& m, int f){
        pub.publish(b, m, f);
    });

    Decoder dec518([&](char bit){ nav518.receive_bit(bit); });
    Decoder dec490([&](char bit){ nav490.receive_bit(bit); });

    // fir2 splits the signal into upper (518kHz) and lower (490kHz) channels
    Fir2 fir2(
        [&](double I, double Q){ dec518.sample_in(I, Q); },  // 518 channel
        [&](double I, double Q){ dec490.sample_in(I, Q); }   // 490 channel
    );

    Fir1 fir1([&](double I, double Q){
        fir2.sample_in(I, Q);
    });

    short xi, xq;
    while (g_queue->pop(xi, xq))
        fir1.sample_in((double)xi, (double)xq);
}

int main() {
    sdrplay_api_ErrT err;
    sdrplay_api_DeviceT devs[6];
    unsigned int numDevs = 0;
    sdrplay_api_DeviceParamsT* params = nullptr;

    // Initialize API
    printf("sdr-dsp: checking SDRPlay API service...\n"); fflush(stdout);
    if ((err = sdrplay_api_Open()) != sdrplay_api_Success) {
        fprintf(stderr, "sdrplay_api_Open error: %s\n", sdrplay_api_GetErrorString(err));
        return 1;
    }
    printf("sdr-dsp: API opened.\n"); fflush(stdout);

    // Get device list
    printf("sdr-dsp: enumerating devices...\n"); fflush(stdout);
    if ((err = sdrplay_api_GetDevices(devs, &numDevs, 6)) != sdrplay_api_Success) {
        fprintf(stderr, "sdrplay_api_GetDevices error: %s\n", sdrplay_api_GetErrorString(err));
        sdrplay_api_Close();
        return 1;
    }
    printf("sdr-dsp: found %u device(s).\n", numDevs); fflush(stdout);

    if (numDevs == 0) {
        fprintf(stderr, "Error: No SDRPlay devices found.\n");
        sdrplay_api_Close();
        return 1;
    }

    // Select the first device
    sdrplay_api_DeviceT chosenDev = devs[0];
    printf("sdr-dsp: selecting device %s...\n", chosenDev.SerNo); fflush(stdout);
    if ((err = sdrplay_api_SelectDevice(&chosenDev)) != sdrplay_api_Success) {
        fprintf(stderr, "sdrplay_api_SelectDevice error: %s\n", sdrplay_api_GetErrorString(err));
        sdrplay_api_Close();
        return 1;
    }

    // Get device parameters
    printf("sdr-dsp: fetching parameters...\n"); fflush(stdout);
    if ((err = sdrplay_api_GetDeviceParams(chosenDev.dev, &params)) != sdrplay_api_Success) {
        fprintf(stderr, "sdrplay_api_GetDeviceParams error: %s\n", sdrplay_api_GetErrorString(err));
        sdrplay_api_Close();
        return 1;
    }

    // Configure RF frequency and sample rate
    // WE USE THE STANDARD 2.048 MHz BASE RATE
    params->devParams->fsFreq.fsHz = 2048000.0;
    params->rxChannelA->tunerParams.rfFreq.rfHz = (double)CFG_FREQ_TUNER() * 1000.0;
    params->rxChannelA->tunerParams.gain.LNAstate = CFG_LNA_STATE();

    // Hardware decimation
    params->rxChannelA->ctrlParams.decimation.enable         = 1;
    params->rxChannelA->ctrlParams.decimation.decimationFactor= (unsigned char)CFG_H_DECIMATION();

    // Antenna selection (A/B/HiZ — mapped from env CFG_ANTENNA)
    char ant = CFG_ANTENNA();
    printf("sdr-dsp: configuring antenna %c (LNA state: %d)...\n", ant, CFG_LNA_STATE());
    switch (ant) {
        case 'B': params->devParams->rspDxParams.antennaSel = sdrplay_api_RspDx_ANTENNA_B; break;
        case 'C': params->devParams->rspDxParams.antennaSel = sdrplay_api_RspDx_ANTENNA_C; break;
        default:  params->devParams->rspDxParams.antennaSel = sdrplay_api_RspDx_ANTENNA_A; break;
    }
    fflush(stdout);

    sdrplay_api_CallbackFnsT cbs{};
    cbs.StreamACbFn = stream_callback;

    printf("sdr-dsp: initializing hardware (Base: %.2f MHz, Decim: %d)...\n", 
           params->devParams->fsFreq.fsHz / 1e6, CFG_H_DECIMATION()); fflush(stdout);
    if ((err = sdrplay_api_Init(chosenDev.dev, &cbs, nullptr)) != sdrplay_api_Success) {
        fprintf(stderr, "sdrplay_api_Init error: %s\n", sdrplay_api_GetErrorString(err));
        sdrplay_api_Close();
        return 1;
    }

    // Initialize IQ Queue (8 seconds buffer)
    IqQueue queue((size_t)(CFG_IN_SAMPLE_RATE() * 8));
    g_queue = &queue;

    // Start consumer thread ONLY after hardware is ready
    std::thread consumer(consumer_thread);

    printf("sdr-dsp: streaming started (%.0f kHz → fir1 → fir2 → decoder → nav_b_sm → broker)\n",
           CFG_IN_SAMPLE_RATE() / 1000.0);
    fflush(stdout);

    // Run until interrupted
    std::signal(SIGINT, [](int){ g_running = false; });
    
    static short last_xi = 0, last_xq = 0;
    int log_divider = 0;
    while (g_running) {
        std::this_thread::sleep_for(std::chrono::seconds(1));
        if (++log_divider >= 5) {
            double avg = g_energy_count > 0 ? std::sqrt(g_energy_sum / g_energy_count) : 0;
            printf("sdr-dsp: avg signal level: %.1f (samples processed: %ld)\n", avg, (long)g_energy_count);
            // Check if the signal is varying (crude check)
            printf("sdr-dsp: raw sample check: xi=%d, xq=%d\n", (int)g_last_xi, (int)g_last_xq);
            fflush(stdout);
            g_energy_sum = 0;
            g_energy_count = 0;
            log_divider = 0;
        }
    }

    printf("sdr-dsp: shutting down...\n"); fflush(stdout);
    queue.close();
    consumer.join();
    sdrplay_api_Uninit(chosenDev.dev);
    sdrplay_api_Close();
    return 0;
}
