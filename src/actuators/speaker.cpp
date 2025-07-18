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
    if (audioMP3) {
        audioMP3->stop();
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
}

const char* Speaker::getFilePath(AudioFile audioFile) const {
    switch (audioFile) {
        case AudioFile::BREEZE:  return "/breeze.mp3";
        case AudioFile::CHIME:   return "/chime.mp3";
        case AudioFile::CHIRP:   return "/chirp.mp3";
        case AudioFile::POP:     return "/pop.mp3";
        case AudioFile::SPLASH:  return "/splash.mp3";
        default:                 return nullptr;
    }
}

void Speaker::playFile(AudioFile file) {
    if (!initialized || muted) {
        SerialQueueManager::getInstance().queueMessage("Speaker not ready or muted");
        return;
    }
    
    const char* filename = getFilePath(file);  // Use 'file' instead of 'audioFile'
    if (!filename) {
        SerialQueueManager::getInstance().queueMessage("✗ Invalid audio file");
        return;
    }
    
    // Stop any current playback
    stopPlayback();
    
    SerialQueueManager::getInstance().queueMessage("Starting playback: " + String(filename));
    
    // Now audioFile-> refers to the member variable (AudioFileSourceSPIFFS*)
    if (!audioFile->open(filename)) {
        SerialQueueManager::getInstance().queueMessage("✗ Could not open: " + String(filename));
        return;
    }
    
    // Set volume
    audioOutput->SetGain(currentVolume);
    
    // Begin playback
    if (!audioMP3->begin(audioID3, audioOutput)) {
        SerialQueueManager::getInstance().queueMessage("✗ MP3 begin failed");
        audioFile->close(); // Clean up the opened file
        return;
    }
    
    // Set playing state
    isCurrentlyPlaying = true;
    currentFilename = filename;
    
    SerialQueueManager::getInstance().queueMessage("✓ Audio playback started");
    
    // Return immediately - playback continues in background via update() calls
}

void Speaker::setMuted(bool shouldMute) {
    muted = shouldMute;
    if (initialized && audioMP3 && audioMP3->isRunning() && shouldMute) {
        audioMP3->stop(); // Stop current playback if muting
    }
}

void Speaker::setVolume(float volume) {
    currentVolume = constrain(volume, 0.0f, 4.0f);
    if (initialized && audioOutput) {
        audioOutput->SetGain(currentVolume);
    }
}

void Speaker::update() {
    if (!initialized || !isCurrentlyPlaying) return;
    
    // Keep the audio playing
    if (audioMP3->isRunning()) {
        if (!audioMP3->loop()) {
            // Audio finished or error occurred
            stopPlayback();
        }
    } else {
        // Audio finished
        stopPlayback();
    }
}

void Speaker::stopPlayback() {
    if (audioMP3 && audioMP3->isRunning()) {
        audioMP3->stop();
        SerialQueueManager::getInstance().queueMessage("Audio playback stopped");
    }
    isCurrentlyPlaying = false;
    currentFilename = "";
}

bool Speaker::isPlaying() const {
    return isCurrentlyPlaying && audioMP3 && audioMP3->isRunning();
}
