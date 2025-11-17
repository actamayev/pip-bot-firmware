#include "dino_runner.h"

#include "networking/serial_manager.h"
#include "networking/websocket_manager.h"

void DinoRunner::start_game() {
    if (gameActive) return;

    SerialQueueManager::get_instance().queue_message("Starting Dino Runner game");
    gameActive = true;
    gameState = GameState::MENU;

    // Reset all game state
    reset_game();

    lastFrameMs = millis();
    lastScoreMs = millis();
    lastSpawnMs = millis();

    // Clear obstacles
    for (int i = 0; i < MAX_OBSTACLES; ++i) {
        obstacles[i].active = false;
    }
}

void DinoRunner::stop_game() {
    if (!gameActive) return;

    SerialQueueManager::get_instance().queue_message("Stopping Dino Runner - Final score: " + String(score));
    gameActive = false;
    gameState = GameState::MENU;
    score = 0;
}

void DinoRunner::reset_game() {
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

void DinoRunner::handle_button_press(bool rightPressed) {
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
            reset_game();
            gameState = GameState::RUNNING;
            rightButtonPressed = false;
        }
        // Draw menu to display buffer
        uint8_t buffer[DISPLAY_BUFFER_SIZE];
        memset(buffer, 0, DISPLAY_BUFFER_SIZE);
        draw_menu(buffer);
        DisplayScreen::get_instance().show_custom_buffer(buffer);
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
        update_dino(dt);

        // Spawn obstacles
        if ((millis() - lastSpawnMs) >= spawnIntervalMs) {
            spawn_obstacle();
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
        update_obstacles(dt);

        // Update score
        if (millis() - lastScoreMs >= 100) {
            score += 1;
            lastScoreMs = millis();
        }

        // Draw game to buffer
        uint8_t buffer[DISPLAY_BUFFER_SIZE];
        memset(buffer, 0, DISPLAY_BUFFER_SIZE);
        draw_to_buffer(buffer);
        DisplayScreen::get_instance().show_custom_buffer(buffer);
    } else if (gameState == GameState::GAME_OVER) {
        if (rightButtonPressed) {
            reset_game();
            gameState = GameState::RUNNING;
            rightButtonPressed = false;
        }

        // Draw game over screen
        uint8_t buffer[DISPLAY_BUFFER_SIZE];
        memset(buffer, 0, DISPLAY_BUFFER_SIZE);
        draw_game_over(buffer);
        DisplayScreen::get_instance().show_custom_buffer(buffer);
    }
}

void DinoRunner::update_dino(float dt) {
    // Apply gravity
    dinoVY += GRAVITY;
    if (dinoVY > MAX_FALL_VEL) dinoVY = MAX_FALL_VEL;
    dinoY += dinoVY;

    // Ground collision
    if (dinoY < (GROUND_Y - DINO_H)) return;
    dinoY = (GROUND_Y - DINO_H);
    dinoVY = 0.0f;
    onGround = true;
}

void DinoRunner::spawn_obstacle() {
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

void DinoRunner::update_obstacles(float dt) {
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
        if (check_collision(obstacles[i])) {
            game_over();
            return;
        }
    }
}

bool DinoRunner::check_collision(const Obstacle& o) {
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

void DinoRunner::game_over() {
    gameState = GameState::GAME_OVER;
    SerialQueueManager::get_instance().queue_message("Dino game over - Score: " + String(score));

    // Send score via available communication channels
    if (WebSocketManager::get_instance().is_ws_connected()) {
        WebSocketManager::get_instance().send_dino_score(score);
    } else if (SerialManager::get_instance().is_serial_connected()) {
        SerialManager::get_instance().send_dino_score(score);
    }
}

// Drawing methods using buffer manipulation
void DinoRunner::set_pixel(uint8_t* buffer, int x, int y, bool on) {
    if (x < 0 || x >= SCREEN_WIDTH || y < 0 || y >= SCREEN_HEIGHT) return;

    int index = x + (y / 8) * SCREEN_WIDTH;
    int bit = y % 8;

    if (on) {
        buffer[index] |= (1 << bit);
    } else {
        buffer[index] &= ~(1 << bit);
    }
}

void DinoRunner::fill_rect(uint8_t* buffer, int x, int y, int w, int h, bool on) {
    for (int i = 0; i < w; i++) {
        for (int j = 0; j < h; j++) {
            set_pixel(buffer, x + i, y + j, on);
        }
    }
}

void DinoRunner::draw_dino_sprite(int x, int y, bool onGround, Adafruit_SSD1306& display) {
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
    if (!onGround) {
        display.fillRect(x + 6, y + 8, 3, 2, SSD1306_WHITE);
        return;
    }
    if (animFrame == 0) {
        display.fillRect(x + 5, y + 8, 2, 3, SSD1306_WHITE);
        display.fillRect(x + 9, y + 8, 2, 3, SSD1306_WHITE);
    } else {
        display.fillRect(x + 5, y + 9, 2, 2, SSD1306_WHITE);
        display.fillRect(x + 9, y + 7, 2, 4, SSD1306_WHITE);
    }
}

void DinoRunner::draw_to_buffer(uint8_t* buffer) {
    DisplayScreen& displayScreen = DisplayScreen::get_instance();

    displayScreen.display.clearDisplay();

    // Draw ground line
    displayScreen.display.drawLine(0, GROUND_Y, SCREEN_WIDTH, GROUND_Y, SSD1306_WHITE);

    // Draw dino sprite directly to display
    draw_dino_sprite((int)dinoX, (int)dinoY, onGround, displayScreen.display);

    // Draw obstacles directly to display
    for (int i = 0; i < MAX_OBSTACLES; ++i) {
        if (obstacles[i].active) {
            displayScreen.display.fillRect((int)obstacles[i].x, GROUND_Y - obstacles[i].h, obstacles[i].w, obstacles[i].h, SSD1306_WHITE);
        }
    }

    // Use display text rendering for score
    char scoreText[16];
    snprintf(scoreText, sizeof(scoreText), "S:%d", score);
    displayScreen.draw_text(scoreText, SCREEN_WIDTH - 46, 2, 1);

    // Copy display buffer to our custom buffer
    memcpy(buffer, displayScreen.display.getBuffer(), DISPLAY_BUFFER_SIZE);
}

void DinoRunner::draw_menu(uint8_t* buffer) {
    DisplayScreen& display = DisplayScreen::get_instance();

    display.display.clearDisplay();
    display.draw_centered_text("DINO RUNNER", 10, 1);
    display.draw_centered_text("Press RIGHT to start", 30, 1);

    // Copy display buffer to our custom buffer
    memcpy(buffer, display.display.getBuffer(), DISPLAY_BUFFER_SIZE);
}

void DinoRunner::draw_game_over(uint8_t* buffer) {
    DisplayScreen& display = DisplayScreen::get_instance();

    display.display.clearDisplay();
    display.draw_centered_text("GAME OVER", 10, 1);

    char scoreText[32];
    snprintf(scoreText, sizeof(scoreText), "Score: %d", score);
    display.draw_centered_text(scoreText, 25, 1);

    display.draw_centered_text("Press RIGHT to retry", 45, 1);

    // Copy display buffer to our custom buffer
    memcpy(buffer, display.display.getBuffer(), DISPLAY_BUFFER_SIZE);
}
