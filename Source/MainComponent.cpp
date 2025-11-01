#include "MainComponent.h"
#include "OfflineRenderer.h"

// =========================================================
// Constructor / Destructor
// =========================================================

MainComponent::MainComponent()
{
    // --- Title image ---
    {
        juce::Image logo = juce::ImageFileFormat::loadFrom(
            BinaryData::logotitleblack_png,
            BinaryData::logotitleblack_pngSize
        );
        titleImage.setImage(logo, juce::RectanglePlacement::centred);
        addAndMakeVisible(titleImage);
    }

    // --- Mode selector ---
    modeSelector.addItem("OG SHAKER", 1);
    modeSelector.addItem("VELVET SHAKER", 2);
    modeSelector.setSelectedId(1);
    addAndMakeVisible(modeSelector);

    modeSelector.onChange = [this]()
        {
            variationPlayer.setMode(isVelvetMode()
                ? VariationPlayer::Mode::Velvet
                : VariationPlayer::Mode::OG);

            updateVisibleSliders();
        };

    // --- Labels ---
    filesLabel.setJustificationType(juce::Justification::centred);
    filesLabel.setColour(juce::Label::textColourId, juce::Colours::black);

    auto makeLabelBlack = [](juce::Label& lab)
        {
            lab.setColour(juce::Label::textColourId, juce::Colours::black);
        };
    makeLabelBlack(pitchLabel);
    makeLabelBlack(gainLabel);
    makeLabelBlack(offsetLabel);
    makeLabelBlack(lpfMinLabel);
    makeLabelBlack(lpfMaxLabel);
    makeLabelBlack(velvetStrengthLabel);
    makeLabelBlack(velvetDelayLabel);

    // --- Sliders ---
    auto setupSlider = [](juce::Slider& s, juce::Slider::Listener* l,
        double min, double max, double init)
        {
            s.setRange(min, max, 0.1);
            s.setValue(init);
            s.setSliderStyle(juce::Slider::LinearHorizontal);
            s.setTextBoxStyle(juce::Slider::TextBoxRight, false, 60, 20);
            s.addListener(l);
        };

    setupSlider(pitchRangeSlider, this, 0.0, 10.0, 5.0);
    setupSlider(gainRangeSlider, this, 0.0, 6.0, 3.0);
    setupSlider(offsetMsSlider, this, 0.0, 15.0, 5.0);
    setupSlider(lpfMinSlider, this, 2000.0, 20000.0, 4000.0);
    setupSlider(lpfMaxSlider, this, 2000.0, 20000.0, 12000.0);

    setupSlider(velvetStrengthSlider, this, 0.0, 0.3, 0.08);
    setupSlider(velvetDelaySlider, this, 2.0, 20.0, 10.0);

    auto setSliderTextColors = [](juce::Slider& s)
        {
            s.setColour(juce::Slider::textBoxTextColourId, juce::Colours::black);
            s.setColour(juce::Slider::textBoxBackgroundColourId, juce::Colours::white.withAlpha(0.9f));
            s.setColour(juce::Slider::textBoxOutlineColourId, juce::Colours::black.withAlpha(0.2f));
        };
    setSliderTextColors(pitchRangeSlider);
    setSliderTextColors(gainRangeSlider);
    setSliderTextColors(offsetMsSlider);
    setSliderTextColors(lpfMinSlider);
    setSliderTextColors(lpfMaxSlider);
    setSliderTextColors(velvetStrengthSlider);
    setSliderTextColors(velvetDelaySlider);

    // --- Buttons ---
    triggerButton.addListener(this);
    exportButton.addListener(this);

    triggerButton.setColour(juce::TextButton::buttonColourId, juce::Colours::black);
    triggerButton.setColour(juce::TextButton::textColourOffId, juce::Colours::white);
    exportButton.setColour(juce::TextButton::buttonColourId, juce::Colours::black);
    exportButton.setColour(juce::TextButton::textColourOffId, juce::Colours::white);

    // --- Add components ---
    addAndMakeVisible(triggerButton);
    addAndMakeVisible(exportButton);
    addAndMakeVisible(slidersGroup);
    addAndMakeVisible(filesLabel);

    slidersGroup.addAndMakeVisible(pitchLabel);
    slidersGroup.addAndMakeVisible(pitchRangeSlider);
    slidersGroup.addAndMakeVisible(gainLabel);
    slidersGroup.addAndMakeVisible(gainRangeSlider);
    slidersGroup.addAndMakeVisible(offsetLabel);
    slidersGroup.addAndMakeVisible(offsetMsSlider);
    slidersGroup.addAndMakeVisible(lpfMinLabel);
    slidersGroup.addAndMakeVisible(lpfMinSlider);
    slidersGroup.addAndMakeVisible(lpfMaxLabel);
    slidersGroup.addAndMakeVisible(lpfMaxSlider);

    slidersGroup.addAndMakeVisible(velvetStrengthLabel);
    slidersGroup.addAndMakeVisible(velvetStrengthSlider);
    slidersGroup.addAndMakeVisible(velvetDelayLabel);
    slidersGroup.addAndMakeVisible(velvetDelaySlider);

    // --- DSP hookup ---
    variationPlayer.setSamplePool(&samplePool);

    // Params initialisation
    currentParams.pitchPercentRange = (float)pitchRangeSlider.getValue();
    currentParams.gainDbRange = (float)gainRangeSlider.getValue();
    currentParams.maxOffsetMs = (float)offsetMsSlider.getValue();
    currentParams.lpfMinHz = (float)lpfMinSlider.getValue();
    currentParams.lpfMaxHz = (float)lpfMaxSlider.getValue();
    currentParams.velvetStrength = (float)velvetStrengthSlider.getValue();
    currentParams.velvetMinDelayMs = 2.0f;
    currentParams.velvetMaxDelayMs = (float)velvetDelaySlider.getValue();
    currentParams.velvetNumTaps = 50;

    variationPlayer.setParams(currentParams);
    setAudioChannels(0, 2);
    setSize(800, 500);

    updateVisibleSliders();
}

