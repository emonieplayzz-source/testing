#pragma once
#include <juce_opengl/juce_opengl.h>
#include "PluginProcessor.h"

class VisualizerComponent : public juce::Component, private juce::OpenGLRenderer
{
public:
    explicit VisualizerComponent (LumivoreProcessor&);
    ~VisualizerComponent() override;
    void paint (juce::Graphics&) override;   // draws shader error text, if any

private:
    void newOpenGLContextCreated() override;
    void renderOpenGL() override;
    void openGLContextClosing() override;
    bool buildProgram (int scene);
    float param (const char* id) const { return apvts.getRawParameterValue (id)->load(); }

    LumivoreProcessor& proc;
    juce::AudioProcessorValueTreeState& apvts;
    juce::OpenGLContext ctx;
    std::unique_ptr<juce::OpenGLShaderProgram> program;
    lumivore::Analyzer analyzer;
    int builtScene = -1;
    unsigned int vao = 0, tex = 0;
    double startMs = 0;
    std::array<float, 512 * 2> texData {};

    juce::SpinLock errLock;
    juce::String errorText;
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (VisualizerComponent)
};
