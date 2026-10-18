#pragma once
#include <juce_core/juce_core.h>
#include <vector>

namespace lumivore
{
struct SceneInfo { const char* name; const char* body; };
const std::vector<SceneInfo>& scenes();
juce::StringArray sceneNames();
const char* vertexSource();
const char* commonSource();
juce::StringArray paletteNames();
}
