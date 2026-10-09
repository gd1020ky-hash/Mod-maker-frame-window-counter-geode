#include <Geode/Geode.hpp>
#include <Geode/modify/GJBaseGameLayer.hpp>
#include <Geode/modify/PlayLayer.hpp>
#include <string>
#include <map>
#include <format> // C++23 現代化字串格式化工具

using namespace geode::prelude;

// 全局遊戲狀態追蹤結構
struct FrameTracker {
    int last_click_frame = 0;
    int current_frame = 0;
    int total_inputs = 0;
    std::map<int, int> window_buckets;
    float total_sigma_precision = 0.0f; 
};

static FrameTracker g_tracker;
CCLabelBMFont* g_nan_hud_label = nullptr;

// 經典 NaN GD 顏色分配矩陣
ccColor3B getNaNGDColor(int window) {
    if (window == 1) return {255, 0, 0};       // 紅色 (☠️ 1幀神蹟)
    if (window == 2) return {255, 127, 0};     // 橘色
    if (window == 3) return {255, 255, 0};     // 黃色
    if (window >= 4 && window <= 6) return {0, 255, 0}; // 綠色
    return {0, 191, 255};                      // 藍色
}

// 播放 NaN GD 經典漸變音效（按幀縮小降低音調）
void playWindowSound(int window) {
    // 💡 修正：Geode v5.x 移除舊版音效引擎，全面改用內建的 FMOD 系統指引
    auto* fmod_sys = log_cast<FMOD::System*>(FMODAudioEngine::sharedEngine()->m_system);
    if (!fmod_sys) return;

    // 基礎音調設定：當點擊幀越接近 1 幀（越難），音調就越低沉、沉重
    float pitch_modifier = 0.5f + (static_cast<float>(window - 1) * 0.15f);
    if (pitch_modifier > 2.0f) pitch_modifier = 2.0f; 

    // 調用 Geode 檔案管理員載入剛剛在 mod.json 綁定的音效檔案
    auto sfx_path = Mod::get()->getResourcesDir() / "nan_bell.mp3";
    
    FMOD::Sound* sound = nullptr;
    fmod_sys->createSound(sfx_path.string().c_str(), FMOD_DEFAULT, nullptr, &sound);
    
    if (sound) {
        FMOD::Channel* channel = nullptr;
        fmod_sys->playSound(sound, nullptr, false, &channel);
        if (channel) {
            channel->setPitch(pitch_modifier);
        }
    }
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
                    
                    float precision_weight = 10.0f / static_cast<float>(delta_frames);
                    g_tracker.total_sigma_precision += precision_weight;

                    // 觸發音效與動畫層
                    playWindowSound(delta_frames);
                    spawnVisualCircle(delta_frames);
                    updateNaNDLHUD(delta_frames);
                }
            }
            g_tracker.last_click_frame = g_tracker.current_frame;
        }
    }

    // 在玩家方塊上方渲染動態浮動擴散圈圈
    void spawnVisualCircle(int window) {
        auto player_pos = m_player1->getPosition();
        auto* dot = CCDrawNode::create();
        ccColor3B dot_color = getNaNGDColor(window);
        
        float radius = 12.0f - (window * 0.3f);
        if (radius < 4.0f) radius = 4.0f;
        
        dot->drawDot({0, 0}, radius, ccc4f(dot_color.r/255.0f, dot_color.g/255.0f, dot_color.b/255.0f, 1.0f));
        dot->setPosition(player_pos);
        this->addChild(dot, 1000);

        auto text_str = std::to_string(window);
        auto* text_marker = CCLabelBMFont::create(text_str.c_str(), "chatFont.fnt");
        text_marker->setPosition({player_pos.x, player_pos.y + 20.0f});
        text_marker->setScale(0.5f);
        text_marker->setColor(dot_color);
        this->addChild(text_marker, 1001);

        // 漸隱並向上飄移的 Cocos2d 動態軌跡
        auto* fade_out = CCFadeOut::create(0.4f);
        auto* move_up = CCMoveBy::create(0.4f, {0, 15.0f});
        auto* spawn_actions = CCSpawn::create(fade_out, move_up, nullptr);
        auto* remove_node = CCRemoveSelf::create();
        auto* sequence = CCSequence::create(spawn_actions, remove_node, nullptr);

        dot->runAction(static_cast<CCAction*>(sequence->clone()));
        text_marker->runAction(sequence);
    }

    void updateNaNDLHUD(int latest_window) {
        if (!g_nan_hud_label) return;

        float live_sigma = 0.0f;
        if (g_tracker.total_inputs > 0) {
            live_sigma = (g_tracker.total_sigma_precision / (g_tracker.current_frame / 240.0f)) * 15.0f;
        }

        // 使用 C++23 的 std::format 進行高速字串安全拼接，杜絕舊版崩潰
        std::string hud_text = std::format("F-Window: {} | NaNDL Precision: {} σ/s", latest_window, static_cast<int>(live_sigma));
        
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
3
