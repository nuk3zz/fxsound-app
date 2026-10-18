# FxSound fork: current handoff

Updated 18 October 2026 after owner-authorized continuation and publication.

## Source and status

- Fork branch: https://github.com/nuk3zz/fxsound-app/tree/feature/windows-preset-toggle
- Windows checkout: `C:\Projects\fxsound-app`.
- Published branch: `feature/windows-preset-toggle`; main remains upstream base
  `d8e7a23d37ed5939c2a3090a1c1756c7f2500b17`. No merge or deployment.
- Windows implementation: `e6b8b12`. Native Mac build verified at
  `6dc5c21395da0f3544b8446f0953dfd8ab68eeb7`. Subsequent checkpoint edits are documentation only.
- Linux implementation is deferred to the owner's Kali session; see KALI_HANDOFF.md.

## Windows delivery

`artifacts/FxSound-Windows-x64.zip` includes the executable, Factsoft presets,
external fonts, license, usage guide and per-file SHA256 manifest. ZIP SHA256:
`1B622F6B62A515678A47F7B45188CBA62E2686614ADA884DDA5C8F2469326A11`.

Native x64 Release build succeeds. Thirteen controller scenarios and ten settings
migration scenarios using real JUCE PropertySet passed. Independent review found
no blocking toggle defects. Default shortcut Ctrl+Alt+B switches General and the
chosen alternate (default Bass Boost); configure it in General Preferences.

Quit the official FxSound instance before launching the fork, since FxSound
permits one instance. The official app/driver was not replaced, closed or modified
by this task. Live hotkey/audio acceptance remains with the owner.

## macOS preview delivery

`artifacts/fxsound-mac-preview-0.1.0-universal.zip`, SHA256:
`e9c684793c76df8fb8f1b11609b89847b4581cba62841b4f2aa52d4a46b3c509`.
A same-named `.sha256` file accompanies it. The local handoff also preserves the
ZIP, so the GitHub artifact's retention period is not the only copy.

[Native macOS CI](https://github.com/nuk3zz/fxsound-app/actions/runs/39101841348) passed:
Apple Silicon and Intel compilation, native CTest, both architecture checks,
input privacy metadata, ad hoc signature verification, and ZIP/checksum packaging.
The downloaded ZIP independently matches the checksum; archive inspection confirms
both Mach-O slices, signature records, macOS13.0 minimum and source revision.
Portable Windows core tests, shared-UI syntax and Bash syntax also pass.

This is a separate small EQ preview, not the original FxSound DSP. It requires
BlackHole2ch for system audio, uses Cmd+Option+B, and is ad hoc signed rather than
notarized. Follow `ports/macos/README.md` for input privacy, device setup and manual
output restoration. Physical audio, hotkey, unplug/sleep and sustained-playback
acceptance wait for the repaired Mac.

## Tools and exact next step

Windows-native VS2022 Build Tools: `C:\DevTools\VS2022BuildTools`,
MSVC14.44.35207, SDK10.0.26100.0, CMake3.31.6-msvc6. Windows JUCE6.1.6:
`C:\Projects\_toolchains\JUCE-6.1.6`; separate preview JUCE9.0.4:
`C:\Projects\_toolchains\JUCE-9.0.4`. Keep native dependencies separate per OS.

Owner accepts the Windows toggle and later tests the Mac preview on the repaired
machine. Linux work resumes from KALI_HANDOFF.md in its native ext4 checkout.
Full original-DSP portability and release signing/notarization remain future work.
No audio route or system default was changed and no upstream PR was created.

Plan: DEVELOPMENT_PLAN.md. Local source patches, native logs, package inspection
and verification receipt: `C:\Projects\_handoff\fxsound-20261018`.
