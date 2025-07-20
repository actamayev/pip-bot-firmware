#pragma once
#include <queue>
#include <SPIFFS.h>
#include "Arduino.h"
#include "utils/config.h"
#include "utils/singleton.h"
#include "networking/serial_queue_manager.h"
#include "AudioFileSourceSPIFFS.h"
#include "AudioFileSourceID3.h"
#include "AudioGeneratorMP3.h"
#include "AudioOutputI2S.h"

enum class AudioFile {
    CHIME,
    CHIRP,
    POP,
    DROP,
    FART,
    MONKEY,
    ELEPHANT,
    PARTY,
    UFO,
    COUNTDOWN,
    ENGINE,
    ROBOT
};

class Speaker : public Singleton<Speaker> {
    friend class Singleton<Speaker>;
    
    public:
        bool initialize();
        void setMuted(bool muted);
        bool isMuted() const { return muted; }
        
        // New methods for flexibility
        void playFile(AudioFile file);
        void setVolume(float volume); // 0.0 to 4.0

        void update(); // Call this periodically to keep audio playing
        bool isPlaying() const;
        void stopPlayback();
        void clearQueue(); // Clear any queued audio

    private:
        Speaker() = default;
        ~Speaker();
        
        bool muted = false;
        bool initialized = false;
        float currentVolume = 1.0f;
        
        // Audio objects
        AudioFileSourceSPIFFS* audioFile = nullptr;
        AudioFileSourceID3* audioID3 = nullptr;
        AudioGeneratorMP3* audioMP3 = nullptr;
        AudioOutputI2S* audioOutput = nullptr;
        
        // Queue system for handling rapid requests
        std::queue<AudioFile> audioQueue;
        bool isCurrentlyPlaying = false;
        bool isStoppingPlayback = false;
        unsigned long stopRequestTime = 0;
        static const unsigned long STOP_DELAY_MS = 50; // Delay between stop and start
        
        String currentFilename = "";
        
        bool initializeSPIFFS();
        bool initializeAudio();
        void cleanup();
        bool safeStopPlayback();
        bool safeStartPlayback(AudioFile file);

        const char* getFilePath(AudioFile audioFile) const;
};
