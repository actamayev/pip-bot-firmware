#pragma once

#include <Arduino.h>

#include "networking/serial_queue_manager.h"
#include "utils/singleton.h"
#include "utils/structs.h"

namespace games {
enum class GameType : uint8_t { NONE, DINO_RUNNER };
}

class GameManager : public Singleton<GameManager> {
    friend class Singleton<GameManager>;

  public:
    bool start_game(games::GameType game_type);
    void stop_current_game();
    void update();

    games::GameType get_current_game() const {
        return _current_game;
    }
    bool is_any_game_active() const {
        return _current_game != games::GameType::NONE;
    }

    // Button input handling
    void handle_button_press(bool right_pressed);

  private:
    GameManager() = default;

    void disable_current_game();
    bool enable_game(games::GameType gameType);
    const char* get_game_name(games::GameType gameType) const;

    games::GameType _current_game = games::GameType::NONE;
};
