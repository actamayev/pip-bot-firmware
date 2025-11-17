#include "game_manager.h"

#include "dino_runner.h"

bool GameManager::startGame(Games::GameType gameType) {
    if (gameType == currentGame) {
        SerialQueueManager::get_instance().queueMessage("Game already running");
        return true;
    }

    // Stop current game if running
    DisplayScreen::get_instance().turn_display_on();
    if (currentGame != Games::GameType::NONE) {
        stopCurrentGame();
    }

    return enableGame(gameType);
}

void GameManager::stopCurrentGame() {
    if (currentGame == Games::GameType::NONE) return;

    SerialQueueManager::get_instance().queueMessage("Stopping game: " + String(getGameName(currentGame)));
    disableCurrentGame();
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

void GameManager::handleButtonPress(bool rightPressed) {
    if (currentGame == Games::GameType::NONE) return;

    switch (currentGame) {
        case Games::GameType::DINO_RUNNER:
            DinoRunner::get_instance().handleButtonPress(rightPressed);
            break;
        default:
            break;
    }
}

void GameManager::disableCurrentGame() {
    switch (currentGame) {
        case Games::GameType::DINO_RUNNER:
            DinoRunner::get_instance().stopGame();
            break;
        default:
            break;
    }
}

bool GameManager::enableGame(Games::GameType gameType) {
    SerialQueueManager::get_instance().queueMessage("Starting game: " + String(getGameName(gameType)));

    bool success = false;
    switch (gameType) {
        case Games::GameType::DINO_RUNNER:
            DinoRunner::get_instance().startGame();
            success = DinoRunner::get_instance().isGameActive();
            break;
        default:
            SerialQueueManager::get_instance().queueMessage("Unknown game type");
            return false;
    }

    if (success) {
        currentGame = gameType;
        SerialQueueManager::get_instance().queueMessage("Game started successfully");
    } else {
        SerialQueueManager::get_instance().queueMessage("Failed to start game");
    }

    return success;
}

const char* GameManager::getGameName(Games::GameType gameType) const {
    switch (gameType) {
        case Games::GameType::NONE:
            return "None";
        case Games::GameType::DINO_RUNNER:
            return "Dino Runner";
        default:
            return "Unknown";
    }
}
