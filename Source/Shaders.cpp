// All shaders are embedded as strings: the VST3 is one self-contained DLL, no loose asset files.
#include "Shaders.h"

namespace lumivore
{
const char* vertexSource()
{
    return R"GLSL(#version 330 core
void main()
{
    // Fullscreen triangle from gl_VertexID, no vertex buffer needed.
    vec2 p = vec2((gl_VertexID << 1) & 2, gl_VertexID & 2);
    gl_Position = vec4(p * 2.0 - 1.0, 0.0, 1.0);
}
)GLSL";
}

const char* commonSource()
{
    return R"GLSL(#version 330 core
uniform vec2  iResolution;
uniform float iTime;
uniform vec4  uBands;      // x bass, y mid, z treble, w rms (already sensitivity-scaled, 0..~1.5)
uniform float uBeat;       // 1 on beat, decaying
uniform float uIntensity;
uniform float uZoom;
uniform float uSym;        // mirror segments, 1 = off
uniform float uRot;        // radians
uniform int   uPalette;
uniform sampler2D uAudio;  // row0 (v=.25): log spectrum 0..1, row1 (v=.75): waveform 0..1
out vec4 fragColor;

const float PI = 3.14159265359;
const float TAU = 6.28318530718;

float spec(float x) { return texture(uAudio, vec2(clamp(x, 0.0, 1.0), 0.25)).r; }
float wave(float x) { return texture(uAudio, vec2(clamp(x, 0.0, 1.0), 0.75)).r * 2.0 - 1.0; }
mat2  rot(float a)  { float c = cos(a), s = sin(a); return mat2(c, -s, s, c); }

vec3 cosPal(float t, vec3 a, vec3 b, vec3 c, vec3 d) { return a + b * cos(TAU * (c * t + d)); }
vec3 palette(float t)
{
    if (uPalette == 0) return cosPal(t, vec3(.5), vec3(.5), vec3(1.0), vec3(.00, .33, .67));   // Prism
    if (uPalette == 1) return cosPal(t, vec3(.5), vec3(.5), vec3(1.0), vec3(.00, .10, .20));   // Ember
    if (uPalette == 2) return cosPal(t, vec3(.4, .5, .6), vec3(.4), vec3(1.0), vec3(.55, .60, .70)); // Glacier
    if (uPalette == 3) return cosPal(t, vec3(.5), vec3(.5), vec3(1.0, 1.0, .5), vec3(.80, .90, .30)); // Toxic
    if (uPalette == 4) return cosPal(t, vec3(.5), vec3(.5), vec3(2.0, 1.0, 0.0), vec3(.50, .20, .25)); // Neon
    return vec3(0.5 + 0.5 * cos(TAU * t * 0.0)) * (0.4 + 0.6 * t);                               // Mono
}

vec2 kaleido(vec2 p, float n)
{
    float a = atan(p.y, p.x), r = length(p), s = TAU / n;
    a = abs(mod(a, s) - s * 0.5);
    return r * vec2(cos(a), sin(a));
}

// Centered, zoomed, rotated, optionally mirrored coordinates (height-normalised).
vec2 sceneUV()
{
    vec2 p = (gl_FragCoord.xy - 0.5 * iResolution) / iResolution.y;
    p /= max(uZoom, 0.05);
    p *= rot(uRot);
    if (uSym > 1.5) p = kaleido(p, uSym);
    return p;
}
vec3 finish(vec3 c) { return 1.0 - exp(-c * uIntensity * 1.4); }  // soft tone-map
)GLSL";
}

static const char* kRadial = R"GLSL(
void main()
{
    vec2 p = sceneUV();
    float r = length(p);
    float a = abs(atan(p.y, p.x) / PI);              // mirror left/right: 0..1
    float s = spec(a);
    float base = 0.22 + uBands.x * 0.06 + uBeat * 0.03;
    float h = s * 0.32 * uIntensity;
    float bars = smoothstep(0.0, 0.004, r - base) * smoothstep(0.004, 0.0, r - base - h);
    float seg = step(0.15, fract(a * 64.0));          // gaps between bars
    float ring = 0.003 / abs(r - base);
    vec3 col = palette(a + iTime * 0.05 + uBands.y) * (bars * seg * 1.2 + ring * (0.6 + uBeat));
    col += palette(0.6 + iTime * 0.02) * 0.02 / (r + 0.05) * (uBands.x + uBeat * 0.5);   // core glow
    fragColor = vec4(finish(col), 1.0);
}
)GLSL";

static const char* kScope = R"GLSL(
void main()
{
    vec2 p = sceneUV();
    float x = p.x * 0.55 + 0.5;                       // map screen x to waveform position
    float w = wave(x) * (0.35 + uBands.w * 0.8) * uIntensity;
    float d = abs(p.y - w);
    float line = 0.0035 / (d + 0.0015);
    float w2 = wave(x + 0.01) * 0.6 * uIntensity;    // faint echo trace
    float line2 = 0.002 / (abs(p.y + 0.18 - w2) + 0.002);
    float grid = (smoothstep(0.012, 0.0, abs(fract(p.x * 8.0) - 0.5) - 0.49) +
                  smoothstep(0.012, 0.0, abs(fract(p.y * 8.0) - 0.5) - 0.49)) * 0.06;
    vec3 col = palette(0.3 + uBands.x) * line * (0.5 + uBeat * 0.8)
             + palette(0.7 + iTime * 0.03) * line2 * 0.3
             + vec3(0.05, 0.1, 0.1) * grid;
    fragColor = vec4(finish(col * 0.6), 1.0);
}
)GLSL";

static const char* kTunnel = R"GLSL(
void main()
{
    vec2 p = sceneUV();
    float r = length(p) + 1e-4;
    float a = atan(p.y, p.x);
    float speed = 0.35 + uBands.x * 1.6 + uBeat * 0.8;
    vec2 q = vec2(a / PI * 2.0 + iTime * 0.04, 0.35 / r + iTime * speed);
    vec3 col = vec3(0.0);
    for (int i = 0; i < 5; ++i)
    {
        q = abs(fract(q * 1.35) - 0.5) * 2.0;
        q = q * (1.15 + uBands.y * 0.6) - vec2(0.25, 0.15);
        float d = length(q);
        col += palette(d * 0.7 + float(i) * 0.17 + iTime * 0.05) * (0.04 / (abs(d - 0.5 - uBands.z * 0.3) + 0.03));
    }
    col *= smoothstep(0.0, 0.25, r);                  // fade the vanishing point
    col *= 0.5 + uBeat * 0.9 + uBands.w;
    fragColor = vec4(finish(col * 0.45), 1.0);
}
)GLSL";

const std::vector<SceneInfo>& scenes()
{
    static const std::vector<SceneInfo> list = {
        { "Radial Spectrum", kRadial },
        { "Oscilloscope",    kScope  },
        { "Fractal Tunnel",  kTunnel },
    };
    return list;
}

juce::StringArray sceneNames()
{
    juce::StringArray n; for (auto& s : scenes()) n.add (s.name); return n;
}

juce::StringArray paletteNames() { return { "Prism", "Ember", "Glacier", "Toxic", "Neon", "Mono" }; }
}
