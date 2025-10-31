#include "MainComponent.h"
#include "OfflineRenderer.h"

// =========================================================
// Constructor / Destructor
// =========================================================

MainComponent::MainComponent()
{
    //
    // 1. Configure labels
    //
    /*
    titleLabel.setFont(juce::Font(24.0f, juce::Font::bold));
    titleLabel.setJustificationType(juce::Justification::centred);
    titleLabel.setColour(juce::Label::textColourId, juce::Colours::black);
    */

    // Load logo from BinaryData
    juce::Image logo = juce::ImageFileFormat::loadFrom(BinaryData::logotitleblack_png,
        BinaryData::logotitleblack_pngSize);
    titleImage.setImage(logo, juce::RectanglePlacement::centred);
    addAndMakeVisible(titleImage);


    filesLabel.setJustificationType(juce::Justification::centred);
    filesLabel.setColour(juce::Label::textColourId, juce::Colours::black);

    pitchLabel.setColour(juce::Label::textColourId, juce::Colours::black);
    gainLabel.setColour(juce::Label::textColourId, juce::Colours::black);
    offsetLabel.setColour(juce::Label::textColourId, juce::Colours::black);
    lpfMinLabel.setColour(juce::Label::textColourId, juce::Colours::black);
    lpfMaxLabel.setColour(juce::Label::textColourId, juce::Colours::black);

    //
    // 2. Configure sliders
    //
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

    // enforce black numbers on white background for each slider
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


    // Make slider look good on lemon background:
    auto& laf = getLookAndFeel();
    laf.setColour(juce::Slider::trackColourId, juce::Colours::black.withAlpha(0.5f));
    laf.setColour(juce::Slider::thumbColourId, juce::Colours::black);
    laf.setColour(juce::Slider::textBoxTextColourId, juce::Colours::black);
    laf.setColour(juce::Slider::textBoxBackgroundColourId, juce::Colours::white.withAlpha(0.9f));
    laf.setColour(juce::Slider::textBoxOutlineColourId, juce::Colours::black.withAlpha(0.2f));

    //
    // 3. Configure buttons
    //
    triggerButton.addListener(this);
    exportButton.addListener(this);

    triggerButton.setColour(juce::TextButton::buttonColourId, juce::Colours::black);
    triggerButton.setColour(juce::TextButton::textColourOffId, juce::Colours::white);
    triggerButton.setColour(juce::TextButton::textColourOnId, juce::Colours::white);

    exportButton.setColour(juce::TextButton::buttonColourId, juce::Colours::black);
    exportButton.setColour(juce::TextButton::textColourOffId, juce::Colours::white);
    exportButton.setColour(juce::TextButton::textColourOnId, juce::Colours::white);

    //
    // 4. Add child components to display
    //
    //addAndMakeVisible(titleLabel);
    addAndMakeVisible(triggerButton);
    addAndMakeVisible(exportButton);

    // The sliders + their labels will be placed inside slidersGroup
    addAndMakeVisible(slidersGroup);

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

    addAndMakeVisible(filesLabel);

    //
    // 5. Audio engine hookup
    //
    variationPlayer.setSamplePool(&samplePool);

    // Initialize parameter struct from sliders
    currentParams.pitchPercentRange = (float)pitchRangeSlider.getValue();
    currentParams.gainDbRange = (float)gainRangeSlider.getValue();
    currentParams.maxOffsetMs = (float)offsetMsSlider.getValue();
    currentParams.lpfMinHz = (float)lpfMinSlider.getValue();
    currentParams.lpfMaxHz = (float)lpfMaxSlider.getValue();
    // sampleRate filled in prepareToPlay()

    variationPlayer.setParams(currentParams);
    offlineRenderer = std::make_unique<OfflineRenderer>();
    offlineRenderer->setSamplePool(&samplePool);
    offlineRenderer->setParams(currentParams);

    // Request 0 inputs, 2 outputs
    setAudioChannels(0, 2);

    setSize(800, 500);
}

MainComponent::~MainComponent()
{
    shutdownAudio();
}

// =========================================================
// AudioAppComponent
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

void MainComponent::releaseResources()
{
    variationPlayer.releaseResources();
}

// =========================================================
// Painting / layout
// =========================================================

