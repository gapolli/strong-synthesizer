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
    , driveGain_(1.0) { // Default unity pre-gain drive level
    setParameters(1000.0, 0.707);
}

StateVariableFilter::~StateVariableFilter() {}

void StateVariableFilter::setSampleRate(double sampleRate) {
    sampleRate_ = sampleRate;
    setParameters(cutoff_, resonance_);
}

void StateVariableFilter::setParameters(double cutoffHz, double resonance) {
    cutoff_ = std::clamp(cutoffHz, 20.0, sampleRate_ * 0.49);
    resonance_ = std::clamp(resonance, 0.01, 10.0);
    
    constexpr double pi = 3.14159265358979323846;
    f_ = 2.0 * std::sin(pi * cutoff_ / sampleRate_);
    q_ = 1.0 / resonance_;
}

void StateVariableFilter::setMode(FilterMode mode) {
    mode_ = mode;
}

void StateVariableFilter::setDrive(double driveAmount) {
    // Maps standard normalized UI input values up into an exponential [1.0, 20.0] gain space
    driveGain_ = 1.0 + (std::pow(std::clamp(driveAmount, 0.0, 1.0), 2.0) * 19.0);
}

void StateVariableFilter::reset() {
    lowState_ = 0.0;
    bandState_ = 0.0;
}

double StateVariableFilter::shapeDistortion(double sample) const {
    if (driveGain_ <= 1.001) return sample;

    // Apply pre-gain multiplier driven by interface sliders
    double inputSignal = sample * driveGain_;

    // Introduce an asymmetric DC bias offset to yield rich even-harmonic overtones
    constexpr double dcAsymmetryOffset = 0.15;
    double biasedSignal = inputSignal + dcAsymmetryOffset;

    // Hyperbolic tangent soft-clipping waveshaping transfer algorithm formula
    double saturatedSignal = std::tanh(biasedSignal);

    // Remove the mathematical DC offset post-saturation to prevent sub-bass frequency bias drift
    double outputSignal = saturatedSignal - std::tanh(dcAsymmetryOffset);

    // Normalize output master gain behavior to balance saturation level increases
    double compensationGain = 1.0 / std::sqrt(driveGain_);
    return outputSignal * compensationGain;
}

double StateVariableFilter::process(double input) {
    // Run Chamberlin filter calculations
    double high = input - lowState_ - (q_ * bandState_);
    bandState_ += f_ * high;
    lowState_ += f_ * bandState_;
    
    double filterOutput = 0.0;
    switch (mode_) {
        case FilterMode::HIGH_PASS: filterOutput = high; break;
        case FilterMode::BAND_PASS: filterOutput = bandState_; break;
        case FilterMode::LOW_PASS:
        default:                   filterOutput = lowState_; break;
    }

    // Apply the custom non-linear waveshaping distortion block downstream
    return shapeDistortion(filterOutput);
}
