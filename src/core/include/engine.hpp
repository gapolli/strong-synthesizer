#ifndef ENGINE_HPP
#define ENGINE_HPP

#include "poly_manager.hpp"
#include "granular_engine.hpp"
#include "filter.hpp"
#include <vector>
#include <string>
#include <mutex>
#include <cstddef>

class Engine {
public:
    enum class EngineMode {
        ANALOG_VA,
        FM_CHIP,
        GRANULAR
    };

    enum class LFOWaveform {
        SINE = 0,
        TRIANGLE = 1,
        SAW = 2,
        SQUARE = 3
    };

    Engine(double sampleRate = 48000.0);
    ~Engine();

    void setEngineMode(EngineMode mode);
    void triggerNoteOn(int note, int velocity);
    void triggerNoteOff(int note);
    void updateControlChange(int controlId, int value);
    void configureFilter(int mode, double cutoff, double resonance);
    void configureGranular(double position, double durationMs, int density);
    void configureLfo(int waveformIdx, double frequencyHz, double depthPercent);
    void updateFmEnvelope(int opIndex, double attack, double decay, double sustain, double release);
    
    void loadGranularSample(const float* dataBuffer, size_t sampleCount);
    
    double renderNextSample();
    void setSampleRate(double sampleRate);
    
    bool startRecording();
    void stopRecording();
    size_t getRecordedSamplesCount() const;
    void copyRecordedSamples(float* destBuffer);
    
    void copyScopeData(float* destBuffer, size_t count);

private:
    struct AnalogVoiceSlot {
        int midiNote = -1;
        bool active = false;
        
        // Multi-oscillator cluster for 3-voice unison detuning
        double phases[3] = {0.0, 0.0, 0.0};
        double baseIncrement = 0.0;
    };

    double sampleRate_;
    EngineMode currentMode_;
    
    PolyphonyVoiceManager polyFmManager_;
    GranularEngine granularEngine_;
    StateVariableFilter filter_;
    LFOWaveform lfoWaveform_ = LFOWaveform::SINE;
    
    std::vector<AnalogVoiceSlot> analogVoices_;
    bool isRecording_;
    std::vector<float> recordingBuffer_;

    double lfoPhase_;
    double lfoIncrement_;
    double lfoDepth_;
    double baseCutoffHz_;
    
    // Virtual Analog Engine parameter expansions
    double unisonDetuneAmount_;
    double pitchBendSemiTones_;

    std::vector<float> scopeCircularBuffer_;
    size_t scopeWriteIndex_;
    std::mutex scopeMutex_;

    double renderAnalogVA();
    double midiNoteToFreq(int note) const;
};

#endif
