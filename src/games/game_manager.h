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
        bool start_game(Games::GameType gameType);
        void stop_current_game();
        void update();
        
        Games::GameType get_current_game() const { return currentGame; }
        bool isAnyGameActive() const { return currentGame != Games::GameType::NONE; }
        
        // Button input handling
        void handle_button_press(bool rightPressed);
        
    private:
        GameManager() = default;
        
        void disable_current_game();
        bool enable_game(Games::GameType gameType);
        const char* get_game_name(Games::GameType gameType) const;
        
        Games::GameType currentGame = Games::GameType::NONE;
};
