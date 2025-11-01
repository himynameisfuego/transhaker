#include "VariationPlayer.h"
#include <cmath>

// =========================================================
// Helper functions
// =========================================================

static inline float dbToLinear(float db)
{
    return std::pow(10.0f, db / 20.0f);
}

static inline float lerpSample(const juce::AudioSampleBuffer& buf, double index)
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

// Simple 1-pole LPF
struct SimpleOnePoleLPF
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

// =========================================================
// VariationPlayer
// =========================================================

VariationPlayer::VariationPlayer() {}

void VariationPlayer::setSamplePool(SamplePool* p)
{
    samplePool = p;
}

void VariationPlayer::setParams(const Params& p)
{
    params = p;
}

void VariationPlayer::prepareToPlay(int samplesPerBlockExpected, double sampleRate)
{
    juce::ignoreUnused(samplesPerBlockExpected);
    params.sampleRate = sampleRate;
}

void VariationPlayer::releaseResources()
{
    currentBuffer.setSize(0, 0);
}

void VariationPlayer::triggerRandom()
{
    if (mode == Mode::Velvet)
    {
        triggerVelvetVariation();
        return;
    }

    // OG SHAKER mode
    if (samplePool == nullptr)
        return;

    const juce::AudioSampleBuffer* chosen = samplePool->getRandomSample();
    if (chosen == nullptr || chosen->getNumSamples() == 0)
        return;

    // --- randomize gain ---
    const float gainSpread = params.gainDbRange;
    const float gainDb = rng.nextFloat() * (2.0f * gainSpread) - gainSpread;
    const float gainLin = dbToLinear(gainDb);

    // --- randomize start offset ---
    const float offsMs = rng.nextFloat() * params.maxOffsetMs; // [0..maxOffsetMs]
    int startOffsetSamps = (int)std::round((offsMs / 1000.0f) * (float)params.sampleRate);
    if (startOffsetSamps >= chosen->getNumSamples())
        startOffsetSamps = juce::jmax(0, chosen->getNumSamples() - 1);

    // --- randomize pitch ---
    float pct = params.pitchPercentRange;
    float sign = (rng.nextFloat() * 2.0f) - 1.0f; // [-1..1]
    float pctDelta = sign * pct * 0.01f;          // % → ratio
    double playbackRate = 1.0 + (double)pctDelta;
    if (playbackRate < 0.5)  playbackRate = 0.5;
    if (playbackRate > 2.0)  playbackRate = 2.0;

    // --- randomize LPF cutoff ---
    float minHz = params.lpfMinHz;
    float maxHz = params.lpfMaxHz;
    if (maxHz < minHz)
        std::swap(minHz, maxHz);
    float cutoff = minHz + rng.nextFloat() * (maxHz - minHz);

    SimpleOnePoleLPF lpf;
    lpf.prepare(params.sampleRate);
    lpf.setCutoff(cutoff);

    // Copy chosen buffer to our working buffer
    const int N = chosen->getNumSamples();
    currentBuffer.setSize(1, N);
    currentBuffer.clear();

    const float* src = chosen->getReadPointer(0);
    float* dst = currentBuffer.getWritePointer(0);

    double playPos = startOffsetSamps;
    for (int n = 0; n < N; ++n)
    {
        float dry = lerpSample(*chosen, playPos);
        float afterGain = dry * gainLin;
        float filtered = lpf.process(afterGain);
        dst[n] = filtered;

        playPos += playbackRate;
        if (playPos >= chosen->getNumSamples())
            break;
    }

    currentPosition = 0;
}

void VariationPlayer::getNextAudioBlock(const juce::AudioSourceChannelInfo& bufferToFill)
{
    auto* outL = bufferToFill.buffer->getWritePointer(0, bufferToFill.startSample);
    auto* outR = bufferToFill.buffer->getNumChannels() > 1
        ? bufferToFill.buffer->getWritePointer(1, bufferToFill.startSample)
        : nullptr;
    const int numOut = bufferToFill.numSamples;

    if (currentBuffer.getNumSamples() == 0)
    {
        bufferToFill.buffer->clear(bufferToFill.startSample, numOut);
        return;
    }

    const int N = currentBuffer.getNumSamples();
    const float* src = currentBuffer.getReadPointer(0);

    for (int n = 0; n < numOut; ++n)
    {
        float v = 0.0f;
        if (currentPosition < N)
            v = src[currentPosition++];
        outL[n] = v;
        if (outR)
            outR[n] = v;
    }

    if (currentPosition >= N)
        currentBuffer.setSize(0, 0); // finished
}

// =========================================================
// VELVET SHAKER
// =========================================================

void VariationPlayer::triggerVelvetVariation()
{
    if (samplePool == nullptr)
        return;

    const juce::AudioSampleBuffer* chosen = samplePool->getRandomSample();
    if (chosen == nullptr || chosen->getNumSamples() == 0)
        return;

    const int N = chosen->getNumSamples();
    currentBuffer.setSize(1, N);
    currentBuffer.copyFrom(0, 0, *chosen, 0, 0, N);

    // Apply velvet noise decorrelation
    applyVelvetNoise(currentBuffer);

    currentPosition = 0;
}

void VariationPlayer::applyVelvetNoise(juce::AudioBuffer<float>& buffer)
{
    const int N = buffer.getNumSamples();
    if (N <= 0)
        return;

    // --- PARAMETERS ---
    const int numTaps = params.velvetNumTaps;
    const float strength = params.velvetStrength;
    const float minDelayMs = params.velvetMinDelayMs;
    const float maxDelayMs = params.velvetMaxDelayMs;

    const double sr = params.sampleRate;
    const int minDelaySamps = (int)std::round(minDelayMs * sr * 0.001);
    const int maxDelaySamps = (int)std::round(maxDelayMs * sr * 0.001);

    juce::Random rng;

    // make copy of original input
    juce::AudioBuffer<float> orig(buffer);
    float* x = buffer.getWritePointer(0);

    for (int t = 0; t < numTaps; ++t)
    {
        const int delay = rng.nextInt({ minDelaySamps, maxDelaySamps });
        const float sign = rng.nextBool() ? 1.0f : -1.0f;

        for (int n = delay; n < N; ++n)
            x[n] += strength * sign * orig.getSample(0, n - delay);
    }

    // mild normalization to avoid clipping
    buffer.applyGain(0.95f);
}
