#include "game_manager.h"

#include "dino_runner.h"

bool GameManager::start_game(games::GameType gameType) {
    if (gameType == _current_game) {
        SerialQueueManager::get_instance().queue_message("Game already running");
        return true;
    }

    // Stop current game if running
    DisplayScreen::get_instance().turn_display_on();
    if (_current_game != games::GameType::NONE) {
        stop_current_game();
    }

    return enable_game(gameType);
}

void GameManager::stop_current_game() {
    if (_current_game == games::GameType::NONE) return;

    SerialQueueManager::get_instance().queue_message("Stopping game: " + String(get_game_name(_current_game)));
    disable_current_game();
    _current_game = games::GameType::NONE;
}

void GameManager::update() {
    if (_current_game == games::GameType::NONE) return;

    switch (_current_game) {
        case games::GameType::DINO_RUNNER:
            DinoRunner::get_instance().update();
            break;
        default:
            break;
    }
}

void GameManager::handle_button_press(bool right_pressed) {
    if (_current_game == games::GameType::NONE) return;

    switch (_current_game) {
        case games::GameType::DINO_RUNNER:
            DinoRunner::get_instance().handle_button_press(right_pressed);
            break;
        default:
            break;
    }
}

void GameManager::disable_current_game() {
    switch (_current_game) {
        case games::GameType::DINO_RUNNER:
            DinoRunner::get_instance().stop_game();
            break;
        default:
            break;
    }
}

bool GameManager::enable_game(games::GameType gameType) {
    SerialQueueManager::get_instance().queue_message("Starting game: " + String(get_game_name(gameType)));

    bool success = false;
    switch (gameType) {
        case games::GameType::DINO_RUNNER:
            DinoRunner::get_instance().start_game();
            success = DinoRunner::get_instance().is_game_active();
            break;
        default:
            SerialQueueManager::get_instance().queue_message("Unknown game type");
            return false;
    }

    if (success) {
        _current_game = gameType;
        SerialQueueManager::get_instance().queue_message("Game started successfully");
    } else {
        SerialQueueManager::get_instance().queue_message("Failed to start game");
    }

    return success;
}

const char* GameManager::get_game_name(games::GameType gameType) const {
    switch (gameType) {
        case games::GameType::NONE:
            return "None";
        case games::GameType::DINO_RUNNER:
            return "Dino Runner";
        default:
            return "Unknown";
    }
}
