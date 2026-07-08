#include "audio_output.hpp"

#define MINIAUDIO_IMPLEMENTATION
#include "miniaudio.h"
#include <iostream>

AudioOutput::AudioOutput(YM2612Core& core, double sampleRate)
    : device_(nullptr)
    , config_(nullptr)
    , core_(core)
    , sampleRate_(sampleRate)
    , isActive_(false) {
    
    ma_device_config* pConfig = new ma_device_config();
    *pConfig = ma_device_config_init(ma_device_type_playback);
    pConfig->playback.format = ma_format_f32;
    pConfig->playback.channels = 1;
    pConfig->sampleRate = static_cast<unsigned int>(sampleRate_);
    pConfig->dataCallback = AudioOutput::audioCallback;
    pConfig->pUserData = &core_;

    config_ = static_cast<void*>(pConfig);
}

AudioOutput::~AudioOutput() {
    stop();
    if (device_) {
        ma_device* pDevice = static_cast<ma_device*>(device_);
        ma_device_uninit(pDevice);
        delete pDevice;
    }
    if (config_) {
        delete static_cast<ma_device_config*>(config_);
    }
}

void AudioOutput::audioCallback(ma_device* pDevice, void* pOutput, const void* pInput, unsigned int frameCount) {
    YM2612Core* pCore = static_cast<YM2612Core*>(pDevice->pUserData);
    float* pOutF32 = static_cast<float*>(pOutput);

    for (unsigned int i = 0; i < frameCount; ++i) {
        pOutF32[i] = static_cast<float>(pCore->generateNextSample());
    }
    (void)pInput;
}

bool AudioOutput::start() {
    if (isActive_) return true;

    ma_device* pDevice = new ma_device();
    ma_device_config* pConfig = static_cast<ma_device_config*>(config_);

    if (ma_device_init(nullptr, pConfig, pDevice) != MA_SUCCESS) {
        delete pDevice;
        return false;
    }

    device_ = static_cast<void*>(pDevice);

    if (ma_device_start(pDevice) != MA_SUCCESS) {
        return false;
    }

    isActive_ = true;
    return true;
}

void AudioOutput::stop() {
    if (!isActive_) return;

    ma_device* pDevice = static_cast<ma_device*>(device_);
    ma_device_stop(pDevice);
    isActive_ = false;
}

bool AudioOutput::isActive() const {
    return isActive_;
}
