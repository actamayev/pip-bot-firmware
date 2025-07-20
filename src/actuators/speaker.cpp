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
    // Create audio objects
    audioFile = new AudioFileSourceSPIFFS();
    audioID3 = new AudioFileSourceID3(audioFile);
    audioMP3 = new AudioGeneratorMP3();
    audioOutput = new AudioOutputI2S();
    
    if (!audioFile || !audioID3 || !audioMP3 || !audioOutput) {
        cleanup();
        return false;
    }
    
    // Configure I2S pins
    audioOutput->SetPinout(I2S_BCLK, I2S_LRC, I2S_DOUT);
    audioOutput->SetGain(currentVolume);
    
    return true;
}

void Speaker::cleanup() {
    // Stop any ongoing playback first
    if (audioMP3 && audioMP3->isRunning()) {
        audioMP3->stop();
    }
    
    // Small delay to ensure cleanup
    delay(10);
    
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
}

const char* Speaker::getFilePath(AudioFile audioFile) const {
    switch (audioFile) {
        case AudioFile::CHIME:   return "/chime.mp3";
        case AudioFile::CHIRP:   return "/chirp.mp3";
        case AudioFile::POP:     return "/pop.mp3";
        case AudioFile::DROP:    return "/drop.mp3";
        case AudioFile::FART:    return "/fart.mp3";
        default:                 return nullptr;
    }
}

void Speaker::playFile(AudioFile file) {
    if (!initialized || muted) {
        SerialQueueManager::getInstance().queueMessage("Speaker not ready or muted");
        return;
    }

    // Log the volume
    SerialQueueManager::getInstance().queueMessage("Speaker volume: " + String(currentVolume));
    
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
    
    if (audioMP3 && audioMP3->isRunning()) {
        audioMP3->stop();
        SerialQueueManager::getInstance().queueMessage("Audio playback stopped");
    }
    
    // Set stopping state and timer
    isStoppingPlayback = true;
    stopRequestTime = millis();
    isCurrentlyPlaying = false;
    currentFilename = "";
    
    return true;
}

bool Speaker::safeStartPlayback(AudioFile file) {
    const char* filename = getFilePath(file);
    if (!filename) {
        SerialQueueManager::getInstance().queueMessage("✗ Invalid audio file");
        return false;
    }
    
    SerialQueueManager::getInstance().queueMessage("Starting playback: " + String(filename));
    
    // Ensure audio objects are ready
    if (!audioFile || !audioID3 || !audioMP3 || !audioOutput) {
        SerialQueueManager::getInstance().queueMessage("✗ Audio objects not ready");
        return false;
    }
    
    // Close any existing file first
    audioFile->close();
    
    // Open new file
    if (!audioFile->open(filename)) {
        SerialQueueManager::getInstance().queueMessage("✗ Could not open: " + String(filename));
        return false;
    }
    
    // Set volume
    audioOutput->SetGain(currentVolume);
    
    // Begin playback
    if (!audioMP3->begin(audioID3, audioOutput)) {
        SerialQueueManager::getInstance().queueMessage("✗ MP3 begin failed");
        audioFile->close();
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
                AudioFile nextFile = audioQueue.front();
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
                isCurrentlyPlaying = false;
                currentFilename = "";
                
                // Process any queued audio
                if (!audioQueue.empty()) {
                    AudioFile nextFile = audioQueue.front();
                    audioQueue.pop();
                    // Add small delay before starting next audio
                    stopRequestTime = millis();
                    isStoppingPlayback = true;
                }
            }
        } else {
            // Audio finished
            isCurrentlyPlaying = false;
            currentFilename = "";
            
            // Process any queued audio
            if (!audioQueue.empty()) {
                AudioFile nextFile = audioQueue.front();
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
