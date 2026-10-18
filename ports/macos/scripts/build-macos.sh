#!/usr/bin/env bash
set -euo pipefail

if [[ "$(uname -s)" != Darwin ]]; then
    echo "Build the native app on macOS with Xcode. Portable core tests also run on Windows/Linux." >&2
    exit 1
fi

port_dir="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
repo_dir="$(cd "$port_dir/../.." && pwd)"
build_dir="${1:-${FXSOUND_MAC_BUILD_DIR:-$port_dir/build}}"
mkdir -p "$build_dir"
build_dir="$(cd "$build_dir" && pwd)"
for command in cmake git xcrun lipo codesign ditto plutil shasum; do
    command -v "$command" >/dev/null || { echo "Required command missing: $command" >&2; exit 1; }
done
xcrun --find clang >/dev/null

juce_revision=3fb5cbc0340758596dac39f7c8de47d562b4896e
juce_dir="${JUCE_SOURCE_DIR:-$build_dir/_deps/JUCE-9.0.4}"
if [[ ! -d "$juce_dir" ]]; then
    mkdir -p "$(dirname "$juce_dir")"
    git clone --depth 1 --branch 9.0.4 https://github.com/juce-framework/JUCE.git "$juce_dir"
fi
if [[ "$(git -C "$juce_dir" rev-parse HEAD)" != "$juce_revision" ]]; then
    echo "JUCE_SOURCE_DIR must be the pinned JUCE 9.0.4 revision $juce_revision." >&2
    exit 1
fi

cmake -S "$port_dir" -B "$build_dir" -G Xcode \
    -DFXSOUND_BUILD_MAC_APP=ON -DBUILD_TESTING=ON \
    -DJUCE_SOURCE_DIR="$juce_dir" \
    -DCMAKE_OSX_ARCHITECTURES="arm64;x86_64" \
    -DCMAKE_OSX_DEPLOYMENT_TARGET=13.0
cmake --build "$build_dir" --config Release --parallel "${CMAKE_BUILD_PARALLEL_LEVEL:-4}" \
    --target FxSoundMacPreview fxsound_preview_tests
ctest --test-dir "$build_dir" -C Release --output-on-failure

app_name="FxSound Mac Preview.app"
app="$build_dir/FxSoundMacPreview_artefacts/Release/$app_name"
binary="$app/Contents/MacOS/FxSound Mac Preview"
[[ -f "$binary" ]] || { echo "Expected app executable not found: $binary" >&2; exit 1; }
lipo "$binary" -verify_arch arm64 x86_64
plutil -extract NSMicrophoneUsageDescription raw -o - "$app/Contents/Info.plist" >/dev/null
# Ad hoc signing uses no certificate. It is not Developer ID signing or notarization.
codesign --force --deep --sign - "$app"
codesign --verify --deep --strict "$app"

package_dir="$(mktemp -d "$build_dir/package.XXXXXX")"
ditto "$app" "$package_dir/$app_name"
cp "$port_dir/README.md" "$package_dir/README.md"
cp "$repo_dir/LICENSE" "$package_dir/LICENSE"
source_revision="${GITHUB_SHA:-$(git -C "$repo_dir" rev-parse HEAD)}"
source_state=clean
if [[ -n "$(git -C "$repo_dir" status --porcelain --untracked-files=normal)" ]]; then
    source_state="local changes present; revision alone does not reproduce this build"
fi
cat > "$package_dir/SOURCE.txt" <<EOF
FxSound Mac Preview 0.1.0 - experimental native EQ/routing preview
Source: https://github.com/nuk3zz/fxsound-app
Revision: $source_revision
Source state: $source_state
Source directory: ports/macos
JUCE source: https://github.com/juce-framework/JUCE/tree/$juce_revision
License: AGPL-3.0 (see LICENSE); JUCE is used under its AGPLv3 option.
No original FxSound DSP, virtual audio driver, or BlackHole driver is bundled.
This package has an ad hoc signature and is not notarized.
EOF
artifact_dir="$port_dir/artifacts"
mkdir -p "$artifact_dir"
zip_file="$artifact_dir/fxsound-mac-preview-0.1.0-universal.zip"
ditto -c -k --sequesterRsrc "$package_dir" "$zip_file"
(cd "$artifact_dir" && shasum -a 256 "$(basename "$zip_file")" > "$(basename "$zip_file").sha256")
echo "Packaged: $zip_file"
