#include "ym2612_core.hpp"
#include <cmath>

YM2612Core::YM2612Core(double sampleRate)
    : sampleRate_(sampleRate)
    , algorithm_(0)
    , psgEngine_(sampleRate) {
    for (int i = 0; i < 4; ++i) {
        operators_.emplace_back(440.0, Operator::Waveform::SINE, sampleRate_);
        envelopes_.emplace_back();
        envelopes_[i].setSampleRate(sampleRate_);
    }
}

YM2612Core::~YM2612Core() {}

void YM2612Core::setSampleRate(double sampleRate) {
    sampleRate_ = sampleRate;
    psgEngine_.setSampleRate(sampleRate_);
    for (size_t i = 0; i < operators_.size(); ++i) {
        operators_[i].setSampleRate(sampleRate_);
        envelopes_[i].setSampleRate(sampleRate_);
    }
}

void YM2612Core::setAlgorithm(int algorithm) {
    if (algorithm >= 0 && algorithm <= 7) {
        algorithm_ = algorithm;
    }
}

void YM2612Core::setFrequency(double frequency) {
    for (auto& op : operators_) {
        op.setFrequency(frequency);
    }
}

void YM2612Core::setOperatorEnvelope(int opIndex, double attack, double decay, double sustain, double release) {
    if (opIndex >= 0 && opIndex < 4) {
        envelopes_[opIndex].setParameters(attack, decay, sustain, release);
    }
}

PSGChip& YM2612Core::getPsg() {
    return psgEngine_;
}

void YM2612Core::noteOn() {
    for (auto& env : envelopes_) {
        env.gate(true);
    }
}

void YM2612Core::noteOff() {
    for (auto& env : envelopes_) {
        env.gate(false);
    }
}

double YM2612Core::processAlgorithm(double op1, double op2, double op3, double op4) {
    // Exact mapping of all 8 native YM2612 FM operator algorithms
    switch (algorithm_) {
        case 0:
            // Linear Cascade Chain: Op1 -> Op2 -> Op3 -> Op4
            operators_[1].setPhaseModulation(op1);
            operators_[2].setPhaseModulation(operators_[1].nextSample() * envelopes_[1].nextSample());
            operators_[3].setPhaseModulation(operators_[2].nextSample() * envelopes_[2].nextSample());
            return operators_[3].nextSample() * envelopes_[3].nextSample();

        case 1:
            // Combined Parallel Modulators: (Op1 + Op2) -> Op3 -> Op4
            operators_[2].setPhaseModulation(op1 + op2);
            operators_[3].setPhaseModulation(operators_[2].nextSample() * envelopes_[2].nextSample());
            return operators_[3].nextSample() * envelopes_[3].nextSample();

        case 2:
            // Branched Mixing Cascade: Op1 -> Op3 \ -> Op4
            //                         Op2 ------- /
            operators_[2].setPhaseModulation(op1);
            operators_[3].setPhaseModulation((operators_[2].nextSample() * envelopes_[2].nextSample()) + op2);
            return operators_[3].nextSample() * envelopes_[3].nextSample();

        case 3:
            // Parallel Cascade Pair: Op1 -> Op2 \ -> Op4
            //                       Op3 ------- /
            operators_[1].setPhaseModulation(op1);
            operators_[3].setPhaseModulation((operators_[1].nextSample() * envelopes_[1].nextSample()) + op3);
            return operators_[3].nextSample() * envelopes_[3].nextSample();

        case 4:
            // Independent Modulators Sub-Groups: Op1 -> Op2 \ -> Mixed Output
            //                                    Op3 -> Op4 /
            operators_[1].setPhaseModulation(op1);
            operators_[3].setPhaseModulation(op3);
            return ((operators_[1].nextSample() * envelopes_[1].nextSample()) + 
                    (operators_[3].nextSample() * envelopes_[3].nextSample())) * 0.5;

        case 5:
            // Single Common Modulator Routing: Op1 -> Op2 \ 
            //                                         -> Op3  -> Mixed Output
            //                                         -> Op4 /
            operators_[1].setPhaseModulation(op1);
            operators_[2].setPhaseModulation(op1);
            operators_[3].setPhaseModulation(op1);
            return ((operators_[1].nextSample() * envelopes_[1].nextSample()) + 
                    (operators_[2].nextSample() * envelopes_[2].nextSample()) + 
                    (operators_[3].nextSample() * envelopes_[3].nextSample())) * 0.33;

        case 6:
            // Multiple Carriers Added: Op1 -> Op2 \ 
            //                                 Op3   -> Op4 -> Mixed Output
            //                                 Op4  /
            operators_[1].setPhaseModulation(op1);
            return ((operators_[1].nextSample() * envelopes_[1].nextSample()) + op2 + op3) * 0.33;

        case 7:
        default:
            // Pure Additive Mode / 4-Carrier Organ Matrix Setup
            return (op1 + op2 + op3 + op4) * 0.25;
    }
}

double YM2612Core::generateNextSample() {
    double op1_out = operators_[0].nextSample() * envelopes_[0].nextSample();
    double op2_out = operators_[1].nextSample() * envelopes_[1].nextSample();
    double op3_out = operators_[2].nextSample() * envelopes_[2].nextSample();
    double op4_out = operators_[3].nextSample() * envelopes_[3].nextSample();

    double fmOutput = processAlgorithm(op1_out, op2_out, op3_out, op4_out);
    double psgOutput = psgEngine_.generateNextSample();
    
    return (fmOutput * 0.7) + (psgOutput * 0.3);
}
