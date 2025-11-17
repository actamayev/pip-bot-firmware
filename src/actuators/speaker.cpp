#include "speaker.h"

// Define the static constexpr LED sequence array
constexpr const Speaker::MelodyNote Speaker::entertainerLedSequence[];

Speaker::~Speaker() {
    cleanup();
    if (!audioMutex) return;
    vSemaphoreDelete(audioMutex);
    audioMutex = nullptr;
}

bool Speaker::initialize() {
    if (initialized) return true;

    SerialQueueManager::get_instance().queue_message("Initializing Speaker...");

    // Create mutex for thread safety
    if (!audioMutex) {
        audioMutex = xSemaphoreCreateMutex();
        if (!audioMutex) {
            SerialQueueManager::get_instance().queue_message("✗ Speaker: Failed to create mutex");
            return false;
        }
    }

    if (!initialize_audio()) {
        SerialQueueManager::get_instance().queue_message("✗ Speaker: Audio init failed");
        return false;
    }

    initialized = true;
    audioObjectsValid = true;
    SerialQueueManager::get_instance().queue_message("✓ Speaker initialized");
    return true;
}

bool Speaker::initialize_audio() {
    // Clean up any existing objects first
    cleanup();

    audioOutput = new (std::nothrow) AudioOutputI2S();
    if (!audioOutput) {
        SerialQueueManager::get_instance().queue_message("✗ Failed to create AudioOutputI2S");
        cleanup();
        return false;
    }

    // Configure I2S pins
    audioOutput->SetPinout(I2S_BCLK, I2S_LRC, I2S_DOUT);
    audioOutput->SetGain(currentVolume);

    audioObjectsValid = true;
    return true;
}

bool Speaker::recreate_audio_objects() {
    SerialQueueManager::get_instance().queue_message("Recreating audio objects...");

    // Clean up existing objects
    cleanup();

    vTaskDelay(pdMS_TO_TICKS(100));

    // Recreate audio objects
    bool success = initialize_audio();

    if (success) {
        SerialQueueManager::get_instance().queue_message("✓ Audio objects recreated");
    } else {
        SerialQueueManager::get_instance().queue_message("✗ Failed to recreate audio objects");
    }

    return success;
}

bool Speaker::validate_audio_objects() {
    return audioObjectsValid && audioOutput != nullptr;
}

void Speaker::cleanup() {
    // Take mutex if available (don't block forever)
    bool hasMutex = false;
    if (audioMutex && xSemaphoreTake(audioMutex, pdMS_TO_TICKS(200)) == pdTRUE) {
        hasMutex = true;
    }

    if (toneGenerator && toneGenerator->isRunning()) {
        toneGenerator->stop();
        vTaskDelay(pdMS_TO_TICKS(50));
    }

    if (toneSource) {
        delete toneSource;
        toneSource = nullptr;
    }
    if (toneGenerator) {
        delete toneGenerator;
        toneGenerator = nullptr;
    }
    isPlayingTone = false;

    if (rtttlGenerator && rtttlGenerator->isRunning()) {
        rtttlGenerator->stop();
        vTaskDelay(pdMS_TO_TICKS(50));
    }

    if (rtttlSource) {
        delete rtttlSource;
        rtttlSource = nullptr;
    }
    if (rtttlGenerator) {
        delete rtttlGenerator;
        rtttlGenerator = nullptr;
    }
    if (audioOutput) {
        delete audioOutput;
        audioOutput = nullptr;
    }

    isMelodyPlaying = false;
    audioObjectsValid = false;

    if (hasMutex) {
        xSemaphoreGive(audioMutex);
    }
    vTaskDelay(pdMS_TO_TICKS(50));
}

void Speaker::set_muted(bool shouldMute) {
    muted = shouldMute;
    SerialQueueManager::get_instance().queue_message("Speaker muted: " + String(shouldMute ? "MUTING" : "UNMUTING"));
    if (shouldMute) {
        stop_all_sounds();
    }
}

