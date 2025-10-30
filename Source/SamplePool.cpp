#include "SamplePool.h"

bool SamplePool::loadFile(const juce::File& file)
{
    if (!file.existsAsFile())
        return false;

    // We only care about audio formats JUCE can read (wav, aiff, etc.).
    static juce::AudioFormatManager fm;
    static bool fmInit = false;
    if (!fmInit)
    {
        fm.registerBasicFormats(); // wav, aiff, etc.
        fmInit = true;
    }

    std::unique_ptr<juce::AudioFormatReader> reader(fm.createReaderFor(file));
    if (reader.get() == nullptr)
        return false;

    // We'll force mono for now, because Transhaker
    // will treat these as percussive hits / footsteps etc.
    const int numSamples = (int)reader->lengthInSamples;
    if (numSamples <= 0)
        return false;

    SampleData data;
    data.sampleRate = reader->sampleRate;
    data.name = file.getFileNameWithoutExtension();
    data.buffer.setSize(1, numSamples);

    // Read either left channel or mixdown to mono if needed:
    juce::AudioSampleBuffer tempBuf((int)reader->numChannels, numSamples);
    reader->read(&tempBuf,
        0,
        numSamples,
        0,
        true,   // fillLeft
        true);  // fillRight

    // If file is mono already: just copy that.
    if (tempBuf.getNumChannels() == 1)
    {
        data.buffer.copyFrom(0, 0,
            tempBuf,
            0, 0,
            numSamples);
    }
    else
    {
        // simple L+R average -> mono
        auto* dest = data.buffer.getWritePointer(0);
        auto* ch0 = tempBuf.getReadPointer(0);
        auto* ch1 = tempBuf.getReadPointer(1);
        for (int i = 0; i < numSamples; ++i)
            dest[i] = 0.5f * (ch0[i] + ch1[i]);
    }

    samples.push_back(std::move(data));
    return true;
}

int SamplePool::loadFiles(const juce::StringArray& paths)
{
    int loadedCount = 0;
    for (auto& p : paths)
    {
        juce::File f(p);
        if (loadFile(f))
            ++loadedCount;
    }
    return loadedCount;
}

const juce::AudioSampleBuffer* SamplePool::getRandomSample()
{
    if (samples.empty())
        return nullptr;

    const int idx = rng.nextInt((int)samples.size());
    return &samples[(size_t)idx].buffer;
}
