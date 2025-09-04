#pragma once

#include <Arduino.h>
#include "utils/singleton.h"
#include "utils/structs.h"
#include "networking/serial_queue_manager.h"

namespace Games {
    enum class GameType {
        NONE,
        DINO_RUNNER
    };
}

class GameManager : public Singleton<GameManager> {
    friend class Singleton<GameManager>;

    public:
        bool startGame(Games::GameType gameType);
        void stopCurrentGame();
        void update();
        
        Games::GameType getCurrentGame() const { return currentGame; }
        bool isAnyGameActive() const { return currentGame != Games::GameType::NONE; }
        
        // Button input handling
        void handleButtonPress(bool rightPressed);
        
    private:
        GameManager() = default;
        
        void disableCurrentGame();
        bool enableGame(Games::GameType gameType);
        const char* getGameName(Games::GameType gameType) const;
        
        Games::GameType currentGame = Games::GameType::NONE;
};
