#pragma once

#include <JuceHeader.h>
#include "SamplePool.h"
#include "VariationPlayer.h"

// We'll add a real OfflineRenderer later.
class OfflineRenderer;

class MainComponent
    : public juce::AudioAppComponent
    , public juce::FileDragAndDropTarget
    , public juce::Button::Listener
    , public juce::Slider::Listener
{
public:
    MainComponent();
    ~MainComponent() override;

    // AudioAppComponent
    void prepareToPlay(int samplesPerBlockExpected, double sampleRate) override;
    void getNextAudioBlock(const juce::AudioSourceChannelInfo& bufferToFill) override;
    void releaseResources() override;

    // Painting / layout
    void paint(juce::Graphics& g) override;
    void resized() override;

    // Drag & drop of audio files
    bool isInterestedInFileDrag(const juce::StringArray& files) override;
    void filesDropped(const juce::StringArray& files, int x, int y) override;

    // UI callbacks
    void buttonClicked(juce::Button* b) override;
    void sliderValueChanged(juce::Slider* s) override;

private:
    // === UI ===
    juce::Label titleLabel{ {}, "Transhaker" };
    juce::Label filesLabel{ {}, "No samples loaded" };

    juce::TextButton triggerButton{ "TRIGGER" };
    juce::TextButton exportButton{ "EXPORT BATCH" };

    juce::Slider pitchRangeSlider;
    juce::Slider gainRangeSlider;
    juce::Slider offsetMsSlider;
    juce::Slider lpfMinSlider;
    juce::Slider lpfMaxSlider;

    juce::Label  pitchLabel{ {}, "Pitch range (%)" };
    juce::Label  gainLabel{ {}, "Gain range (dB)" };
    juce::Label  offsetLabel{ {}, "Start offset max (ms)" };
    juce::Label  lpfMinLabel{ {}, "LPF min (Hz)" };
    juce::Label  lpfMaxLabel{ {}, "LPF max (Hz)" };

    // === audio / processing ===
    SamplePool      samplePool;
    VariationPlayer variationPlayer;
    VariationPlayer::Params currentParams;

    std::unique_ptr<OfflineRenderer> offlineRenderer;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(MainComponent)

};
