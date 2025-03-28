// #include "./speaker.h"
// #include "../sounds/chime.h"
// #include "../utils/config.h"

// Speaker speaker;

// // Audio playback variables
// volatile int currentChimeSample = WAV_HEADER_SIZE; // Start after WAV header
// volatile bool isChimePlaying = false;
// volatile bool isSpeakerMuted = false;

// // Timer for chime playback
// hw_timer_t * chimeTimer = NULL;
// portMUX_TYPE chimeTimerMux = portMUX_INITIALIZER_UNLOCKED;

// void IRAM_ATTR onChimeTimer() {
//     portENTER_CRITICAL_ISR(&chimeTimerMux);
    
//     if (currentChimeSample < AUDIO_DATA_LENGTH && isChimePlaying) {
//         // Only output audio if not muted
//         if (!isSpeakerMuted) {
//             // Read sample from progmem and output to PWM
//             uint8_t sample = pgm_read_byte(&chimeAudio[currentChimeSample]);
            
//             // Output to PWM
//             ledcWrite(0, sample);
//         }
        
//         currentChimeSample++;
//     } else if (isChimePlaying) {
//         // Playback finished
//         isChimePlaying = false;
//         currentChimeSample = WAV_HEADER_SIZE; // Reset for next time

//         // Stop output to prevent noise
//         if (!isSpeakerMuted) {
//             ledcWrite(0, 128); // Output silence (mid-level)
//         }
//     }

//     portEXIT_CRITICAL_ISR(&chimeTimerMux);
// }

// Speaker::Speaker() {
//     // Initialize audio pin with PWM
//     ledcSetup(0, 50000, 8); // Channel 0, 50kHz PWM, 8-bit resolution
//     ledcAttachPin(AUDIO_PIN, 0);
    
//     // Set up timer interrupt for audio playback
//     chimeTimer = timerBegin(0, 80, true); // 80MHz clock divided by 80 = 1MHz
//     timerAttachInterrupt(chimeTimer, &onChimeTimer, true);
    
//     // Set alarm to trigger interrupt at the chime sample rate
//     timerAlarmWrite(chimeTimer, 1000000/AUDIO_SAMPLE_RATE, true);
//     timerAlarmEnable(chimeTimer);
    
//     // Output silence initially
//     ledcWrite(0, 128);
    
//     // Initialize mute state
//     isMuted = false;
//     isSpeakerMuted = false;
// }

// void Speaker::mute() {
//     if (!isMuted) {
//         isMuted = true;
//         isSpeakerMuted = true;
        
//         // Immediately silence the speaker
//         ledcWrite(0, 0);
        
//         Serial.println("Speaker muted");
//     }
// }

// void Speaker::unmute() {
//     if (isMuted) {
//         isMuted = false;
//         isSpeakerMuted = false;
        
//         // Set to mid-level to avoid pop when unmuting
//         ledcWrite(0, 128);
        
//         Serial.println("Speaker unmuted");
//     }
// }

// void Speaker::setMuted(bool muted) {
//     if (muted) {
//         mute();
//     } else {
//         unmute();
//     }
// }

// bool Speaker::getMuted() const {
//     return isMuted;
// }

// void Speaker::chime() {
//     if (isChimePlaying) return;
//     // Reset playback position
//     currentChimeSample = WAV_HEADER_SIZE;
    
//     // Start playback
//     isChimePlaying = true;
    
//     Serial.println("Playing chime...");
// }

// bool Speaker::isPlaying() const {
//     return isChimePlaying;
// }
