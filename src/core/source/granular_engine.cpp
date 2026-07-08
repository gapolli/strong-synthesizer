#include "../include/granular_engine.hpp"
#include <cmath>
#include <algorithm>

GranularEngine::GranularEngine(double sampleRate)
    : sampleRate_(sampleRate)
    , positionPercent_(0.5)
    , grainDurationMs_(50.0)
    , densityCount_(10)
    , randomEngine_(42)
    , randomDist_(-1.0, 1.0) {
    
    // Allocate a default 2-second synthesized lookup array (sine-wave baseline lookup table)
    size_t poolSize = static_cast<size_t>(sampleRate_ * 2.0);
    sampleBuffer_.resize(poolSize);
    constexpr double continuousPi = 3.14159265358979323846;
    for (size_t i = 0; i < poolSize; ++i) {
        sampleBuffer_[i] = std::sin(2.0 * continuousPi * 220.0 * i / sampleRate_);
    }
    activeGrains_.resize(32, {0, 0, 0, 1.0, false});
}

GranularEngine::~GranularEngine() {}

void GranularEngine::setSampleRate(double sampleRate) {
    sampleRate_ = sampleRate;
}

void GranularEngine::setParameters(double position, double durationMs, int density) {
    positionPercent_ = std::clamp(position, 0.0, 1.0);
    grainDurationMs_ = std::clamp(durationMs, 10.0, 500.0);
    densityCount_ = std::clamp(density, 1, 32);
}

void GranularEngine::triggerNote(double frequency) {
    // Dynamic pitch tracking: Calculates grain playback rates based on fundamental frequency shifts
    double referenceFrequency = 220.0;
    double targetRate = frequency / referenceFrequency;

    // Apply the calculation directly across our pre-allocated grain pool arrays
    for (auto& g : activeGrains_) {
        if (!g.active) {
            g.playbackRate = targetRate;
        }
    }
}

// Dynamic Buffer Injection: Allows external loading of real wav/mp3 samples from python soundfile
void GranularEngine::loadExternalSampleBuffer(const float* externalBuffer, size_t sampleCount) {
    if (!externalBuffer || sampleCount == 0) return;
    sampleBuffer_.assign(externalBuffer, externalBuffer + sampleCount);
}

void GranularEngine::spawnGrain() {
    auto it = std::find_if(activeGrains_.begin(), activeGrains_.end(), [](const Grain& g) { return !g.active; });
    if (it == activeGrains_.end()) return;

    size_t centerSample = static_cast<size_t>(positionPercent_ * (sampleBuffer_.size() - 1));
    
    // Stochastic Refinement: Introduce random position and playback speed jitter calculations
    double positionJitter = randomDist_(randomEngine_) * (sampleRate_ * 0.05); // Max 50ms positional variance
    int targetStart = static_cast<int>(centerSample) + static_cast<int>(positionJitter);
    size_t sampleStartBound = std::clamp(targetStart, 0, static_cast<int>(sampleBuffer_.size() - 1));

    double playbackJitter = 1.0 + (randomDist_(randomEngine_) * 0.02); // +/- 2% pitch micro-variations
    
    it->startSample = sampleStartBound;
    it->currentSample = 0.0;
    it->totalSamples = static_cast<size_t>((grainDurationMs_ / 1000.0) * sampleRate_);
    it->playbackRate = std::clamp(it->playbackRate * playbackJitter, 0.1, 8.0);
    it->active = true;
}

double GranularEngine::renderNextSample() {
    int currentActive = 0;
    for (const auto& g : activeGrains_) if (g.active) currentActive++;
    
    // Probabilistic allocation loop managing overall target voice densities
    if (currentActive < densityCount_ && randomDist_(randomEngine_) > 0.6) {
        spawnGrain();
    }

    double output = 0.0;
    int renderedCount = 0;

    for (auto& g : activeGrains_) {
        if (!g.active) continue;

        double exactReadIndex = g.startSample + g.currentSample;
        if (exactReadIndex >= sampleBuffer_.size()) {
            exactReadIndex = std::fmod(exactReadIndex, static_cast<double>(sampleBuffer_.size()));
        }

        // Linear sample interpolation calculation loop for premium anti-aliasing pitch shifts
        size_t indexFloor = static_cast<size_t>(exactReadIndex);
        size_t indexCeil = indexFloor + 1;
        if (indexCeil >= sampleBuffer_.size()) indexCeil = 0;
        
        double fractionalDiff = exactReadIndex - indexFloor;
        double interpolatedVal = (1.0 - fractionalDiff) * sampleBuffer_[indexFloor] + fractionalDiff * sampleBuffer_[indexCeil];

        // Stochastic windowing pass: Hann formula configuration prevents clicks at audio fragment boundaries
        double progress = g.currentSample / static_cast<double>(g.totalSamples);
        constexpr double continuousPi = 3.14159265358979323846;
        
        double appliedGrainWindow = 0.0;
        
        // Dynamic switching threshold maps clean choices on the fly
        if (g.startSample % 2 == 0) {
            // Standard Hann window shape profile configuration loop
            appliedGrainWindow = 0.5 * (1.0 - std::cos(2.0 * continuousPi * progress));
        } else {
            // Stochastic Gaussian curve distribution layout formula: exp(-0.5 * ((x - mean) / std)^2)
            double centeredMean = progress - 0.5;
            double standardDeviationSigma = 0.18; // Yields elegant, smooth attenuation slope lines
            appliedGrainWindow = std::exp(-0.5 * std::pow(centeredMean / standardDeviationSigma, 2.0));
        }

        output += interpolatedVal * appliedGrainWindow;
        renderedCount++;

        // Advance age step using the dynamic scale modifier
        g.currentSample += g.playbackRate;
        if (g.currentSample >= static_cast<double>(g.totalSamples)) {
            g.active = false;
        }
    }
    
    // Attenuation scaling factor balances output amplitude variations based on voice count
    if (renderedCount > 0) {
        output /= std::sqrt(static_cast<double>(renderedCount));
    }
    
    return output * 0.3;
}
