# 🥁 Transhaker (aka Transient Shaker) 🥁
*A JUCE-based tiny tool for procedural transients variation generation. Oriented for game audio design.*

Repetitive transients used as sound effects (footsteps, gunshots, UI clicks) kill realism fast. Why not add some variation to them?

![Transhaker UI](Docs/screenshot_ui.png)

---

## 🎯 Overview
**Transhaker** is a standalone audio tool built with JUCE that generates natural-sounding variations of one-shot sound effects typically used in game audio, e.g. footsteps, crashes, impacts, gunshots, knocks, or any others transients.

The goal of this mini project is to create a lightweight standalone tool with a fast and intuitive way to produce multiple believable variations from a single input sound, ready for use in FMOD, Wwise, or Unity.

---

## ✨ Features
| Category | Description |
|-----------|-------------|
| 🎚 Randomization | Each playback varies in **gain**, **pitch**, **transient start offset**, and **low-pass filtering** |
| ⚡ Real-time preview | Click **SHAKER** to instantly audition randomized versions |
| 📁 Drag & drop import | Drop any `.wav` file (or multiple) into the window to load them into the sample pool |
| 📦 Batch export | Generate 20 randomized variations offline with **EXPORT BATCH** — saved as individual `.wav` files |
| 🎨 Custom UI | Lemon-yellow interface, clean black typography, large controls for quick parameter adjustment |
| 🧠 Built for pipelines | Produces consistent variation assets for middleware (FMOD, Wwise) or procedural playback in engines (Unity, Unreal) |

---

## 🧩 Parameters
| Control | Range | Effect |
|----------|--------|--------|
| **Pitch range (%)** | 0–10 | Random ±% pitch shift (resampling) |
| **Gain range (dB)** | 0–6 | Random ± dB volume |
| **Start offset max (ms)** | 0–15 | Random skip at sample start — simulates slightly different transient attacks |
| **LPF min (Hz)** | 2 000–20 000 | Lower limit of random low-pass cutoff |
| **LPF max (Hz)** | 2 000–20 000 | Upper limit of random low-pass cutoff |

---

## 🛠 Architecture
Transhaker is composed of lightweight JUCE components:

| Class | Role |
|-------|------|
| `SamplePool` | Handles loading and storing of user-imported audio files |
| `VariationPlayer` | Applies real-time randomization and playback |
| `OfflineRenderer` | Renders multiple randomized samples offline and writes to disk |
| `AudioFileWriter` | Utility for 24-bit WAV file export |
| `MainComponent` | Manages UI, parameters, and audio routing |

---

## 🚀 Building

### Requirements
- **JUCE 7** or newer  
- **Visual Studio 2022** (Windows) or Xcode (macOS)  
- C++17 enabled  

### Build steps
1. Open the project’s `.jucer` file in **Projucer**.
2. Add all source files if not already present (`Source/*.h` and `.cpp`).
3. Set exporter to *Visual Studio 2022* (or your IDE of choice).
4. Click *Save Project and Open in IDE*.
5. Build and run.

---

## 🎧 Usage
1. **Run Transhaker.**
2. **Drag & drop** one or more `.wav` files (mono or stereo).
3. Adjust sliders to define variation ranges.
4. Press **SHAKER** → hear a random variation live.
5. Press **EXPORT BATCH** → choose a folder → get 20 rendered variations.

## Next steps
* fix missing titlebar/taskbar icon loading
* add batch size selection
* add time-stretching features
* add stereo widening / random pan
* add UI oscilloscope / waveform preview of the chosen sample 
* add JSON preset export for game engines
* add loudness normalization (dBFS)
