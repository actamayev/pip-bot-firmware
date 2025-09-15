#include "dino_runner.h"
#include "networking/websocket_manager.h"
#include "networking/serial_manager.h"

void DinoRunner::startGame() {
    if (gameActive) return;
    
    SerialQueueManager::getInstance().queueMessage("Starting Dino Runner game");
    gameActive = true;
    gameState = GameState::MENU;
    
    // Reset all game state
    resetGame();
    
    lastFrameMs = millis();
    lastScoreMs = millis();
    lastSpawnMs = millis();
    
    // Clear obstacles
    for (int i = 0; i < MAX_OBSTACLES; ++i) {
        obstacles[i].active = false;
    }
}

void DinoRunner::stopGame() {
    if (!gameActive) return;
    
    SerialQueueManager::getInstance().queueMessage("Stopping Dino Runner - Final score: " + String(score));
    gameActive = false;
    gameState = GameState::MENU;
    score = 0;
}

void DinoRunner::resetGame() {
    // Reset dino
    dinoX = 20;
    dinoY = GROUND_Y - DINO_H;
    dinoVY = 0.0f;
    onGround = true;
    
    // Clear obstacles
    for (int i = 0; i < MAX_OBSTACLES; ++i) {
        obstacles[i].active = false;
    }
    
    // Reset difficulty & score
    spawnIntervalMs = 1400;
    obstacleSpeed = 1.2f;
    score = 0;
    lastSpawnMs = millis();
    lastScoreMs = millis();
    lastFrameMs = millis();
}

void DinoRunner::handleButtonPress(bool rightPressed) {
    if (!gameActive) return;
    
    // Only respond to right button
    if (rightPressed) {
        rightButtonPressed = true;
    }
}

void DinoRunner::update() {
    if (!gameActive) return;
    
    unsigned long now = millis();
    unsigned long dtMs = now - lastFrameMs;
    lastFrameMs = now;
    float dt = dtMs / 16.0f; // normalize relative to ~60FPS
    
    if (gameState == GameState::MENU) {
        if (rightButtonPressed) {
            resetGame();
            gameState = GameState::RUNNING;
            rightButtonPressed = false;
        }
        // Draw menu to display buffer
        uint8_t buffer[DISPLAY_BUFFER_SIZE];
        memset(buffer, 0, DISPLAY_BUFFER_SIZE);
        drawMenu(buffer);
        DisplayScreen::getInstance().showCustomBuffer(buffer);
        return;
    }
    
    if (gameState == GameState::RUNNING) {
        // Handle jump
        if (rightButtonPressed && onGround) {
            dinoVY = JUMP_VELOCITY;
            onGround = false;
            rightButtonPressed = false;
        }
        
        // Update physics
        updateDino(dt);
        
        // Spawn obstacles
        if ((millis() - lastSpawnMs) >= spawnIntervalMs) {
            spawnObstacle();
            lastSpawnMs = millis();
        }
        
        // Ramp difficulty
        static unsigned long lastRamp = 0;
        if ((millis() - lastRamp) >= DIFFICULTY_RAMP_MS) {
            if (spawnIntervalMs > 350) {
                spawnIntervalMs = max(350UL, spawnIntervalMs - 120);
            }
            obstacleSpeed += SPEED_INCREMENT;
            lastRamp = millis();
        }
        
        // Update obstacles & check collisions
        updateObstacles(dt);
        
        // Update score
        if (millis() - lastScoreMs >= 100) {
            score += 1;
            lastScoreMs = millis();
        }
        
        // Draw game to buffer
        uint8_t buffer[DISPLAY_BUFFER_SIZE];
        memset(buffer, 0, DISPLAY_BUFFER_SIZE);
        drawToBuffer(buffer);
        DisplayScreen::getInstance().showCustomBuffer(buffer);
        
    } else if (gameState == GameState::GAME_OVER) {
        if (rightButtonPressed) {
            resetGame();
            gameState = GameState::RUNNING;
            rightButtonPressed = false;
        }
        
        // Draw game over screen
        uint8_t buffer[DISPLAY_BUFFER_SIZE];
        memset(buffer, 0, DISPLAY_BUFFER_SIZE);
        drawGameOver(buffer);
        DisplayScreen::getInstance().showCustomBuffer(buffer);
    }
}

void DinoRunner::updateDino(float dt) {
    // Apply gravity
    dinoVY += GRAVITY;
    if (dinoVY > MAX_FALL_VEL) dinoVY = MAX_FALL_VEL;
    dinoY += dinoVY;
    
    // Ground collision
    if (dinoY >= (GROUND_Y - DINO_H)) {
        dinoY = (GROUND_Y - DINO_H);
        dinoVY = 0.0f;
        onGround = true;
    }
}

void DinoRunner::spawnObstacle() {
    // Find free slot
    int slot = -1;
    for (int i = 0; i < MAX_OBSTACLES; ++i) {
        if (!obstacles[i].active) {
            slot = i;
            break;
        }
    }
    if (slot == -1) return;
    
    // Create obstacle
    obstacles[slot].x = SCREEN_WIDTH;
    obstacles[slot].w = random(6, 12);
    obstacles[slot].h = random(10, 20);
    obstacles[slot].active = true;
}

void DinoRunner::updateObstacles(float dt) {
    for (int i = 0; i < MAX_OBSTACLES; ++i) {
        if (!obstacles[i].active) continue;
        
        // Move left
        obstacles[i].x -= obstacleSpeed * dt;
        
        // Off-screen? deactivate
        if (obstacles[i].x + obstacles[i].w < 0) {
            obstacles[i].active = false;
            continue;
        }
        
        // Collision check
        if (checkCollision(obstacles[i])) {
            gameOver();
            return;
        }
    }
}

