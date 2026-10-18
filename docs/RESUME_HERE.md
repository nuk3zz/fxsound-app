# FxSound fork: current handoff

Updated 18 October 2026. Work resumed after the owner-requested checkpoint pause.

- Fork: https://github.com/nuk3zz/fxsound-app (created and verified).
- Windows checkout: `C:\Projects\fxsound-app`.
- Branch: `feature/windows-preset-toggle`, based on upstream
  `d8e7a23d37ed5939c2a3090a1c1756c7f2500b17`.
- Windows feature checkpoint: `e6b8b12` (local). The prepared macOS source and
  handoff are saved in the next local checkpoint. No feature push has been made;
  publication and native macOS CI await owner approval.
- Preset toggle and settings controls implemented; 13 controller scenarios and
  10 settings migration scenarios passed with native MSVC. Migration checks use
  real JUCE PropertySet fallback behavior. Evidence:
  `C:\Projects\_handoff\fxsound-20261018\controller-tests\RESULTS.md`.
- Independent source review found no blocking toggle defects. Packaging now stages
  Factsoft presets and fonts beside the executable.
- Windows x64 Release build and package succeeded. ZIP SHA256:
  `1B622F6B62A515678A47F7B45188CBA62E2686614ADA884DDA5C8F2469326A11`.
  All staged asset hashes match. Two existing non-const references to temporary
  strings in FxWindow.cpp were corrected for the documented JUCE6.1.6/MSVC build.
- Official FxSound remains installed/running; no driver installation, default
  audio-route change or live audio/hotkey acceptance was performed.
- Linux implementation deferred to Kali by the owner; see KALI_HANDOFF.md.
- macOS preview source/build preparation is implemented in `ports/macos`.
  Portable CTest passes on Windows with warnings-as-errors; shared UI syntax
  check and Bash syntax checks pass. CoreAudio/Carbon and the native universal
  package remain unverified until macOS CI. Physical acceptance awaits the
  repaired Mac. This is a small EQ foundation, not the original FxSound DSP.

## Toolchains

Windows-native Visual Studio 2022 Build Tools installed successfully (exit0) at
`C:\DevTools\VS2022BuildTools`, MSVC14.44.35207, Windows SDK10.0.26100.0,
CMake3.31.6-msvc6. JUCE6.1.6 for the Windows app is at
`C:\Projects\_toolchains\JUCE-6.1.6` (commit2f980209cc4091a4490bb1bafc5d530f16834e58).
The separate preview dependency JUCE9.0.4 is at
`C:\Projects\_toolchains\JUCE-9.0.4` (commit3fb5cbc0340758596dac39f7c8de47d562b4896e).
Do not copy these dependencies into Kali or macOS.

## Exact next work

Windows package: `artifacts/FxSound-Windows-x64.zip`. Quit the official FxSound
instance before owner acceptance. Source/driver/DSP processing is not replaced.
The fork-only macOS workflow is prepared but not yet pushed or run. After
owner approval, push only `feature/windows-preset-toggle` to origin and inspect
its macOS preview build. Fix native errors on that branch, then download/verify
the app ZIP. The owner tests physical routing, privacy, reconnect and hotkeys
on the repaired Mac. Linux implementation resumes only on Kali.
Review source before requesting any push: the workspace policy says
“Do not auto-push: pushes may deploy production.” Only the fork feature branch
should be considered; no upstream submission or main merge is authorized.
Read DEVELOPMENT_PLAN.md for the paired plan and WINDOWS_PRESET_TOGGLE.md for use.