void Speaker::set_volume(float volume) {
    currentVolume = constrain(volume, 0.0f, 3.9f);
    SerialQueueManager::get_instance().queue_message("Speaker initialized: " + String(initialized));
    SerialQueueManager::get_instance().queue_message("Audio output set: " + String(audioOutput != nullptr));
    if (initialized && audioOutput) {
        SerialQueueManager::get_instance().queue_message("Setting volume to: " + String(currentVolume));
        audioOutput->SetGain(currentVolume);
    }
}

void Speaker::stop_all_sounds() {
    stop_tone();

    SerialQueueManager::get_instance().queue_message("Stopped playback");

    // Stop RTTTL melody if playing
    if (isMelodyPlaying && rtttlGenerator && rtttlGenerator->isRunning()) {
        SerialQueueManager::get_instance().queue_message("Stopping RTTTL");
        rtttlGenerator->stop();
        isMelodyPlaying = false;

        // Stop LED sequence and turn off LEDs
        isLedSequencePlaying = false;
        rgbLed.turn_all_leds_off();

        SerialQueueManager::get_instance().queue_message("Stopped entertainer melody and LED sync");

        // Clean up RTTTL resources
        if (rtttlSource) {
            delete rtttlSource;
            rtttlSource = nullptr;
        }
    }
}

void Speaker::update() {
    if (!initialized) return;

    if (isPlayingTone) {
        static unsigned long lastUpdateDebug = 0;
        if (millis() - lastUpdateDebug > 1000) {
            SerialQueueManager::get_instance().queue_message("Speaker::update() calling updateContinuousTone()");
            lastUpdateDebug = millis();
        }

        update_continuous_tone();
        return;
    }

    // Keep melody processing as before but protect shared state
    if (isMelodyPlaying) {
        // melody functions will take audioMutex internally (see update_melody)
        update_melody();
        return;
    }
}

void Speaker::update_melody() {
    // Acquire mutex around operations that touch audio/LED state
    if (!audioMutex || xSemaphoreTake(audioMutex, pdMS_TO_TICKS(100)) != pdTRUE) {
        return;
    }

    if (!isMelodyPlaying || !rtttlGenerator) {
        xSemaphoreGive(audioMutex);
        return;
    }

    // LED sequence update (doesn't touch audio objects, but keep under mutex to keep state consistent)
    if (isLedSequencePlaying) {
        unsigned long currentTime = millis();
        if (currentTime >= ledStepStartTime + entertainerLedSequence[currentLedStep].duration) {
            currentLedStep++;
            if (currentLedStep >= ENTERTAINER_LED_SEQUENCE_LENGTH) {
                isLedSequencePlaying = false;
                rgbLed.turn_all_leds_off();
            } else {
                const MelodyNote& note = entertainerLedSequence[currentLedStep];
                rgbLed.set_main_board_leds_to_color(note.ledR, note.ledG, note.ledB);
                ledStepStartTime = currentTime;
            }
        }
    }

    // Keep RTTTL generator running safely while mutex held
    if (rtttlGenerator->isRunning()) {
        if (!rtttlGenerator->loop()) {
            isMelodyPlaying = false;
            isLedSequencePlaying = false;
            rgbLed.turn_all_leds_off();
            SerialQueueManager::get_instance().queue_message("Entertainer melody completed");
            if (rtttlSource) {
                delete rtttlSource;
                rtttlSource = nullptr;
            }
        }
    }

    xSemaphoreGive(audioMutex);
}

