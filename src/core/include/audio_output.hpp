#ifndef AUDIO_OUTPUT_HPP
#define AUDIO_OUTPUT_HPP

#include "ym2612_core.hpp"

struct ma_device;

class AudioOutput {
public:
    AudioOutput(YM2612Core& core, double sampleRate = 48000.0);
    ~AudioOutput();

    bool start();
    void stop();
    bool isActive() const;

private:
    void* device_;
    void* config_;
    YM2612Core& core_;
    double sampleRate_;
    bool isActive_;

    static void audioCallback(ma_device* pDevice, void* pOutput, const void* pInput, unsigned int frameCount);
};

#endif
