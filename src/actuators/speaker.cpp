#include "speaker.h"

// Define the static constexpr LED sequence array
constexpr const Speaker::MelodyNote Speaker::entertainer_led_sequence[];

Speaker::~Speaker() {
    cleanup();
    if (!audio_mutex) return;
    vSemaphoreDelete(audio_mutex);
    audio_mutex = nullptr;
}

bool Speaker::initialize() {
    if (initialized) return true;

    SerialQueueManager::get_instance().queue_message("Initializing Speaker...");

    // Create mutex for thread safety
    if (!audio_mutex) {
        audio_mutex = xSemaphoreCreateMutex();
        if (!audio_mutex) {
            SerialQueueManager::get_instance().queue_message("✗ Speaker: Failed to create mutex");
            return false;
        }
    }

    if (!initialize_audio()) {
        SerialQueueManager::get_instance().queue_message("✗ Speaker: Audio init failed");
        return false;
    }

    initialized = true;
    audio_objects_valid = true;
    SerialQueueManager::get_instance().queue_message("✓ Speaker initialized");
    return true;
}

bool Speaker::initialize_audio() {
    // Clean up any existing objects first
    cleanup();

    audio_output = new (std::nothrow) AudioOutputI2S();
    if (!audio_output) {
        SerialQueueManager::get_instance().queue_message("✗ Failed to create AudioOutputI2S");
        cleanup();
        return false;
    }

    // Configure I2S pins
    audio_output->SetPinout(I2S_BCLK, I2S_LRC, I2S_DOUT);
    audio_output->SetGain(current_volume);

    audio_objects_valid = true;
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
    return audio_objects_valid && audio_output != nullptr;
}

void Speaker::cleanup() {
    // Take mutex if available (don't block forever)
    bool hasMutex = false;
    if (audio_mutex && xSemaphoreTake(audio_mutex, pdMS_TO_TICKS(200)) == pdTRUE) {
        hasMutex = true;
    }

    if (tone_generator && tone_generator->isRunning()) {
        tone_generator->stop();
        vTaskDelay(pdMS_TO_TICKS(50));
    }

    if (tone_source) {
        delete tone_source;
        tone_source = nullptr;
    }
    if (tone_generator) {
        delete tone_generator;
        tone_generator = nullptr;
    }
    is_playing_tone = false;

    if (rtttl_generator && rtttl_generator->isRunning()) {
        rtttl_generator->stop();
        vTaskDelay(pdMS_TO_TICKS(50));
    }

    if (rtttl_source) {
        delete rtttl_source;
        rtttl_source = nullptr;
    }
    if (rtttl_generator) {
        delete rtttl_generator;
        rtttl_generator = nullptr;
    }
    if (audio_output) {
        delete audio_output;
        audio_output = nullptr;
    }

    is_melody_playing = false;
    audio_objects_valid = false;

    if (hasMutex) {
        xSemaphoreGive(audio_mutex);
    }
    vTaskDelay(pdMS_TO_TICKS(50));
}

void Speaker::set_muted(bool should_mute) {
    muted = shouldMute;
    SerialQueueManager::get_instance().queue_message("Speaker muted: " + String(shouldMute ? "MUTING" : "UNMUTING"));
    if (shouldMute) {
        stop_all_sounds();
    }
}

void Speaker::set_volume(float volume) {
    current_volume = constrain(volume, 0.0f, 3.9f);
    SerialQueueManager::get_instance().queue_message("Speaker initialized: " + String(initialized));
    SerialQueueManager::get_instance().queue_message("Audio output set: " + String(audio_output != nullptr));
    if (initialized && audio_output) {
        SerialQueueManager::get_instance().queue_message("Setting volume to: " + String(current_volume));
        audio_output->SetGain(current_volume);
    }
}

void Speaker::stop_all_sounds() {
    stop_tone();

    SerialQueueManager::get_instance().queue_message("Stopped playback");

    // Stop RTTTL melody if playing
    if (is_melody_playing && rtttl_generator && rtttl_generator->isRunning()) {
        SerialQueueManager::get_instance().queue_message("Stopping RTTTL");
        rtttl_generator->stop();
        is_melody_playing = false;

        // Stop LED sequence and turn off LEDs
        is_led_sequence_playing = false;
        rgbLed.turn_all_leds_off();

        SerialQueueManager::get_instance().queue_message("Stopped entertainer melody and LED sync");

        // Clean up RTTTL resources
        if (rtttl_source) {
            delete rtttl_source;
            rtttl_source = nullptr;
        }
    }
}

