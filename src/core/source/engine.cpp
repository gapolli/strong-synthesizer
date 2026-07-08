#include "engine.hpp"
#include <cmath>
#include <algorithm>

Engine::Engine(double sampleRate)
    : sampleRate_(sampleRate)
    , currentMode_(EngineMode::FM_CHIP)
    , polyFmManager_(sampleRate, 4)
    , granularEngine_(sampleRate)
    , isRecording_(false)
    , lfoPhase_(0.0)
    , lfoIncrement_(1.0 / sampleRate)
    , lfoDepth_(0.0)
    , baseCutoffHz_(2000.0)
    , unisonDetuneAmount_(0.01)    // Default clean micro-detune tracking
    , pitchBendSemiTones_(0.0)       // Centered pitch wheel state
    , scopeWriteIndex_(0) {
    filter_.setSampleRate(sampleRate_);
    analogVoices_.resize(4);
    for (auto& v : analogVoices_) {
        v.phases[0] = v.phases[1] = v.phases[2] = 0.0;
    }
    scopeCircularBuffer_.resize(512, 0.0f);
}

Engine::~Engine() {}

void Engine::setEngineMode(EngineMode mode) {
    currentMode_ = mode;
}

void Engine::setSampleRate(double sampleRate) {
    sampleRate_ = sampleRate;
    polyFmManager_.setSampleRate(sampleRate);
    granularEngine_.setSampleRate(sampleRate);
    filter_.setSampleRate(sampleRate);
    lfoIncrement_ = 1.0 / sampleRate_;
}

double Engine::midiNoteToFreq(int note) const {
    // Dynamic tracking applying pitch wheel semi-tone adjustments inside calculation phase
    return 440.0 * std::pow(2.0, (note + pitchBendSemiTones_ - 69.0) / 12.0);
}

void Engine::triggerNoteOn(int note, int velocity) {
    double frequency = midiNoteToFreq(note);
    if (currentMode_ == EngineMode::FM_CHIP) {
        polyFmManager_.noteOn(note, velocity);
    } else if (currentMode_ == EngineMode::GRANULAR) {
        granularEngine_.triggerNote(frequency);
    } else if (currentMode_ == EngineMode::ANALOG_VA) {
        auto it = std::find_if(analogVoices_.begin(), analogVoices_.end(), [](const AnalogVoiceSlot& v) { return !v.active; });
        if (it == analogVoices_.end()) it = analogVoices_.begin();
        it->midiNote = note;
        it->active = true;
        it->phases[0] = 0.0;
        it->phases[1] = 0.25; // Phase offsetting removes immediate cancellation artifacts
        it->phases[2] = 0.5;
        it->baseIncrement = frequency / sampleRate_;
    }
}

void Engine::triggerNoteOff(int note) {
    if (currentMode_ == EngineMode::FM_CHIP) {
        polyFmManager_.noteOff(note);
    } else if (currentMode_ == EngineMode::ANALOG_VA) {
        for (auto& v : analogVoices_) {
            if (v.midiNote == note) v.active = false;
        }
    }
}

void Engine::updateControlChange(int controlId, int value) {
    if (controlId == 1 && currentMode_ == EngineMode::FM_CHIP) {
        polyFmManager_.setAlgorithm(static_cast<int>((value / 127.0) * 7.0));
    } else if (controlId == 74) {
        baseCutoffHz_ = 20.0 + (std::pow(value / 127.0, 2.0) * 12000.0);
    } else if (controlId == 13) {
        filter_.setDrive(value / 127.0);
    } else if (controlId == 14) { // Map custom MIDI CC ID 14 to Unison Width Detune percentage scaling
        unisonDetuneAmount_ = (value / 127.0) * 0.08; 
    } else if (controlId == 128) { // Explicit tracking matching pitch bend definitions inside midi_manager
        double shiftValue = (value - 64) / 64.0; // Map down into a standard active range [-1.0, 1.0]
        pitchBendSemiTones_ = shiftValue * 2.0;    // Standard target scale +/- 2 semitone range allocation
        
        // Dynamically update frequencies across currently active polyphonic analog voices
        for (auto& v : analogVoices_) {
            if (v.active) {
                v.baseIncrement = midiNoteToFreq(v.midiNote) / sampleRate_;
            }
        }
    }
}

