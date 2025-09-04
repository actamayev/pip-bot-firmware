#include "game_manager.h"
#include "dino_runner.h"

bool GameManager::startGame(Games::GameType gameType) {
    if (gameType == currentGame) {
        SerialQueueManager::getInstance().queueMessage("Game already running");
        return true;
    }
    
    // Stop current game if running
    DisplayScreen::getInstance().turnDisplayOn();
    if (currentGame != Games::GameType::NONE) {
        stopCurrentGame();
    }
    
    return enableGame(gameType);
}

void GameManager::stopCurrentGame() {
    if (currentGame == Games::GameType::NONE) return;
    
    SerialQueueManager::getInstance().queueMessage("Stopping game: " + String(getGameName(currentGame)));
    disableCurrentGame();
    currentGame = Games::GameType::NONE;
}

void GameManager::update() {
    if (currentGame == Games::GameType::NONE) return;
    
    switch (currentGame) {
        case Games::GameType::DINO_RUNNER:
            DinoRunner::getInstance().update();
            break;
        default:
            break;
    }
}

void GameManager::handleButtonPress(bool rightPressed) {
    if (currentGame == Games::GameType::NONE) return;
    
    switch (currentGame) {
        case Games::GameType::DINO_RUNNER:
            DinoRunner::getInstance().handleButtonPress(rightPressed);
            break;
        default:
            break;
    }
}

void GameManager::disableCurrentGame() {
    switch (currentGame) {
        case Games::GameType::DINO_RUNNER:
            DinoRunner::getInstance().stopGame();
            break;
        default:
            break;
    }
}

bool GameManager::enableGame(Games::GameType gameType) {
    SerialQueueManager::getInstance().queueMessage("Starting game: " + String(getGameName(gameType)));
    
    bool success = false;
    switch (gameType) {
        case Games::GameType::DINO_RUNNER:
            DinoRunner::getInstance().startGame();
            success = DinoRunner::getInstance().isGameActive();
            break;
        default:
            SerialQueueManager::getInstance().queueMessage("Unknown game type");
            return false;
    }
    
    if (success) {
        currentGame = gameType;
        SerialQueueManager::getInstance().queueMessage("Game started successfully");
    } else {
        SerialQueueManager::getInstance().queueMessage("Failed to start game");
    }
    
    return success;
}

const char* GameManager::getGameName(Games::GameType gameType) const {
    switch (gameType) {
        case Games::GameType::NONE: return "None";
        case Games::GameType::DINO_RUNNER: return "Dino Runner";
        default: return "Unknown";
    }
}