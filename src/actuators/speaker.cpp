#include "speaker.h"

Speaker speaker;

Speaker::Speaker() {
    ledcSetup(0, 50000, 8); // Channel 0, 50kHz PWM, 8-bit resolution
    ledcWrite(0, 0); // mute on startup
    ledcAttachPin(AUDIO_PIN, 0);

    isMuted = true;
}

void Speaker::setMuted(bool muted) {
    if (muted) mute();
    else unMute();
}

void Speaker::mute() {
    if (isMuted) return;
    isMuted = true;
    
    // Immediately silence the speaker
    ledcWrite(0, 0);
    
    SerialQueueManager::getInstance().queueMessage("Speaker muted");
}

void Speaker::unMute() {
    if (!isMuted) return;
    isMuted = false;

    SerialQueueManager::getInstance().queueMessage("Speaker unmuted");
}

void Speaker::chime() {
    return;
}
