#pragma once
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
    BREEZE,
    CHIME,
    CHIRP,
    POP,
    SPLASH
};

class Speaker : public Singleton<Speaker> {
    friend class Singleton<Speaker>;
    
    public:
        bool initialize();
        void setMuted(bool muted);
        bool isMuted() const { return muted; }
        
        // New methods for flexibility
        void playFile(AudioFile file);  // Changed parameter name
        void setVolume(float volume); // 0.0 to 4.0

        void update(); // Call this periodically to keep audio playing
        bool isPlaying() const;
        void stopPlayback();

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
        
        bool initializeSPIFFS();
        bool initializeAudio();
        void cleanup();

        const char* getFilePath(AudioFile audioFile) const;
        bool isCurrentlyPlaying = false;
        String currentFilename = "";
};
