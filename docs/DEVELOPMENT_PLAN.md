# Fork development plan

Owner: `nuk3zz` · Started: 18 October 2026 · Primary development OS: Windows

Fork: https://github.com/nuk3zz/fxsound-app

Work proceeds in pairs. Windows preset toggle comes first. The owner deferred Linux implementation to
Kali and requested as much macOS source/build preparation as possible before
their repaired Mac is available. A macOS preview must be distinguished from a
complete port of FxSound's Windows audio engine.

| Pair | Step | Deliverable | Status |
| --- | --- | --- | --- |
| 1 | 1 | Create the GitHub fork, separate Windows checkout, feature branch and Resources submodule. | Complete |
| 1 | 2 | Audit hotkeys, preset changes, settings migration and Windows build requirements. | Complete |
| 2 | 3 | Add General ↔ selected preset global hotkey using existing preset handling. | Implemented; verification in pair 3 |
| 2 | 4 | Add alternate-preset selection and editable shortcut in General settings. | Implemented; verification in pair 3 |
| 3 | 5 | Compile Windows x64 and verify behavior with focused tests. | Build + 23 isolated checks passed; physical acceptance pending |
| 3 | 6 | Independently review the diff, document usage and package the verified build. | Complete for Windows; ZIP and staged hashes verified |
| 4 | 7 | Prepare a Kali/Linux implementation handoff; resume Linux work on Kali. | Complete; implementation deferred by owner |
| 4 | 8 | Build a macOS preview foundation and package what can be verified; physical Mac acceptance deferred. | Universal app build, native tests and package checks passed; physical acceptance pending |

## Windows acceptance

- Default pair: General and Bass Boost; default shortcut: Ctrl+Alt+B.
- The alternate preset and shortcut persist across restarts.
- General switches to the selected alternate; any other preset switches to General.
- Global shortcut works while minimized, respects disabled shortcuts and avoids
  repeated toggles when a key is held.
- Existing preset autosave, UI and notification behavior is reused.
- A renamed target follows its new name. Missing/deleted targets are visible and
  never silently replaced with an unrelated preset.
- Existing installations receive missing defaults without restoring a shortcut
  the user explicitly cleared or overwriting another assigned shortcut.
- DSP and Windows audio-driver code remain outside this feature's source changes.
- Build success and runtime audio acceptance are recorded separately.

## Source and build boundaries

The Windows checkout is `C:\Projects\fxsound-app`, on
`feature/windows-preset-toggle`. `origin` is the owner's fork and `upstream` is
`fxsound2/fxsound-app`. The initial source checkout is shallow to avoid downloading
all bundled-binary history; the GitHub fork retains upstream history.

Native build prerequisites: Visual Studio 2022 C++ tools (v143), Windows SDK,
JUCE 6.1.6, and the pinned Resources submodule. Running system-wide processing also
requires the installed FxSound virtual audio driver. Dependencies and credentials
remain local to each OS.

Source publication must target only the owner's fork after reviewing workflows;
there is no upstream submission, production deployment or main-branch merge in
this plan.

## Verified Windows build — 18 October 2026

Native x64 Release build succeeds. The package is
`artifacts/FxSound-Windows-x64.zip`, SHA256
`1B622F6B62A515678A47F7B45188CBA62E2686614ADA884DDA5C8F2469326A11`.
All staged SHA256 entries match; General and Bass Boost preset names were verified
in the packaged files. Controller13 and settings migration10 scenarios passed.
Full app hotkey/audio acceptance is still pending; the official instance was not
replaced. Two existing `auto&` references to temporary names in FxWindow.cpp were
changed to value copies to permit the documented JUCE6.1.6/MSVC build.

The separate macOS preview portable core also compiles with warnings-as-errors
on Windows, and its CTest suite passes. This is not a native macOS build result.

## Verified macOS preview build — 18 October 2026

The fork feature branch is published. [macOS CI](https://github.com/nuk3zz/fxsound-app/actions/runs/39101841348)
passes at `6dc5c21395da0f3544b8446f0953dfd8ab68eeb7`. The native CoreAudio/Carbon app
is compiled for arm64 and x86_64, its native portable-core test passes, and privacy
metadata, ad hoc signature and app ZIP/checksum packaging checks pass. The separate
EQ preview and physical-device acceptance boundaries still apply. Main and
upstream are unchanged; Linux implementation remains deferred to Kali.

The downloaded macOS ZIP was independently verified, including both Mach-O CPU
slices, signature records, privacy metadata and source revision. Local package:
`artifacts/fxsound-mac-preview-0.1.0-universal.zip`; SHA256
`e9c684793c76df8fb8f1b11609b89847b4581cba62841b4f2aa52d4a46b3c509`.
The app requires macOS13+ and BlackHole2ch; physical acceptance awaits the owner.
