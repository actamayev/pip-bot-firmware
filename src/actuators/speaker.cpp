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
    
    SerialQueueManager::getInstance().queueMessage("Initializing Speaker...");
    
    // Create mutex for thread safety
    if (!audioMutex) {
        audioMutex = xSemaphoreCreateMutex();
        if (!audioMutex) {
            SerialQueueManager::getInstance().queueMessage("✗ Speaker: Failed to create mutex");
            return false;
        }
    }
    
    if (!initializeLittleFS()) {
        SerialQueueManager::getInstance().queueMessage("✗ Speaker: LittleFS init failed");
        return false;
    }
    
    if (!initializeAudio()) {
        SerialQueueManager::getInstance().queueMessage("✗ Speaker: Audio init failed");
        return false;
    }
    
    initialized = true;
    audioObjectsValid = true;
    SerialQueueManager::getInstance().queueMessage("✓ Speaker initialized");
    return true;
}

bool Speaker::initializeLittleFS() {
    if (!LittleFS.begin(true)) return false;
    SerialQueueManager::getInstance().queueMessage("✓ LittleFS mounted");
    return true;
}

bool Speaker::initializeAudio() {
    // Clean up any existing objects first
    cleanup();
    
    // Create audio objects with error checking
    audioFile = new (std::nothrow) AudioFileSourceLittleFS();
    if (!audioFile) {
        SerialQueueManager::getInstance().queueMessage("✗ Failed to create AudioFileSourceLittleFS");
        cleanup();
        return false;
    }
    
    audioID3 = new (std::nothrow) AudioFileSourceID3(audioFile);
    if (!audioID3) {
        SerialQueueManager::getInstance().queueMessage("✗ Failed to create AudioFileSourceID3");
        cleanup();
        return false;
    }
    
    audioMP3 = new (std::nothrow) AudioGeneratorMP3();
    if (!audioMP3) {
        SerialQueueManager::getInstance().queueMessage("✗ Failed to create AudioGeneratorMP3");
        cleanup();
        return false;
    }
    
    audioOutput = new (std::nothrow) AudioOutputI2S();
    if (!audioOutput) {
        SerialQueueManager::getInstance().queueMessage("✗ Failed to create AudioOutputI2S");
        cleanup();
        return false;
    }
    
    // Configure I2S pins
    audioOutput->SetPinout(I2S_BCLK, I2S_LRC, I2S_DOUT);
    audioOutput->SetGain(currentVolume);
    
    audioObjectsValid = true;
    return true;
}

bool Speaker::recreateAudioObjects() {
    SerialQueueManager::getInstance().queueMessage("Recreating audio objects...");
    
    // Clean up existing objects
    cleanup();

    vTaskDelay(pdMS_TO_TICKS(100));
    
    // Recreate audio objects
    bool success = initializeAudio();
    
    if (success) {
        SerialQueueManager::getInstance().queueMessage("✓ Audio objects recreated");
        forceRecreateObjects = false;
    } else {
        SerialQueueManager::getInstance().queueMessage("✗ Failed to recreate audio objects");
    }
    
    return success;
}

bool Speaker::validateAudioObjects() {
    return audioObjectsValid && 
           audioFile != nullptr && 
           audioID3 != nullptr && 
           audioMP3 != nullptr && 
           audioOutput != nullptr;
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
    
    // ADD with other deletes:
    if (toneSource) { delete toneSource; toneSource = nullptr; }
    if (toneGenerator) { delete toneGenerator; toneGenerator = nullptr; }
    
    // ADD with other resets:
    isPlayingTone = false;

    // Stop any ongoing playback first
    if (audioMP3 && audioMP3->isRunning()) {
        audioMP3->stop();
        vTaskDelay(pdMS_TO_TICKS(50));
    }

    if (rtttlGenerator && rtttlGenerator->isRunning()) {
        rtttlGenerator->stop();
        vTaskDelay(pdMS_TO_TICKS(50));
    }

    if (audioFile) {
        audioFile->close();
    }

    if (audioMP3) { delete audioMP3; audioMP3 = nullptr; }
    if (audioID3) { delete audioID3; audioID3 = nullptr; }
    if (audioFile) { delete audioFile; audioFile = nullptr; }
    if (rtttlSource) { delete rtttlSource; rtttlSource = nullptr; }
    if (rtttlGenerator) { delete rtttlGenerator; rtttlGenerator = nullptr; }
    if (audioOutput) { delete audioOutput; audioOutput = nullptr; }

    isCurrentlyPlaying = false;
    isStoppingPlayback = false;
    isMelodyPlaying = false;
    audioObjectsValid = false;
    currentFilename = "";

    if (hasMutex) {
        xSemaphoreGive(audioMutex);
    }
    vTaskDelay(pdMS_TO_TICKS(50));
}

