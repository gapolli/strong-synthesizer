#include "psg_chip.hpp"
#include <cmath>

PSGChip::PSGChip(double sampleRate)
    : sampleRate_(sampleRate)
    , latchedChannel_(0)
    , latchedType_(0)
    , noiseShiftRegister_(0x4000)
    , randomEngine_(1337)
    , randomDist_(0, 1) {
    for (int i = 0; i < 4; ++i) {
        registers_[i] = 0;
        volumes_[i] = 0x0F; // Muted by default
        channelCounters_[i] = 0.0;
        channelStates_[i] = 1;
    }
}

PSGChip::~PSGChip() {}

void PSGChip::setSampleRate(double sampleRate) {
    sampleRate_ = sampleRate;
}

void PSGChip::writeRegister(uint8_t data) {
    if (data & 0x80) { // Latch byte
        latchedChannel_ = (data >> 5) & 0x03;
        latchedType_ = (data >> 4) & 0x01;
        
        if (latchedType_ == 0) { // Tone or Noise data adjustment
            if (latchedChannel_ < 3) {
                registers_[latchedChannel_] = (registers_[latchedChannel_] & 0x03F0) | (data & 0x0F);
            } else {
                registers_[latchedChannel_] = data & 0x0F; // Noise register setup
            }
        } else { // Volume setting tracking
            volumes_[latchedChannel_] = data & 0x0F;
        }
    } else { // Extension data byte pass
        if (latchedType_ == 0 && latchedChannel_ < 3) {
            registers_[latchedChannel_] = (registers_[latchedChannel_] & 0x000F) | ((data & 0x3F) << 4);
        }
    }
}

double PSGChip::generateNextSample() {
    double mixedOutput = 0.0;
    
    // Process Tone Channels 1, 2, and 3
    for (int i = 0; i < 3; ++i) {
        if (volumes_[i] >= 0x0F || registers_[i] == 0) continue;
        
        // Convert internal frequency register data into a baseline frequency step
        double freq = (sampleRate_ * 2.0) / (32.0 * registers_[i]);
        double increment = freq / sampleRate_;
        
        channelCounters_[i] += increment;
        if (channelCounters_[i] >= 0.5) {
            channelCounters_[i] -= 0.5;
            channelStates_[i] = -channelStates_[i];
        }
        
        double attenuation = std::pow(10.0, (-2.0 * volumes_[i]) / 20.0);
        mixedOutput += channelStates_[i] * attenuation * 0.25;
    }
    
    // Process White/Periodic Noise Channel 4
    if (volumes_[3] < 0x0F) {
        int noiseMode = (registers_[3] >> 2) & 0x01;
        int shiftRate = registers_[3] & 0x03;
        
        double noiseFreq = 0.0;
        if (shiftRate == 3) {
            noiseFreq = (sampleRate_ * 2.0) / (32.0 * registers_[2]);
        } else {
            noiseFreq = (sampleRate_ * 2.0) / (32.0 * (0x10 << shiftRate));
        }
        
        channelCounters_[3] += noiseFreq / sampleRate_;
        if (channelCounters_[3] >= 0.5) {
            channelCounters_[3] -= 0.5;
            
            if (noiseMode == 1) { // White Noise Implementation pattern
                int feedback = ((noiseShiftRegister_ >> 0) ^ (noiseShiftRegister_ >> 3)) & 1;
                noiseShiftRegister_ = (noiseShiftRegister_ >> 1) | (feedback << 14);
            } else { // Periodic Noise pattern configuration
                int feedback = noiseShiftRegister_ & 1;
                noiseShiftRegister_ = (noiseShiftRegister_ >> 1) | (feedback << 14);
            }
        }
        
        double noiseAttenuation = std::pow(10.0, (-2.0 * volumes_[3]) / 20.0);
        mixedOutput += ((noiseShiftRegister_ & 1) ? 1.0 : -1.0) * noiseAttenuation * 0.25;
    }

    return mixedOutput;
}
