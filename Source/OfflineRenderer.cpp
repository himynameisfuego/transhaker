#include "OfflineRenderer.h"
#include "AudioFileWriter.h"
#include <cmath>

// we'll reuse some helpers from VariationPlayer logic

static inline float dbToLinear_offline(float db)
{
    return std::pow(10.0f, db / 20.0f);
}

// simple linear interpolation on a mono buffer
static inline float lerpSample_offline(const juce::AudioSampleBuffer& buf, double index)
{
    const int numSamples = buf.getNumSamples();
    if (numSamples <= 0)
        return 0.0f;

    int i0 = (int)index;
    int i1 = i0 + 1;
    if (i0 >= numSamples) i0 = numSamples - 1;
    if (i1 >= numSamples) i1 = numSamples - 1;

    float frac = (float)(index - (double)i0);
    float s0 = buf.getSample(0, i0);
    float s1 = buf.getSample(0, i1);

    return s0 + frac * (s1 - s0);
}

// 1-pole LPF struct (offline copy)
struct SimpleOnePoleLPF_Offline
{
    void prepare(double sampleRateHz)
    {
        sr = sampleRateHz;
        z = 0.0f;
        setCutoff(10000.0f);
    }

    void setCutoff(float newCutHz)
    {
        const float x = std::exp(-2.0f * juce::MathConstants<float>::pi * newCutHz / (float)sr);
        a = 1.0f - x;
        b = x;
    }

    inline float process(float xIn)
    {
        z = a * xIn + b * z;
        return z;
    }

    double sr = 48000.0;
    float a = 1.0f, b = 0.0f;
    float z = 0.0f;
};

bool OfflineRenderer::renderSingleVariation(juce::AudioBuffer<float>& outBuffer,
    double& outSampleRate)
{
    if (pool == nullptr)
        return false;

    // pick a random sample from pool
    const juce::AudioSampleBuffer* chosen = pool->getRandomSample();
    if (chosen == nullptr || chosen->getNumSamples() == 0)
        return false;

    // --- randomize gain ---
    const float gainSpread = params.gainDbRange;
    const float gainDb = rng.nextFloat() * (2.0f * gainSpread) - gainSpread;
    const float gainLin = dbToLinear_offline(gainDb);

    // --- randomize start offset ---
    const float offsMs = rng.nextFloat() * params.maxOffsetMs; // in [0..maxOffsetMs]
    int startOffsetSamps = (int)std::round((offsMs / 1000.0f) * (float)params.sampleRate);
    if (startOffsetSamps >= chosen->getNumSamples())
        startOffsetSamps = juce::jmax(0, chosen->getNumSamples() - 1);

    // --- randomize pitch => playbackRate ---
    float pct = params.pitchPercentRange;
    float sign = (rng.nextFloat() * 2.0f) - 1.0f; // [-1..1]
    float pctDelta = sign * pct * 0.01f;          // convert % to ratio
    double playbackRate = 1.0 + (double)pctDelta;
    if (playbackRate < 0.5)  playbackRate = 0.5;
    if (playbackRate > 2.0)  playbackRate = 2.0;

    // --- randomize LPF cutoff ---
    float minHz = params.lpfMinHz;
    float maxHz = params.lpfMaxHz;
    if (maxHz < minHz)
        std::swap(minHz, maxHz);
    float cutoff = minHz + rng.nextFloat() * (maxHz - minHz);

    SimpleOnePoleLPF_Offline lpf;
    lpf.prepare(params.sampleRate);
    lpf.setCutoff(cutoff);

    // We now synthesize the processed variation entirely offline into outBuffer.
    // We'll just generate until the source runs out.
    const int maxLen = chosen->getNumSamples() - startOffsetSamps;
    if (maxLen <= 0)
        return false;

    outBuffer.setSize(1, maxLen); // mono
    auto* dst = outBuffer.getWritePointer(0);

    double playPos = 0.0;
    for (int n = 0; n < maxLen; ++n)
    {
        double absoluteIndex = (double)startOffsetSamps + playPos;
        float dry = lerpSample_offline(*chosen, absoluteIndex);

        float afterGain = dry * gainLin;
        float filtered = lpf.process(afterGain);

        dst[n] = filtered;

        playPos += playbackRate;

        if (absoluteIndex >= chosen->getNumSamples() - 1)
        {
            // ran out of source
            outBuffer.setSize(1, n + 1, true, true, true);
            break;
        }
    }

    outSampleRate = params.sampleRate;
    return true;
}

void OfflineRenderer::renderBatch(juce::Component* parentForChooser)
{
    if (pool == nullptr)
        return;

    // Ask user for target directory
    juce::FileChooser chooser("Select a folder to save variations...",
        juce::File::getSpecialLocation(juce::File::userDesktopDirectory),
        juce::String(),
        true);

    if (!chooser.browseForDirectory())
        return;

    auto folder = chooser.getResult();

    const int numToRender = 20; // fixed for now

    for (int i = 0; i < numToRender; ++i)
    {
        juce::AudioBuffer<float> rendered;
        double sr = 44100.0;
        if (!renderSingleVariation(rendered, sr))
            continue;

        juce::String fname = "transhaker_variation_" +
            juce::String(i + 1).paddedLeft('0', 3) +
            ".wav";

        juce::File outFile = folder.getChildFile(fname);
        AudioFileWriter::writeMonoBufferToWav(rendered, sr, outFile);
    }
}
