#include "AudioFileWriter.h"

namespace AudioFileWriter
{
    bool writeMonoBufferToWav(const juce::AudioBuffer<float>& buffer,
        double sampleRate,
        const juce::File& fileToWrite)
    {
        juce::WavAudioFormat wav;

        // createOutputStream() will overwrite if file exists
        std::unique_ptr<juce::FileOutputStream> out(fileToWrite.createOutputStream());
        if (!out)
            return false;

        // 24-bit mono WAV
        std::unique_ptr<juce::AudioFormatWriter> writer(
            wav.createWriterFor(out.get(),
                sampleRate,
                1,          // numChannels
                24,         // bitsPerSample
                {},         // metadata
                0)
        );

        if (!writer)
            return false;

        // The writer now owns the stream
        out.release();

        writer->writeFromAudioSampleBuffer(buffer, 0, buffer.getNumSamples());
        return true;
    }
}
