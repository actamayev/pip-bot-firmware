#include "dino_runner.h"

#include <algorithm>
#include <cmath>

#include "networking/serial_manager.h"
#include "networking/websocket_manager.h"

void DinoRunner::start_game() {
    if (game_active) {
        return;
    }

    SerialQueueManager::get_instance().queue_message("Starting Dino Runner game");
    game_active = true;
    game_state = GameState::MENU;

    // Reset all game state
    reset_game();

    last_frame_ms = millis();
    last_score_ms = millis();
    last_spawn_ms = millis();

    // Clear obstacles
    for (auto& obstacle : obstacles) {
        obstacle.active = false;
    }
}

void DinoRunner::stop_game() {
    if (!game_active) {
        return;
    }

    SerialQueueManager::get_instance().queue_message("Stopping Dino Runner - Final score: " + String(score));
    game_active = false;
    game_state = GameState::MENU;
    score = 0;
}

void DinoRunner::reset_game() {
    // Reset dino
    dino_x = 20;
    dino_y = GROUND_Y - DINO_H;
    dino_vy = 0.0f;
    on_ground = true;

    // Clear obstacles
    for (auto& obstacle : obstacles) {
        obstacle.active = false;
    }

    // Reset difficulty & score
    spawn_interval_ms = 1400;
    obstacle_speed = 1.2f;
    score = 0;
    last_spawn_ms = millis();
    last_score_ms = millis();
    last_frame_ms = millis();
}

void DinoRunner::handle_button_press(bool right_pressed) {
    if (!game_active) {
        return;
    }

    // Only respond to right button
    if (right_pressed) {
        right_button_pressed = true;
    }
}

