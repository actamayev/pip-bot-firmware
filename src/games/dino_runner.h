#pragma once

#include <Arduino.h>

#include "actuators/display_screen.h"
#include "networking/serial_queue_manager.h"
#include "utils/singleton.h"

class DinoRunner : public Singleton<DinoRunner> {
    friend class Singleton<DinoRunner>;
    friend class GameManager;

  public:
    void start_game();
    void stop_game();
    void update();
    void handle_button_press(bool rightPressed);

    bool isGameActive() const {
        return gameActive;
    }
    int get_current_score() const {
        return score;
    }

  private:
    DinoRunner() = default;

    // Game states
    enum class GameState { MENU, RUNNING, GAME_OVER };
    GameState gameState = GameState::MENU;
    bool gameActive = false;

    // Dino physics
    float dinoX = 20;
    float dinoY = 40; // Will be calculated relative to ground
    float dinoVY = 0.0f;
    bool onGround = true;

    // Animation
    uint32_t lastAnimMs = 0;
    uint8_t animFrame = 0;
    static constexpr uint32_t ANIM_MS = 150;

    // Obstacles
    struct Obstacle {
        float x;
        int w;
        int h;
        bool active;
    };
    static constexpr int MAX_OBSTACLES = 6;
    Obstacle obstacles[MAX_OBSTACLES];

    // Game timing
    uint32_t lastSpawnMs = 0;
    uint32_t lastFrameMs = 0;
    uint32_t lastScoreMs = 0;

    // Difficulty progression
    uint32_t spawnIntervalMs = 1400;
    float obstacleSpeed = 1.2f;
    static constexpr float SPEED_INCREMENT = 0.2f;
    static constexpr uint32_t DIFFICULTY_RAMP_MS = 3000;

    // Score
    int score = 0;

    // Physics constants
    static constexpr int GROUND_Y = 52; // Adjusted for 64px display
    static constexpr int DINO_W = 16;
    static constexpr int DINO_H = 12;
    static constexpr float GRAVITY = 0.6f;
    static constexpr float JUMP_VELOCITY = -7.5f;
    static constexpr float MAX_FALL_VEL = 8.0f;

    // Button handling
    bool rightButtonPressed = false;

    // Game logic methods
    void reset_game();
    void spawn_obstacle();
    void update_obstacles(float dt);
    void update_dino(float dt);
    bool check_collision(const Obstacle& o);
    void game_over();

    // Drawing methods (using display buffer)
    void draw_to_buffer(uint8_t* buffer);
    void draw_dino_sprite(int x, int y, bool onGround, Adafruit_SSD1306& display);
    void draw_menu(uint8_t* buffer);
    void draw_game_over(uint8_t* buffer);

    // Buffer drawing utilities
    void set_pixel(uint8_t* buffer, int x, int y, bool on);
    void fill_rect(uint8_t* buffer, int x, int y, int w, int h, bool on);
};