void Speaker::update() {
    if (!initialized) return;

    if (is_playing_tone) {
        static unsigned long lastUpdateDebug = 0;
        if (millis() - lastUpdateDebug > 1000) {
            SerialQueueManager::get_instance().queue_message("Speaker::update() calling updateContinuousTone()");
            lastUpdateDebug = millis();
        }

        update_continuous_tone();
        return;
    }

    // Keep melody processing as before but protect shared state
    if (is_melody_playing) {
        // melody functions will take audio_mutex internally (see update_melody)
        update_melody();
        return;
    }
}

void Speaker::update_melody() {
    // Acquire mutex around operations that touch audio/LED state
    if (!audio_mutex || xSemaphoreTake(audio_mutex, pdMS_TO_TICKS(100)) != pdTRUE) {
        return;
    }

    if (!is_melody_playing || !rtttl_generator) {
        xSemaphoreGive(audio_mutex);
        return;
    }

    // LED sequence update (doesn't touch audio objects, but keep under mutex to keep state consistent)
    if (is_led_sequence_playing) {
        unsigned long currentTime = millis();
        if (currentTime >= led_step_start_time + entertainer_led_sequence[current_led_step].duration) {
            current_led_step++;
            if (current_led_step >= ENTERTAINER_LED_SEQUENCE_LENGTH) {
                is_led_sequence_playing = false;
                rgbLed.turn_all_leds_off();
            } else {
                const MelodyNote& note = entertainer_led_sequence[current_led_step];
                rgbLed.set_main_board_leds_to_color(note.led_r, note.led_g, note.led_b);
                led_step_start_time = currentTime;
            }
        }
    }

    // Keep RTTTL generator running safely while mutex held
    if (rtttl_generator->isRunning()) {
        if (!rtttl_generator->loop()) {
            is_melody_playing = false;
            is_led_sequence_playing = false;
            rgbLed.turn_all_leds_off();
            SerialQueueManager::get_instance().queue_message("Entertainer melody completed");
            if (rtttl_source) {
                delete rtttl_source;
                rtttl_source = nullptr;
            }
        }
    }

    xSemaphoreGive(audio_mutex);
}

void Speaker::start_entertainer_melody() {
    if (muted || is_melody_playing) return;

    SerialQueueManager::get_instance().queue_message("Starting The Entertainer melody with LED sync");

    // Take mutex for RTTTL creation and start
    if (!audio_mutex || xSemaphoreTake(audio_mutex, pdMS_TO_TICKS(200)) != pdTRUE) {
        SerialQueueManager::get_instance().queue_message("✗ Failed to acquire audio mutex for melody start");
        return;
    }

    static const uint8_t entertainerRTTTL[] = "TheEntertainer:d=4,o=5,b=140:8d,8d#,8e,c6,8e,c6,8e,c6,c,8c6,8a,8g,8f#,8a,8c6,8e,8d,8c,8a,2d";

    if (!rtttl_generator) {
        rtttl_generator = new AudioGeneratorRTTTL();
    }
    if (rtttl_source) {
        delete rtttl_source;
        rtttl_source = nullptr;
    }
    rtttl_source = new AudioFileSourcePROGMEM(entertainerRTTTL, sizeof(entertainerRTTTL));

    if (audio_output && rtttl_generator->begin(rtttl_source, audio_output)) {
        is_melody_playing = true;
        is_led_sequence_playing = true;
        current_led_step = 0;
        led_step_start_time = millis();
        const MelodyNote& firstNote = entertainer_led_sequence[0];
        rgbLed.set_main_board_leds_to_color(firstNote.led_r, firstNote.led_g, firstNote.led_b);
        SerialQueueManager::get_instance().queue_message("✓ Entertainer melody and LED sync started");
    } else {
        SerialQueueManager::get_instance().queue_message("✗ Failed to start Entertainer playback");
        if (rtttl_source) {
            delete rtttl_source;
            rtttl_source = nullptr;
        }
    }

    xSemaphoreGive(audio_mutex);
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
    if (is_playing_tone && current_tone == tone) {
        // Update refresh timestamp to prevent auto-stop
        last_tone_refresh_time = millis();
        SerialQueueManager::get_instance().queue_message("Already playing this tone - continuing");
        return;
    }

    // Update refresh timestamp for new tone requests
    last_tone_refresh_time = millis();

    // Take mutex for tone creation and start
    if (!audio_mutex || xSemaphoreTake(audio_mutex, pdMS_TO_TICKS(200)) != pdTRUE) {
        SerialQueueManager::get_instance().queue_message("Failed to acquire mutex for tone");
        return;
    }

    SerialQueueManager::get_instance().queue_message("Stopping any existing playback");

    // Stop melody if playing
    if (is_melody_playing && rtttl_generator && rtttl_generator->isRunning()) {
        rtttl_generator->stop();
        is_melody_playing = false;
        is_led_sequence_playing = false;
        rgbLed.turn_all_leds_off();
    }

    // Stop existing tone if any
    if (is_playing_tone && tone_generator && tone_generator->isRunning()) {
        tone_generator->stop();
        vTaskDelay(pdMS_TO_TICKS(20));
    }

    // Clean up old tone resources
    if (tone_source) {
        delete tone_source;
        tone_source = nullptr;
    }
    if (tone_generator) {
        delete tone_generator;
        tone_generator = nullptr;
    }

    // Create new tone generator
    const char* rtttlString = get_tone_rtttl(tone);
    if (!rtttlString) {
        SerialQueueManager::get_instance().queue_message("Failed to get RTTTL for tone");
        xSemaphoreGive(audio_mutex);
        return;
    }

    tone_generator = new AudioGeneratorRTTTL();
    tone_source = new AudioFileSourcePROGMEM(rtttlString, strlen(rtttlString) + 1);

    if (audio_output && tone_generator->begin(tone_source, audio_output)) {
        current_tone = tone;
        is_playing_tone = true;
        SerialQueueManager::get_instance().queue_message("Tone playback started successfully");
    } else {
        SerialQueueManager::get_instance().queue_message("Failed to start tone playback");
        if (tone_source) {
            delete tone_source;
            tone_source = nullptr;
        }
        if (tone_generator) {
            delete tone_generator;
            tone_generator = nullptr;
        }
    }

    xSemaphoreGive(audio_mutex);
}

