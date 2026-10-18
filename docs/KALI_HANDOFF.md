# Kali / Linux handoff

Owner deferred Linux implementation to a later Kali session on 18 October 2026.
There is no Linux audio backend or verified Linux build in this fork yet.

## Source and OS boundary

- Windows checkout: `C:\Projects\fxsound-app`.
- Planned Kali checkout: `/home/nuke/Desktop/Projects/fxsound-app` (not created by
  this Windows task).
- Fork: https://github.com/nuk3zz/fxsound-app
- Feature branch: `feature/windows-preset-toggle`.
- Read `docs/DEVELOPMENT_PLAN.md`, the workspace `PROJECT_WORKFLOW.md`,
  `PROJECT_INDEX.md`, `OS_HANDOFF.md`, and repository guidance first.
- The feature branch is published. The final handoff records verified build
  revisions and packages. Fetch this branch into the native Kali checkout.
- Inspect any existing Kali checkout's dirty state before fetching/switching.
  Never reset it or copy Windows dependencies/build output into Kali.

Before booting Kali, save/stop Windows development processes and update the
workspace OS handoff. Use the ordinary NTFS driver checks for the shared handoff
volume; refuse writes if hibernated, dirty or read-only. Do not force mounting.

## First Kali session: two steps together

1. **Inventory and baseline:** identify the current distribution, desktop,
   X11/Wayland session, compiler and audio stack. Useful read-only commands:
   `uname -r`, `cat /etc/os-release`, `printf '%s\n' "$XDG_SESSION_TYPE"`,
   `pipewire --version`, `wpctl status`, `pactl info`, `cmake --version`,
   `c++ --version`. Record missing tools rather than assuming they are installed.
   Get the feature source into a native ext4 checkout and run available portable
   tests. Keep the existing Macro Project and its key routing unchanged.
2. **Dependency audit:** map Windows-only dependencies in `dsp/`, `audiopassthru/`,
   `FxController`, settings, global hotkeys and crash handling. Identify which DSP
   processing code can compile without Windows headers, registry, COM or device
   calls. Record compile failures before changing architecture.

## Next pair: portable DSP and PipeWire prototype

3. Define a small audio-backend interface for device discovery, format, start/stop,
   frame processing and device-loss reports. Separate the original DSP math and
   preset values from registry, paths, timers and device management. Add native
   offline numerical/real-time checks before claiming the Windows DSP is portable.
   Keep AGPL notices and audit third-party dependencies.
4. Implement an explicitly started PipeWire filter/virtual sink and route its
   output to the selected physical sink. Initially test with generated signals
   and a disposable graph, restoring the previous route on stop/failure. Verify
   sample rates, channels, device changes, latency and reconnect. Do not replace
   the system's audio configuration during setup.

## Final pair: Linux UX and acceptance

5. Reuse the General ↔ selected preset contract. Persist names/IDs rather than
   array positions, preserve edits, handle missing targets, and suppress key
   repeat. X11 and Wayland need different global-hotkey integrations; on Wayland
   verify desktop portal/compositor support rather than assuming X11 hooks work.
   Keep a CLI/control path so the owner's existing macro setup can integrate
   later without reworking its input routing.
6. Validate on the actual Kali desktop: build, output selection, audible EQ,
   minimized shortcut, disable/rebind/conflict, restart, sleep/reconnect and crash
   recovery. Package only after the default route recovers correctly. Record
   measured results and unfinished gates in the canonical handoff.

## macOS preview is a prototype

`ports/macos` is a separate preview foundation. Its small EQ is not the original
FxSound DSP and does not prove `.fac` preset compatibility or sound parity. Its
portable tests may help bootstrap Linux work, but the Windows virtual driver and
Core Audio routing cannot be reused as a PipeWire backend.

Primary references: [PipeWire filter-chain](https://docs.pipewire.org/page_module_filter_chain.html),
[upstream architecture](https://github.com/fxsound2/fxsound-app#build-instructions).
