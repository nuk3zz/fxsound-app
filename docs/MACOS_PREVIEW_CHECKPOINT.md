# macOS preview checkpoint

Status: resumed after the owner's pause; preview foundation implemented on 2026-10-18. Native macOS compilation and physical Mac acceptance are pending. Linux implementation remains deferred to Kali.

## Implemented

All implementation files are isolated under `ports/macos/`:

- `CMakeLists.txt`: C++17 portable core/test target; native `FxSoundMacPreview` GUI defaults on only for Apple. Pinned JUCE 9.0.4 revision `3fb5cbc0340758596dac39f7c8de47d562b4896e`; local source is available at `C:\Projects\_toolchains\JUCE-9.0.4`.
- `Source/PreviewProcessor.h` and `Tests/PreviewTests.cpp`: independent stereo General/Bass Boost/Speech EQ with headroom, 25 ms preset crossfades, fixed audio-thread storage and numerical/state tests. This is not the original FxSound DSP and does not claim Windows sound parity.
- `Source/Main.cpp`: explicit Start/Stop, separate loopback/physical-output pickers, persistent alternate preset, hidden-window/menu-bar behavior, callback mute gates and device monitoring. No audio device opens on launch; the user routes and restores macOS system output manually.
- `Source/DeviceRoute.{h,cpp}`: CoreAudio ID/UID/unique-name/channel/transport checks; BlackHole-only input and supported physical output; rejects same-device, aggregate, virtual-output and ambiguous routes.
- `Source/GlobalHotkey.{h,cpp}`: Carbon Cmd+Option+B with press/release latch and registration failure reporting.
- `scripts/build-macos.sh`: shallow clone and exact JUCE revision check, universal arm64+x86_64 Xcode build, CTest, lipo/privacy checks, ad hoc signing/verification, ZIP and SHA256 under `ports/macos/artifacts/`. No certificate or notarization.
- `README.md` and `.gitignore`: build/first-test instructions, explicit preview limitations, native acceptance gates and excluded build artifacts.

## Important implementation correction

The earlier proposed `AudioDeviceManager` design was replaced. JUCE 9.0.4's `audioDeviceListChanged()` can reopen default devices after hotplug even when the initial call disables fallback. The preview instead directly owns the CoreAudio `AudioIODeviceType` and its explicitly selected `AudioIODevice`. Callbacks stay muted until the route is checked after open/start; stop, error or backend restart clears that permission. No automatic microphone/default-device fallback is used.

## Verification completed on Windows

- MSVC Release portable core build with `/W4 /WX`: passed.
- Portable CTest: 1/1 passed (frequency response, flat General, silence, stereo isolation, finite/bounded/stable output, null/extra channels, ramp duration/interruption and deterministic toggle).
- Shared `Main.cpp` syntax compilation against local JUCE 9.0.4 headers with MSVC C++17 `/W4`: passed. CoreAudio/Carbon implementations were not compiled by this check.
- `bash -n ports/macos/scripts/build-macos.sh`: passed.
- No driver installation or system audio routing was performed on the Windows host.

## Exact next steps

Review source and commit locally with the parent task's Windows work. A native macOS workflow is prepared by the parent task; run it only when source push is authorized. Inspect both-architecture build/test/package results before claiming a successful macOS build. When the owner's Mac is repaired, follow `ports/macos/README.md` acceptance steps, especially capture privacy, no feedback/default microphone capture, held-key behavior, device unplug/sleep/rate changes, output restoration and sustained playback. Full FxSound DSP parity and release signing/notarization remain future work.
