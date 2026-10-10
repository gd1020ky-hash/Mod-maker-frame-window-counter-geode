#include <Geode/Geode.hpp>
#include <Geode/modify/PlayLayer.hpp>
#include <Geode/modify/PauseLayer.hpp>
#include <Geode/modify/PlayerObject.hpp>
#include <fstream>
#include <vector>
#include "nandl.hpp"

using namespace geode::prelude;

struct $modify(FBPlayLayer, PlayLayer) {
    struct Fields {
        std::vector<NaNdL::FrameAction> sessionActions;
        std::vector<NaNdL::WindowPreset> windowPresets;
        NaNdL::PrecisionParams precisionParams;
        NaNdL::PrecisionResults precisionResults;
        
        CCLabelBMFont* liveHudLabel = nullptr;
        CCLabelBMFont* precisionHudLabel = nullptr;
        
        double lastInputTime = 0.0;
        int currentFrameIndex = 0;
    };

    bool init(GJGameLevel* level, bool secret, bool practice) {
        if (!PlayLayer::init(level, secret, practice)) return false;

        m_fields->windowPresets = {
            {1, 1, "1F", true, 0},
            {2, 2, "2F", true, 0},
            {3, 4, "Tight", true, 0}
        };

        auto winSize = CCDirector::sharedDirector()->getWinSize();

        m_fields->liveHudLabel = CCLabelBMFont::create("OP FWC: 0", "bigFont.fnt");
        m_fields->liveHudLabel->setScale(0.35f);
        m_fields->liveHudLabel->setAnchorPoint({0.0f, 1.0f});
        m_fields->liveHudLabel->setPosition({5.0f, winSize.height - 5.0f});
        m_fields->liveHudLabel->setZOrder(999);
        this->addChild(m_fields->liveHudLabel);

        m_fields->precisionHudLabel = CCLabelBMFont::create("L*: 0.00", "bigFont.fnt");
        m_fields->precisionHudLabel->setScale(0.35f);
        m_fields->precisionHudLabel->setAnchorPoint({0.0f, 0.0f});
        m_fields->precisionHudLabel->setPosition({5.0f, 5.0f});
        m_fields->precisionHudLabel->setZOrder(999);
        this->addChild(m_fields->precisionHudLabel);

        return true;
    }

    void update(float dt) {
        PlayLayer::update(dt);

        if (m_fields->liveHudLabel) {
            std::string hudText = fmt::format("Inputs: {}", m_fields->sessionActions.size());
            for (const auto& p : m_fields->windowPresets) {
                if (p.showInHud) hudText += fmt::format(" | {}: {}", p.labelText, p.hitCount);
            }
            m_fields->liveHudLabel->setString(hudText.c_str());
        }

        if (m_fields->precisionHudLabel && m_fields->precisionResults.isDirty) {
            m_fields->precisionResults = NaNdL::solve_all_dimensions(
                m_fields->sessionActions, m_fields->precisionParams
            );
            
            std::string lText = fmt::format(
                "L* Base: {:.2f} | Total: {:.2f}",
                m_fields->precisionResults.base_L,
                m_fields->precisionResults.All_L
            );
            m_fields->precisionHudLabel->setString(lText.c_str());
        }
    }

    void resetLevel() {
        if (!m_fields->sessionActions.empty()) {
            this->exportMacroFiles();
            m_fields->sessionActions.clear();
            for (auto& p : m_fields->windowPresets) p.hitCount = 0;
            m_fields->currentFrameIndex = 0;
            m_fields->lastInputTime = 0.0;
        }
        PlayLayer::resetLevel();
    }

    void exportMacroFiles() {
        matjson::Value root;
        root["mod"] = "OP frame window counter";
        root["level_id"] = m_level->m_levelID.value();
        root["level_name"] = std::string(m_level->m_levelName);
        root["l_star_final"] = m_fields->precisionResults.All_L;

        // V3 Safe JSON Array
        std::vector<matjson::Value> actArray;
        for (const auto& act : m_fields->sessionActions) {
            actArray.push_back(act.toJson());
        }
        root["inputs"] = actArray;

        std::filesystem::path saveDir = Mod::get()->getSaveDir();
        std::filesystem::create_directories(saveDir);
        std::filesystem::path fullPath = saveDir / fmt::format("run_{}.fwc.json", std::time(nullptr));

        std::ofstream outFile(fullPath);
        if (outFile.is_open()) {
            outFile << root.dump(matjson::NO_INDENT);
            outFile.close();
        }
    }
};

struct $modify(FBPlayerObject, PlayerObject) {
    void pushButton(PlayerButton btn) {
        PlayerObject::pushButton(btn);

        auto playLayer = PlayLayer::get();
        if (!playLayer) return;

        auto fbPlayLayer = static_cast<FBPlayLayer*>(playLayer);

        double currentTime = fbPlayLayer->m_gameState.m_levelTime;
        double deltaMs = (currentTime - fbPlayLayer->m_fields->lastInputTime) * 1000.0;
        if (deltaMs <= 0.0) deltaMs = 16.667;

        fbPlayLayer->m_fields->lastInputTime = currentTime;
        fbPlayLayer->m_fields->currentFrameIndex++;

        CCPoint camPos = playLayer->getPosition();
        float camScale = playLayer->getScale();
        
        bool isPlayer2 = (this == playLayer->m_player2);
        double yVel = this->m_yVelocity;

        NaNdL::FrameAction action;
        action.frame = fbPlayLayer->m_fields->currentFrameIndex;
        action.timestampSeconds = currentTime;
        action.windowMs = deltaMs;
        action.levelPercent = playLayer->getCurrentPercentInt();
        action.isPlayer2 = isPlayer2;
        action.camX = camPos.x;
        action.camY = camPos.y;
        action.camZoom = camScale;
        action.yVelocity = yVel;

        fbPlayLayer->m_fields->sessionActions.push_back(action);
        fbPlayLayer->m_fields->precisionResults.isDirty = true;

        int frames = std::max(1, static_cast<int>(std::round(deltaMs / (1000.0 / 240.0))));
        for (auto& preset : fbPlayLayer->m_fields->windowPresets) {
            if (frames >= preset.minFrames && frames <= preset.maxFrames) {
                preset.hitCount++;
            }
        }
    }
};

struct $modify(FBPauseLayer, PauseLayer) {
    void customSetup() {
        PauseLayer::customSetup();

        auto menu = this->getChildByID("right-button-menu");
        if (!menu) return;

        auto btnSprite = CCSprite::createWithSpriteFrameName("GJ_timeIcon_001.png");
        auto btn = CCMenuItemSpriteExtra::create(
            btnSprite,
            this,
            menu_selector(FBPauseLayer::onOpenFWCEditor)
        );
        menu->addChild(btn);
        menu->updateLayout();
    }

    void onOpenFWCEditor(CCObject* sender) {
        FLAlertLayer::create(
            "OP FWC NaNdL",
            "Data tracking active.\nCamera Matrices and Physics logging enabled.\nData exports to Android config folder on death.",
            "OK"
        )->show();
    }
};
