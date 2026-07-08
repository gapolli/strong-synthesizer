#include "filter.hpp"
#include <cmath>
#include <algorithm>

StateVariableFilter::StateVariableFilter()
    : sampleRate_(48000.0)
    , mode_(FilterMode::LOW_PASS)
    , cutoff_(1000.0)
    , resonance_(0.707)
    , f_(0.0)
    , q_(0.0)
    , lowState_(0.0)
    , bandState_(0.0)
    , driveGain_(1.0) {
    setParameters(1000.0, 0.707);
}

StateVariableFilter::~StateVariableFilter() {}

void StateVariableFilter::setSampleRate(double sampleRate) {
    sampleRate_ = sampleRate;
    setParameters(cutoff_, resonance_);
}

void StateVariableFilter::setParameters(double cutoffHz, double resonance) {
    // Rigid safety ceiling clamp preventing Chamberlin state blowups completely
    cutoff_ = std::clamp(cutoffHz, 20.0, sampleRate_ * 0.18); 
    resonance_ = std::clamp(resonance, 0.1, 10.0);
    
    constexpr double pi = 3.14159265358979323846;
    // Standard Chamberlin approximation tuning formula
    f_ = 2.0 * std::sin(pi * cutoff_ / sampleRate_);
    q_ = 1.0 / resonance_;
}

void StateVariableFilter::setMode(FilterMode mode) {
    mode_ = mode;
}

void StateVariableFilter::setDrive(double driveAmount) {
    driveGain_ = 1.0 + (std::pow(std::clamp(driveAmount, 0.0, 1.0), 2.0) * 19.0);
}

void StateVariableFilter::reset() {
    lowState_ = 0.0;
    bandState_ = 0.0;
}

double StateVariableFilter::shapeDistortion(double sample) const {
    if (driveGain_ <= 1.001) return sample;

    double inputSignal = sample * driveGain_;
    constexpr double dcAsymmetryOffset = 0.15;
    double biasedSignal = inputSignal + dcAsymmetryOffset;
    double saturatedSignal = std::tanh(biasedSignal);
    double outputSignal = saturatedSignal - std::tanh(dcAsymmetryOffset);
    double compensationGain = 1.0 / std::sqrt(driveGain_);
    
    return outputSignal * compensationGain;
}

double StateVariableFilter::process(double input) {
    if (std::isnan(lowState_) || std::isinf(lowState_) || std::isnan(bandState_) || std::isinf(bandState_)) {
        reset();
    }

    double high = input - lowState_ - (q_ * bandState_);
    bandState_ += f_ * high;
    lowState_ += f_ * bandState_;

    lowState_ = std::clamp(lowState_, -4.0, 4.0);
    bandState_ = std::clamp(bandState_, -4.0, 4.0);

    double filterOutput = 0.0;
    switch (mode_) {
        case FilterMode::HIGH_PASS: 
            filterOutput = high; 
            break;
        case FilterMode::BAND_PASS: 
            filterOutput = bandState_; 
            break;
        case FilterMode::LOW_PASS:
        default:                   
            filterOutput = lowState_; 
            break;
    }

    return shapeDistortion(filterOutput);
}