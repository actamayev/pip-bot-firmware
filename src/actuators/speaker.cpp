#include "speaker.h"

Speaker::~Speaker() {
    cleanup();
}

bool Speaker::initialize() {
    if (initialized) return true;
    
    SerialQueueManager::getInstance().queueMessage("Initializing Speaker...");
    
    if (!initializeSPIFFS()) {
        SerialQueueManager::getInstance().queueMessage("✗ Speaker: SPIFFS init failed");
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

bool Speaker::initializeSPIFFS() {
    if (SPIFFS.begin(true)) {
        SerialQueueManager::getInstance().queueMessage("✓ SPIFFS mounted");
        return true;
    }
    return false;
}

bool Speaker::initializeAudio() {
    // Clean up any existing objects first
    cleanup();
    
    // Create audio objects with error checking
    audioFile = new (std::nothrow) AudioFileSourceSPIFFS();
    if (!audioFile) {
        SerialQueueManager::getInstance().queueMessage("✗ Failed to create SoundTypeSourceSPIFFS");
        cleanup();
        return false;
    }
    
    audioID3 = new (std::nothrow) AudioFileSourceID3(audioFile);
    if (!audioID3) {
        SerialQueueManager::getInstance().queueMessage("✗ Failed to create SoundTypeSourceID3");
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
    
    // Wait for cleanup to complete
    delay(100);
    
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
    // Stop any ongoing playback first
    if (audioMP3 && audioMP3->isRunning()) {
        audioMP3->stop();
        delay(50); // Give time for stop to complete
    }
    
    // Close any open files
    if (audioFile) {
        audioFile->close();
    }
    
    // Delete in reverse order of creation
    if (audioMP3) {
        delete audioMP3;
        audioMP3 = nullptr;
    }
    if (audioID3) {
        delete audioID3;
        audioID3 = nullptr;
    }
    if (audioFile) {
        delete audioFile;
        audioFile = nullptr;
    }
    if (audioOutput) {
        delete audioOutput;
        audioOutput = nullptr;
    }
    
    // Clear queue and reset state
    while (!audioQueue.empty()) {
        audioQueue.pop();
    }
    
    isCurrentlyPlaying = false;
    isStoppingPlayback = false;
    audioObjectsValid = false;
    currentFilename = "";
    
    // Give time for I2S resources to be released
    delay(50);
}

const char* Speaker::getFilePath(SoundType audioFile) const {
    switch (audioFile) {
        case SoundType::CHIME:          return "/chime.mp3";
        case SoundType::CHIRP:          return "/chirp.mp3";
        case SoundType::POP:            return "/pop.mp3";
        case SoundType::DROP:           return "/drop.mp3";
        case SoundType::FART:           return "/fart.mp3";
        case SoundType::MONKEY:         return "/monkey.mp3";
        case SoundType::ELEPHANT:       return "/elephant.mp3";
        case SoundType::PARTY:          return "/party.mp3";
        case SoundType::UFO:            return "/alien.mp3";
        case SoundType::COUNTDOWN:      return "/arcade.mp3";
        case SoundType::ENGINE:         return "/engine_rev.mp3";
        case SoundType::ROBOT:          return "/robot.mp3";
        default:                        return nullptr;
    }
}

void Speaker::playFile(SoundType file) {
    if (!initialized || muted) {
        SerialQueueManager::getInstance().queueMessage("Speaker not ready or muted");
        return;
    }
    
    // NEW: Prevent rapid successive calls
    unsigned long currentTime = millis();
    if (currentTime - lastPlaybackTime < MIN_PLAYBACK_INTERVAL_MS) {
        SerialQueueManager::getInstance().queueMessage("Playback too rapid, ignoring request");
        return;
    }
    lastPlaybackTime = currentTime;

    // Log the volume
    SerialQueueManager::getInstance().queueMessage("Speaker volume: " + String(currentVolume));
    
    // NEW: Check if we need to recreate audio objects
    if (forceRecreateObjects || !validateAudioObjects()) {
        if (!recreateAudioObjects()) {
            SerialQueueManager::getInstance().queueMessage("✗ Failed to recreate audio objects");
            return;
        }
    }
    
    // If currently playing or stopping, queue the request
    if (isCurrentlyPlaying || isStoppingPlayback) {
        // Clear any existing queue and add this new request
        while (!audioQueue.empty()) {
            audioQueue.pop();
        }
        audioQueue.push(file);
        
        // If not already stopping, initiate stop
        if (!isStoppingPlayback) {
            safeStopPlayback();
        }
        return;
    }
    
    // Direct playback if nothing is playing
    safeStartPlayback(file);
}

bool Speaker::safeStopPlayback() {
    if (!isCurrentlyPlaying) return true;
    
    SerialQueueManager::getInstance().queueMessage("Stopping audio playback...");
    
    if (audioMP3 && audioMP3->isRunning()) {
        audioMP3->stop();
        // Give more time for ESP32-S3 to clean up I2S resources
        delay(20);
        SerialQueueManager::getInstance().queueMessage("Audio playback stopped");
    }
    
    // Close any open files
    if (audioFile) {
        audioFile->close();
    }
    
    // Set stopping state and timer
    isStoppingPlayback = true;
    stopRequestTime = millis();
    isCurrentlyPlaying = false;
    currentFilename = "";
    
    return true;
}

bool Speaker::safeStartPlayback(SoundType file) {
    const char* filename = getFilePath(file);
    if (!filename) {
        SerialQueueManager::getInstance().queueMessage("✗ Invalid audio file");
        return false;
    }
    
    SerialQueueManager::getInstance().queueMessage("Starting playback: " + String(filename));
    
    // Validate audio objects before use
    if (!validateAudioObjects()) {
        SerialQueueManager::getInstance().queueMessage("✗ Audio objects invalid, recreating...");
        if (!recreateAudioObjects()) {
            SerialQueueManager::getInstance().queueMessage("✗ Failed to recreate audio objects");
            return false;
        }
    }
    
    // Close any existing file first
    audioFile->close();
    
    // Small delay to ensure file is properly closed
    delay(10);
    
    // Open new file
    if (!audioFile->open(filename)) {
        SerialQueueManager::getInstance().queueMessage("✗ Could not open: " + String(filename));
        // Mark for recreation on next attempt
        forceRecreateObjects = true;
        return false;
    }
    
    // Set volume
    audioOutput->SetGain(currentVolume);
    
    // Begin playback with error handling
    if (!audioMP3->begin(audioID3, audioOutput)) {
        SerialQueueManager::getInstance().queueMessage("✗ MP3 begin failed");
        audioFile->close();
        // Mark for recreation on next attempt
        forceRecreateObjects = true;
        return false;
    }
    
    // Set playing state
    isCurrentlyPlaying = true;
    isStoppingPlayback = false;
    currentFilename = filename;
    
    SerialQueueManager::getInstance().queueMessage("✓ Audio playback started");
    return true;
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

void Speaker::update() {
    if (!initialized) return;
    
    // Handle delayed start after stop
    if (isStoppingPlayback) {
        if (millis() - stopRequestTime >= STOP_DELAY_MS) {
            isStoppingPlayback = false;
            
            // Process any queued audio
            if (!audioQueue.empty()) {
                SoundType nextFile = audioQueue.front();
                audioQueue.pop();
                safeStartPlayback(nextFile);
            }
        }
        return;
    }
    
    // Keep current audio playing
    if (isCurrentlyPlaying) {
        if (audioMP3 && audioMP3->isRunning()) {
            if (!audioMP3->loop()) {
                // Audio finished or error occurred
                SerialQueueManager::getInstance().queueMessage("Audio playback completed");
                isCurrentlyPlaying = false;
                currentFilename = "";
                
                // Process any queued audio after a delay
                if (!audioQueue.empty()) {
                    SoundType nextFile = audioQueue.front();
                    audioQueue.pop();
                    // Add delay before starting next audio
                    stopRequestTime = millis();
                    isStoppingPlayback = true;
                }
            }
        } else {
            // Audio finished unexpectedly
            SerialQueueManager::getInstance().queueMessage("Audio playback ended unexpectedly");
            isCurrentlyPlaying = false;
            currentFilename = "";
            
            // Mark for recreation since something went wrong
            forceRecreateObjects = true;
            
            // Process any queued audio
            if (!audioQueue.empty()) {
                SoundType nextFile = audioQueue.front();
                audioQueue.pop();
                safeStartPlayback(nextFile);
            }
        }
    }
}

void Speaker::stopPlayback() {
    safeStopPlayback();
    clearQueue();
}

void Speaker::clearQueue() {
    while (!audioQueue.empty()) {
        audioQueue.pop();
    }
}

bool Speaker::isPlaying() const {
    return isCurrentlyPlaying && audioMP3 && audioMP3->isRunning();
}
