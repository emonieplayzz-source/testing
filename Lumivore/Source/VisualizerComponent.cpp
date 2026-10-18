#include "VisualizerComponent.h"
#include "Shaders.h"

using namespace juce::gl;

VisualizerComponent::VisualizerComponent (LumivoreProcessor& p) : proc (p), apvts (p.apvts)
{
    setOpaque (true);
    ctx.setOpenGLVersionRequired (juce::OpenGLContext::openGL3_2);
    ctx.setRenderer (this);
    ctx.setContinuousRepainting (true);
    ctx.setComponentPaintingEnabled (true);
    ctx.attachTo (*this);
}

VisualizerComponent::~VisualizerComponent()
{
    ctx.detach();   // stops the render thread and fires openGLContextClosing() before members die
}

void VisualizerComponent::paint (juce::Graphics& g)
{
    juce::String e; { const juce::SpinLock::ScopedLockType sl (errLock); e = errorText; }
    if (e.isNotEmpty())
    {
        g.fillAll (juce::Colour (0xff0b0b10));
        g.setColour (juce::Colours::orangered);
        g.setFont (juce::Font (juce::FontOptions (13.0f)));
        g.drawFittedText ("Shader error:\n" + e, getLocalBounds().reduced (12), juce::Justification::topLeft, 40);
    }
}

void VisualizerComponent::newOpenGLContextCreated()
{
    glGenVertexArrays (1, &vao);
    glGenTextures (1, &tex);
    glBindTexture (GL_TEXTURE_2D, tex);
    glTexImage2D (GL_TEXTURE_2D, 0, GL_R32F, 512, 2, 0, GL_RED, GL_FLOAT, nullptr);
    glTexParameteri (GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri (GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri (GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri (GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    analyzer.setSampleRate (proc.getCurrentSampleRate());
    proc.fifo.clearFromConsumer();            // drop stale audio from while the editor was closed
    startMs = juce::Time::getMillisecondCounterHiRes();
    builtScene = -1;
}

bool VisualizerComponent::buildProgram (int scene)
{
    builtScene = scene;
    program.reset();
    const auto& list = lumivore::scenes();
    const int idx = juce::jlimit (0, (int) list.size() - 1, scene);
    auto p = std::make_unique<juce::OpenGLShaderProgram> (ctx);
    const juce::String frag = juce::String (lumivore::commonSource()) + list[(size_t) idx].body;
    if (p->addVertexShader (lumivore::vertexSource()) && p->addFragmentShader (frag) && p->link())
    {
        program = std::move (p);
        { const juce::SpinLock::ScopedLockType sl (errLock); errorText.clear(); }
        juce::MessageManager::callAsync ([safe = juce::Component::SafePointer<VisualizerComponent> (this)] { if (safe) safe->repaint(); });
        return true;
    }
    { const juce::SpinLock::ScopedLockType sl (errLock); errorText = p->getLastError(); }
    juce::MessageManager::callAsync ([safe = juce::Component::SafePointer<VisualizerComponent> (this)] { if (safe) safe->repaint(); });
    return false;
}

void VisualizerComponent::renderOpenGL()
{
    const float scale = (float) ctx.getRenderingScale();
    const int w = juce::roundToInt (scale * (float) getWidth()), h = juce::roundToInt (scale * (float) getHeight());
    glViewport (0, 0, w, h);
    glClearColor (0.03f, 0.03f, 0.05f, 1.f);
    glClear (GL_COLOR_BUFFER_BIT);

    analyzer.setSampleRate (proc.getCurrentSampleRate());
    analyzer.pump (proc.fifo);
    const auto& f = analyzer.features();

    const int scene = (int) param (ParamID::scene);
    if (scene != builtScene) buildProgram (scene);
    if (program == nullptr) return;

    analyzer.logSpectrum (texData.data(), 512);
    for (int i = 0; i < 512; ++i) texData[(size_t) (512 + i)] = f.wave[(size_t) i] * 0.5f + 0.5f;
    glActiveTexture (GL_TEXTURE0);
    glBindTexture (GL_TEXTURE_2D, tex);
    glTexSubImage2D (GL_TEXTURE_2D, 0, 0, 0, 512, 2, GL_RED, GL_FLOAT, texData.data());

    program->use();
    program->setUniform ("iResolution", (float) w, (float) h);
    program->setUniform ("iTime", (float) ((juce::Time::getMillisecondCounterHiRes() - startMs) * 0.001));
    const float sb = param (ParamID::sensBass), sm = param (ParamID::sensMid), sh = param (ParamID::sensHigh);
    program->setUniform ("uBands", f.bass * sb, f.mid * sm, f.treble * sh, f.rms * 2.0f);
    program->setUniform ("uBeat", f.beatPulse);
    program->setUniform ("uIntensity", param (ParamID::intensity));
    program->setUniform ("uZoom", param (ParamID::zoom));
    program->setUniform ("uSym", (float) (int) param (ParamID::symmetry));
    program->setUniform ("uRot", juce::degreesToRadians (param (ParamID::rotation)));
    program->setUniform ("uPalette", (int) param (ParamID::palette));
    program->setUniform ("uAudio", 0);

    glBindVertexArray (vao);
    glDrawArrays (GL_TRIANGLES, 0, 3);
    glBindVertexArray (0);
}

void VisualizerComponent::openGLContextClosing()
{
    program.reset();
    if (tex) { glDeleteTextures (1, &tex); tex = 0; }
    if (vao) { glDeleteVertexArrays (1, &vao); vao = 0; }
    builtScene = -1;
}
