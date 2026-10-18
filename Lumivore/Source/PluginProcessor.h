#pragma once
#include <juce_audio_processors/juce_audio_processors.h>
#include "AudioCore.h"

namespace ParamID
{
    inline const char* scene = "scene";       inline const char* intensity = "intensity";
    inline const char* sensBass = "sens_bass"; inline const char* sensMid = "sens_mid";
    inline const char* sensHigh = "sens_high"; inline const char* palette = "palette";
    inline const char* zoom = "zoom";          inline const char* symmetry = "symmetry";
    inline const char* rotation = "rotation";
}

class LumivoreProcessor : public juce::AudioProcessor
{
public:
    LumivoreProcessor();
    ~LumivoreProcessor() override = default;

    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override {}
    bool isBusesLayoutSupported (const BusesLayout&) const override;
    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }

    const juce::String getName() const override { return "Lumivore"; }
    bool acceptsMidi() const override { return false; }
    bool producesMidi() const override { return false; }
    double getTailLengthSeconds() const override { return 0.0; }
    int getNumPrograms() override { return 1; }
    int getCurrentProgram() override { return 0; }
    void setCurrentProgram (int) override {}
    const juce::String getProgramName (int) override { return {}; }
    void changeProgramName (int, const juce::String&) override {}

    void getStateInformation (juce::MemoryBlock&) override;
    void setStateInformation (const void*, int) override;

    double getCurrentSampleRate() const noexcept { return sampleRate.load(); }

    juce::AudioProcessorValueTreeState apvts;
    lumivore::SpscFifo<float, 1 << 16> fifo;   // audio thread -> render thread

private:
    static juce::AudioProcessorValueTreeState::ParameterLayout createLayout();
    std::atomic<double> sampleRate { 44100.0 };
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (LumivoreProcessor)
};
