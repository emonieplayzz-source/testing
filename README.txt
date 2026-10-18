LUMIVORE - audio-reactive visualizer plugin (VST3, Windows x64)
Version 0.1.0 (stage 1 of the plan - see "What is in this build")

WHAT IT IS
An effect plugin. Put it on a mixer insert (the Master works well). Audio passes through
unchanged; the plugin window is the visualizer.

WHAT IS IN THIS BUILD (honest status)
  Done:   lock-free audio tap, Hann-windowed FFT, smoothed spectrum, bass/mid/treble, RMS,
          waveform, beat detection (unit-tested), 3 scenes (Radial Spectrum, Oscilloscope,
          Fractal Tunnel), palettes, intensity / sensitivity / zoom / rotation / symmetry,
          automatable parameters, resizable editor, shader error overlay.
  Not yet: scenes 4-10, fullscreen/second window, presets, MP4 recording, Spout, quality
          setting, settings menu, installer .exe.
  Not verified: loading in FL Studio, and the GPU shaders on real hardware.

GETTING THE COMPILED PLUGIN (pick one)
A) No tools on your PC - GitHub builds it for you
   1. Create a free GitHub account and a new empty repository.
   2. Upload this whole folder (including the hidden .github folder) to it.
   3. Open the "Actions" tab -> "build-windows-vst3" -> wait for the green check.
   4. Open the finished run -> Artifacts -> download "Lumivore-Windows-x64".
      It contains Lumivore-Windows-x64.zip with the .vst3 and the installer script.
   The workflow also runs the unit tests and fails if the plugin needs the VC++ runtime.

B) Build on your own PC
   Install: Visual Studio 2022 (workload "Desktop development with C++"), CMake 3.22+, Git.
   Then double-click scripts\build-windows.bat (needs internet once, to fetch JUCE 8.0.4).
   Output: Lumivore-Windows-x64.zip and the release\ folder.

INSTALLING
  1. Unzip Lumivore-Windows-x64.zip.
  2. Right-click install.bat -> Run as administrator (or run install.ps1).
     It copies Lumivore.vst3 to  C:\Program Files\Common Files\VST3\
     (manual alternative: copy the Lumivore.vst3 FOLDER there yourself).

FL STUDIO
  1. Options -> Manage plugins.
  2. Make sure C:\Program Files\Common Files\VST3 is in the plugin search paths.
  3. Click "Find installed plugins" (start scan). Lumivore should appear under Installed -> Effects.
  4. Mixer: pick an insert (e.g. Master), click an empty FX slot, choose Lumivore.
  5. Open the plugin window. Play audio.
If it does not appear, see TROUBLESHOOTING_FL_STUDIO.txt.

DEVELOPER NOTES
  Source/AudioCore.h       JUCE-free FIFO / FFT / analysis (tested: tests/core_test.cpp)
  Source/Shaders.cpp       embedded GLSL scenes; add a scene = add a body string + list entry
  Real-time rule: processBlock only copies samples into the FIFO. All analysis and all GL
  work happens on the render thread.
