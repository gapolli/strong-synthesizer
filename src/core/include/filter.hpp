#ifndef FILTER_HPP
#define FILTER_HPP

class StateVariableFilter {
public:
    enum class FilterMode {
        LOW_PASS = 0,
        HIGH_PASS = 1,
        BAND_PASS = 2
    };

private:
    double sampleRate_;
    FilterMode mode_;
    double cutoff_;
    double resonance_;
    
    double f_;
    double q_;
    double lowState_;
    double bandState_;
    double driveGain_;

    double shapeDistortion(double sample) const;

public:
    StateVariableFilter();
    ~StateVariableFilter();

    void setSampleRate(double sampleRate);
    void setParameters(double cutoffHz, double resonance);
    void setMode(FilterMode mode);
    void setDrive(double driveAmount);
    void reset();
    
    double process(double input);
};

#endif