void MainComponent::paint(juce::Graphics& g)
{
    // Lemon background
    g.fillAll(juce::Colour::fromRGB(255, 249, 152)); // light yellow

    // --- top-right signature ---
    {
        g.setColour(juce::Colours::black.withAlpha(0.6f));
        g.setFont(juce::Font(14.0f, juce::Font::plain));

        auto bounds = getLocalBounds().reduced(10);
        auto topRightArea = juce::Rectangle<int>(
            bounds.getRight() - 200, // x
            bounds.getY(),           // y (top)
            190,                     // width
            30                       // height
        );

        g.drawText("2025 Leonardo Fierro",
            topRightArea,
            juce::Justification::centredRight,
            false);
    }

    // Bottom instruction text
    g.setColour(juce::Colours::black);
    g.setFont(juce::Font(15.0f, juce::Font::plain));
    g.drawText("Drag & drop your .wav files anywhere in this window.",
        getLocalBounds().reduced(10),
        juce::Justification::centredBottom,
        true);
}


void MainComponent::resized()
{
    // We'll lay out in vertical zones:
    // [ title ]
    // [ buttons row ]
    // [ slidersGroup ]
    // [ filesLabel status ]
    // [ bottom text painted in paint() ]

    auto area = getLocalBounds().reduced(12);

    // Title at top
    //auto titleArea = area.removeFromTop(40);
    //titleLabel.setBounds(titleArea);
    auto titleArea = area.removeFromTop(180); // give it more height
    titleImage.setBounds(titleArea.reduced(10,5)); // center nicely



    // Buttons row under title
    auto buttonRow = area.removeFromTop(40);
    {
        auto left = buttonRow.removeFromLeft(140);
        triggerButton.setBounds(left.reduced(4));

        auto right = buttonRow.removeFromLeft(160);
        exportButton.setBounds(right.reduced(4));
    }

    // Sliders group block
    auto slidersArea = area.removeFromTop(160);
    slidersGroup.setBounds(slidersArea.reduced(4));

    // Inside slidersGroup, we stack rows like:
    // [label | slider]
    {
        auto sgArea = slidersGroup.getLocalBounds().reduced(4);
        auto rowH = 30;

        auto placeRow = [&sgArea, rowH](juce::Label& lab, juce::Slider& s)
            {
                auto row = sgArea.removeFromTop(rowH);
                auto labW = 200;
                lab.setBounds(row.removeFromLeft(labW));
                s.setBounds(row);
            };

        placeRow(pitchLabel, pitchRangeSlider);
        placeRow(gainLabel, gainRangeSlider);
        placeRow(offsetLabel, offsetMsSlider);
        placeRow(lpfMinLabel, lpfMinSlider);
        placeRow(lpfMaxLabel, lpfMaxSlider);
    }

    // File status label (how many samples loaded)
    auto fileRow = area.removeFromTop(30);
    filesLabel.setBounds(fileRow);
}

// =========================================================
// Drag & drop
// =========================================================

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

// =========================================================
// UI callbacks
// =========================================================

void MainComponent::buttonClicked(juce::Button* b)
{
    if (b == &triggerButton)
    {
        // Play one random, randomized variation
        variationPlayer.triggerRandom();
    }
    else if (b == &exportButton)
    {
        DBG("Export button pressed");
        if (offlineRenderer)
        {
            DBG("Calling offlineRenderer->renderBatch");
            offlineRenderer->renderBatch(this);
        }
        else
        {
            DBG("offlineRenderer is null!");
        }
    }

}

void MainComponent::sliderValueChanged(juce::Slider* s)
{
    juce::ignoreUnused(s);

    // sync GUI -> param struct
    currentParams.pitchPercentRange = (float)pitchRangeSlider.getValue();
    currentParams.gainDbRange = (float)gainRangeSlider.getValue();
    currentParams.maxOffsetMs = (float)offsetMsSlider.getValue();
    currentParams.lpfMinHz = (float)lpfMinSlider.getValue();
    currentParams.lpfMaxHz = (float)lpfMaxSlider.getValue();
    // sampleRate already stored in currentParams.sampleRate

    variationPlayer.setParams(currentParams);
    if (offlineRenderer)
        offlineRenderer->setParams(currentParams);
}
