#include "speaker.h"

// Define the static constexpr LED sequence array
constexpr const Speaker::MelodyNote Speaker::ENTERTAINER_LED_SEQUENCE[];

Speaker::~Speaker() {
    cleanup();
    if (_audio_mutex == nullptr) {
        return;
    }
    vSemaphoreDelete(_audio_mutex);
    _audio_mutex = nullptr;
}

bool Speaker::initialize() {
    if (_initialized) {
        return true;
    }

    SerialQueueManager::get_instance().queue_message("Initializing Speaker...");

    // Create mutex for thread safety
    if (_audio_mutex == nullptr) {
        _audio_mutex = xSemaphoreCreateMutex();
        if (_audio_mutex == nullptr) {
            SerialQueueManager::get_instance().queue_message("✗ Speaker: Failed to create mutex");
            return false;
        }
    }

    if (!initialize_audio()) {
        SerialQueueManager::get_instance().queue_message("✗ Speaker: Audio init failed");
        return false;
    }

    _initialized = true;
    _audio_objects_valid = true;
    SerialQueueManager::get_instance().queue_message("✓ Speaker _initialized");
    return true;
}

bool Speaker::initialize_audio() {
    // Clean up any existing objects first
    cleanup();

    _audio_output = new (std::nothrow) AudioOutputI2S();
    if (_audio_output == nullptr) {
        SerialQueueManager::get_instance().queue_message("✗ Failed to create AudioOutputI2S");
        cleanup();
        return false;
    }

    // Configure I2S pins
    _audio_output->SetPinout(I2S_BCLK, I2S_LRC, I2S_DOUT);
    _audio_output->SetGain(_current_volume);

    _audio_objects_valid = true;
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
    return _audio_objects_valid && _audio_output != nullptr;
}

void Speaker::cleanup() {
    // Take mutex if available (don't block forever)
    bool has_mutex = false;
    if ((_audio_mutex != nullptr) && xSemaphoreTake(_audio_mutex, pdMS_TO_TICKS(200)) == pdTRUE) {
        has_mutex = true;
    }

    if ((_tone_generator != nullptr) && _tone_generator->isRunning()) {
        _tone_generator->stop();
        vTaskDelay(pdMS_TO_TICKS(50));
    }

    if (_tone_source != nullptr) {
        delete _tone_source;
        _tone_source = nullptr;
    }
    if (_tone_generator != nullptr) {
        delete _tone_generator;
        _tone_generator = nullptr;
    }
    _is_playing_tone = false;

    if ((_rtttl_generator != nullptr) && _rtttl_generator->isRunning()) {
        _rtttl_generator->stop();
        vTaskDelay(pdMS_TO_TICKS(50));
    }

    if (_rtttl_source != nullptr) {
        delete _rtttl_source;
        _rtttl_source = nullptr;
    }
    if (_rtttl_generator != nullptr) {
        delete _rtttl_generator;
        _rtttl_generator = nullptr;
    }
    if (_audio_output != nullptr) {
        delete _audio_output;
        _audio_output = nullptr;
    }

    _is_melody_playing = false;
    _audio_objects_valid = false;

    if (has_mutex) {
        xSemaphoreGive(_audio_mutex);
    }
    vTaskDelay(pdMS_TO_TICKS(50));
}

void Speaker::set_muted(bool should_mute) {
    _muted = should_mute;
    SerialQueueManager::get_instance().queue_message("Speaker _muted: " + String(should_mute ? "MUTING" : "UNMUTING"));
    if (should_mute) {
        stop_all_sounds();
    }
}

void Speaker::set_volume(float volume) {
    _current_volume = constrain(volume, 0.0f, 3.9f);
    SerialQueueManager::get_instance().queue_message("Speaker _initialized: " + String(_initialized));
    SerialQueueManager::get_instance().queue_message("Audio output set: " + String(_audio_output != nullptr));
    if (_initialized && (_audio_output != nullptr)) {
        SerialQueueManager::get_instance().queue_message("Setting volume to: " + String(_current_volume));
        _audio_output->SetGain(_current_volume);
    }
}

void Speaker::stop_all_sounds() {
    stop_tone();

    SerialQueueManager::get_instance().queue_message("Stopped playback");

    // Stop RTTTL melody if playing
    if (_is_melody_playing && (_rtttl_generator != nullptr) && _rtttl_generator->isRunning()) {
        SerialQueueManager::get_instance().queue_message("Stopping RTTTL");
        _rtttl_generator->stop();
        _is_melody_playing = false;

        // Stop LED sequence and turn off LEDs
        _is_led_sequence_playing = false;
        rgb_led.turn_all_leds_off();

        SerialQueueManager::get_instance().queue_message("Stopped entertainer melody and LED sync");

        // Clean up RTTTL resources
        if (_rtttl_source != nullptr) {
            delete _rtttl_source;
            _rtttl_source = nullptr;
        }
    }
}

