#ifndef ENVELOPE_HPP
#define ENVELOPE_HPP

class ADSREnvelope {
public:
    enum class State {
        IDLE,
        ATTACK,
        DECAY,
        SUSTAIN,
        RELEASE
    };

    ADSREnvelope();
    ~ADSREnvelope();

    void setSampleRate(double sampleRate);
    void setParameters(double attack, double decay, double sustain, double release);
    
    void gate(bool on);
    double nextSample();
    State getState() const;

private:
    double sampleRate_;
    double attackRate_;
    double decayRate_;
    double sustainLevel_;
    double releaseRate_;
    
    double currentLevel_;
    State state_;
};

#endif
