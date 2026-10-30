# OFF SESSION VOCAL STRIP v0.1.0

**NYXV × OFF SESSION AUDIO** — an original modular vocal channel strip for Windows VST3 and Ableton Live.

The project is intentionally separate from GUMZYY CALL FX. It contains eight reorderable modules; every audio module has an enable switch, editable parameters and host automation through JUCE's AudioProcessorValueTreeState.

## Modules

1. **OPTO LEVELER** — optical compressor inspired by classic LA-2A-style operation. Peak Reduction, Gain, Emphasis, Compress/Limit and Mix. The timing is program-dependent; it deliberately does not expose fake manual attack/release knobs.
2. **PARAMETRIC EQ** — variable high-pass / low-pass and four parametric bands (Frequency, Gain and Q).
3. **DE-ESSER** — detector frequency, threshold, amount, attack, release, maximum reduction and Split/Wide modes.
4. **CONSOLE EQ** — low shelf, low-mid, high-mid and high shelf with a musical Drive stage.
5. **AIR ENHANCER** — original mid/high harmonic presence processor with Tone and Trim.
6. **FET COMPRESSOR** — threshold, ratio, attack, release, makeup and parallel mix.
7. **REVERB** — algorithmic Room/Plate/Hall character, Size, Decay, Pre-Delay, Damping, Width and wet filtering.
8. **DELAY** — Stereo/Ping-Pong, Time, Feedback, Ducking and feedback-path filtering.

## Interaction

- Drag the module cards in the top rack to change the serial signal path.
- Click a card to open its controls.
- Use **MODULE ACTIVE** to bypass only the selected module.
- The order, enable states and all exposed parameters are stored in the plug-in state and are host-automatable where appropriate.
- The factory starting points are **Lead Vocal**, **R&B Air**, **Trap Forward**, **Intimate** and **Ad-Lib Space**.
- Global Bypass and Input / Output trim are always available.

## Low-latency design

- The processor calls `setLatencySamples(0)` and uses no lookahead.
- The compressors use causal envelope followers; EQ and filtering use causal IIR biquads.
- The reverb is algorithmic; the delay and reverb pre-delay are intentional wet-path effects.
- No convolution, FFT processing, linear-phase filters, oversampling or block-based lookahead is used.
- Delay, pre-delay and reverb scratch memory are allocated in `prepareToPlay`, not in `processBlock`.

**Important:** 0 samples means zero *reported added plug-in latency*, not zero total recording latency. The Volt 1 driver, Ableton buffer, converter and monitoring path still determine round-trip latency. CPU use and subjective sound must be checked in Ableton on the target computer before production use.

## Build and install (one ZIP for the user)

This source package includes the Windows GitHub Actions workflow and installer scripts.

1. Extract this ZIP to a normal folder.
2. Double-click `Publish-To-GitHub.bat`. It checks Git and GitHub CLI (`git` and `gh`), signs in through the browser if needed, creates a **separate** public repository named `sleeepboii123-bot/OFF-SESSION-VOCAL-STRIP`, and pushes this project. It does not alter GUMZYY CALL FX.
3. The Windows Actions workflow downloads JUCE 8.0.6 in CI, builds the VST3, checks dry-path integrity, exercises all eight modules individually, verifies that changing the Opto/EQ order changes the result, runs common sample rates (44.1 / 48 / 96 kHz), and uploads one installable ZIP.
4. Download the single artifact named `OFF-SESSION-VOCAL-STRIP-v0.1.0-Windows-x64-VST3` from the successful Actions run.
5. Extract it and double-click `Install-OFF-SESSION-VOCAL-STRIP.bat`. Approve the Windows permission prompt.
6. Restart Ableton or rescan VST3 plug-ins, then find **OFF SESSION VOCAL STRIP** under VST3 audio effects.

The publish helper requires Git for Windows and GitHub CLI to be installed and authenticated. It does not silently install developer tools or alter system settings.

## Source build

Requirements: CMake 3.24+, a C++17 compiler, Git and a network connection for FetchContent to download JUCE 8.0.6.

```powershell
cmake -S . -B build -A x64 -DCMAKE_BUILD_TYPE=Release -DBUILD_TESTING=ON
cmake --build build --config Release --parallel
ctest --test-dir build -C Release --output-on-failure
```

## Known validation boundary

The DSP smoke tests cover dry-path integrity with all modules bypassed, finite output with modules active, multiple common sample rates and a reordered chain. They are not a substitute for listening tests, host automation checks, CPU profiling or a Volt 1 round-trip latency measurement in Ableton Live.

## Third-party licensing

JUCE 8.0.6 is dual-licensed under AGPLv3 or a commercial licence. Review the JUCE licence terms before distributing a closed-source or commercial version. This project does not bundle third-party compressor or exciter code.
