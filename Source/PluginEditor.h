#pragma once
#include "PluginProcessor.h"
#include "VisualizerComponent.h"

class LumivoreEditor : public juce::AudioProcessorEditor, private juce::Timer
{
public:
    explicit LumivoreEditor (LumivoreProcessor&);
    ~LumivoreEditor() override;
    void paint (juce::Graphics&) override;
    void resized() override;

private:
    void timerCallback() override;
    void stepScene (int delta);

    LumivoreProcessor& proc;
    VisualizerComponent viz;
    juce::TextButton prev { "<" }, next { ">" };
    juce::Label sceneLabel;
    juce::ComboBox paletteBox;
    juce::Slider intensity, bass, mids, highs, zoom, symmetry, rotation;
    juce::Label  lIntensity { {}, "Intensity" }, lBass { {}, "Bass" }, lMids { {}, "Mids" }, lHighs { {}, "Highs" },
                 lZoom { {}, "Zoom" }, lSym { {}, "Symmetry" }, lRot { {}, "Rotate" };
    using SA = juce::AudioProcessorValueTreeState::SliderAttachment;
    using CA = juce::AudioProcessorValueTreeState::ComboBoxAttachment;
    std::unique_ptr<SA> aInt, aBass, aMid, aHigh, aZoom, aSym, aRot;   // declared after widgets -> destroyed first
    std::unique_ptr<CA> aPal;
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (LumivoreEditor)
};
