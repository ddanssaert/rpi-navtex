#include "control_server.h"
#include "httplib.h"
#include <json.hpp>
#include <cstdio>
#include <string>

using nlohmann::json;

ControlServer::ControlServer(sdrplay_api_DeviceT* device,
                             sdrplay_api_DeviceParamsT* params,
                             std::mutex& sdr_mutex)
    : device_(device), params_(params), sdr_mutex_(sdr_mutex),
      svr_(std::make_unique<httplib::Server>()) {}

ControlServer::~ControlServer() { stop(); }

void ControlServer::start(const char* host, int port) {
    svr_->Get("/control/health", [this](const httplib::Request&, httplib::Response& res) {
        if (!streaming_active.load()) {
            res.status = 503;
            res.set_content("{\"status\":\"starting\"}", "application/json");
            return;
        }
        res.set_content("ok", "text/plain");
    });

    svr_->Post("/control/config",
        [this](const httplib::Request& req, httplib::Response& res) {
            if (!streaming_active.load()) {
                res.status = 503;
                res.set_content("{\"status\":\"error\",\"detail\":\"streaming not active\"}",
                                "application/json");
                return;
            }

            json body;
            try { body = json::parse(req.body); }
            catch (const std::exception& e) {
                res.status = 400;
                res.set_content(std::string("{\"status\":\"error\",\"detail\":\"invalid json: ")
                                + e.what() + "\"}", "application/json");
                return;
            }

            // Validate fields
            if (!body.contains("antenna") || !body["antenna"].is_string()) {
                res.status = 400;
                res.set_content("{\"status\":\"error\",\"detail\":\"antenna must be string\"}",
                                "application/json");
                return;
            }
            if (!body.contains("lna_gain") || !body["lna_gain"].is_number_integer()) {
                res.status = 400;
                res.set_content("{\"status\":\"error\",\"detail\":\"lna_gain must be int\"}",
                                "application/json");
                return;
            }
            std::string ant_s = body["antenna"].get<std::string>();
            int lna = body["lna_gain"].get<int>();
            if (ant_s.size() != 1 || ant_s[0] < 'A' || ant_s[0] > 'C') {
                res.status = 400;
                res.set_content("{\"status\":\"error\",\"detail\":\"antenna must be A, B, or C\"}",
                                "application/json");
                return;
            }
            if (lna < 0 || lna > 9) {
                res.status = 400;
                res.set_content("{\"status\":\"error\",\"detail\":\"lna_gain must be 0..9\"}",
                                "application/json");
                return;
            }
            char ant = ant_s[0];

            // Apply under mutex (stream callback runs on a different thread)
            std::lock_guard<std::mutex> lock(sdr_mutex_);

            // 1) LNA gain — works on all RSP devices
            params_->rxChannelA->tunerParams.gain.LNAstate = (unsigned char)lna;
            sdrplay_api_ErrT e1 = sdrplay_api_Update(
                device_->dev, device_->tuner,
                sdrplay_api_Update_Tuner_Gr,
                sdrplay_api_Update_Ext1_None);

            // 2) Antenna — device-specific dispatch (mirror main.cpp)
            sdrplay_api_ErrT e2 = sdrplay_api_Success;
            if (device_->hwVer == SDRPLAY_RSPdx_ID || device_->hwVer == SDRPLAY_RSPdxR2_ID) {
                switch (ant) {
                    case 'B': params_->devParams->rspDxParams.antennaSel = sdrplay_api_RspDx_ANTENNA_B; break;
                    case 'C': params_->devParams->rspDxParams.antennaSel = sdrplay_api_RspDx_ANTENNA_C; break;
                    default:  params_->devParams->rspDxParams.antennaSel = sdrplay_api_RspDx_ANTENNA_A; break;
                }
                e2 = sdrplay_api_Update(device_->dev, device_->tuner,
                                        sdrplay_api_Update_None,
                                        sdrplay_api_Update_RspDx_AntennaControl);
            } else if (device_->hwVer == SDRPLAY_RSP2_ID) {
                params_->rxChannelA->rsp2TunerParams.antennaSel =
                    (ant == 'B') ? sdrplay_api_Rsp2_ANTENNA_B : sdrplay_api_Rsp2_ANTENNA_A;
                e2 = sdrplay_api_Update(device_->dev, device_->tuner,
                                        sdrplay_api_Update_Rsp2_AntennaControl,
                                        sdrplay_api_Update_Ext1_None);
            }
            // RSP1/1A/1B: single antenna — no update call needed.

            if (e1 != sdrplay_api_Success || e2 != sdrplay_api_Success) {
                res.status = 500;
                json err;
                err["status"] = "error";
                err["detail"] = "sdrplay_api_Update failed";
                err["lna_err"] = sdrplay_api_GetErrorString(e1);
                err["antenna_err"] = sdrplay_api_GetErrorString(e2);
                res.set_content(err.dump(), "application/json");
                fprintf(stderr, "control_server: update failed lna=%s ant=%s\n",
                        sdrplay_api_GetErrorString(e1), sdrplay_api_GetErrorString(e2));
                return;
            }

            printf("control_server: applied antenna=%c lna_gain=%d\n", ant, lna);
            fflush(stdout);
            json ok;
            ok["status"] = "ok";
            ok["antenna"] = std::string(1, ant);
            ok["lna_gain"] = lna;
            res.set_content(ok.dump(), "application/json");
        });

    thr_ = std::thread([this, host, port]() {
        printf("control_server: listening on %s:%d\n", host, port);
        fflush(stdout);
        svr_->listen(host, port);  // blocking
    });
}

void ControlServer::stop() {
    if (svr_) svr_->stop();
    if (thr_.joinable()) thr_.join();
}
