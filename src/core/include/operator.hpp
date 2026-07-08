#ifndef OPERATOR_HPP
#define OPERATOR_HPP

#include <string>

class Operator {
public:
    enum class Waveform {
        SINE,
        SQUARE,
        SAW,
        TRIANGLE
    };

    Operator();
    Operator(double frequency, Waveform waveform, double sampleRate = 48000.0);
    ~Operator();

    void setFrequency(double frequency);
    void setWaveform(Waveform waveform);
    void setSampleRate(double sampleRate);
    void setPhaseModulation(double modulation);

    double getFrequency() const;
    Waveform getWaveform() const;

    double nextSample();
    void reset();

private:
    double frequency_;
    Waveform waveform_;
    double sampleRate_;
    double phase_;
    double phaseModulation_;

    double generateSine(double phase) const;
    double generateSquare(double phase) const;
    double generateSaw(double phase) const;
    double generateTriangle(double phase) const;
};

#endif