bool Speaker::safeStopPlayback() {
    if (!isCurrentlyPlaying) return true;

    // Try to take the mutex first with a reasonable timeout
    if (!audioMutex || xSemaphoreTake(audioMutex, pdMS_TO_TICKS(200)) != pdTRUE) {
        SerialQueueManager::getInstance().queueMessage("✗ Failed to acquire audio mutex for stop");
        return false;
    }

    bool stopSuccess = true;
    // Re-check state under mutex
    if (isCurrentlyPlaying && audioMP3 && audioObjectsValid) {
        bool canStop = audioMP3->isRunning();
        if (canStop) {
            SerialQueueManager::getInstance().queueMessage("audioMP3->stop");
            // safe to call stop while holding mutex
            audioMP3->stop();
            vTaskDelay(pdMS_TO_TICKS(50));
        }
    }

    if (audioFile && audioObjectsValid) {
        SerialQueueManager::getInstance().queueMessage("audioFile->close");
        audioFile->close();
        vTaskDelay(pdMS_TO_TICKS(20));
    }

    // Set stopping state and timer
    isStoppingPlayback = true;
    stopRequestTime = millis();
    isCurrentlyPlaying = false;
    currentFilename = "";

    // Release mutex
    xSemaphoreGive(audioMutex);
    return stopSuccess;
}

void Speaker::setMuted(bool shouldMute) {
    muted = shouldMute;
    SerialQueueManager::getInstance().queueMessage("Speaker muted: " + String(shouldMute ? "MUTING" : "UNMUTING"));
    if (initialized && audioMP3 && audioMP3->isRunning() && shouldMute) {
        safeStopPlayback();
    }
}

void Speaker::setVolume(float volume) {
    currentVolume = constrain(volume, 0.0f, 3.9f);
    SerialQueueManager::getInstance().queueMessage("Speaker initialized: " + String(initialized));
    SerialQueueManager::getInstance().queueMessage("Audio output set: " + String(audioOutput != nullptr));
    if (initialized && audioOutput) {
        SerialQueueManager::getInstance().queueMessage("Setting volume to: " + String(currentVolume));
        audioOutput->SetGain(currentVolume);
    }
}

void Speaker::stopAllSounds() {
    stopTone();
    stopHorn();
    if (initialized && isCurrentlyPlaying) {
        safeStopPlayback();
    }

    SerialQueueManager::getInstance().queueMessage("Stopped playback");
    
    // Stop RTTTL melody if playing
    if (isMelodyPlaying && rtttlGenerator && rtttlGenerator->isRunning()) {
        SerialQueueManager::getInstance().queueMessage("Stopping RTTTL");
        rtttlGenerator->stop();
        isMelodyPlaying = false;
        
        // Stop LED sequence and turn off LEDs
        isLedSequencePlaying = false;
        rgbLed.turn_all_leds_off();
        
        SerialQueueManager::getInstance().queueMessage("Stopped entertainer melody and LED sync");
        
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
            SerialQueueManager::getInstance().queueMessage("Speaker::update() calling updateContinuousTone()");
            lastUpdateDebug = millis();
        }
        
        updateContinuousTone(); // CHANGED from updateTone()
        return;
    }

    // Keep melody processing as before but protect shared state
    if (isMelodyPlaying) {
        // melody functions will take audioMutex internally (see updateMelody)
        updateMelody();
        return;
    }

    // Handle delayed clearing of stopping state
    if (isStoppingPlayback) {
        if (millis() - stopRequestTime >= STOP_DELAY_MS) {
            isStoppingPlayback = false;
        }
        return;
    }

    // If we need to read or run audioMP3, take the mutex
    if (!audioMutex || xSemaphoreTake(audioMutex, pdMS_TO_TICKS(100)) != pdTRUE) {
        // failed to lock, just return — safer than touching audio objects
        return;
    }

    if (isCurrentlyPlaying) {
        if (audioMP3 && audioMP3->isRunning()) {
            // call loop() while holding the mutex so object cannot be deleted under us
            if (!audioMP3->loop()) {
                SerialQueueManager::getInstance().queueMessage("Audio playback completed");
                isCurrentlyPlaying = false;
                currentFilename = "";
            }
        } else {
            SerialQueueManager::getInstance().queueMessage("Audio playback ended unexpectedly");
            isCurrentlyPlaying = false;
            currentFilename = "";
            forceRecreateObjects = true;
        }
    }

    xSemaphoreGive(audioMutex);
}

