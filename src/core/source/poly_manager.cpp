#include "poly_manager.hpp"
#include <cmath>
#include <algorithm>

PolyphonyVoiceManager::PolyphonyVoiceManager(double sampleRate, size_t maxVoices)
    : sampleRate_(sampleRate)
    , maxVoices_(maxVoices)
    , masterCycleCounter_(0)
    , globalAlgorithm_(5) {

    for (size_t i = 0; i < maxVoices_; ++i) {
        voices_.emplace_back(Voice{-1, false, 0, YM2612Core(sampleRate_)});
        voices_[i].chipInstance.setAlgorithm(globalAlgorithm_);
    }
}

PolyphonyVoiceManager::~PolyphonyVoiceManager() {}

void PolyphonyVoiceManager::setSampleRate(double sampleRate) {
    sampleRate_ = sampleRate;
    for (auto& v : voices_) {
        v.chipInstance.setSampleRate(sampleRate_);
    }
}

void PolyphonyVoiceManager::setAlgorithm(int algorithm) {
    globalAlgorithm_ = algorithm;
    for (auto& v : voices_) {
        v.chipInstance.setAlgorithm(globalAlgorithm_);
    }
}

void PolyphonyVoiceManager::updateEnvelope(int opIndex, double attack, double decay, double sustain, double release) {
    if (opIndex < 0 || opIndex >= 4) return;
    for (auto& v : voices_) {
        v.chipInstance.setOperatorEnvelope(opIndex, attack, decay, sustain, release);
    }
}

int PolyphonyVoiceManager::findFreeVoiceIndex() {
    for (size_t i = 0; i < voices_.size(); ++i) {
        if (!voices_[i].active) return static_cast<int>(i);
    }
    return -1;
}

int PolyphonyVoiceManager::findOldestVoiceIndex() {
    uint32_t oldestCycle = 0xFFFFFFFF;
    int oldestIdx = 0;
    for (size_t i = 0; i < voices_.size(); ++i) {
        if (voices_[i].lastTriggeredCycle < oldestCycle) {
            oldestCycle = voices_[i].lastTriggeredCycle;
            oldestIdx = static_cast<int>(i);
        }
    }
    return oldestIdx;
}

void PolyphonyVoiceManager::noteOn(int note, int velocity) {
    masterCycleCounter_++;
    int targetIdx = findFreeVoiceIndex();
    
    if (targetIdx == -1) {
        targetIdx = findOldestVoiceIndex();
        voices_[targetIdx].chipInstance.noteOff();
    }

    auto& v = voices_[targetIdx];
    v.midiNote = note;
    v.active = true;
    v.lastTriggeredCycle = masterCycleCounter_;
    
    double frequency = 440.0 * std::pow(2.0, (note - 69) / 12.0);
    v.chipInstance.setFrequency(frequency);
    v.chipInstance.noteOn();
}

void PolyphonyVoiceManager::noteOff(int note) {
    for (auto& v : voices_) {
        if (v.active && v.midiNote == note) {
            v.active = false;
            v.chipInstance.noteOff();
        }
    }
}

double PolyphonyVoiceManager::renderNextMixedSample() {
    double mixedOutput = 0.0;
    for (auto& v : voices_) {
        mixedOutput += v.chipInstance.generateNextSample();
    }
    return mixedOutput / static_cast<double>(maxVoices_);
}
