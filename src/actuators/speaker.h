#pragma once
#include "Arduino.h"
#include "../sounds/chime.h"
#include "../utils/config.h"

class Speaker  {
    public:
        Speaker();

        // Play a chime sound in a non-blocking way
        void chime();
        
        // Check if a chime is currently playing
        bool isPlaying() const;
        
        // This should be called periodically from a task to update the speaker
        void mute();
        void unmute();
        void setMuted(bool muted);
        bool getMuted() const;
    private:
        bool isMuted;

};

extern Speaker speaker;