void Speaker::updateMelody() {
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
            SerialQueueManager::getInstance().queueMessage("Entertainer melody completed");
            if (rtttlSource) { delete rtttlSource; rtttlSource = nullptr; }
        }
    }

    xSemaphoreGive(audioMutex);
}

void Speaker::startEntertainerMelody() {
    if (muted || isMelodyPlaying) return;

    SerialQueueManager::getInstance().queueMessage("Starting The Entertainer melody with LED sync");

    // Take mutex for RTTTL creation and start
    if (!audioMutex || xSemaphoreTake(audioMutex, pdMS_TO_TICKS(200)) != pdTRUE) {
        SerialQueueManager::getInstance().queueMessage("✗ Failed to acquire audio mutex for melody start");
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
        SerialQueueManager::getInstance().queueMessage("✓ Entertainer melody and LED sync started");
    } else {
        SerialQueueManager::getInstance().queueMessage("✗ Failed to start Entertainer playback");
        if (rtttlSource) { delete rtttlSource; rtttlSource = nullptr; }
    }

    xSemaphoreGive(audioMutex);
}

void Speaker::playTone(ToneType tone) {
    if (!initialized) {
        SerialQueueManager::getInstance().queueMessage("ERROR: Speaker not initialized!");
        return;
    }
    
    if (muted) {
        SerialQueueManager::getInstance().queueMessage("Speaker muted");
        return;
    }
    
    uint8_t toneIndex = static_cast<uint8_t>(tone);
    
    // tone = 0 means stop
    if (toneIndex == 0) {
        SerialQueueManager::getInstance().queueMessage("Stopping tone");
        stopTone();
        return;
    }
    
    // Validate tone range
    if (toneIndex > 7) {
        SerialQueueManager::getInstance().queueMessage("Invalid tone: " + String(toneIndex));
        return;
    }
    
    // ADD THIS: Update refresh timestamp whenever tone is requested
    lastToneRefreshTime = millis();
    
    // If already playing this exact tone, continue (don't restart)
    if (isPlayingTone && currentTone == tone) {
        SerialQueueManager::getInstance().queueMessage("Already playing this tone - continuing");
        return;
    }
    
    // Take mutex for tone creation and start
    if (!audioMutex || xSemaphoreTake(audioMutex, pdMS_TO_TICKS(200)) != pdTRUE) {
        SerialQueueManager::getInstance().queueMessage("Failed to acquire mutex for tone");
        return;
    }
    
    SerialQueueManager::getInstance().queueMessage("Stopping any existing playback");
    
    // Stop any MP3 playback
    if (isCurrentlyPlaying) {
        if (audioMP3 && audioMP3->isRunning()) {
            audioMP3->stop();
        }
        isCurrentlyPlaying = false;
    }
    
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
    const char* rtttlString = getToneRTTTL(tone);
    if (!rtttlString) {
        SerialQueueManager::getInstance().queueMessage("Failed to get RTTTL for tone");
        xSemaphoreGive(audioMutex);
        return;
    }
    
    toneGenerator = new AudioGeneratorRTTTL();
    toneSource = new AudioFileSourcePROGMEM(rtttlString, strlen(rtttlString) + 1);
    
    if (audioOutput && toneGenerator->begin(toneSource, audioOutput)) {
        currentTone = tone;
        isPlayingTone = true;
        SerialQueueManager::getInstance().queueMessage("Tone playback started successfully");
    } else {
        SerialQueueManager::getInstance().queueMessage("Failed to start tone playback");
        if (toneSource) { delete toneSource; toneSource = nullptr; }
        if (toneGenerator) { delete toneGenerator; toneGenerator = nullptr; }
    }
    
    xSemaphoreGive(audioMutex);
}

