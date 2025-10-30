#include "VariationPlayer.h"
#include <cmath>

static inline float dbToLinear(float db)
{
    // linear = 10^(db/20)
    return std::pow(10.0f, db / 20.0f);
}

void VariationPlayer::prepareToPlay(int samplesPerBlockExpected, double sampleRate)
{
    juce::ignoreUnused(samplesPerBlockExpected);
    const juce::ScopedLock sl(lock);

    currentSampleRate = sampleRate;
    params.sampleRate = sampleRate;

    readPos = 0;
    totalLen = 0;
    gainLinear = 1.0f;
    startOffsetSamps = 0;
    playbackRate = 1.0;
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

    for (int i = 0; i < info.numSamples; ++i)
    {
        if (readPos >= totalLen)
        {
            // finished playing the one-shot
            totalLen = 0;
            currentSample.setSize(0, 0); // drop buffer to mark "nothing armed"
            break;
        }

        // naive no-interp playbackRate==1.0 for now
        const int srcIndex = startOffsetSamps + readPos;
        if (srcIndex >= currentSample.getNumSamples())
        {
            totalLen = 0;
            currentSample.setSize(0, 0);
            break;
        }

        float s = currentSample.getSample(0, srcIndex);

        float out = s * gainLinear;

        outL[i] = out;
        outR[i] = out;

        ++readPos;
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

    // --- Randomize gain ---
    // pick gain delta in range [-gainDbRange, +gainDbRange]
    const float gainSpread = params.gainDbRange;
    const float gainDb = rng.nextFloat() * (2.0f * gainSpread) - gainSpread; // uniform [-spread..+spread]
    gainLinear = dbToLinear(gainDb);

    // --- Randomize start offset ---
    // convert ms -> samples
    const float maxMs = params.maxOffsetMs;
    const float offsMs = rng.nextFloat() * maxMs; // [0..maxMs]
    startOffsetSamps = (int)std::round((offsMs / 1000.0f) * (float)params.sampleRate);

    // clamp so we don't run past end
    if (startOffsetSamps >= currentSample.getNumSamples())
        startOffsetSamps = juce::jmax(0, currentSample.getNumSamples() - 1);

    // --- Future: Randomize pitch (playbackRate) & LPF cutoff
    playbackRate = 1.0; // we'll handle pitch later with resampling

    // Reset playback state
    readPos = 0;
    totalLen = currentSample.getNumSamples() - startOffsetSamps;
    if (totalLen < 0) totalLen = 0;
}
