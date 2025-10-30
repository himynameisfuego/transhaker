#pragma once

#include <JuceHeader.h>
#include "SamplePool.h"

// super simple one-pole lowpass
struct SimpleOnePoleLPF
{
    void prepare(double sampleRateHz)
    {
        sr = sampleRateHz;
        zL = 0.0f;
        zR = 0.0f;
        setCutoff(10000.0f); // default
    }

    void setCutoff(float newCutHz)
    {
        // one-pole lowpass coefficient:
        // y[n] = a * x[n] + b * y[n-1]
        // a = 1 - e^(-2*pi*fc/sr)
        // b = e^(-2*pi*fc/sr)
        const float x = std::exp(-2.0f * juce::MathConstants<float>::pi * newCutHz / (float)sr);
        a = 1.0f - x;
        b = x;
    }

    inline float processL(float xIn)
    {
        zL = a * xIn + b * zL;
        return zL;
    }

    inline float processR(float xIn)
    {
        zR = a * xIn + b * zR;
        return zR;
    }

    double sr = 48000.0;
    float a = 1.0f, b = 0.0f;
    float zL = 0.0f, zR = 0.0f;
};


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
    double playPos = 0.0;   // fractional playback position (in samples, after offset)
    int    totalLen = 0;     // total available samples after offset
    float  gainLinear = 1.0f;  // baked-in random gain
    int    startOffsetSamps = 0;   // baked-in random offset
    double playbackRate = 1.0;   // >1.0 plays faster/pitch up, <1.0 slower/pitch down

    SimpleOnePoleLPF lpf;
    float currentCutoffHz = 10000.0f;

    Params params;
    juce::Random rng;

    double currentSampleRate = 44100.0;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(VariationPlayer)
};
