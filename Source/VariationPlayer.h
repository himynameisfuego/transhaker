#pragma once

#include <JuceHeader.h>
#include "SamplePool.h"

// VariationPlayer
//  - Generates per-trigger one-shot variations for playback.
//  - Two modes:
//      * OG: pitch/gain/start offset/LPF randomization
//      * Velvet: velvet-noise-style microvariation (placeholder impl now)
//  - Acts as the audio source feeding the main output.
class VariationPlayer
{
public:
    // Parameters controlled by UI sliders
    struct Params
    {
        double sampleRate = 44100.0;  // gets set in prepareToPlay()

        float pitchPercentRange = 5.0f;     // +/- % pitch shift range
        float gainDbRange = 3.0f;     // +/- dB gain variation
        float maxOffsetMs = 5.0f;     // max random start offset
        float lpfMinHz = 2000.0f;  // LPF lower bound
        float lpfMaxHz = 12000.0f; // LPF upper bound

        float velvetStrength = 0.08f;
        float velvetMinDelayMs = 2.0f;
        float velvetMaxDelayMs = 12.0f;
        int   velvetNumTaps = 50;
    };

    // Playback algorithm mode: OG SHAKER vs VELVET SHAKER
    enum class Mode { OG, Velvet };

    VariationPlayer();

    // hook up the shared sample pool so we can pick random source files
    void setSamplePool(SamplePool* p);

    // push updated UI params (sliders etc.)
    void setParams(const Params& p);

    // choose which algorithm to use
    void setMode(Mode m) { mode = m; }

    // JUCE audio lifecycle integration
    void prepareToPlay(int samplesPerBlockExpected, double sampleRate);
    void getNextAudioBlock(const juce::AudioSourceChannelInfo& bufferToFill);
    void releaseResources();

    // called when user hits SHAKER (or when OfflineRenderer wants a new variation)
    void triggerRandom();

private:
    // --- Internal helpers ---

    // Variant for velvet-noise style shaking
    void triggerVelvetVariation();

    // Applies a "velvet noise" style modulation to buffer in-place.
    // Current implementation is a simplified placeholder:
    //   - Generate sparse ±1 impulses (velvet noise)
    //   - Smooth slightly
    //   - Use result to modulate amplitude
    //
    // We'll later upgrade this to match the DAFx "one-to-many" decorrelation kernel.
    void applyVelvetNoise(juce::AudioBuffer<float>& buffer);

private:
    SamplePool* samplePool = nullptr;   // not owned

    Params params;                      // current randomization ranges from UI

    juce::AudioBuffer<float> currentBuffer; // mono buffer to stream out
    int currentPosition = 0;                // playback cursor into currentBuffer

    juce::Random rng;
    Mode mode = Mode::OG;
};
