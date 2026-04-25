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

static void stream_callback(short* xi, short* xq,
                            sdrplay_api_StreamCbParamsT* /*params*/,
                            unsigned int num_samples, unsigned int /*reset*/,
                            void* /*ctx*/) {
    for (unsigned int i = 0; i < num_samples; i++)
        g_queue->push(xi[i], xq[i]);
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
    // 8 seconds of IQ buffer at 252kHz input sample rate
    IqQueue queue((size_t)(CFG_IN_SAMPLE_RATE() * 8));
    g_queue = &queue;

    std::thread consumer(consumer_thread);

    // SDRPlay API init — see reference/backend/receiver/capt_sched.c for full
    // error handling and device enumeration. Abbreviated here for clarity.
    sdrplay_api_Open();

    sdrplay_api_DeviceT device{};
    unsigned int num_devices = 0;
    sdrplay_api_GetDevices(&device, &num_devices, 1);
    sdrplay_api_SelectDevice(&device);

    sdrplay_api_DeviceParamsT* params = nullptr;
    sdrplay_api_GetDeviceParams(device.dev, &params);

    // Configure RF frequency and sample rate
    params->devParams->fsFreq.fsHz = CFG_IN_SAMPLE_RATE();
    params->rxChannelA->tunerParams.rfFreq.rfHz = (double)CFG_FREQ_TUNER() * 1000.0;
    params->rxChannelA->tunerParams.gain.LNAstate = CFG_LNA_STATE();

    // Hardware decimation
    params->rxChannelA->ctrlParams.decimation.enable         = 1;
    params->rxChannelA->ctrlParams.decimation.decimationFactor= (unsigned char)CFG_H_DECIMATION();

    // Antenna selection (A/B/HiZ — mapped from env CFG_ANTENNA)
    switch (CFG_ANTENNA()) {
        case 'B': params->rxChannelA->tunerParams.rfFreq.nodeNum = sdrplay_api_RspDx_ANTENNA_B; break;
        case 'C': params->rxChannelA->tunerParams.rfFreq.nodeNum = sdrplay_api_RspDx_ANTENNA_C; break;
        default:  params->rxChannelA->tunerParams.rfFreq.nodeNum = sdrplay_api_RspDx_ANTENNA_A; break;
    }

    sdrplay_api_CallbackFnsT cbs{};
    cbs.StreamACbFn = stream_callback;
    sdrplay_api_Init(device.dev, &cbs, nullptr);

    printf("sdr-dsp: streaming started (%.0f kHz → fir1 → fir2 → decoder → nav_b_sm → broker)\n",
           CFG_IN_SAMPLE_RATE() / 1000.0);

    // Run until interrupted
    std::signal(SIGINT, [](int){ g_running = false; });
    while (g_running) std::this_thread::sleep_for(std::chrono::seconds(1));

    printf("sdr-dsp: shutting down...\n");
    queue.close();
    consumer.join();
    sdrplay_api_Uninit(device.dev);
    sdrplay_api_Close();
    return 0;
}
