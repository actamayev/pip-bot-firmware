#pragma once
#include <SPIFFS.h>
#include "Arduino.h"
#include "utils/config.h"
#include "utils/singleton.h"
#include "networking/protocol.h"
#include "networking/serial_queue_manager.h"
#include "AudioFileSourceSPIFFS.h"
#include "AudioFileSourceID3.h"
#include "AudioGeneratorMP3.h"
#include "AudioOutputI2S.h"

class Speaker : public Singleton<Speaker> {
    friend class Singleton<Speaker>;
    
    public:
        bool initialize();
        void setMuted(bool muted);
        bool isMuted() const { return muted; }
        
        // New methods for flexibility
        void playFile(SoundType file);
        void setVolume(float volume); // 0.0 to 4.0

        void update(); // Call this periodically to keep audio playing

    private:
        Speaker() = default;
        ~Speaker();
        
        bool muted = false;
        bool initialized = false;
        float currentVolume = 3.9f;
        
        // Audio objects
        AudioFileSourceSPIFFS* audioFile = nullptr;
        AudioFileSourceID3* audioID3 = nullptr;
        AudioGeneratorMP3* audioMP3 = nullptr;
        AudioOutputI2S* audioOutput = nullptr;
        
        // Simplified state management
        bool isCurrentlyPlaying = false;
        bool isStoppingPlayback = false;
        unsigned long stopRequestTime = 0;
        unsigned long lastPlaybackTime = 0;
        static const unsigned long STOP_DELAY_MS = 200; // Increased delay for ESP32-S3
        static const unsigned long MIN_PLAYBACK_INTERVAL_MS = 150; // Minimum interval between plays
        
        String currentFilename = "";
        
        // Enhanced state management
        bool audioObjectsValid = false;
        bool forceRecreateObjects = false;
        
        bool initializeSPIFFS();
        bool initializeAudio();
        void cleanup();
        bool safeStopPlayback();
        bool safeStartPlayback(SoundType file);
        bool recreateAudioObjects();
        bool validateAudioObjects();

        const char* getFilePath(SoundType audioFile) const;
};
