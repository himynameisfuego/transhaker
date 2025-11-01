#pragma once

#include <JuceHeader.h>
#include "SamplePool.h"
#include "VariationPlayer.h"

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

    void prepareToPlay(int samplesPerBlockExpected, double sampleRate) override;
    void getNextAudioBlock(const juce::AudioSourceChannelInfo& bufferToFill) override;
    void releaseResources() override;

    void paint(juce::Graphics& g) override;
    void resized() override;

    bool isInterestedInFileDrag(const juce::StringArray& files) override;
    void filesDropped(const juce::StringArray& files, int x, int y) override;

    void buttonClicked(juce::Button* b) override;
    void sliderValueChanged(juce::Slider* s) override;

private:
    // === UI elements ===
    juce::ImageComponent titleImage;
    juce::Label filesLabel{ {}, "No samples loaded" };

    juce::TextButton triggerButton{ "SHAKER" };
    juce::TextButton exportButton{ "EXPORT BATCH" };

    // Mode selector
    juce::ComboBox modeSelector;

    // --- OG Shaker sliders ---
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

    // --- Velvet Shaker sliders ---
    juce::Slider velvetStrengthSlider;
    juce::Slider velvetDelaySlider;
    juce::Label velvetStrengthLabel{ {}, "Velvet Strength" };
    juce::Label velvetDelayLabel{ {}, "Delay Range (ms)" };

    // container for laying out the rows
    juce::Component slidersGroup;

    // processing
    SamplePool                  samplePool;
    VariationPlayer             variationPlayer;
    VariationPlayer::Params     currentParams;
    std::unique_ptr<OfflineRenderer> offlineRenderer;

    void updateVisibleSliders();
    bool isVelvetMode() const;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(MainComponent)
};
