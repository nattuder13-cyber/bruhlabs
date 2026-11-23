# MorphKick v0.1 (draft)

Sine-to-triangle kick synth. VST3 + Standalone. JUCE 8.

## Build (Windows, Visual Studio 2022 + CMake)
    cd MorphKick
    cmake -B build -G "Visual Studio 17 2022"
    cmake --build build --config Release
The VST3 is copied to C:\Program Files\Common Files\VST3 (run the shell as Administrator
the first time, or copy MorphKick.vst3 there by hand).

## macOS / Linux
    cmake -B build -DCMAKE_BUILD_TYPE=Release && cmake --build build

## FL Studio
1. Options > Manage plugins > Find installed plugins (verify scanning is on for the VST3 folder).
2. Add MorphKick from the Plugin database (Generators > Installed), or Add > More plugins.
3. Open the piano roll, draw a note, press play. Or click TRIGGER in the plugin.
4. Automate any knob: right-click it > Link to controller / Create automation clip.
5. Set the plugin to a mono-friendly layout if FL asks; stereo out is the default.

## Pitch limits
Pitch decay >= 15 ms and depth >= 4 semitones, in param ranges and again in Voice::tick2x.
Constants: kMinPitchDecayMs, kMinPitchDepthSemis (Source/Dsp.h).

## No-compiler route (GitHub builds it for you)
1. Create a free GitHub account, then a new repository (private is fine).
2. Upload everything in this folder, including the hidden .github folder
   (easiest: install GitHub Desktop, add this folder, commit, publish).
3. Open the repo's Actions tab. The "Build MorphKick (Windows)" run starts on its own
   (or click Run workflow). It takes roughly 10 minutes.
4. Open the finished run, download the "MorphKick-Windows" artifact, unzip it.
5. Copy MorphKick.vst3 to C:\Program Files\Common Files\VST3, then rescan in FL Studio.
   MorphKick.exe is the standalone version for a quick test without FL.