void Speaker::stop_tone() {
    if (!is_playing_tone) return;

    if (!audio_mutex || xSemaphoreTake(audio_mutex, pdMS_TO_TICKS(200)) != pdTRUE) {
        return;
    }

    if (tone_generator && tone_generator->isRunning()) {
        tone_generator->stop();
        vTaskDelay(pdMS_TO_TICKS(20));
    }

    if (tone_source) {
        delete tone_source;
        tone_source = nullptr;
    }
    if (tone_generator) {
        delete tone_generator;
        tone_generator = nullptr;
    }

    is_playing_tone = false;
    SerialQueueManager::get_instance().queue_message("Tone stopped");

    xSemaphoreGive(audio_mutex);
}

void Speaker::update_continuous_tone() {
    if (!audio_mutex || xSemaphoreTake(audio_mutex, pdMS_TO_TICKS(100)) != pdTRUE) {
        return;
    }

    if (!is_playing_tone || !tone_generator) {
        xSemaphoreGive(audio_mutex);
        return;
    }

    if (tone_generator->isRunning()) {
        if (!tone_generator->loop()) {
            // Tone finished - restart it QUICKLY to minimize gap

            // OPTIMIZATION: Pre-get the RTTTL string before any deletions
            const char* rtttlString = get_tone_rtttl(current_tone);
            if (!rtttlString) {
                SerialQueueManager::get_instance().queue_message("Failed to get RTTTL for restart");
                is_playing_tone = false;
                xSemaphoreGive(audio_mutex);
                return;
            }

            // OPTIMIZATION: Create new source BEFORE deleting old one
            AudioFileSourcePROGMEM* newSource = new AudioFileSourcePROGMEM(rtttlString, strlen(rtttlString) + 1);

            // Now delete the old source
            if (tone_source) {
                delete tone_source;
            }
            tone_source = newSource;

            // Restart immediately - no delay between delete and begin
            if (!tone_generator->begin(tone_source, audio_output)) {
                SerialQueueManager::get_instance().queue_message("Failed to restart tone");
                is_playing_tone = false;
                if (tone_source) {
                    delete tone_source;
                    tone_source = nullptr;
                }
                if (tone_generator) {
                    delete tone_generator;
                    tone_generator = nullptr;
                }
            }
            // If successful, tone continues seamlessly (with minimal 20-50ms gap)
        }
    } else {
        // Tone unexpectedly stopped
        SerialQueueManager::get_instance().queue_message("Tone stopped unexpectedly");
        is_playing_tone = false;
    }

    xSemaphoreGive(audio_mutex);
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
