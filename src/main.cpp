#include <Geode/Geode.hpp>
#include <Geode/modify/GJBaseGameLayer.hpp>
#include <Geode/modify/PlayLayer.hpp>
#include <string>
#include <map>
#include <cmath>

using namespace geode::prelude;

// Global engine state tracking container
struct FrameTracker {
    int last_click_frame = 0;
    int current_frame = 0;
    int total_inputs = 0;
    std::map<int, int> window_buckets;
    float total_sigma_precision = 0.0f; 
};

static FrameTracker g_tracker;
CCLabelBMFont* g_nan_hud_label = nullptr;

// Classic NaN GD color tier mapping profile
ccColor3B getNaNGDColor(int window) {
    if (window == 1) return {255, 0, 0};       // Red (☠️ Frame Perfect)
    if (window == 2) return {255, 127, 0};     // Orange
    if (window == 3) return {255, 255, 0};     // Yellow
    if (window >= 4 && window <= 6) return {0, 255, 0}; // Green
    return {0, 191, 255};                      // Blue
}

// Router to trigger the customized audio layers
void playWindowSound(int window) {
    auto sfx_engine = FMODAudioEngine::sharedEngine();
    if (!sfx_engine) return;

    std::string sfx_file = "";

    // Maps audio assets relative to the compiled resources package
    if (window == 1) {
        sfx_file = "frame_perfect.mp3";
    } else if (window == 2 || window == 3) {
        sfx_file = "tight_window.mp3";
    } else {
        sfx_file = "normal_click.mp3";
    }

    sfx_engine->playEffect(sfx_file);
}

class $modify(MyGameLayer, GJBaseGameLayer) {
    void update(float dt) {
        GJBaseGameLayer::update(dt);
        g_tracker.current_frame++;
    }

    void handleButton(bool down, int buttonId, bool isPlayer1) {
        GJBaseGameLayer::handleButton(down, buttonId, isPlayer1);

        if (down && isPlayer1 && m_player1) {
            if (g_tracker.last_click_frame != 0) {
                int delta_frames = g_tracker.current_frame - g_tracker.last_click_frame;

                if (delta_frames >= 1 && delta_frames <= 20) {
                    g_tracker.window_buckets[delta_frames]++;
                    g_tracker.total_inputs++;
                    
                    // Math computing custom continuous NaNDL precision statistics 
                    float precision_weight = 10.0f / static_cast<float>(delta_frames);
                    g_tracker.total_sigma_precision += precision_weight;

                    // Trigger the visual animations and audio nodes
                    playWindowSound(delta_frames);
                    spawnVisualCircle(delta_frames);
                    updateNaNDLHUD(delta_frames);
                }
            }
            g_tracker.last_click_frame = g_tracker.current_frame;
        }
    }

    // Renders the iconic expanding and fading timing dot layout over the player node
    void spawnVisualCircle(int window) {
        auto player_pos = m_player1->getPosition();
        auto dot = CCDrawNode::create();
        ccColor3B dot_color = getNaNGDColor(window);
        
        float radius = 12.0f - (window * 0.3f);
        if (radius < 4.0f) radius = 4.0f;
        
        dot->drawDot({0, 0}, radius, ccc4f(dot_color.r/255.0f, dot_color.g/255.0f, dot_color.b/255.0f, 1.0f));
        dot->setPosition(player_pos);
        this->addChild(dot, 1000);

        std::string label_str = std::to_string(window);
        auto text_marker = CCLabelBMFont::create(label_str.c_str(), "chatFont.fnt");
        text_marker->setPosition({player_pos.x, player_pos.y + 20.0f});
        text_marker->setScale(0.5f);
        text_marker->setColor(dot_color);
        this->addChild(text_marker, 1001);

        // Core animation layer rules (fading out while drifting upward)
        auto fade_out = CCFadeOut::create(0.4f);
        auto move_up = CCMoveBy::create(0.4f, {0, 15.0f});
        auto spawn_actions = CCSpawn::create(fade_out, move_up, nullptr);
        auto remove_node = CCRemoveSelf::create();
        auto sequence = CCSequence::create(spawn_actions, remove_node, nullptr);

        dot->runAction(safe_cast<CCAction*>(sequence->clone()));
        text_marker->runAction(sequence);
    }

    void updateNaNDLHUD(int latest_window) {
        if (!g_nan_hud_label) return;

        float live_sigma = 0.0f;
        if (g_tracker.total_inputs > 0) {
            live_sigma = (g_tracker.total_sigma_precision / (g_tracker.current_frame / 240.0f)) * 15.0f;
        }

        std::string hud_text = "F-Window: " + std::to_string(latest_window) + 
                               " | NaNDL Precision: " + std::to_string(static_cast<int>(live_sigma)) + " σ/s";
        
        g_nan_hud_label->setString(hud_text.c_str());
        g_nan_hud_label->setColor(getNaNGDColor(latest_window));
    }
};

class $modify(MyPlayLayer, PlayLayer) {
    bool init(GJGameLevel* level, bool useReplay, bool dontRun) {
        if (!PlayLayer::init(level, useReplay, dontRun)) return false;

        g_tracker = FrameTracker();

        auto winSize = CCDirector::get()->getWinSize();
        g_nan_hud_label = CCLabelBMFont::create("F-Window: None | NaNDL: 0 σ/s", "goldFont.fnt");
        g_nan_hud_label->setScale(0.45f);
        g_nan_hud_label->setPosition({winSize.width / 2, winSize.height - 20.0f});
        
        this->addChild(g_nan_hud_label, 999);
        return true;
    }
};
