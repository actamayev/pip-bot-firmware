#pragma once

#include <Arduino.h>
#include "utils/singleton.h"
#include "actuators/display_screen.h"
#include "networking/serial_queue_manager.h"

class DinoRunner : public Singleton<DinoRunner> {
    friend class Singleton<DinoRunner>;
    friend class GameManager;
    
    public:
        void startGame();
        void stopGame();
        void update();
        void handleButtonPress(bool leftPressed, bool rightPressed);
        
        bool isGameActive() const { return gameActive; }
        int getCurrentScore() const { return score; }
        
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
        unsigned long lastAnimMs = 0;
        uint8_t animFrame = 0;
        static constexpr unsigned long ANIM_MS = 150;
        
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
        unsigned long lastSpawnMs = 0;
        unsigned long lastFrameMs = 0;
        unsigned long lastScoreMs = 0;
        
        // Difficulty progression
        unsigned long spawnIntervalMs = 1400;
        float obstacleSpeed = 1.2f;
        static constexpr float SPEED_INCREMENT = 0.2f;
        static constexpr unsigned long DIFFICULTY_RAMP_MS = 3000;
        
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
        void resetGame();
        void spawnObstacle();
        void updateObstacles(float dt);
        void updateDino(float dt);
        bool checkCollision(const Obstacle& o);
        void gameOver();
        
        // Drawing methods (using display buffer)
        void drawToBuffer(uint8_t* buffer);
        void drawDinoSprite(int x, int y, bool onGround, Adafruit_SSD1306& display);
        void drawMenu(uint8_t* buffer);
        void drawGameOver(uint8_t* buffer);
        
        // Buffer drawing utilities
        void setPixel(uint8_t* buffer, int x, int y, bool on);
        void fillRect(uint8_t* buffer, int x, int y, int w, int h, bool on);
};