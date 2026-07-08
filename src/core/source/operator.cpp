#include "../include/operator.hpp"
#include <cmath>

Operator::Operator()
    : frequency_(440.0)
    , waveform_(Waveform::SINE)
    , sampleRate_(48000.0)
    , phase_(0.0)
    , phaseModulation_(0.0) {}

Operator::Operator(double frequency, Waveform waveform, double sampleRate)
    : frequency_(frequency)
    , waveform_(waveform)
    , sampleRate_(sampleRate)
    , phase_(0.0)
    , phaseModulation_(0.0) {}

Operator::~Operator() {}

void Operator::setFrequency(double frequency) {
    frequency_ = frequency;
}

void Operator::setWaveform(Waveform waveform) {
    waveform_ = waveform;
}

void Operator::setSampleRate(double sampleRate) {
    sampleRate_ = sampleRate;
}

void Operator::setPhaseModulation(double modulation) {
    phaseModulation_ = modulation;
}

double Operator::getFrequency() const {
    return frequency_;
}

Operator::Waveform Operator::getWaveform() const {
    return waveform_;
}

void Operator::reset() {
    phase_ = 0.0;
    phaseModulation_ = 0.0;
}

double Operator::nextSample() {
    double phaseIncrement = frequency_ / sampleRate_;
    phase_ += phaseIncrement;
    if (phase_ >= 1.0) {
        phase_ -= 1.0;
    }

    double currentPhase = phase_ + phaseModulation_;
    currentPhase = currentPhase - std::floor(currentPhase);

    double output = 0.0;
    switch (waveform_) {
        case Waveform::SINE:
            output = generateSine(currentPhase);
            break;
        case Waveform::SQUARE:
            output = generateSquare(currentPhase);
            break;
        case Waveform::SAW:
            output = generateSaw(currentPhase);
            break;
        case Waveform::TRIANGLE:
            output = generateTriangle(currentPhase);
            break;
    }

    phaseModulation_ = 0.0;
    return output;
}

double Operator::generateSine(double phase) const {
    constexpr double pi = 3.14159265358979323846;
    return std::sin(2.0 * pi * phase);
}

double Operator::generateSquare(double phase) const {
    return (phase < 0.5) ? 1.0 : -1.0;
}

double Operator::generateSaw(double phase) const {
    return 2.0 * phase - 1.0;
}

double Operator::generateTriangle(double phase) const {
    if (phase < 0.25) {
        return 4.0 * phase;
    } else if (phase < 0.75) {
        return 2.0 - 4.0 * phase;
    } else {
        return 4.0 * phase - 4.0;
    }
}