void Speaker::start_entertainer_melody() {
    if (muted || isMelodyPlaying) return;

    SerialQueueManager::get_instance().queue_message("Starting The Entertainer melody with LED sync");

    // Take mutex for RTTTL creation and start
    if (!audioMutex || xSemaphoreTake(audioMutex, pdMS_TO_TICKS(200)) != pdTRUE) {
        SerialQueueManager::get_instance().queue_message("✗ Failed to acquire audio mutex for melody start");
        return;
    }

    static const uint8_t entertainerRTTTL[] = "TheEntertainer:d=4,o=5,b=140:8d,8d#,8e,c6,8e,c6,8e,c6,c,8c6,8a,8g,8f#,8a,8c6,8e,8d,8c,8a,2d";

    if (!rtttlGenerator) {
        rtttlGenerator = new AudioGeneratorRTTTL();
    }
    if (rtttlSource) {
        delete rtttlSource;
        rtttlSource = nullptr;
    }
    rtttlSource = new AudioFileSourcePROGMEM(entertainerRTTTL, sizeof(entertainerRTTTL));

    if (audioOutput && rtttlGenerator->begin(rtttlSource, audioOutput)) {
        isMelodyPlaying = true;
        isLedSequencePlaying = true;
        currentLedStep = 0;
        ledStepStartTime = millis();
        const MelodyNote& firstNote = entertainerLedSequence[0];
        rgbLed.set_main_board_leds_to_color(firstNote.ledR, firstNote.ledG, firstNote.ledB);
        SerialQueueManager::get_instance().queue_message("✓ Entertainer melody and LED sync started");
    } else {
        SerialQueueManager::get_instance().queue_message("✗ Failed to start Entertainer playback");
        if (rtttlSource) {
            delete rtttlSource;
            rtttlSource = nullptr;
        }
    }

    xSemaphoreGive(audioMutex);
}

void Speaker::play_tone(ToneType tone) {
    if (!initialized) {
        SerialQueueManager::get_instance().queue_message("ERROR: Speaker not initialized!");
        return;
    }

    if (muted) {
        SerialQueueManager::get_instance().queue_message("Speaker muted");
        return;
    }

    uint8_t toneIndex = static_cast<uint8_t>(tone);

    // Validate tone range (1-8, where 8 is TONE_OFF)
    if (toneIndex < 1 || toneIndex > 8) {
        SerialQueueManager::get_instance().queue_message("Invalid tone: " + String(toneIndex));
        return;
    }

    // If already playing this exact tone, continue (don't restart)
    if (isPlayingTone && currentTone == tone) {
        // Update refresh timestamp to prevent auto-stop
        lastToneRefreshTime = millis();
        SerialQueueManager::get_instance().queue_message("Already playing this tone - continuing");
        return;
    }

    // Update refresh timestamp for new tone requests
    lastToneRefreshTime = millis();

    // Take mutex for tone creation and start
    if (!audioMutex || xSemaphoreTake(audioMutex, pdMS_TO_TICKS(200)) != pdTRUE) {
        SerialQueueManager::get_instance().queue_message("Failed to acquire mutex for tone");
        return;
    }

    SerialQueueManager::get_instance().queue_message("Stopping any existing playback");

    // Stop melody if playing
    if (isMelodyPlaying && rtttlGenerator && rtttlGenerator->isRunning()) {
        rtttlGenerator->stop();
        isMelodyPlaying = false;
        isLedSequencePlaying = false;
        rgbLed.turn_all_leds_off();
    }

    // Stop existing tone if any
    if (isPlayingTone && toneGenerator && toneGenerator->isRunning()) {
        toneGenerator->stop();
        vTaskDelay(pdMS_TO_TICKS(20));
    }

    // Clean up old tone resources
    if (toneSource) {
        delete toneSource;
        toneSource = nullptr;
    }
    if (toneGenerator) {
        delete toneGenerator;
        toneGenerator = nullptr;
    }

    // Create new tone generator
    const char* rtttlString = get_tone_rtttl(tone);
    if (!rtttlString) {
        SerialQueueManager::get_instance().queue_message("Failed to get RTTTL for tone");
        xSemaphoreGive(audioMutex);
        return;
    }

    toneGenerator = new AudioGeneratorRTTTL();
    toneSource = new AudioFileSourcePROGMEM(rtttlString, strlen(rtttlString) + 1);

    if (audioOutput && toneGenerator->begin(toneSource, audioOutput)) {
        currentTone = tone;
        isPlayingTone = true;
        SerialQueueManager::get_instance().queue_message("Tone playback started successfully");
    } else {
        SerialQueueManager::get_instance().queue_message("Failed to start tone playback");
        if (toneSource) {
            delete toneSource;
            toneSource = nullptr;
        }
        if (toneGenerator) {
            delete toneGenerator;
            toneGenerator = nullptr;
        }
    }

    xSemaphoreGive(audioMutex);
}