void Speaker::stopTone() {
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
    SerialQueueManager::getInstance().queueMessage("Tone stopped");
    
    xSemaphoreGive(audioMutex);
}

void Speaker::updateContinuousTone() {
    if (!audioMutex || xSemaphoreTake(audioMutex, pdMS_TO_TICKS(100)) != pdTRUE) {
        return;
    }

    if (!isPlayingTone || !toneGenerator) {
        xSemaphoreGive(audioMutex);
        return;
    }

    // Auto-stop if tone hasn't been refreshed recently (unless in horn mode)
    if (!isHornMode && millis() - lastToneRefreshTime > TONE_AUTO_STOP_MS) {
        SerialQueueManager::getInstance().queueMessage("Tone auto-stopped (not refreshed)");
        if (toneGenerator->isRunning()) {
            toneGenerator->stop();
        }
        isPlayingTone = false;
        xSemaphoreGive(audioMutex);
        return;
    }
    
    if (toneGenerator->isRunning()) {
        if (!toneGenerator->loop()) {
            // Tone finished - restart it QUICKLY to minimize gap
            
            // OPTIMIZATION: Pre-get the RTTTL string before any deletions
            const char* rtttlString = getToneRTTTL(currentTone);
            if (!rtttlString) {
                SerialQueueManager::getInstance().queueMessage("Failed to get RTTTL for restart");
                isPlayingTone = false;
                xSemaphoreGive(audioMutex);
                return;
            }
            
            // OPTIMIZATION: Create new source BEFORE deleting old one
            AudioFileSourcePROGMEM* newSource = new AudioFileSourcePROGMEM(
                rtttlString, 
                strlen(rtttlString) + 1
            );
            
            // Now delete the old source
            if (toneSource) {
                delete toneSource;
            }
            toneSource = newSource;
            
            // Restart immediately - no delay between delete and begin
            if (!toneGenerator->begin(toneSource, audioOutput)) {
                SerialQueueManager::getInstance().queueMessage("Failed to restart tone");
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
        SerialQueueManager::getInstance().queueMessage("Tone stopped unexpectedly");
        isPlayingTone = false;
    }
    
    xSemaphoreGive(audioMutex);
}

const char* Speaker::getToneRTTTL(ToneType tone) {
    // RTTTL format: each note plays for 1 second and repeats
    // Format: name:settings:notes
    // d=1 means whole note (slowest), o=5 is middle octave, b=60 is slow tempo
    switch (tone) {
        case ToneType::TONE_A: return "ToneA:d=1,o=5,b=10:a";
        case ToneType::TONE_B: return "ToneB:d=1,o=5,b=10:b";
        case ToneType::TONE_C: return "ToneC:d=1,o=5,b=10:c";
        case ToneType::TONE_D: return "ToneD:d=1,o=5,b=10:d";
        case ToneType::TONE_E: return "ToneE:d=1,o=5,b=10:e";
        case ToneType::TONE_F: return "ToneF:d=1,o=5,b=10:f";
        case ToneType::TONE_G: return "ToneG:d=1,o=5,b=10:g";
        default: return nullptr;
    }
}

void Speaker::startHorn() {
    SerialQueueManager::getInstance().queueMessage("Starting horn (continuous F tone)");
    isHornMode = true;
    playTone(ToneType::TONE_F);
}

void Speaker::stopHorn() {
    SerialQueueManager::getInstance().queueMessage("Stopping horn");
    isHornMode = false;
    stopTone();
}
