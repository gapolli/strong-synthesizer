#include "../core/include/engine.hpp"
#include "../core/include/audio_output.hpp"

#define MINIAUDIO_IMPLEMENTATION
#include "miniaudio.h"

extern "C" {

#ifdef _WIN32
    #define SYNTH_API __declspec(dllexport)
#else
    #define SYNTH_API __attribute__((visibility("default")))
#endif

SYNTH_API Engine* synth_create_orchestrator(double sample_rate) {
    return new Engine(sample_rate);
}

SYNTH_API void synth_destroy_orchestrator(Engine* engine) {
    delete engine;
}

SYNTH_API void synth_orchestrator_set_mode(Engine* engine, int mode) {
    engine->setEngineMode(static_cast<Engine::EngineMode>(mode));
}

SYNTH_API void synth_orchestrator_note_on(Engine* engine, int note, int velocity) {
    engine->triggerNoteOn(note, velocity);
}

SYNTH_API void synth_orchestrator_note_off(Engine* engine, int note) {
    engine->triggerNoteOff(note);
}

SYNTH_API void synth_orchestrator_control_change(Engine* engine, int control_id, int value) {
    engine->updateControlChange(control_id, value);
}

SYNTH_API void synth_orchestrator_configure_filter(Engine* engine, int mode, double cutoff, double resonance) {
    engine->configureFilter(mode, cutoff, resonance);
}

SYNTH_API int synth_orchestrator_start_recording(Engine* engine) {
    return engine->startRecording() ? 1 : 0;
}

SYNTH_API void synth_orchestrator_stop_recording(Engine* engine) {
    engine->stopRecording();
}

SYNTH_API size_t synth_orchestrator_get_recording_count(Engine* engine) {
    return engine->getRecordedSamplesCount();
}

SYNTH_API void synth_orchestrator_get_recording_data(Engine* engine, float* dest_buffer) {
    engine->copyRecordedSamples(dest_buffer);
}

void audio_orchestrator_callback(ma_device* pDevice, void* pOutput, const void* pInput, unsigned int frameCount) {
    Engine* pEngine = static_cast<Engine*>(pDevice->pUserData);
    float* pOutF32 = static_cast<float*>(pOutput);

    for (unsigned int i = 0; i < frameCount; ++i) {
        pOutF32[i] = static_cast<float>(pEngine->renderNextSample());
    }
    (void)pInput;
}

SYNTH_API void* synth_orchestrator_start_audio(Engine* engine, double sample_rate) {
    ma_device* pDevice = new ma_device();
    ma_device_config config = ma_device_config_init(ma_device_type_playback);
    
    config.playback.format = ma_format_f32;
    config.playback.channels = 1;
    config.sampleRate = static_cast<unsigned int>(sample_rate);
    config.dataCallback = audio_orchestrator_callback;
    config.pUserData = engine;

    if (ma_device_init(nullptr, &config, pDevice) != MA_SUCCESS) {
        delete pDevice;
        return nullptr;
    }

    if (ma_device_start(pDevice) != MA_SUCCESS) {
        ma_device_uninit(pDevice);
        delete pDevice;
        return nullptr;
    }

    return static_cast<void*>(pDevice);
}

SYNTH_API void synth_orchestrator_stop_audio(void* device) {
    if (device) {
        ma_device* pDevice = static_cast<ma_device*>(device);
        ma_device_stop(pDevice);
        ma_device_uninit(pDevice);
        delete pDevice;
    }
}

SYNTH_API void synth_orchestrator_configure_granular(Engine* engine, double position, double duration_ms, int density) {
    engine->configureGranular(position, duration_ms, density);
}

SYNTH_API void synth_orchestrator_configure_lfo(Engine* engine, int waveform_idx, double frequency_hz, double depth_percent) {
    engine->configureLfo(waveform_idx, frequency_hz, depth_percent);
}

SYNTH_API void synth_orchestrator_get_scope_data(Engine* engine, float* dest_buffer, size_t count) {
    engine->copyScopeData(dest_buffer, count);
}

SYNTH_API void synth_orchestrator_update_fm_envelope(Engine* engine, int op_index, double attack, double decay, double sustain, double release) {
    engine->updateFmEnvelope(op_index, attack, decay, sustain, release);
}

SYNTH_API void synth_orchestrator_set_filter_drive(Engine* engine, double drive_amount) {
    engine->updateControlChange(13, static_cast<int>(drive_amount * 127.0));
}

SYNTH_API void synth_orchestrator_load_granular_sample(Engine* engine, const float* data_buffer, size_t sample_count) {
    if (engine) {
        engine->loadGranularSample(data_buffer, sample_count);
    }
}

SYNTH_API void synth_orchestrator_get_active_voices(Engine* engine, int* dest_buffer) {
    if (!engine || !dest_buffer) return;
    for (int i = 0; i < 4; ++i) {
        dest_buffer[i] = 0; // Stub initialization pass
    }
    dest_buffer[0] = 1; // Keep voice 1 active as a basic heartbeat layout indicator
}

}
