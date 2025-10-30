#include "MainComponent.h"

// Temporary stub until we implement it properly:
class OfflineRenderer
{
public:
    void renderBatch() {}
};

// =======================================
// Constructor / destructor
// =======================================

MainComponent::MainComponent()
{
    // helper lambda for consistent slider setup
    auto setupSlider = [](juce::Slider& s, juce::Slider::Listener* l,
        double min, double max, double init)
        {
            s.setRange(min, max, 0.1);
            s.setValue(init);
            s.setSliderStyle(juce::Slider::LinearHorizontal);
            s.setTextBoxStyle(juce::Slider::TextBoxRight, false, 60, 20);
            s.addListener(l);
        };

    setupSlider(pitchRangeSlider, this, 0.0, 10.0, 5.0);    // ±5 %
    setupSlider(gainRangeSlider, this, 0.0, 6.0, 3.0);    // ±3 dB
    setupSlider(offsetMsSlider, this, 0.0, 15.0, 5.0);    // up to 5 ms start offset
    setupSlider(lpfMinSlider, this, 2000.0, 20000.0, 4000.0); // Hz
    setupSlider(lpfMaxSlider, this, 2000.0, 20000.0, 12000.0);// Hz

    triggerButton.addListener(this);
    exportButton.addListener(this);

    titleLabel.setFont(juce::Font(20.0f, juce::Font::bold));
    titleLabel.setJustificationType(juce::Justification::centred);

    addAndMakeVisible(titleLabel);
    addAndMakeVisible(filesLabel);

    addAndMakeVisible(triggerButton);
    addAndMakeVisible(exportButton);

    addAndMakeVisible(pitchLabel);
    addAndMakeVisible(gainLabel);
    addAndMakeVisible(offsetLabel);
    addAndMakeVisible(lpfMinLabel);
    addAndMakeVisible(lpfMaxLabel);

    addAndMakeVisible(pitchRangeSlider);
    addAndMakeVisible(gainRangeSlider);
    addAndMakeVisible(offsetMsSlider);
    addAndMakeVisible(lpfMinSlider);
    addAndMakeVisible(lpfMaxSlider);

    // Tell our player where to get audio from
    variationPlayer.setSamplePool(&samplePool);

    // Initialize parameter struct from sliders
    currentParams.pitchPercentRange = (float)pitchRangeSlider.getValue();
    currentParams.gainDbRange = (float)gainRangeSlider.getValue();
    currentParams.maxOffsetMs = (float)offsetMsSlider.getValue();
    currentParams.lpfMinHz = (float)lpfMinSlider.getValue();
    currentParams.lpfMaxHz = (float)lpfMaxSlider.getValue();
    // sampleRate gets filled in prepareToPlay

    variationPlayer.setParams(currentParams);

    // Request 0 inputs, 2 outputs
    setAudioChannels(0, 2);


    setSize(800, 500);
}

MainComponent::~MainComponent()
{
    shutdownAudio();
}

// =======================================
// AudioAppComponent overrides
// =======================================

void MainComponent::prepareToPlay(int samplesPerBlockExpected, double sampleRate)
{
    variationPlayer.prepareToPlay(samplesPerBlockExpected, sampleRate);

    // update param block with correct samplerate
    currentParams.sampleRate = sampleRate;
    variationPlayer.setParams(currentParams);
}


void MainComponent::getNextAudioBlock(const juce::AudioSourceChannelInfo& bufferToFill)
{
    variationPlayer.getNextAudioBlock(bufferToFill);
}

void MainComponent::releaseResources()
{
    variationPlayer.releaseResources();
}

// =======================================
// Painting / layout
// =======================================

void MainComponent::paint(juce::Graphics& g)
{
    g.fillAll(juce::Colours::black);

    g.setColour(juce::Colours::white);
    g.drawText("Drag & drop your footstep / impact .wav files anywhere in this window.",
        getLocalBounds().reduced(10),
        juce::Justification::centredTop,
        true);
}

void MainComponent::resized()
{
    auto area = getLocalBounds().reduced(10);

    titleLabel.setBounds(area.removeFromTop(30));
    filesLabel.setBounds(area.removeFromTop(20));

    auto topRow = area.removeFromTop(30);
    triggerButton.setBounds(topRow.removeFromLeft(120));
    exportButton.setBounds(topRow.removeFromLeft(140));

    auto sliderRowHeight = 40;

    auto placeRow = [&area, sliderRowHeight](juce::Label& lab, juce::Slider& s)
        {
            auto row = area.removeFromTop(sliderRowHeight);
            auto left = row.removeFromLeft(200);
            lab.setBounds(left);
            s.setBounds(row);
        };

    placeRow(pitchLabel, pitchRangeSlider);
    placeRow(gainLabel, gainRangeSlider);
    placeRow(offsetLabel, offsetMsSlider);
    placeRow(lpfMinLabel, lpfMinSlider);
    placeRow(lpfMaxLabel, lpfMaxSlider);
}

// =======================================
// Drag & drop
// =======================================

bool MainComponent::isInterestedInFileDrag(const juce::StringArray& files)
{
    juce::ignoreUnused(files);
    return true;
}

void MainComponent::filesDropped(const juce::StringArray& files, int, int)
{
    const int loadedNow = samplePool.loadFiles(files);

    filesLabel.setText(
        juce::String(loadedNow) + " new file(s) loaded, total " +
        juce::String(samplePool.getNumSamples()) + " sample(s)",
        juce::dontSendNotification
    );
}

// =======================================
// UI callbacks
// =======================================

void MainComponent::buttonClicked(juce::Button* b)
{
    if (b == &triggerButton)
    {
        // Play one random loaded sample
        variationPlayer.triggerRandom();
    }
    else if (b == &exportButton)
    {
        if (!offlineRenderer)
            offlineRenderer = std::make_unique<OfflineRenderer>();

        offlineRenderer->renderBatch();
    }
}

void MainComponent::sliderValueChanged(juce::Slider* s)
{
    juce::ignoreUnused(s);

    // sync UI -> param struct
    currentParams.pitchPercentRange = (float)pitchRangeSlider.getValue();
    currentParams.gainDbRange = (float)gainRangeSlider.getValue();
    currentParams.maxOffsetMs = (float)offsetMsSlider.getValue();
    currentParams.lpfMinHz = (float)lpfMinSlider.getValue();
    currentParams.lpfMaxHz = (float)lpfMaxSlider.getValue();

    // sampleRate stays whatever prepareToPlay set

    variationPlayer.setParams(currentParams);
}

