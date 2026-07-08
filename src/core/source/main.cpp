#include "../include/ym2612_core.hpp"
#include "../include/audio_output.hpp"
#include <iostream>
#include <chrono>
#include <thread>

int main() {
    YM2612Core core(48000.0);
    AudioOutput audio(core, 48000.0);

    core.setAlgorithm(5);
    core.setFrequency(440.0);

    if (!audio.start()) {
        return 1;
    }

    core.noteOn();
    std::this_thread::sleep_for(std::chrono::seconds(2));

    core.noteOff();
    std::this_thread::sleep_for(std::chrono::seconds(1));

    audio.stop();
    return 0;
}
