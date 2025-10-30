#pragma once
#include <JuceHeader.h>

// Small helper: write a mono buffer to a 24-bit WAV file on disk.
namespace AudioFileWriter
{
    bool writeMonoBufferToWav(const juce::AudioBuffer<float>& buffer,
        double sampleRate,
        const juce::File& fileToWrite);
}
