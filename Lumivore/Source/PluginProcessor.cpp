#include "PluginProcessor.h"
#include "PluginEditor.h"
#include "Shaders.h"

juce::AudioProcessorValueTreeState::ParameterLayout LumivoreProcessor::createLayout()
{
    using namespace juce;
    AudioProcessorValueTreeState::ParameterLayout l;
    l.add (std::make_unique<AudioParameterChoice> (ParameterID { ParamID::scene, 1 }, "Scene", lumivore::sceneNames(), 0));
    l.add (std::make_unique<AudioParameterChoice> (ParameterID { ParamID::palette, 1 }, "Palette", lumivore::paletteNames(), 0));
    auto f = [&] (const char* id, const char* name, float lo, float hi, float def)
    { l.add (std::make_unique<AudioParameterFloat> (ParameterID { id, 1 }, name, NormalisableRange<float> (lo, hi, 0.001f), def)); };
    f (ParamID::intensity, "Intensity", 0.f, 2.f, 1.f);
    f (ParamID::sensBass,  "Bass Sensitivity", 0.f, 3.f, 1.f);
    f (ParamID::sensMid,   "Mids Sensitivity", 0.f, 3.f, 1.f);
    f (ParamID::sensHigh,  "Highs Sensitivity", 0.f, 3.f, 1.f);
    f (ParamID::zoom,      "Zoom", 0.25f, 4.f, 1.f);
    f (ParamID::rotation,  "Rotation", 0.f, 360.f, 0.f);
    l.add (std::make_unique<AudioParameterInt> (ParameterID { ParamID::symmetry, 1 }, "Symmetry", 1, 12, 1));
    return l;
}

LumivoreProcessor::LumivoreProcessor()
    : AudioProcessor (BusesProperties().withInput ("Input", juce::AudioChannelSet::stereo(), true)
                                       .withOutput ("Output", juce::AudioChannelSet::stereo(), true)),
      apvts (*this, nullptr, "LUMIVORE_STATE", createLayout())
{
}

void LumivoreProcessor::prepareToPlay (double sr, int) { sampleRate.store (sr); }

bool LumivoreProcessor::isBusesLayoutSupported (const BusesLayout& l) const
{
    const auto out = l.getMainOutputChannelSet();
    return (out == juce::AudioChannelSet::mono() || out == juce::AudioChannelSet::stereo())
           && out == l.getMainInputChannelSet();
}

// REAL-TIME THREAD: no allocation, no locks, no graphics. Audio passes through untouched (in place);
// a mono copy is pushed into a lock-free FIFO and silently dropped if the visualizer isn't consuming.
void LumivoreProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer&)
{
    juce::ScopedNoDenormals noDenormals;
    const int nCh = juce::jmin (getTotalNumInputChannels(), buffer.getNumChannels());
    const int n = buffer.getNumSamples();
    for (int ch = nCh; ch < buffer.getNumChannels(); ++ch) buffer.clear (ch, 0, n);
    if (nCh == 0) return;

    const float inv = 1.0f / (float) nCh;
    for (int i = 0; i < n; ++i)
    {
        float s = 0.f;
        for (int ch = 0; ch < nCh; ++ch) s += buffer.getReadPointer (ch)[i];
        fifo.push (s * inv);
    }
}

juce::AudioProcessorEditor* LumivoreProcessor::createEditor() { return new LumivoreEditor (*this); }

void LumivoreProcessor::getStateInformation (juce::MemoryBlock& dest)
{
    if (auto xml = apvts.copyState().createXml()) copyXmlToBinary (*xml, dest);
}

void LumivoreProcessor::setStateInformation (const void* data, int size)
{
    if (auto xml = getXmlFromBinary (data, size))
        if (xml->hasTagName (apvts.state.getType())) apvts.replaceState (juce::ValueTree::fromXml (*xml));
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter() { return new LumivoreProcessor(); }
