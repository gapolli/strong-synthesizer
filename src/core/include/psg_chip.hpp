#ifndef PSG_CHIP_HPP
#define PSG_CHIP_HPP

#include <vector>
#include <random>

class PSGChip {
public:
    PSGChip(double sampleRate = 48000.0);
    ~PSGChip();

    void writeRegister(uint8_t data);
    double generateNextSample();
    void setSampleRate(double sampleRate);

private:
    double sampleRate_;
    
    uint16_t registers_[4];     // Tone channel frequency dividers
    uint8_t volumes_[4];        // Attenuation levels (0 = max, 15 = off)
    uint8_t latchedChannel_;
    uint8_t latchedType_;       // 0 = Tone/Noise, 1 = Volume

    double channelCounters_[4];
    int channelStates_[4];
    
    uint16_t noiseShiftRegister_;
    std::mt19937 randomEngine_;
    std::uniform_int_distribution<int> randomDist_;

    void processDataByte(uint8_t data);
};

#endif