MainComponent::~MainComponent() { shutdownAudio(); }

// =========================================================
// Audio
// =========================================================

void MainComponent::prepareToPlay(int samplesPerBlockExpected, double sampleRate)
{
    variationPlayer.prepareToPlay(samplesPerBlockExpected, sampleRate);
    currentParams.sampleRate = sampleRate;
    variationPlayer.setParams(currentParams);
    if (offlineRenderer)
        offlineRenderer->setParams(currentParams);
}

void MainComponent::getNextAudioBlock(const juce::AudioSourceChannelInfo& bufferToFill)
{
    variationPlayer.getNextAudioBlock(bufferToFill);
}

void MainComponent::releaseResources() { variationPlayer.releaseResources(); }

// =========================================================
// Paint / layout
// =========================================================

void MainComponent::paint(juce::Graphics& g)
{
    g.fillAll(juce::Colour::fromRGB(255, 249, 152));

    // --- signature text (moved slightly left) ---
    g.setColour(juce::Colours::black.withAlpha(0.6f));
    g.setFont(juce::Font(14.0f));
    auto sigArea = getLocalBounds().removeFromTop(30);
    sigArea.removeFromRight(20); // add right padding
    g.drawText("2025 Leonardo Fierro", sigArea.removeFromRight(180),
        juce::Justification::centredRight);

    // --- drag/drop text (leave margin for file label above it) ---
    g.setColour(juce::Colours::black);
    g.setFont(juce::Font(15.0f));
    juce::Rectangle<int> bottomArea = getLocalBounds();
    bottomArea.removeFromBottom(15);     // margin from window edge
    bottomArea.removeFromTop(bottomArea.getHeight() - 40); // reserve 40px tall zone
    g.drawText("Drag & drop your footstep / impact .wav files anywhere in this window.",
        bottomArea,
        juce::Justification::centredBottom,
        true);
}

