# FxSound Mac Preview

An experimental macOS app for explicit BlackHole-to-speaker audio routing and a
global **Cmd + Option + B** General/alternate preset toggle. The alternate can be
Bass Boost or Speech, and that choice is saved between launches. Closing the
window hides it; the menu bar icon can show it, toggle presets, or quit.

This preview uses a new, small stereo EQ. **It does not contain the original
FxSound DSP, Windows virtual driver, or Windows presets, and does not claim sound
parity.** General is dry audio, Bass Boost is a +6 dB low shelf at 150 Hz with
-6 dB headroom, and Speech is a 120 Hz high pass plus a +3 dB presence peak near
2.5 kHz with -3 dB headroom. Switching crossfades over approximately 25 ms; output
is bounded to the normal floating-point audio range. Bass Boost increases bass
relative to treble; its headroom means a perceived overall volume drop is normal.

## Build and package on macOS

Prerequisites: macOS 13 or later, full Xcode with its command line tools selected
and license accepted, CMake 3.22 or later, and Git. Install CMake using its
[official installer](https://cmake.org/download/) or your normal package manager.
Both Apple Silicon and Intel binaries are produced in one app. Windows cannot
build or validate the native CoreAudio/Carbon files.

From the repository root:

```bash
bash ports/macos/scripts/build-macos.sh
```

The script shallow-clones JUCE 9.0.4 and checks its exact commit
`3fb5cbc0340758596dac39f7c8de47d562b4896e`, builds both architectures, runs the
portable tests, checks the executable architecture and microphone privacy text,
and packages an ad hoc signed app ZIP and SHA256 under `ports/macos/artifacts/`.
The ZIP also includes this README, the license, and source revision information.
It has no Developer ID certificate or notarization. Gatekeeper may require using
macOS Privacy & Security to allow an app you built or downloaded from this fork;
do not disable Gatekeeper globally. A release intended for other users still
needs signing, notarization and native acceptance testing.

Optional build directory and existing pinned JUCE checkout:

```bash
JUCE_SOURCE_DIR=/path/to/JUCE-9.0.4 bash ports/macos/scripts/build-macos.sh /path/to/build
```

For a manual native build, CMake also accepts `JUCE_SOURCE_DIR`; otherwise it
fetches that immutable JUCE commit. Use the script above to avoid a full Git
history download and to perform the packaging checks.

## First audio test on the repaired Mac

1. Install [BlackHole 2ch](https://github.com/ExistentialAudio/BlackHole) separately
   using its own installation instructions. No driver is included here. Restart
   if its installer requests it.
2. Launch the preview. It opens no audio devices on launch and does not change
   the system output. Select BlackHole 2ch as input and a distinct physical
   stereo output such as built-in speakers, USB headphones or an audio interface.
   The preview rejects microphone inputs, virtual outputs and aggregate devices.
   AirPlay and devices with an unknown transport type are also excluded.
3. Lower the physical output volume. Set macOS Sound Output to BlackHole 2ch.
   Do not create a Multi-Output Device routing the processed output back into the
   selected input; this preview expects the simple one-way route shown below.
4. Click **Start audio**. macOS may request microphone/input permission even
   though the selected input is a virtual loopback device. Grant permission to
   this app to read BlackHole. If denied, allow it in Privacy & Security, then
   restart the app and retry. A failed start must leave the app stopped.
5. Play audio and choose Bass Boost or Speech as the alternate. Press
   **Cmd + Option + B** once for the alternate and again for General. The shortcut
   is fixed in this preview; if another app owns it, the UI reports failure and
   the Toggle preset button remains available. Holding the shortcut must cause
   only one toggle until it is released.
6. Close the window and test the shortcut again. Reopen through the menu bar icon.
   The audio remains active while the window is hidden.
7. **Restore macOS Sound Output to your physical speakers/headphones before Stop
   or Quit.** The app never changes or restores system defaults; leaving the
   default on BlackHole after quitting will leave system playback inaudible.

```text
System audio -> BlackHole 2ch -> preview EQ -> physical speakers/headphones
```

Only the first two channels are processed. Start requests 48 kHz/512 samples;
the exact supported CoreAudio rate/buffer may differ, and incompatible device
pairs can fail to open. Choose matching formats in Audio MIDI Setup if needed.
JUCE creates its private CoreAudio aggregate internally for separate input/output
devices and handles their clocks. The preview does not select it as the macOS
default device. Bluetooth output adds its own latency.

If a selected device disappears, changes identity, fails, or restarts, processing
is muted/stopped and must be started again explicitly after refreshing. Direct
`AudioIODevice` ownership avoids `AudioDeviceManager`'s automatic default-device
fallback after hotplug. Device IDs, UIDs, unique names, transport and channel
counts are checked before/after opening and while running. Hardware recovery and
permission behavior still require native testing.

## Portable numerical checks

The DSP and toggle tests have no JUCE dependency and run on Windows, Linux or macOS:

```bash
cmake -S ports/macos -B ports/macos/build-core -DFXSOUND_BUILD_MAC_APP=OFF
cmake --build ports/macos/build-core --config Release
ctest --test-dir ports/macos/build-core -C Release --output-on-failure
```

They verify dry passthrough, actual frequency response at 8–384 kHz, silence,
stereo isolation, finite/bounded output, tail decay, crossfade duration and
interruption, null/extra channels, and deterministic preset toggle state. The
callback uses fixed storage and lock-free atomic flags; it performs no allocation,
locking, logging, file I/O or preference writes.

## Current verification and remaining work

The portable tests pass on Windows with MSVC and warnings treated as errors.
[macOS CI](https://github.com/nuk3zz/fxsound-app/actions/runs/39101841348) also passes for
source revision `6dc5c21395da0f3544b8446f0953dfd8ab68eeb7`: Apple Silicon and
Intel app compilation, native CTest, universal-architecture verification, input
privacy metadata, ad hoc signature verification, and ZIP/checksum packaging.
The resulting package is `fxsound-mac-preview-0.1.0-universal.zip` in the run's
artifact. It uses an ad hoc signature and is not notarized. CI compilation and
numerical tests do not establish physical audio, global shortcut or GUI acceptance;
those checks await the owner's repaired Mac.

Native acceptance gates: both architectures launch; loopback capture privacy is
correct; audio is stereo and stable; sustained playback has acceptable latency;
toggles are free of audible clicks; held keys do not repeat; hotkey conflicts are
reported; window hiding/reopening works; Stop releases capture; unplugging,
sleep/wake, rate changes and permission denial stop without capturing a default
microphone or routing into feedback. Verify that system output remains unchanged
by the app and that manual restoration works after Stop, Quit and failure.

Later work includes original DSP portability, configurable shortcuts, device
selection persistence, release signing/notarization, packaging/install UX and
physical Mac regression tests. Linux implementation is deferred to the Kali
checkout; see the repository's Kali handoff document.

Source is in [this fork](https://github.com/nuk3zz/fxsound-app) under the repository's
AGPL-3.0 license. [JUCE 9](https://github.com/juce-framework/JUCE/tree/3fb5cbc0340758596dac39f7c8de47d562b4896e)
is used under its AGPLv3 option. Follow the same source-availability and notice
requirements when distributing a modified preview.