void Speaker::update() {
    if (!_initialized) {
        return;
    }

    if (_is_playing_tone) {
        static uint32_t last_update_debug = 0;
        if (millis() - last_update_debug > 1000) {
            SerialQueueManager::get_instance().queue_message("Speaker::update() calling updateContinuousTone()");
            last_update_debug = millis();
        }

        update_continuous_tone();
        return;
    }

    // Keep melody processing as before but protect shared state
    if (_is_melody_playing) {
        // melody functions will take _audio_mutex internally (see update_melody)
        update_melody();
        return;
    }
}

void Speaker::update_melody() {
    // Acquire mutex around operations that touch audio/LED state
    if ((_audio_mutex == nullptr) || xSemaphoreTake(_audio_mutex, pdMS_TO_TICKS(100)) != pdTRUE) {
        return;
    }

    if (!_is_melody_playing || (_rtttl_generator == nullptr)) {
        xSemaphoreGive(_audio_mutex);
        return;
    }

    // LED sequence update (doesn't touch audio objects, but keep under mutex to keep state consistent)
    if (_is_led_sequence_playing) {
        uint32_t current_time = millis();
        if (current_time >= _led_step_start_time + entertainer_led_sequence[_current_led_step].duration) {
            _current_led_step++;
            if (_current_led_step >= ENTERTAINER_LED_SEQUENCE_LENGTH) {
                _is_led_sequence_playing = false;
                rgb_led.turn_all_leds_off();
            } else {
                const MelodyNote& note = entertainer_led_sequence[_current_led_step];
                rgb_led.set_main_board_leds_to_color(note.led_r, note.led_g, note.led_b);
                _led_step_start_time = current_time;
            }
        }
    }

    // Keep RTTTL generator running safely while mutex held
    if (_rtttl_generator->isRunning()) {
        if (!_rtttl_generator->loop()) {
            _is_melody_playing = false;
            _is_led_sequence_playing = false;
            rgb_led.turn_all_leds_off();
            SerialQueueManager::get_instance().queue_message("Entertainer melody completed");
            if (_rtttl_source != nullptr) {
                delete _rtttl_source;
                _rtttl_source = nullptr;
            }
        }
    }

    xSemaphoreGive(_audio_mutex);
}

void Speaker::start_entertainer_melody() {
    if (_muted || _is_melody_playing) {
        return;
    }

    SerialQueueManager::get_instance().queue_message("Starting The Entertainer melody with LED sync");

    // Take mutex for RTTTL creation and start
    if ((_audio_mutex == nullptr) || xSemaphoreTake(_audio_mutex, pdMS_TO_TICKS(200)) != pdTRUE) {
        SerialQueueManager::get_instance().queue_message("✗ Failed to acquire audio mutex for melody start");
        return;
    }

    static const uint8_t ENTERTAINER_RTTTL[] = "TheEntertainer:d=4,o=5,b=140:8d,8d#,8e,c6,8e,c6,8e,c6,c,8c6,8a,8g,8f#,8a,8c6,8e,8d,8c,8a,2d";

    if (_rtttl_generator == nullptr) {
        _rtttl_generator = new AudioGeneratorRTTTL();
    }
    if (_rtttl_source != nullptr) {
        delete _rtttl_source;
        _rtttl_source = nullptr;
    }
    _rtttl_source = new AudioFileSourcePROGMEM(ENTERTAINER_RTTTL, sizeof(ENTERTAINER_RTTTL));

    if ((_audio_output != nullptr) && _rtttl_generator->begin(_rtttl_source, _audio_output)) {
        _is_melody_playing = true;
        _is_led_sequence_playing = true;
        _current_led_step = 0;
        _led_step_start_time = millis();
        const MelodyNote& first_note = entertainer_led_sequence[0];
        rgb_led.set_main_board_leds_to_color(firstNote.led_r, firstNote.led_g, firstNote.led_b);
        SerialQueueManager::get_instance().queue_message("✓ Entertainer melody and LED sync started");
    } else {
        SerialQueueManager::get_instance().queue_message("✗ Failed to start Entertainer playback");
        if (_rtttl_source != nullptr) {
            delete _rtttl_source;
            _rtttl_source = nullptr;
        }
    }

    xSemaphoreGive(_audio_mutex);
}

