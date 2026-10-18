[CmdletBinding()]
param(
    [string]$JuceModulesPath = $env:FXSOUND_JUCE_MODULES,
    [string]$MSBuildPath,
    [ValidateSet('Debug', 'Release')][string]$Configuration = 'Release'
)
$ErrorActionPreference = 'Stop'
$fxRepo = Split-Path -Parent $PSScriptRoot
if (-not $JuceModulesPath) { $JuceModulesPath = 'C:\JUCE\modules' }
if (-not (Test-Path -LiteralPath (Join-Path $JuceModulesPath 'juce_core\juce_core.h'))) {
    throw 'Provide -JuceModulesPath pointing to the modules folder of JUCE 6.1.6.'
}
$JuceModulesPath = (Resolve-Path -LiteralPath $JuceModulesPath).Path
if (-not $MSBuildPath) {
    $fxCandidates = @(
        'C:\DevTools\VS2022BuildTools\MSBuild\Current\Bin\MSBuild.exe',
        "${env:ProgramFiles(x86)}\Microsoft Visual Studio\2022\BuildTools\MSBuild\Current\Bin\MSBuild.exe",
        "$env:ProgramFiles\Microsoft Visual Studio\2022\Community\MSBuild\Current\Bin\MSBuild.exe"
    )
    $fxVswhere = "${env:ProgramFiles(x86)}\Microsoft Visual Studio\Installer\vswhere.exe"
    if (Test-Path -LiteralPath $fxVswhere) {
        $fxDetected = & $fxVswhere -latest -products '*' -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -find 'MSBuild\**\Bin\MSBuild.exe'
        $fxCandidates = @($fxDetected) + $fxCandidates
    }
    $MSBuildPath = $fxCandidates | Where-Object { $_ -and (Test-Path -LiteralPath $_) } | Select-Object -First 1
}
if (-not $MSBuildPath -or -not (Test-Path -LiteralPath $MSBuildPath)) {
    throw 'Install Visual Studio 2022 Build Tools with v143 C++ tools and Windows SDK, or provide -MSBuildPath.'
}
if (-not (Test-Path -LiteralPath (Join-Path $fxRepo 'Resources\Presets'))) {
    throw 'Initialize the Resources submodule: git submodule update --init --recursive'
}
$fxArtifacts = Join-Path $fxRepo 'artifacts'
New-Item -ItemType Directory -Path $fxArtifacts -Force | Out-Null
$fxLog = Join-Path $fxArtifacts 'windows-build.log'
$fxSolution = Join-Path $fxRepo 'fxsound\Project\FxSound.sln'
$fxBuildArguments = @(
    $fxSolution, '/t:Build', '/m:2', '/nr:false', '/nologo', '/verbosity:minimal',
    "/p:Configuration=$Configuration", '/p:Platform=x64',
    "/p:JuceModulesPath=$JuceModulesPath", '/p:PostBuildEventUseInBuild=false',
    '/fileLogger', "/fileLoggerParameters:LogFile=$fxLog;Verbosity=normal;Encoding=UTF-8"
)
& $MSBuildPath @fxBuildArguments *>&1 | Tee-Object -FilePath (Join-Path $fxArtifacts 'windows-console.log')
if ($LASTEXITCODE -ne 0) { throw "Windows build failed. See $fxLog" }
$fxOutput = Join-Path $fxRepo "fxsound\Project\x64\$Configuration\App\FxSound.exe"
if (-not (Test-Path -LiteralPath $fxOutput)) { throw "Build produced no executable at $fxOutput" }
# Stage runtime presets and font files beside the executable: Main.cpp sets cwd
# to the executable directory, so the raw linker output is not a runnable bundle.
$fxStage = Join-Path $fxArtifacts 'FxSound-Windows-x64'
New-Item -ItemType Directory -Path $fxStage -Force | Out-Null
Copy-Item -LiteralPath $fxOutput -Destination (Join-Path $fxStage 'FxSound.exe') -Force
Copy-Item -LiteralPath (Join-Path $fxRepo 'Installer\Resources\Factsoft') -Destination $fxStage -Recurse -Force
Get-ChildItem -LiteralPath (Join-Path $fxRepo 'Installer\Resources\Fonts') -File | Copy-Item -Destination $fxStage -Force
Copy-Item -LiteralPath (Join-Path $fxRepo 'LICENSE') -Destination $fxStage -Force
Copy-Item -LiteralPath (Join-Path $fxRepo 'docs\WINDOWS_PRESET_TOGGLE.md') -Destination (Join-Path $fxStage 'README.md') -Force
@"
Modified FxSound fork by nuk3zz, 18 October 2026.
Source: https://github.com/nuk3zz/fxsound-app
Branch: feature/windows-preset-toggle
Includes configurable General / alternate preset hotkey and settings migration.
See LICENSE (AGPL-3.0) and the source repository for corresponding source.
This bundle requires the existing FxSound Windows virtual audio driver.
"@ | Set-Content -LiteralPath (Join-Path $fxStage 'SOURCE.txt') -Encoding utf8
$fxManifest = Get-ChildItem -LiteralPath $fxStage -File -Recurse | ForEach-Object {
    $fxHash = Get-FileHash -LiteralPath $_.FullName -Algorithm SHA256
    [PSCustomObject]@{ path = $_.FullName.Substring($fxStage.Length + 1); sha256 = $fxHash.Hash }
}
ConvertTo-Json -InputObject @($fxManifest) -Depth 3 | Set-Content -LiteralPath (Join-Path $fxStage 'SHA256.json') -Encoding utf8
$fxArchive = Join-Path $fxArtifacts 'FxSound-Windows-x64.zip'
Compress-Archive -LiteralPath $fxStage -DestinationPath $fxArchive -Force
Write-Output "Built: $fxOutput"
Write-Output "Runnable folder: $fxStage"
Write-Output "Package: $fxArchive"
Write-Output "Log: $fxLog"
