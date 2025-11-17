#include "game_manager.h"

#include "dino_runner.h"

bool GameManager::start_game(Games::GameType gameType) {
    if (gameType == currentGame) {
        SerialQueueManager::get_instance().queue_message("Game already running");
        return true;
    }

    // Stop current game if running
    DisplayScreen::get_instance().turn_display_on();
    if (currentGame != Games::GameType::NONE) {
        stop_current_game();
    }

    return enable_game(gameType);
}

void GameManager::stop_current_game() {
    if (currentGame == Games::GameType::NONE) return;

    SerialQueueManager::get_instance().queue_message("Stopping game: " + String(get_game_name(currentGame)));
    disable_current_game();
    currentGame = Games::GameType::NONE;
}

void GameManager::update() {
    if (currentGame == Games::GameType::NONE) return;

    switch (currentGame) {
        case Games::GameType::DINO_RUNNER:
            DinoRunner::get_instance().update();
            break;
        default:
            break;
    }
}

void GameManager::handle_button_press(bool rightPressed) {
    if (currentGame == Games::GameType::NONE) return;

    switch (currentGame) {
        case Games::GameType::DINO_RUNNER:
            DinoRunner::get_instance().handle_button_press(rightPressed);
            break;
        default:
            break;
    }
}

void GameManager::disable_current_game() {
    switch (currentGame) {
        case Games::GameType::DINO_RUNNER:
            DinoRunner::get_instance().stop_game();
            break;
        default:
            break;
    }
}

bool GameManager::enable_game(Games::GameType gameType) {
    SerialQueueManager::get_instance().queue_message("Starting game: " + String(get_game_name(gameType)));

    bool success = false;
    switch (gameType) {
        case Games::GameType::DINO_RUNNER:
            DinoRunner::get_instance().start_game();
            success = DinoRunner::get_instance().isGameActive();
            break;
        default:
            SerialQueueManager::get_instance().queue_message("Unknown game type");
            return false;
    }

    if (success) {
        currentGame = gameType;
        SerialQueueManager::get_instance().queue_message("Game started successfully");
    } else {
        SerialQueueManager::get_instance().queue_message("Failed to start game");
    }

    return success;
}

const char* GameManager::get_game_name(Games::GameType gameType) const {
    switch (gameType) {
        case Games::GameType::NONE:
            return "None";
        case Games::GameType::DINO_RUNNER:
            return "Dino Runner";
        default:
            return "Unknown";
    }
}
