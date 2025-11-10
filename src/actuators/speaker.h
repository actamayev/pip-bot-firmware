#pragma once
#include "Arduino.h"
#include "utils/config.h"
#include "utils/singleton.h"
#include "actuators/led/rgb_led.h"
#include "networking/protocol.h"
#include "networking/serial_queue_manager.h"
#include "custom_interpreter/bytecode_structs.h"
#include "AudioGeneratorRTTTL.h"
#include "AudioFileSourcePROGMEM.h"
#include "AudioOutputI2S.h"

class Speaker : public Singleton<Speaker> {
    friend class Singleton<Speaker>;
    friend class TaskManager;

    public:
        void setMuted(bool muted);
        void setVolume(float volume); // 0.0 to 4.0
        void stopAllSounds();
        void startEntertainerMelody();
        void playTone(ToneType tone);
        void stopTone();
        void startHorn();
        void stopHorn();

    private:
        Speaker() = default;
        ~Speaker();
    
        bool initialize();

        void update(); // Call this periodically to keep audio playing
        bool muted = false;
        bool initialized = false;
        float currentVolume = 3.9f;
        
        // Audio objects
        AudioOutputI2S* audioOutput = nullptr;
        
        // RTTTL objects for melody playback
        AudioGeneratorRTTTL* rtttlGenerator = nullptr;
        AudioFileSourcePROGMEM* rtttlSource = nullptr;
        
        // Enhanced state management
        bool audioObjectsValid = false;
        
        // Thread safety
        SemaphoreHandle_t audioMutex = nullptr;
        
        bool initializeAudio();
        void cleanup();
        bool recreateAudioObjects();
        bool validateAudioObjects();
        
        // RTTTL melody methods
        void updateMelody();
        
        // Melody playback state
        bool isMelodyPlaying = false;
        
        // LED synchronization for entertainer melody
        struct MelodyNote {
            unsigned long duration; // in milliseconds
            uint8_t ledR, ledG, ledB; // LED color for this note
        };
        
        static constexpr MelodyNote entertainerLedSequence[] = {
            {214, 255, 0, 0},    // 8d - red
            {214, 255, 127, 0},  // 8d# - orange  
            {214, 255, 255, 0},  // 8e - yellow
            {428, 0, 255, 0},    // c6 - green
            {214, 0, 255, 127},  // 8e - cyan
            {428, 0, 255, 0},    // c6 - green
            {214, 0, 255, 127},  // 8e - cyan
            {428, 0, 255, 0},    // c6 - green
            {428, 0, 0, 255},    // c - blue
            {214, 127, 0, 255},  // 8c6 - purple
            {214, 255, 0, 255},  // 8a - magenta
            {214, 255, 127, 127}, // 8g - pink
            {214, 255, 255, 127}, // 8f# - light yellow
            {214, 255, 0, 255},  // 8a - magenta
            {214, 127, 0, 255},  // 8c6 - purple
            {214, 0, 255, 127},  // 8e - cyan
            {214, 255, 255, 0},  // 8d - yellow
            {214, 0, 255, 0},    // 8c - green
            {214, 255, 0, 255},  // 8a - magenta
            {857, 255, 0, 0}     // 2d - red (half note)
        };
        
        static constexpr size_t ENTERTAINER_LED_SEQUENCE_LENGTH = sizeof(entertainerLedSequence) / sizeof(entertainerLedSequence[0]);
        
        // LED sync state
        bool isLedSequencePlaying = false;
        int currentLedStep = 0;
        unsigned long ledStepStartTime = 0;

        const uint8_t I2S_DOUT = 13;
        const uint8_t I2S_BCLK = 14;
        const uint8_t I2S_LRC = 21;

        bool isPlayingTone = false;
        bool isHornMode = false;
        ToneType currentTone = ToneType::TONE_A;

        // RTTTL objects for tone playback (separate from melody)
        AudioGeneratorRTTTL* toneGenerator = nullptr;
        AudioFileSourcePROGMEM* toneSource = nullptr;

        void updateContinuousTone();
        const char* getToneRTTTL(ToneType tone);

        unsigned long lastToneRefreshTime = 0;
        static const unsigned long TONE_AUTO_STOP_MS = 200; // Stop if not refreshed within 200ms
};