void DinoRunner::update() {
    if (!game_active) {
        return;
    }

    uint32_t now = millis();
    uint32_t dt_ms = now - last_frame_ms = 0 = 0;
    last_frame_ms = now;
    float dt = dt_ms / 16.0f; // normalize relative to ~60FPS

    if (game_state == GameState::MENU) {
        if (right_button_pressed) {
            reset_game();
            game_state = GameState::RUNNING;
            right_button_pressed = false;
        }
        // Draw menu to display buffer
        uint8_t buffer[DISPLAY_BUFFER_SIZE];
        memset(buffer, 0, DISPLAY_BUFFER_SIZE);
        draw_menu(buffer);
        DisplayScreen::get_instance().show_custom_buffer(buffer);
        return;
    }

    if (game_state == GameState::RUNNING) {
        // Handle jump
        if (right_button_pressed && on_ground) {
            dino_vy = JUMP_VELOCITY;
            on_ground = false;
            right_button_pressed = false;
        }

        // Update physics
        update_dino(dt);

        // Spawn obstacles
        if ((millis() - last_spawn_ms) >= spawn_interval_ms) {
            spawn_obstacle();
            last_spawn_ms = millis();
        }

        // Ramp difficulty
        static uint32_t last_ramp = 0;
        if ((millis() - last_ramp) >= DIFFICULTY_RAMP_MS) {
            if (spawn_interval_ms > 350) {
                spawn_interval_ms = max(350U, spawn_interval_ms - 120);
            }
            obstacle_speed += SPEED_INCREMENT;
            last_ramp = millis();
        }

        // Update obstacles & check collisions
        update_obstacles(dt);

        // Update score
        if (millis() - last_score_ms >= 100) {
            score += 1;
            last_score_ms = millis();
        }

        // Draw game to buffer
        uint8_t buffer[DISPLAY_BUFFER_SIZE];
        memset(buffer, 0, DISPLAY_BUFFER_SIZE);
        draw_to_buffer(buffer);
        DisplayScreen::get_instance().show_custom_buffer(buffer);
    } else if (game_state == GameState::GAME_OVER) {
        if (right_button_pressed) {
            reset_game();
            game_state = GameState::RUNNING;
            right_button_pressed = false;
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
    dino_vy += GRAVITY;
    dino_vy = std::min(dino_vy, MAX_FALL_VEL);
    dino_y += dino_vy;

    // Ground collision
    if (dino_y < (GROUND_Y - DINO_H)) {
        return;
    }
    dino_y = (GROUND_Y - DINO_H);
    dino_vy = 0.0f;
    on_ground = true;
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
    if (slot == -1) {
        return;
    }

    // Create obstacle
    obstacles[slot].x = SCREEN_WIDTH;
    obstacles[slot].w = random(6, 12);
    obstacles[slot].h = random(10, 20);
    obstacles[slot].active = true;
}

void DinoRunner::update_obstacles(float dt) {
    for (auto& obstacle : obstacles) {
        if (!obstacle.active) {
            continue;
        }

        // Move left
        obstacle.x -= obstacle_speed * dt;

        // Off-screen? deactivate
        if (obstacle.x + obstacle.w < 0) {
            obstacle.active = false;
            continue;
        }

        // Collision check
        if (check_collision(obstacle)) {
            game_over();
            return;
        }
    }
}

bool DinoRunner::check_collision(const Obstacle& o) {
    // Simple bounding box collision
    float d_l = _dino_x;
    float d_r = _dino_x + DINO_W;
    float d_t = _dino_y;
    float d_b = _dino_y + DINO_H;

    float o_l = o.x;
    float o_r = o.x + o.w;
    float o_b = GROUND_Y;
    float o_t = GROUND_Y - o.h;

    bool overlap_x = (d_r > o_l) && (d_l < o_r);
    bool overlap_y = (d_b > o_t) && (d_t < o_b);
    return (overlap_x && overlap_y);
}

void DinoRunner::game_over() {
    game_state = GameState::GAME_OVER;
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
    if (x < 0 || x >= SCREEN_WIDTH || y < 0 || y >= SCREEN_HEIGHT) {
        return;
    }

    int index = x + ((y / 8) * SCREEN_WIDTH);
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

void DinoRunner::draw_dino_sprite(int x, int y, bool on_ground, Adafruit_SSD1306& display) {
    // Update animation frame
    if (millis() - last_anim_ms > ANIM_MS) {
        last_anim_ms = millis();
        anim_frame ^= 1;
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
    if (!on_ground) {
        display.fillRect(x + 6, y + 8, 3, 2, SSD1306_WHITE);
        return;
    }
    if (anim_frame == 0) {
        display.fillRect(x + 5, y + 8, 2, 3, SSD1306_WHITE);
        display.fillRect(x + 9, y + 8, 2, 3, SSD1306_WHITE);
    } else {
        display.fillRect(x + 5, y + 9, 2, 2, SSD1306_WHITE);
        display.fillRect(x + 9, y + 7, 2, 4, SSD1306_WHITE);
    }
}

void DinoRunner::draw_to_buffer(uint8_t* buffer) {
    DisplayScreen& display_screen = DisplayScreen::get_instance();

    display_screen.display.clearDisplay();

    // Draw ground line
    display_screen.display.drawLine(0, GROUND_Y, SCREEN_WIDTH, GROUND_Y, SSD1306_WHITE);

    // Draw dino sprite directly to display
    draw_dino_sprite(static_cast<int>(dino_x), static_cast<int>(dino_y), on_ground, display_screen.display);

    // Draw obstacles directly to display
    for (auto& obstacle : obstacles) {
        if (obstacle.active) {
            display_screen.display.fillRect(static_cast<int>(obstacle.x), GROUND_Y - obstacle.h, obstacle.w, obstacle.h, SSD1306_WHITE);
        }
    }

    // Use display text rendering for score
    char score_text[16];
    snprintf(score_text, sizeof(score_text), "S:%d", score);
    DisplayScreen::draw_text(score_text, SCREEN_WIDTH - 46, 2, 1);

    // Copy display buffer to our custom buffer
    memcpy(buffer, display_screen.display.getBuffer(), DISPLAY_BUFFER_SIZE);
}

void DinoRunner::draw_menu(uint8_t* buffer) {
    DisplayScreen& display = DisplayScreen::get_instance();

    display.display.clearDisplay();
    DisplayScreen::draw_centered_text("DINO RUNNER", 10, 1);
    DisplayScreen::draw_centered_text("Press RIGHT to start", 30, 1);

    // Copy display buffer to our custom buffer
    memcpy(buffer, display.display.getBuffer(), DISPLAY_BUFFER_SIZE);
}

void DinoRunner::draw_game_over(uint8_t* buffer) {
    DisplayScreen& display = DisplayScreen::get_instance();

    display.display.clearDisplay();
    DisplayScreen::draw_centered_text("GAME OVER", 10, 1);

    char score_text[32];
    snprintf(score_text, sizeof(score_text), "Score: %d", score);
    DisplayScreen::draw_centered_text(score_text, 25, 1);

    DisplayScreen::draw_centered_text("Press RIGHT to retry", 45, 1);

    // Copy display buffer to our custom buffer
    memcpy(buffer, display.display.getBuffer(), DISPLAY_BUFFER_SIZE);
}