void MainComponent::resized()
{
    auto area = getLocalBounds().reduced(12);

    auto titleArea = area.removeFromTop(180);
    titleImage.setBounds(titleArea.reduced(10, 5));

    auto comboRow = area.removeFromTop(40);
    modeSelector.setBounds(comboRow.removeFromLeft(200).reduced(5));

    auto buttonRow = area.removeFromTop(40);
    triggerButton.setBounds(buttonRow.removeFromLeft(140).reduced(4));
    exportButton.setBounds(buttonRow.removeFromLeft(160).reduced(4));

    auto sgArea = area.removeFromTop(200);
    slidersGroup.setBounds(sgArea.reduced(4));

    // Reserve consistent rows
    auto groupArea = slidersGroup.getLocalBounds().reduced(4);
    auto rowH = 30;

    auto layoutRow = [&groupArea, rowH](juce::Label& lab, juce::Slider& s)
        {
            auto row = groupArea.removeFromTop(rowH);
            lab.setBounds(row.removeFromLeft(200));
            s.setBounds(row);
        };

    if (!isVelvetMode())
    {
        layoutRow(pitchLabel, pitchRangeSlider);
        layoutRow(gainLabel, gainRangeSlider);
        layoutRow(offsetLabel, offsetMsSlider);
        layoutRow(lpfMinLabel, lpfMinSlider);
        layoutRow(lpfMaxLabel, lpfMaxSlider);
    }
    else
    {
        layoutRow(velvetStrengthLabel, velvetStrengthSlider);
        layoutRow(velvetDelayLabel, velvetDelaySlider);
    }

    // --- files label now sits ABOVE bottom drag text ---
    filesLabel.setBounds(area.removeFromBottom(60).reduced(10, 10));
}

// =========================================================
// File drag/drop
// =========================================================

bool MainComponent::isInterestedInFileDrag(const juce::StringArray&) { return true; }

void MainComponent::filesDropped(const juce::StringArray& files, int, int)
{
    int loadedNow = samplePool.loadFiles(files);
    filesLabel.setText(
        juce::String(loadedNow) + " new file(s) loaded, total " +
        juce::String(samplePool.getNumSamples()) + " sample(s)",
        juce::dontSendNotification);
}

// =========================================================
// Buttons / sliders
// =========================================================

void MainComponent::buttonClicked(juce::Button* b)
{
    if (b == &triggerButton)
        variationPlayer.triggerRandom();
    else if (b == &exportButton && offlineRenderer)
        offlineRenderer->renderBatch(this);
}

void MainComponent::sliderValueChanged(juce::Slider*)
{
    currentParams.pitchPercentRange = (float)pitchRangeSlider.getValue();
    currentParams.gainDbRange = (float)gainRangeSlider.getValue();
    currentParams.maxOffsetMs = (float)offsetMsSlider.getValue();
    currentParams.lpfMinHz = (float)lpfMinSlider.getValue();
    currentParams.lpfMaxHz = (float)lpfMaxSlider.getValue();
    currentParams.velvetStrength = (float)velvetStrengthSlider.getValue();
    currentParams.velvetMaxDelayMs = (float)velvetDelaySlider.getValue();
    currentParams.velvetMinDelayMs = 2.0f;
    currentParams.velvetNumTaps = 50;

    variationPlayer.setParams(currentParams);
    if (offlineRenderer)
        offlineRenderer->setParams(currentParams);
}

// =========================================================
// Visibility control
// =========================================================

bool MainComponent::isVelvetMode() const
{
    return (modeSelector.getSelectedId() == 2);
}

void MainComponent::updateVisibleSliders()
{
    const bool velvet = isVelvetMode();

    pitchRangeSlider.setVisible(!velvet);
    pitchLabel.setVisible(!velvet);
    gainRangeSlider.setVisible(!velvet);
    gainLabel.setVisible(!velvet);
    offsetMsSlider.setVisible(!velvet);
    offsetLabel.setVisible(!velvet);
    lpfMinSlider.setVisible(!velvet);
    lpfMinLabel.setVisible(!velvet);
    lpfMaxSlider.setVisible(!velvet);
    lpfMaxLabel.setVisible(!velvet);

    velvetStrengthSlider.setVisible(velvet);
    velvetStrengthLabel.setVisible(velvet);
    velvetDelaySlider.setVisible(velvet);
    velvetDelayLabel.setVisible(velvet);

    resized();
}
