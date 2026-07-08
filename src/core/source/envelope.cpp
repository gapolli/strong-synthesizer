#include "../include/envelope.hpp"

ADSREnvelope::ADSREnvelope()
    : sampleRate_(48000.0)
    , attackRate_(0.001)
    , decayRate_(0.001)
    , sustainLevel_(0.7)
    , releaseRate_(0.001)
    , currentLevel_(0.0)
    , state_(State::IDLE) {
    setParameters(0.01, 0.1, 0.7, 0.3);
}

ADSREnvelope::~ADSREnvelope() {}

void ADSREnvelope::setSampleRate(double sampleRate) {
    sampleRate_ = sampleRate;
}

void ADSREnvelope::setParameters(double attack, double decay, double sustain, double release) {
    attackRate_ = (attack > 0.0) ? (1.0 / (attack * sampleRate_)) : 1.0;
    decayRate_ = (decay > 0.0) ? ((1.0 - sustain) / (decay * sampleRate_)) : 1.0;
    sustainLevel_ = sustain;
    releaseRate_ = (release > 0.0) ? (sustain / (release * sampleRate_)) : 1.0;
}

void ADSREnvelope::gate(bool on) {
    if (on) {
        state_ = State::ATTACK;
    } else if (state_ != State::IDLE) {
        state_ = State::RELEASE;
    }
}

double ADSREnvelope::nextSample() {
    switch (state_) {
        case State::IDLE:
            currentLevel_ = 0.0;
            break;

        case State::ATTACK:
            currentLevel_ += attackRate_;
            if (currentLevel_ >= 1.0) {
                currentLevel_ = 1.0;
                state_ = State::DECAY;
            }
            break;

        case State::DECAY:
            currentLevel_ -= decayRate_;
            if (currentLevel_ <= sustainLevel_) {
                currentLevel_ = sustainLevel_;
                state_ = State::SUSTAIN;
            }
            break;

        case State::SUSTAIN:
            currentLevel_ = sustainLevel_;
            break;

        case State::RELEASE:
            currentLevel_ -= releaseRate_;
            if (currentLevel_ <= 0.0) {
                currentLevel_ = 0.0;
                state_ = State::IDLE;
            }
            break;
    }
    return currentLevel_;
}

ADSREnvelope::State ADSREnvelope::getState() const {
    return state_;
}