void Speaker::stop_tone() {
    if (!isPlayingTone) return;

    if (!audioMutex || xSemaphoreTake(audioMutex, pdMS_TO_TICKS(200)) != pdTRUE) {
        return;
    }

    if (toneGenerator && toneGenerator->isRunning()) {
        toneGenerator->stop();
        vTaskDelay(pdMS_TO_TICKS(20));
    }

    if (toneSource) {
        delete toneSource;
        toneSource = nullptr;
    }
    if (toneGenerator) {
        delete toneGenerator;
        toneGenerator = nullptr;
    }

    isPlayingTone = false;
    SerialQueueManager::get_instance().queue_message("Tone stopped");

    xSemaphoreGive(audioMutex);
}

void Speaker::update_continuous_tone() {
    if (!audioMutex || xSemaphoreTake(audioMutex, pdMS_TO_TICKS(100)) != pdTRUE) {
        return;
    }

    if (!isPlayingTone || !toneGenerator) {
        xSemaphoreGive(audioMutex);
        return;
    }

    if (toneGenerator->isRunning()) {
        if (!toneGenerator->loop()) {
            // Tone finished - restart it QUICKLY to minimize gap

            // OPTIMIZATION: Pre-get the RTTTL string before any deletions
            const char* rtttlString = get_tone_rtttl(currentTone);
            if (!rtttlString) {
                SerialQueueManager::get_instance().queue_message("Failed to get RTTTL for restart");
                isPlayingTone = false;
                xSemaphoreGive(audioMutex);
                return;
            }

            // OPTIMIZATION: Create new source BEFORE deleting old one
            AudioFileSourcePROGMEM* newSource = new AudioFileSourcePROGMEM(rtttlString, strlen(rtttlString) + 1);

            // Now delete the old source
            if (toneSource) {
                delete toneSource;
            }
            toneSource = newSource;

            // Restart immediately - no delay between delete and begin
            if (!toneGenerator->begin(toneSource, audioOutput)) {
                SerialQueueManager::get_instance().queue_message("Failed to restart tone");
                isPlayingTone = false;
                if (toneSource) {
                    delete toneSource;
                    toneSource = nullptr;
                }
                if (toneGenerator) {
                    delete toneGenerator;
                    toneGenerator = nullptr;
                }
            }
            // If successful, tone continues seamlessly (with minimal 20-50ms gap)
        }
    } else {
        // Tone unexpectedly stopped
        SerialQueueManager::get_instance().queue_message("Tone stopped unexpectedly");
        isPlayingTone = false;
    }

    xSemaphoreGive(audioMutex);
}

const char* Speaker::get_tone_rtttl(ToneType tone) {
    // RTTTL format: each note plays for 1 second and repeats
    // Format: name:settings:notes
    // d=1 means whole note (slowest), o=5 is middle octave, b=60 is slow tempo
    switch (tone) {
        case ToneType::TONE_A:
            return "ToneA:d=1,o=5,b=10:a";
        case ToneType::TONE_B:
            return "ToneB:d=1,o=5,b=10:b";
        case ToneType::TONE_C:
            return "ToneC:d=1,o=5,b=10:c";
        case ToneType::TONE_D:
            return "ToneD:d=1,o=5,b=10:d";
        case ToneType::TONE_E:
            return "ToneE:d=1,o=5,b=10:e";
        case ToneType::TONE_F:
            return "ToneF:d=1,o=5,b=10:f";
        case ToneType::TONE_G:
            return "ToneG:d=1,o=5,b=10:g";
        default:
            return nullptr;
    }
}
