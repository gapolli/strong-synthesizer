#ifndef YM2612_CORE_HPP
#define YM2612_CORE_HPP

#include "operator.hpp"
#include "envelope.hpp"
#include "psg_chip.hpp"
#include <vector>

class YM2612Core {
public:
    YM2612Core(double sampleRate = 48000.0);
    ~YM2612Core();

    void setAlgorithm(int algorithm);
    void setFrequency(double frequency);
    void setOperatorEnvelope(int opIndex, double attack, double decay, double sustain, double release);
    void setSampleRate(double sampleRate);
    
    void noteOn();
    void noteOff();
    
    double generateNextSample();
    PSGChip& getPsg();

private:
    double sampleRate_;
    int algorithm_;
    
    std::vector<Operator> operators_;
    std::vector<ADSREnvelope> envelopes_;
    PSGChip psgEngine_;

    double processAlgorithm(double op1, double op2, double op3, double op4);
};

#endif
