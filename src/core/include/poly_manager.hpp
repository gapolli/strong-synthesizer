#ifndef POLY_MANAGER_HPP
#define POLY_MANAGER_HPP

#include "ym2612_core.hpp"
#include <vector>
#include <cstdint>

class PolyphonyVoiceManager {
public:
    struct Voice {
        int midiNote = -1;
        bool active = false;
        uint32_t lastTriggeredCycle = 0;
        YM2612Core chipInstance;
    };

    PolyphonyVoiceManager(double sampleRate = 48000.0, size_t maxVoices = 4);
    ~PolyphonyVoiceManager();

    void setSampleRate(double sampleRate);
    void setAlgorithm(int algorithm);
    void updateEnvelope(int opIndex, double attack, double decay, double sustain, double release);
    
    void noteOn(int note, int velocity);
    void noteOff(int note);
    
    double renderNextMixedSample();

private:
    double sampleRate_;
    size_t maxVoices_;
    uint32_t masterCycleCounter_;
    
    int globalAlgorithm_;
    struct EnvelopeParams {
        double attack, decay, sustain, release;
    } globalEnvelopes_[4];

    std::vector<Voice> voices_;
    
    int findFreeVoiceIndex();
    int findOldestVoiceIndex();
};

#endif
