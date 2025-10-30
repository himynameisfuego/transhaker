#include "VariationPlayer.h"
#include <cmath>

static inline float dbToLinear(float db)
{
    // linear = 10^(db/20)
    return std::pow(10.0f, db / 20.0f);
}

// simple linear interpolation helper
static inline float lerpSample(const juce::AudioSampleBuffer& buf, int channel, double index)
{
    // guard if buffer empty
    const int numSamples = buf.getNumSamples();
    if (numSamples <= 0)
        return 0.0f;

    // base index
    int i0 = (int)index;
    int i1 = i0 + 1;
    if (i0 >= numSamples) i0 = numSamples - 1;
    if (i1 >= numSamples) i1 = numSamples - 1;

    const float frac = (float)(index - (double)i0);

    const float s0 = buf.getSample(channel, i0);
    const float s1 = buf.getSample(channel, i1);

    return s0 + frac * (s1 - s0);
}

void VariationPlayer::prepareToPlay(int samplesPerBlockExpected, double sampleRate)
{
    juce::ignoreUnused(samplesPerBlockExpected);
    const juce::ScopedLock sl(lock);

    currentSampleRate = sampleRate;
    params.sampleRate = sampleRate;

    lpf.prepare(sampleRate);
    lpf.setCutoff(10000.0f); // default cutoff

    playPos = 0.0;
    totalLen = 0;
    gainLinear = 1.0f;
    startOffsetSamps = 0;
    playbackRate = 1.0;
    currentCutoffHz = 10000.0f;
}

void VariationPlayer::releaseResources()
{
}

void VariationPlayer::getNextAudioBlock(const juce::AudioSourceChannelInfo& info)
{
    info.clearActiveBufferRegion();

    const juce::ScopedLock sl(lock);

    if (totalLen <= 0 || currentSample.getNumSamples() == 0)
        return;

    auto* outL = info.buffer->getWritePointer(0, info.startSample);
    auto* outR = (info.buffer->getNumChannels() > 1)
        ? info.buffer->getWritePointer(1, info.startSample)
        : outL;

    // we'll treat currentSample as mono
    const int availableSamples = totalLen; // samples after offset

    for (int i = 0; i < info.numSamples; ++i)
    {
        // if we've reached the end, drop playback state
        if (playPos >= (double)availableSamples)
        {
            totalLen = 0;
            currentSample.setSize(0, 0); // mark "finished"
            break;
        }

        // read from buffer with startOffset + fractional position
        double absoluteIndex = (double)startOffsetSamps + playPos;

        float dry = lerpSample(currentSample, 0, absoluteIndex);

        float afterGain = dry * gainLinear;

        float l = lpf.processL(afterGain);
        float r = lpf.processR(afterGain);

        outL[i] = l;
        outR[i] = r;


        playPos += playbackRate;
    }
}

void VariationPlayer::triggerRandom()
{
    const juce::ScopedLock sl(lock);

    if (pool == nullptr)
        return;

    const juce::AudioSampleBuffer* chosen = pool->getRandomSample();
    if (chosen == nullptr)
        return;

    // Copy chosen sample into local buffer
    currentSample = *chosen;

    // ---------- Randomize gain ----------
    {
        const float spread = params.gainDbRange; // ±spread dB
        const float gainDb = rng.nextFloat() * (2.0f * spread) - spread;
        gainLinear = dbToLinear(gainDb);
    }

    // ---------- Randomize start offset ----------
    {
        const float maxMs = params.maxOffsetMs;
        const float offsMs = rng.nextFloat() * maxMs;  // [0..maxMs]
        startOffsetSamps = (int)std::round((offsMs / 1000.0f) * (float)params.sampleRate);

        if (startOffsetSamps >= currentSample.getNumSamples())
            startOffsetSamps = juce::jmax(0, currentSample.getNumSamples() - 1);
    }

    // ---------- Randomize pitch / playbackRate ----------
    {
        // params.pitchPercentRange e.g. 5.0 means ±5%
        float pct = params.pitchPercentRange;
        // rng in [-1..1]
        float sign = (rng.nextFloat() * 2.0f) - 1.0f;
        float pctDelta = sign * pct * 0.01f; // convert % to ratio

        // playbackRate = 1 + pctDelta
        playbackRate = 1.0 + (double)pctDelta;

        // clamp so we don't get silly slow/fast
        if (playbackRate < 0.5)  playbackRate = 0.5;
        if (playbackRate > 2.0)  playbackRate = 2.0;
    }

    // ---------- Randomize LPF cutoff ----------
    {
        // We'll pick a random cutoff between [lpfMinHz, lpfMaxHz]
        float minHz = params.lpfMinHz;
        float maxHz = params.lpfMaxHz;
        if (maxHz < minHz)
            std::swap(minHz, maxHz);

        const float r01 = rng.nextFloat(); // [0..1]
        currentCutoffHz = minHz + r01 * (maxHz - minHz);

        lpf.setCutoff(currentCutoffHz);
    }


    // ---------- Prepare playback state ----------
    playPos = 0.0;
    totalLen = currentSample.getNumSamples() - startOffsetSamps;
    if (totalLen < 0) totalLen = 0;
}
