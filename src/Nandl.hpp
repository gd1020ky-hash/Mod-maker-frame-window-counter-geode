#pragma once
#include <cmath>
#include <algorithm>
#include <vector>
#include <string>
#include <Geode/utils/matjson.hpp>

namespace NaNdL {

    struct WindowPreset {
        int minFrames = 1;
        int maxFrames = 1;
        std::string labelText = "1F";
        bool showInHud = true;
        int hitCount = 0;
    };

    struct FrameAction {
        int frame = 0;
        double timestampSeconds = 0.0;
        double windowMs = 16.667;
        float levelPercent = 0.0f;
        bool isPlayer2 = false;
        
        float camX = 0.0f;
        float camY = 0.0f;
        float camZoom = 1.0f;
        double yVelocity = 0.0;

        // V3 Safe JSON Object Initialization
        matjson::Value toJson() const {
            matjson::Value obj; 
            obj["frame"] = frame;
            obj["timestamp"] = timestampSeconds;
            obj["window_ms"] = windowMs;
            obj["percent"] = levelPercent;
            obj["p2"] = isPlayer2;
            obj["cam_x"] = camX;
            obj["cam_y"] = camY;
            obj["cam_zoom"] = camZoom;
            obj["y_velocity"] = yVelocity;
            return obj;
        }
    };

    struct PrecisionParams {
        double targetTPS = 240.0;
        double K_T = 0.35;   
        double K_U = 0.0002; 
        double K_C = 0.05;   
    };

    struct PrecisionResults {
        double base_L = 0.0;
        double All_L = 0.0;
        bool isDirty = false;
    };

    inline double calculate_base_L(double frame_window_ms, double target_window_ms = 16.667) {
        if (frame_window_ms <= 0.0) return 1000.0;
        double sigma = target_window_ms / 3.0;
        double z = frame_window_ms / (sigma * std::sqrt(2.0));
        double erfc_val = std::erfc(z);
        if (erfc_val <= 1e-15) return 1000.0;
        return -std::log10(erfc_val) * 10.0;
    }

    inline PrecisionResults solve_all_dimensions(const std::vector<FrameAction>& actions, const PrecisionParams& params) {
        PrecisionResults results;
        double sumBase = 0.0;
        double sumNerve = 0.0;
        double sumFatigue = 0.0;
        double sumCPS = 0.0;

        int totalInputs = 0;

        for (const auto& act : actions) {
            totalInputs++;
            double base = calculate_base_L(act.windowMs, 1000.0 / params.targetTPS);
            
            double p = std::clamp(act.levelPercent / 100.0f, 0.0f, 1.0f);
            double kt = 1.0 + (params.K_T * std::pow(p, 2.5));
            double ku = 1.0 + (params.K_U * totalInputs);
            
            double cps = (totalInputs > 1 && act.timestampSeconds > 0) ? (totalInputs / act.timestampSeconds) : 1.0;
            double kc = 1.0 + (params.K_C * std::max(0.0, cps - 10.0));

            sumBase += base;
            sumNerve += base * kt;
            sumFatigue += base * ku;
            sumCPS += base * kc;
        }

        results.base_L = sumBase;
        if (sumBase > 0) {
            results.All_L = sumBase * (sumNerve / sumBase) * (sumFatigue / sumBase) * (sumCPS / sumBase);
        } else {
            results.All_L = 0.0;
        }
        results.isDirty = false;

        return results;
    }
}
