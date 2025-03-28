#include "./speaker.h"
#include "../sounds/chime.h"
#include "../utils/config.h"

Speaker speaker;

// Audio playback variables
volatile int currentChimeSample = WAV_HEADER_SIZE; // Start after WAV header
volatile bool isChimePlaying = false;

// Timer for chime playback
hw_timer_t * chimeTimer = NULL;
portMUX_TYPE chimeTimerMux = portMUX_INITIALIZER_UNLOCKED;

// Timer interrupt handler for chime playback
void IRAM_ATTR onChimeTimer() {
    portENTER_CRITICAL_ISR(&chimeTimerMux);
    
    if (currentChimeSample < AUDIO_DATA_LENGTH && isChimePlaying) {
        // Read sample from progmem and output to PWM
        uint8_t sample = pgm_read_byte(&chimeAudio[currentChimeSample]);
        
        // Output to PWM
        ledcWrite(0, sample);

        currentChimeSample++;
    } else if (isChimePlaying) {
        // Playback finished
        isChimePlaying = false;
        currentChimeSample = WAV_HEADER_SIZE; // Reset for next time
        
        // Stop output to prevent noise
        ledcWrite(0, 128); // Output silence (mid-level)
    }

    portEXIT_CRITICAL_ISR(&chimeTimerMux);
}

Speaker::Speaker() {
    // Initialize audio pin with PWM
    ledcSetup(0, 50000, 8); // Channel 0, 50kHz PWM, 8-bit resolution
    ledcAttachPin(AUDIO_PIN, 0);
    
    // Set up timer interrupt for audio playback
    chimeTimer = timerBegin(0, 80, true); // 80MHz clock divided by 80 = 1MHz
    timerAttachInterrupt(chimeTimer, &onChimeTimer, true);
    
    // Set alarm to trigger interrupt at the chime sample rate
    timerAlarmWrite(chimeTimer, 1000000/AUDIO_SAMPLE_RATE, true);
    timerAlarmEnable(chimeTimer);
    
    // Output silence initially
    ledcWrite(0, 128);
}

void Speaker::chime() {
    if (isChimePlaying) return;
    // Reset playback position
    currentChimeSample = WAV_HEADER_SIZE;
    
    // Start playback
    isChimePlaying = true;
    
    Serial.println("Playing chime...");
}

bool Speaker::isPlaying() const {
    return isChimePlaying;
}

void Speaker::update() {
    // This method is called periodically from a task
    // Currently, we don't need to do anything here since playback is handled by the timer interrupt
    // But we could add checks or other functionality here if needed
}