# Windows preset toggle

This fork adds a configurable global shortcut for **General ↔ Bass Boost**.

## Use

1. Open FxSound Preferences → General.
2. Pick the alternate preset under **Toggle preset: General ↔**.
3. Use **Ctrl+Alt+B**, or click the **Toggle preset** shortcut field and assign
   another supported Ctrl+Alt/Shift combination. Delete clears that shortcut.
4. The shortcut works while FxSound is minimized and processing is on. General
   switches to the selected alternate; any other active preset returns to General.

The normal preset-switch path preserves unsaved edits through the existing
preset autosave. Existing notification preferences remain respected. Holding the
shortcut does not repeatedly alternate presets. Disabling keyboard shortcuts
also disables the new shortcut.

The default preset pair and shortcut migrate into older settings. If an existing
FxSound action already uses Ctrl+Alt+B, that action keeps its binding and the new
shortcut starts unassigned; choose a different key in Preferences. A shortcut the
user explicitly cleared stays cleared.

Renaming a selected user preset follows its new name. Deleting/resetting that
preset clears the target. A missing preset produces a visible selection prompt;
it is never silently replaced with a different preset. Choose another alternate
before toggling. If another application owns the shortcut, assign another key.

## Build from source

Install Windows-native Visual Studio 2022 C++ tools (v143), a Windows SDK, and
JUCE 6.1.6. Initialize the pinned Resources submodule. In PowerShell:

```powershell
.\scripts\Build-Windows.ps1 -JuceModulesPath 'C:\path\to\JUCE\modules'
```

The script finds common MSBuild locations or accepts `-MSBuildPath`. It builds
x64 Release, records `artifacts/windows-build.log`, and disables the upstream
post-build copy into tracked `bin/` files. The raw executable is under
`fxsound/Project/x64/Release/App/`. The runnable package is staged under
`artifacts/FxSound-Windows-x64/`, with Factsoft presets and external fonts beside
the executable, and zipped as `artifacts/FxSound-Windows-x64.zip`. Launch from the
staged package, because the raw linker output has no preset assets. The upstream default JUCE path remains
`C:\JUCE\modules` for direct Visual Studio builds.

The normal FxSound installer supplies the Windows virtual audio driver; building
this repository does not build or install that driver. Quit the official FxSound
instance before trying the fork, since FxSound permits only one running instance.
Keep the official installation available for recovery. A successful compile does
not establish physical-device, hotkey, or audible acceptance.

## Focused acceptance checklist

- General → chosen preset → General while minimized; one change per held chord.
- Shortcut customization, clear/Delete, disabled shortcuts and app restart.
- Conflict with another FxSound action and with another application.
- Existing settings upgrade, including an old binding on Ctrl+Alt+B.
- User-preset rename/delete/reset and externally removed preset files.
- Unsaved edits survive switching and can be restored normally.
- Verify physical output, volume and sound quality on the intended device.

See DEVELOPMENT_PLAN.md for paired steps and verification status.
