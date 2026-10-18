#include "PluginEditor.h"
#include "Shaders.h"

LumivoreEditor::LumivoreEditor (LumivoreProcessor& p) : AudioProcessorEditor (p), proc (p), viz (p)
{
    addAndMakeVisible (viz);
    addAndMakeVisible (prev); addAndMakeVisible (next);
    prev.onClick = [this] { stepScene (-1); };
    next.onClick = [this] { stepScene (+1); };
    sceneLabel.setJustificationType (juce::Justification::centred);
    sceneLabel.setColour (juce::Label::textColourId, juce::Colours::white);
    addAndMakeVisible (sceneLabel);

    paletteBox.addItemList (lumivore::paletteNames(), 1);
    addAndMakeVisible (paletteBox);
    aPal = std::make_unique<CA> (proc.apvts, ParamID::palette, paletteBox);

    auto setup = [this] (juce::Slider& s, juce::Label& l)
    {
        s.setSliderStyle (juce::Slider::LinearHorizontal);
        s.setTextBoxStyle (juce::Slider::TextBoxRight, false, 48, 18);
        l.setColour (juce::Label::textColourId, juce::Colour (0xffa0a8b8));
        l.setFont (juce::Font (juce::FontOptions (12.0f)));
        addAndMakeVisible (s); addAndMakeVisible (l);
    };
    setup (intensity, lIntensity); setup (bass, lBass); setup (mids, lMids); setup (highs, lHighs);
    setup (zoom, lZoom); setup (symmetry, lSym); setup (rotation, lRot);
    aInt  = std::make_unique<SA> (proc.apvts, ParamID::intensity, intensity);
    aBass = std::make_unique<SA> (proc.apvts, ParamID::sensBass, bass);
    aMid  = std::make_unique<SA> (proc.apvts, ParamID::sensMid, mids);
    aHigh = std::make_unique<SA> (proc.apvts, ParamID::sensHigh, highs);
    aZoom = std::make_unique<SA> (proc.apvts, ParamID::zoom, zoom);
    aSym  = std::make_unique<SA> (proc.apvts, ParamID::symmetry, symmetry);
    aRot  = std::make_unique<SA> (proc.apvts, ParamID::rotation, rotation);

    setResizable (true, true);
    setResizeLimits (640, 420, 3840, 2160);
    setSize (960, 620);
    startTimerHz (20);
    timerCallback();
}

LumivoreEditor::~LumivoreEditor() { stopTimer(); }

void LumivoreEditor::stepScene (int d)
{
    auto* prm = proc.apvts.getParameter (ParamID::scene);
    const int n = (int) lumivore::scenes().size();
    const int cur = juce::roundToInt (prm->convertFrom0to1 (prm->getValue()));
    prm->beginChangeGesture();
    prm->setValueNotifyingHost (prm->convertTo0to1 ((float) ((cur + d + n) % n)));
    prm->endChangeGesture();
}

void LumivoreEditor::timerCallback()   // keeps the label in sync with host automation
{
    const int i = (int) proc.apvts.getRawParameterValue (ParamID::scene)->load();
    sceneLabel.setText (lumivore::sceneNames()[i], juce::dontSendNotification);
}

void LumivoreEditor::paint (juce::Graphics& g) { g.fillAll (juce::Colour (0xff0e0f14)); }

void LumivoreEditor::resized()
{
    auto r = getLocalBounds();
    auto panel = r.removeFromBottom (136).reduced (10, 6);
    viz.setBounds (r);

    auto top = panel.removeFromTop (28);
    prev.setBounds (top.removeFromLeft (32)); top.removeFromLeft (6);
    next.setBounds (top.removeFromRight (32)); top.removeFromRight (6);
    paletteBox.setBounds (top.removeFromRight (120)); top.removeFromRight (6);
    sceneLabel.setBounds (top);

    panel.removeFromTop (6);
    const int colW = panel.getWidth() / 2;
    auto left = panel.removeFromLeft (colW), right = panel;
    auto row = [] (juce::Rectangle<int>& area, juce::Label& l, juce::Slider& s)
    { auto rr = area.removeFromTop (22); l.setBounds (rr.removeFromLeft (64)); s.setBounds (rr); };
    row (left, lIntensity, intensity); row (left, lBass, bass); row (left, lMids, mids); row (left, lRot, rotation);
    row (right, lHighs, highs); row (right, lZoom, zoom); row (right, lSym, symmetry);
}