bool DinoRunner::checkCollision(const Obstacle& o) {
    // Simple bounding box collision
    float dL = dinoX;
    float dR = dinoX + DINO_W;
    float dT = dinoY;
    float dB = dinoY + DINO_H;
    
    float oL = o.x;
    float oR = o.x + o.w;
    float oB = GROUND_Y;
    float oT = GROUND_Y - o.h;
    
    bool overlapX = (dR > oL) && (dL < oR);
    bool overlapY = (dB > oT) && (dT < oB);
    return (overlapX && overlapY);
}

void DinoRunner::gameOver() {
    gameState = GameState::GAME_OVER;
    SerialQueueManager::getInstance().queueMessage("Dino game over - Score: " + String(score));
    
    // Send score via available communication channels
    if (WebSocketManager::getInstance().isWsConnected()) {
        WebSocketManager::getInstance().sendDinoScore(score);
    } else if (SerialManager::getInstance().isSerialConnected()) {
        SerialManager::getInstance().sendDinoScore(score);
    }
}

// Drawing methods using buffer manipulation
void DinoRunner::setPixel(uint8_t* buffer, int x, int y, bool on) {
    if (x < 0 || x >= SCREEN_WIDTH || y < 0 || y >= SCREEN_HEIGHT) return;
    
    int index = x + (y / 8) * SCREEN_WIDTH;
    int bit = y % 8;
    
    if (on) {
        buffer[index] |= (1 << bit);
    } else {
        buffer[index] &= ~(1 << bit);
    }
}

void DinoRunner::fillRect(uint8_t* buffer, int x, int y, int w, int h, bool on) {
    for (int i = 0; i < w; i++) {
        for (int j = 0; j < h; j++) {
            setPixel(buffer, x + i, y + j, on);
        }
    }
}

void DinoRunner::drawDinoSprite(int x, int y, bool onGround, Adafruit_SSD1306& display) {
    // Update animation frame
    if (millis() - lastAnimMs > ANIM_MS) {
        lastAnimMs = millis();
        animFrame ^= 1;
    }
    
    // Body
    display.fillRect(x + 3, y + 2, 9, 6, SSD1306_WHITE);
    
    // Tail
    display.fillRect(x + 0, y + 5, 3, 2, SSD1306_WHITE);
    
    // Head/neck
    display.fillRect(x + 10, y + 1, 5, 4, SSD1306_WHITE);
    
    // Eye
    display.drawPixel(x + 12, y + 2, SSD1306_BLACK);
    
    // Back spikes
    display.fillRect(x + 4, y + 1, 1, 2, SSD1306_WHITE);
    display.fillRect(x + 6, y + 1, 1, 2, SSD1306_WHITE);
    
    // Legs (animated when on ground)
    if (onGround) {
        if (animFrame == 0) {
            display.fillRect(x + 5, y + 8, 2, 3, SSD1306_WHITE);
            display.fillRect(x + 9, y + 8, 2, 3, SSD1306_WHITE);
        } else {
            display.fillRect(x + 5, y + 9, 2, 2, SSD1306_WHITE);
            display.fillRect(x + 9, y + 7, 2, 4, SSD1306_WHITE);
        }
    } else {
        display.fillRect(x + 6, y + 8, 3, 2, SSD1306_WHITE);
    }
}

void DinoRunner::drawToBuffer(uint8_t* buffer) {
    DisplayScreen& displayScreen = DisplayScreen::getInstance();
    
    displayScreen.display.clearDisplay();
    
    // Draw ground line
    displayScreen.display.drawLine(0, GROUND_Y, SCREEN_WIDTH, GROUND_Y, SSD1306_WHITE);
    
    // Draw dino sprite directly to display
    drawDinoSprite((int)dinoX, (int)dinoY, onGround, displayScreen.display);
    
    // Draw obstacles directly to display
    for (int i = 0; i < MAX_OBSTACLES; ++i) {
        if (obstacles[i].active) {
            displayScreen.display.fillRect((int)obstacles[i].x, GROUND_Y - obstacles[i].h, 
                                          obstacles[i].w, obstacles[i].h, SSD1306_WHITE);
        }
    }
    
    // Use display text rendering for score
    char scoreText[16];
    snprintf(scoreText, sizeof(scoreText), "S:%d", score);
    displayScreen.drawText(scoreText, SCREEN_WIDTH - 46, 2, 1);
    
    // Copy display buffer to our custom buffer
    memcpy(buffer, displayScreen.display.getBuffer(), DISPLAY_BUFFER_SIZE);
}

void DinoRunner::drawMenu(uint8_t* buffer) {
    DisplayScreen& display = DisplayScreen::getInstance();
    
    display.display.clearDisplay();
    display.drawCenteredText("DINO RUNNER", 10, 1);
    display.drawCenteredText("Press RIGHT to start", 30, 1);
    
    // Copy display buffer to our custom buffer
    memcpy(buffer, display.display.getBuffer(), DISPLAY_BUFFER_SIZE);
}

void DinoRunner::drawGameOver(uint8_t* buffer) {
    DisplayScreen& display = DisplayScreen::getInstance();
    
    display.display.clearDisplay();
    display.drawCenteredText("GAME OVER", 10, 1);
    
    char scoreText[32];
    snprintf(scoreText, sizeof(scoreText), "Score: %d", score);
    display.drawCenteredText(scoreText, 25, 1);
    
    display.drawCenteredText("Press RIGHT to retry", 45, 1);
    
    // Copy display buffer to our custom buffer
    memcpy(buffer, display.display.getBuffer(), DISPLAY_BUFFER_SIZE);
}