void Speaker::play_tone(ToneType tone) {
    if (!_initialized) {
        SerialQueueManager::get_instance().queue_message("ERROR: Speaker not _initialized!");
        return;
    }

    if (_muted) {
        SerialQueueManager::get_instance().queue_message("Speaker _muted");
        return;
    }

    auto tone_index = static_cast<uint8_t>(tone);

    // Validate tone range (1-8, where 8 is TONE_OFF)
    if (tone_index < 1 || tone_index > 8) {
        SerialQueueManager::get_instance().queue_message("Invalid tone: " + String(toneIndex));
        return;
    }

    // If already playing this exact tone, continue (don't restart)
    if (_is_playing_tone && _current_tone == tone) {
        // Update refresh timestamp to prevent auto-stop
        _last_tone_refresh_time = millis();
        SerialQueueManager::get_instance().queue_message("Already playing this tone - continuing");
        return;
    }

    // Update refresh timestamp for new tone requests
    _last_tone_refresh_time = millis();

    // Take mutex for tone creation and start
    if ((_audio_mutex == nullptr) || xSemaphoreTake(_audio_mutex, pdMS_TO_TICKS(200)) != pdTRUE) {
        SerialQueueManager::get_instance().queue_message("Failed to acquire mutex for tone");
        return;
    }

    SerialQueueManager::get_instance().queue_message("Stopping any existing playback");

    // Stop melody if playing
    if (_is_melody_playing && (_rtttl_generator != nullptr) && _rtttl_generator->isRunning()) {
        _rtttl_generator->stop();
        _is_melody_playing = false;
        _is_led_sequence_playing = false;
        rgb_led.turn_all_leds_off();
    }

    // Stop existing tone if any
    if (_is_playing_tone && (_tone_generator != nullptr) && _tone_generator->isRunning()) {
        _tone_generator->stop();
        vTaskDelay(pdMS_TO_TICKS(20));
    }

    // Clean up old tone resources
    if (_tone_source != nullptr) {
        delete _tone_source;
        _tone_source = nullptr;
    }
    if (_tone_generator != nullptr) {
        delete _tone_generator;
        _tone_generator = nullptr;
    }

    // Create new tone generator
    const char* rtttl_string = get_tone_rtttl(tone);
    if (rtttl_string == nullptr) {
        SerialQueueManager::get_instance().queue_message("Failed to get RTTTL for tone");
        xSemaphoreGive(_audio_mutex);
        return;
    }

    _tone_generator = new AudioGeneratorRTTTL();
    _tone_source = new AudioFileSourcePROGMEM(rtttl_string, strlen(rtttl_string) + 1);

    if ((_audio_output != nullptr) && _tone_generator->begin(_tone_source, _audio_output)) {
        _current_tone = tone;
        _is_playing_tone = true;
        SerialQueueManager::get_instance().queue_message("Tone playback started successfully");
    } else {
        SerialQueueManager::get_instance().queue_message("Failed to start tone playback");
        if (_tone_source != nullptr) {
            delete _tone_source;
            _tone_source = nullptr;
        }
        if (_tone_generator != nullptr) {
            delete _tone_generator;
            _tone_generator = nullptr;
        }
    }

    xSemaphoreGive(_audio_mutex);
}

void Speaker::stop_tone() {
    if (!_is_playing_tone) {
        return;
    }

    if ((_audio_mutex == nullptr) || xSemaphoreTake(_audio_mutex, pdMS_TO_TICKS(200)) != pdTRUE) {
        return;
    }

    if ((_tone_generator != nullptr) && _tone_generator->isRunning()) {
        _tone_generator->stop();
        vTaskDelay(pdMS_TO_TICKS(20));
    }

    if (_tone_source != nullptr) {
        delete _tone_source;
        _tone_source = nullptr;
    }
    if (_tone_generator != nullptr) {
        delete _tone_generator;
        _tone_generator = nullptr;
    }

    _is_playing_tone = false;
    SerialQueueManager::get_instance().queue_message("Tone stopped");

    xSemaphoreGive(_audio_mutex);
}

void Speaker::update_continuous_tone() {
    if ((_audio_mutex == nullptr) || xSemaphoreTake(_audio_mutex, pdMS_TO_TICKS(100)) != pdTRUE) {
        return;
    }

    if (!_is_playing_tone || (_tone_generator == nullptr)) {
        xSemaphoreGive(_audio_mutex);
        return;
    }

    if (_tone_generator->isRunning()) {
        if (!_tone_generator->loop()) {
            // Tone finished - restart it QUICKLY to minimize gap

            // OPTIMIZATION: Pre-get the RTTTL string before any deletions
            const char* rtttl_string = get_tone_rtttl(_current_tone);
            if (rtttl_string == nullptr) {
                SerialQueueManager::get_instance().queue_message("Failed to get RTTTL for restart");
                _is_playing_tone = false;
                xSemaphoreGive(_audio_mutex);
                return;
            }

            // OPTIMIZATION: Create new source BEFORE deleting old one
            auto* new_source = new AudioFileSourcePROGMEM(rtttl_string, strlen(rtttl_string) + 1);

            // Now delete the old source

            delete _tone_source;

            _tone_source = new_source;

            // Restart immediately - no delay between delete and begin
            if (!_tone_generator->begin(_tone_source, _audio_output)) {
                SerialQueueManager::get_instance().queue_message("Failed to restart tone");
                _is_playing_tone = false;
                if (_tone_source != nullptr) {
                    delete _tone_source;
                    _tone_source = nullptr;
                }
                if (_tone_generator != nullptr) {
                    delete _tone_generator;
                    _tone_generator = nullptr;
                }
            }
            // If successful, tone continues seamlessly (with minimal 20-50ms gap)
        }
    } else {
        // Tone unexpectedly stopped
        SerialQueueManager::get_instance().queue_message("Tone stopped unexpectedly");
        _is_playing_tone = false;
    }

    xSemaphoreGive(_audio_mutex);
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