void Engine::configureFilter(int mode, double cutoff, double resonance) {
    filter_.setMode(static_cast<StateVariableFilter::FilterMode>(mode));
    baseCutoffHz_ = cutoff;
    filter_.setParameters(baseCutoffHz_, resonance);
}

void Engine::configureGranular(double position, double durationMs, int density) {
    granularEngine_.setParameters(position, durationMs, density);
}

void Engine::configureLfo(double frequencyHz, double depthPercent) {
    lfoIncrement_ = frequencyHz / sampleRate_;
    lfoDepth_ = std::clamp(depthPercent, 0.0, 1.0);
}

void Engine::updateFmEnvelope(int opIndex, double attack, double decay, double sustain, double release) {
    polyFmManager_.updateEnvelope(opIndex, attack, decay, sustain, release);
}

void Engine::loadGranularSample(const float* dataBuffer, size_t sampleCount) {
    granularEngine_.loadExternalSampleBuffer(dataBuffer, sampleCount);
}

bool Engine::startRecording() {
    if (isRecording_) return false;
    recordingBuffer_.clear();
    isRecording_ = true;
    return true;
}

void Engine::stopRecording() {
    isRecording_ = false;
}

size_t Engine::getRecordedSamplesCount() const {
    return recordingBuffer_.size();
}

void Engine::copyRecordedSamples(float* destBuffer) {
    if (!destBuffer) return;
    std::copy(recordingBuffer_.begin(), recordingBuffer_.end(), destBuffer);
    recordingBuffer_.clear();
}

void Engine::copyScopeData(float* destBuffer, size_t count) {
    if (!destBuffer) return;
    std::lock_guard<std::mutex> lock(scopeMutex_);
    size_t samplesToCopy = std::min(count, scopeCircularBuffer_.size());
    for (size_t i = 0; i < samplesToCopy; ++i) {
        size_t readIdx = (scopeWriteIndex_ + i) % scopeCircularBuffer_.size();
        destBuffer[i] = scopeCircularBuffer_[readIdx];
    }
}

double Engine::renderAnalogVA() {
    double mixedVABuffer = 0.0;
    int activeCount = 0;
    
    // Pitch factor offsets calculation for the detuned 3-oscillator unison grid
    double detuneOffsets[3] = {1.0, 1.0 - unisonDetuneAmount_, 1.0 + unisonDetuneAmount_};

    for (auto& v : analogVoices_) {
        if (!v.active) continue;
        
        double voiceMixedSignal = 0.0;
        for (int osc = 0; osc < 3; ++osc) {
            v.phases[osc] += v.baseIncrement * detuneOffsets[osc];
            if (v.phases[osc] >= 1.0) v.phases[osc] -= 1.0;
            
            // Standard naive sawtooth signal generation loop formula
            voiceMixedSignal += (2.0 * v.phases[osc] - 1.0);
        }
        
        mixedVABuffer += (voiceMixedSignal / 3.0);
        activeCount++;
    }
    return activeCount > 0 ? (mixedVABuffer / activeCount) : 0.0;
}

double Engine::renderNextSample() {
    lfoPhase_ += lfoIncrement_;
    if (lfoPhase_ >= 1.0) lfoPhase_ -= 1.0;
    double lfoValue = std::sin(2.0 * 3.14159265358979323846 * lfoPhase_);
    
    double modulatedCutoff = baseCutoffHz_ + (lfoValue * lfoDepth_ * 4000.0);
    filter_.setParameters(modulatedCutoff, 0.707);

    double rawSample = 0.0;
    switch (currentMode_) {
        case EngineMode::ANALOG_VA: rawSample = renderAnalogVA(); break;
        case EngineMode::GRANULAR:  rawSample = granularEngine_.renderNextSample(); break;
        case EngineMode::FM_CHIP:
        default:                    rawSample = polyFmManager_.renderNextMixedSample(); break;
    }

    double filteredSample = filter_.process(rawSample);
    
    if (isRecording_) {
        recordingBuffer_.push_back(static_cast<float>(filteredSample));
    }
    
    {
        std::lock_guard<std::mutex> lock(scopeMutex_);
        scopeCircularBuffer_[scopeWriteIndex_] = static_cast<float>(filteredSample);
        scopeWriteIndex_ = (scopeWriteIndex_ + 1) % scopeCircularBuffer_.size();
    }
    
    return filteredSample;
}
