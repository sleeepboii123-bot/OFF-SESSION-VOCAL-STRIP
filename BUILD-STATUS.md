# Build status — OFF SESSION VOCAL STRIP v0.1.0

- [x] Independent JUCE VST3 project created.
- [x] Eight modules represented in the APVTS parameter tree.
- [x] Module enable / bypass parameters and persistent chain order parameters.
- [x] Editor rack with drag-to-reorder tiles and per-module editor pages.
- [x] Causal DSP path with no lookahead and `setLatencySamples(0)`.
- [x] DSP tests added for 44.1, 48 and 96 kHz, dry path, individual response of all eight modules, finite output and a reordered chain with a signal comparison.
- [x] Windows CI workflow and one-click install package scripts included.
- [ ] Windows MSVC build and tests: pending first GitHub Actions run.
- [ ] Ableton Live module automation / drag reorder / CPU listening checks: pending host test.
- [ ] Volt 1 round-trip latency measurement: pending user hardware / driver test.

Do not label this build production-verified until the pending checks pass.
