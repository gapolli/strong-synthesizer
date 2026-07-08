#ifndef FILTER_HPP
#define FILTER_HPP

class StateVariableFilter {
public:
    enum class FilterMode {
        LOW_PASS,
        BAND_PASS,
        HIGH_PASS
    };

    StateVariableFilter();
    ~StateVariableFilter();

    void setSampleRate(double sampleRate);
    void setParameters(double cutoffHz, double resonance);
    void setMode(FilterMode mode);
    
    // Waveshaper control parameters
    void setDrive(double driveAmount);

    double process(double input);
    void reset();

private:
    double sampleRate_;
    FilterMode mode_;
    
    double cutoff_;
    double resonance_;
    
    double f_; 
    double q_; 
    
    double lowState_;
    double bandState_;

    // Saturated waveshaper state parameters
    double driveGain_;

    double shapeDistortion(double sample) const;
};

#endif
