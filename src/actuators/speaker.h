#pragma once
#include "Arduino.h"
#include "AudioFileSourcePROGMEM.h"
#include "AudioGeneratorRTTTL.h"
#include "AudioOutputI2S.h"
#include "actuators/led/rgb_led.h"
#include "custom_interpreter/bytecode_structs.h"
#include "networking/protocol.h"
#include "networking/serial_queue_manager.h"
#include "utils/config.h"
#include "utils/singleton.h"

class Speaker : public Singleton<Speaker> {
    friend class Singleton<Speaker>;
    friend class TaskManager;

  public:
    void set_muted(bool should_mute);
    void set_volume(float volume); // 0.0 to 4.0
    void stop_all_sounds();
    void start_entertainer_melody();
    void play_tone(ToneType tone);
    void stop_tone();

  private:
    Speaker() = default;
    ~Speaker();

    bool initialize();

    void update(); // Call this periodically to keep audio playing
    bool muted = false;
    bool initialized = false;
    float current_volume = 3.9f;

    // Audio objects
    AudioOutputI2S* audio_output = nullptr;

    // RTTTL objects for melody playback
    AudioGeneratorRTTTL* rtttl_generator = nullptr;
    AudioFileSourcePROGMEM* rtttl_source = nullptr;

    // Enhanced state management
    bool audio_objects_valid = false;

    // Thread safety
    SemaphoreHandle_t audio_mutex = nullptr;

    bool initialize_audio();
    void cleanup();
    bool recreate_audio_objects();
    bool validate_audio_objects();

    // RTTTL melody methods
    void update_melody();

    // Melody playback state
    bool is_melody_playing = false;

    // LED synchronization for entertainer melody
    struct MelodyNote {
        unsigned long duration;      // in milliseconds
        uint8_t led_r, led_g, led_b; // LED color for this note
    };

    static constexpr MelodyNote entertainer_led_sequence[] = {
        {214, 255, 0, 0},     // 8d - red
        {214, 255, 127, 0},   // 8d# - orange
        {214, 255, 255, 0},   // 8e - yellow
        {428, 0, 255, 0},     // c6 - green
        {214, 0, 255, 127},   // 8e - cyan
        {428, 0, 255, 0},     // c6 - green
        {214, 0, 255, 127},   // 8e - cyan
        {428, 0, 255, 0},     // c6 - green
        {428, 0, 0, 255},     // c - blue
        {214, 127, 0, 255},   // 8c6 - purple
        {214, 255, 0, 255},   // 8a - magenta
        {214, 255, 127, 127}, // 8g - pink
        {214, 255, 255, 127}, // 8f# - light yellow
        {214, 255, 0, 255},   // 8a - magenta
        {214, 127, 0, 255},   // 8c6 - purple
        {214, 0, 255, 127},   // 8e - cyan
        {214, 255, 255, 0},   // 8d - yellow
        {214, 0, 255, 0},     // 8c - green
        {214, 255, 0, 255},   // 8a - magenta
        {857, 255, 0, 0}      // 2d - red (half note)
    };

    static constexpr size_t ENTERTAINER_LED_SEQUENCE_LENGTH = sizeof(entertainer_led_sequence) / sizeof(entertainer_led_sequence[0]);

    // LED sync state
    bool is_led_sequence_playing = false;
    int current_led_step = 0;
    unsigned long led_step_start_time = 0;

    const uint8_t I2S_DOUT = 13;
    const uint8_t I2S_BCLK = 14;
    const uint8_t I2S_LRC = 21;

    bool is_playing_tone = false;
    ToneType current_tone = ToneType::TONE_A;

    // RTTTL objects for tone playback (separate from melody)
    AudioGeneratorRTTTL* tone_generator = nullptr;
    AudioFileSourcePROGMEM* tone_source = nullptr;

    void update_continuous_tone();
    const char* get_tone_rtttl(ToneType tone);

    unsigned long last_tone_refresh_time = 0;
    static const unsigned long TONE_AUTO_STOP_MS = 200; // Stop if not refreshed within 200ms
};
