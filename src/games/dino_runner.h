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
    static void stop_game();
    void update();
    static void handle_button_press(bool right_pressed);

    bool is_game_active() const {
        return _game_active;
    }
    int get_current_score() const {
        return _score;
    }

  private:
    DinoRunner() = default;

    // Game states
    enum class GameState { MENU, RUNNING, GAME_OVER };
    GameState _game_state = GameState::MENU;
    bool _game_active = false;

    // Dino physics
    float _dino_x = 20;
    float _dino_y = 40; // Will be calculated relative to ground
    float _dino_vy = 0.0f;
    bool _on_ground = true;

    // Animation
    uint32_t _last_anim_ms = 0;
    uint8_t _anim_frame = 0;
    static constexpr uint32_t ANIM_MS = 150;

    // Obstacles
    struct Obstacle {
        float x;
        int w;
        int h;
        bool active;
    };
    static constexpr int MAX_OBSTACLES = 6;
    Obstacle _obstacles[MAX_OBSTACLES]{};

    // Game timing
    uint32_t _last_spawn_ms = 0;
    uint32_t _last_frame_ms = 0;
    uint32_t _last_score_ms = 0;

    // Difficulty progression
    uint32_t _spawn_interval_ms = 1400;
    float _obstacle_speed = 1.2f;
    static constexpr float SPEED_INCREMENT = 0.2f;
    static constexpr uint32_t DIFFICULTY_RAMP_MS = 3000;

    // Score
    int _score = 0;

    // Physics constants
    static constexpr int GROUND_Y = 52; // Adjusted for 64px display
    static constexpr int DINO_W = 16;
    static constexpr int DINO_H = 12;
    static constexpr float GRAVITY = 0.6f;
    static constexpr float JUMP_VELOCITY = -7.5f;
    static constexpr float MAX_FALL_VEL = 8.0f;

    // Button handling
    bool _right_button_pressed = false;

    // Game logic methods
    void reset_game();
    static void spawn_obstacle();
    void update_obstacles(float dt);
    static void update_dino(float dt);
    static bool check_collision(const Obstacle& o);
    static void game_over();

    // Drawing methods (using display buffer)
    void draw_to_buffer(uint8_t* buffer);
    static void draw_dino_sprite(int x, int y, bool on_ground, Adafruit_SSD1306& display);
    static void draw_menu(uint8_t* buffer);
    static void draw_game_over(uint8_t* buffer);

    // Buffer drawing utilities
    static void set_pixel(uint8_t* buffer, int x, int y, bool on);
    static void fill_rect(uint8_t* buffer, int x, int y, int w, int h, bool on);
};