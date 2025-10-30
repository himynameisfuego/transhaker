#pragma once

#include <JuceHeader.h>

// SamplePool holds a bunch of short one-shot sounds (footsteps, hits, etc.)
// in memory as AudioSampleBuffers. We'll later grab a random one for playback.
class SamplePool
{
public:
    SamplePool() = default;

    // Try to load a single file. Returns true if successful.
    bool loadFile(const juce::File& file);

    // Convenience: load many at once.
    // Returns how many actually succeeded.
    int loadFiles(const juce::StringArray& paths);

    // How many samples are currently loaded?
    int getNumSamples() const { return (int)samples.size(); }

    // Get a random buffer pointer (may be nullptr if empty).
    const juce::AudioSampleBuffer* getRandomSample();

private:
    struct SampleData
    {
        juce::AudioSampleBuffer buffer;
        double sampleRate = 44100.0;
        juce::String name;
    };

    juce::Random rng;
    std::vector<SampleData> samples;
};
