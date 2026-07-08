#ifndef GRANULAR_ENGINE_HPP
#define GRANULAR_ENGINE_HPP

#include <vector>
#include <random>
#include <cstddef>

class GranularEngine {
public:
    GranularEngine(double sampleRate = 48000.0);
    ~GranularEngine();

    void setSampleRate(double sampleRate);
    void setParameters(double position, double durationMs, int density);
    double renderNextSample();
    void triggerNote(double frequency);
    
    // API de Injeção de Amostras Externas
    void loadExternalSampleBuffer(const float* externalBuffer, size_t sampleCount);

private:
    struct Grain {
        size_t startSample;
        double currentSample; // Modificado para double para suportar variação de pitch fracionária
        size_t totalSamples;
        double playbackRate;  // Taxa de variação/velocidade do grão individual
        bool active;
    };

    double sampleRate_;
    double positionPercent_;
    double grainDurationMs_;
    int densityCount_;
    
    std::vector<float> sampleBuffer_;
    std::vector<Grain> activeGrains_;
    
    std::mt19937 randomEngine_;
    std::uniform_real_distribution<double> randomDist_;

    void spawnGrain();
};

#endif
