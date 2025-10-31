#pragma once

#include <JuceHeader.h>
#include "SamplePool.h"
#include "VariationPlayer.h"

// OfflineRenderer:
// - takes the same params and samplePool as the live engine
// - repeatedly generates randomized hits
// - writes them as individual WAV files
class OfflineRenderer
{
public:
    OfflineRenderer() = default;

    // must be called before renderBatch()
    void setSamplePool(SamplePool* p) { pool = p; }
    void setParams(const VariationPlayer::Params& p) { params = p; }

    // Render N variations into chosen folder.
    // For now:
    //   - we pop up a chooser
    //   - we hardcode N = 20
    void renderBatch(juce::Component* parentForChooser);

private:
    // render ONE randomized hit into a buffer and return it
    // (basically what VariationPlayer does in realtime, but offline)
    bool renderSingleVariation(juce::AudioBuffer<float>& outBuffer,
        double& outSampleRate);

    SamplePool* pool = nullptr;
    VariationPlayer::Params params;
    juce::Random rng;
    std::unique_ptr<juce::FileChooser> activeChooser;
};
