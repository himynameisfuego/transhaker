#pragma once

#include <JuceHeader.h>
#include "SamplePool.h"

// VariationPlayer:
// - Holds a pointer to SamplePool
// - On triggerRandom(), picks one sample and locks in randomized params
// - Audio thread pulls from that "armed" sample until it's done
class VariationPlayer : public juce::AudioSource
{
public:
    VariationPlayer() = default;

    void setSamplePool(SamplePool* p) { pool = p; }

    // Called by UI thread to set parameter ranges for randomization
    struct Params
    {
        // +/- pitchPercent not yet used (for future pitch shift)
        float pitchPercentRange = 5.0f;      // e.g. 5 -> ±5%
        float gainDbRange = 3.0f;      // e.g. 3 -> ±3 dB
        float maxOffsetMs = 5.0f;      // skip up to this many ms at start
        float lpfMinHz = 4000.0f;   // not used yet
        float lpfMaxHz = 12000.0f;  // not used yet
        double sampleRate = 44100.0;   // host sample rate (filled in prepareToPlay)
    };

    void setParams(const Params& newParams)
    {
        const juce::ScopedLock sl(lock);
        params = newParams;
    }

    // AudioSource
    void prepareToPlay(int samplesPerBlockExpected, double sampleRate) override;
    void releaseResources() override;
    void getNextAudioBlock(const juce::AudioSourceChannelInfo& bufferToFill) override;

    // UI calls this when user clicks TRIGGER
    void triggerRandom();

private:
    juce::CriticalSection lock;

    SamplePool* pool = nullptr;

    // current "voice" state that the audio thread will read
    juce::AudioSampleBuffer currentSample;
    int   readPos = 0;     // current playback position (in samples, after offset)
    int   totalLen = 0;     // total available samples after offset
    float gainLinear = 1.0f;  // baked-in random gain
    int   startOffsetSamps = 0;     // baked-in random offset
    double playbackRate = 1.0;   // for pitch shift later

    Params params;
    juce::Random rng;

    double currentSampleRate = 44100.0;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(VariationPlayer)
};
