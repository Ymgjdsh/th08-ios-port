#!/bin/sh
set -eu

ROOT_DIR=$(CDPATH= cd -- "$(dirname -- "$0")/.." && pwd)
BUILD_DIR=${BUILD_DIR:-"$ROOT_DIR/build-ios"}
CONFIG=${CONFIG:-Release}
IOS_VERSION=${IOS_VERSION:-0.1.0}
IOS_BUILD=${IOS_BUILD:-1}
IOS_CMAKE_GENERATOR=${IOS_CMAKE_GENERATOR:-Unix\ Makefiles}
IOS_SDK=${IOS_SDK:-iphoneos}
IOS_ARCH=${IOS_ARCH:-arm64}
IOS_DEPLOYMENT_TARGET=${IOS_DEPLOYMENT_TARGET:-14.0}
DATA_DIR=${TH08_IOS_DATA_DIR:-}
if [ -z "$DATA_DIR" ] && [ -d "$HOME/TH08-data" ]; then
    DATA_DIR="$HOME/TH08-data"
fi
DEPS_DIR=${TH08_IOS_DEPS_DIR:-}
OUTPUT_IPA=${OUTPUT_IPA:-"$BUILD_DIR/th08-ios-${IOS_VERSION}-${IOS_BUILD}.ipa"}
LOG_DIR="$BUILD_DIR/logs"
mkdir -p "$LOG_DIR"
# Keep the script POSIX-sh compatible on macOS. The previous process
# substitution form is rejected by /bin/sh before any build diagnostics run.
exec > "$LOG_DIR/build-ios.log" 2>&1

echo "TH08 iOS bring-up $IOS_VERSION ($IOS_BUILD)"
echo "Project: $ROOT_DIR"
echo "Data:    ${DATA_DIR:-not bundled}"
echo "Deps:    ${DEPS_DIR:-FetchContent network source}"

command -v cmake >/dev/null 2>&1 || { echo "error: cmake is required"; exit 2; }
command -v xcrun >/dev/null 2>&1 || { echo "error: xcrun is required"; exit 2; }
# Xcode 7's xcrun cannot load on modern macOS because its libxcrun links to
# the removed libauto.dylib.  Resolve the SDK directory directly when the
# requested developer directory contains it; this still uses Xcode 7's clang
# and SDK while avoiding the obsolete xcrun executable.  IOS_SDK_PATH is an
# explicit escape hatch for installations with a custom SDK location.
SDK_PATH=${IOS_SDK_PATH:-}
if [ -z "$SDK_PATH" ] && [ -n "${DEVELOPER_DIR:-}" ]; then
    DEVELOPER_SDK_ROOT=${DEVELOPER_DIR%/Contents/Developer}/Contents/Developer
    if [ -d "$DEVELOPER_SDK_ROOT/Platforms/iPhoneOS.platform/Developer/SDKs/iPhoneOS9.3.sdk" ] &&
       [ "$IOS_SDK" = "iphoneos" ]; then
        SDK_PATH="$DEVELOPER_SDK_ROOT/Platforms/iPhoneOS.platform/Developer/SDKs/iPhoneOS9.3.sdk"
    elif [ -d "$DEVELOPER_SDK_ROOT/Platforms/iPhoneSimulator.platform/Developer/SDKs/iPhoneSimulator9.3.sdk" ] &&
         [ "$IOS_SDK" = "iphonesimulator" ]; then
        SDK_PATH="$DEVELOPER_SDK_ROOT/Platforms/iPhoneSimulator.platform/Developer/SDKs/iPhoneSimulator9.3.sdk"
    fi
fi
if [ -z "$SDK_PATH" ]; then
    SDK_PATH=$(xcrun --sdk "$IOS_SDK" --show-sdk-path)
fi
test -d "$SDK_PATH" || { echo "error: iPhoneOS SDK not found: $SDK_PATH"; exit 2; }

# Asset/storyboard tools are data-only and can safely come from the current
# Xcode.  Keep them separate from the compiler toolchain selected above.
ASSET_DEVELOPER_DIR=${IOS_ASSET_DEVELOPER_DIR:-/Applications/Xcode.app/Contents/Developer}
run_asset_tool()
{
    DEVELOPER_DIR="$ASSET_DEVELOPER_DIR" /usr/bin/xcrun "$@"
}

test -n "$DATA_DIR" || {
    echo "error: set TH08_IOS_DATA_DIR or create $HOME/TH08-data" >&2
    exit 2
}
for REQUIRED_DATA_FILE in th08.dat thbgm.dat msgothic.ttc; do
    test -s "$DATA_DIR/$REQUIRED_DATA_FILE" || {
        echo "error: required iOS resource is missing: $DATA_DIR/$REQUIRED_DATA_FILE" >&2
        exit 2
    }
done

# CMake cannot change generators in an existing build tree.  Remove only the
# generated tree when it was configured with a different generator; source
# files and the build log directory remain untouched.
if [ -f "$BUILD_DIR/CMakeCache.txt" ]; then
    CACHED_GENERATOR=$(sed -n 's/^CMAKE_GENERATOR:INTERNAL=//p' "$BUILD_DIR/CMakeCache.txt" | head -n 1)
    CACHED_SYSROOT=$(sed -n 's/^CMAKE_OSX_SYSROOT:.*=//p' "$BUILD_DIR/CMakeCache.txt" | head -n 1)
    CACHED_ARCH=$(sed -n 's/^CMAKE_OSX_ARCHITECTURES:.*=//p' "$BUILD_DIR/CMakeCache.txt" | head -n 1)
    if [ "$CACHED_GENERATOR" != "$IOS_CMAKE_GENERATOR" ] ||
       [ "$CACHED_SYSROOT" != "$SDK_PATH" ] ||
       [ "$CACHED_ARCH" != "$IOS_ARCH" ]; then
        echo "Reconfiguring build tree"
        echo "  generator: ${CACHED_GENERATOR:-<none>} -> $IOS_CMAKE_GENERATOR"
        echo "  sysroot:   ${CACHED_SYSROOT:-<none>} -> $SDK_PATH"
        echo "  arch:      ${CACHED_ARCH:-<none>} -> $IOS_ARCH"
        rm -rf "$BUILD_DIR/CMakeCache.txt" "$BUILD_DIR/CMakeFiles" \
            "$BUILD_DIR/Makefile" "$BUILD_DIR/cmake_install.cmake" \
            "$BUILD_DIR/$CONFIG-iphoneos" "$BUILD_DIR/th08-modern.app" "$BUILD_DIR/th08.app"
    fi
fi

DEPS_ARGS=""
if [ -n "$DEPS_DIR" ]; then
    DEPS_ARGS="-DFETCHCONTENT_SOURCE_DIR_SDL2=$DEPS_DIR/SDL-release-2.30.9 -DFETCHCONTENT_SOURCE_DIR_SDL2_IMAGE=$DEPS_DIR/SDL_image-release-2.8.2 -DFETCHCONTENT_SOURCE_DIR_SDL2_MIXER=$DEPS_DIR/SDL_mixer-release-2.8.0 -DFETCHCONTENT_SOURCE_DIR_SDL2_TTF=$DEPS_DIR/SDL_ttf-release-2.22.0"
fi

cmake -S "$ROOT_DIR" -B "$BUILD_DIR" -G "$IOS_CMAKE_GENERATOR" \
    -DCMAKE_SYSTEM_NAME=iOS \
    -DCMAKE_OSX_SYSROOT="$SDK_PATH" \
    -DCMAKE_OSX_ARCHITECTURES="$IOS_ARCH" \
    -DCMAKE_OSX_DEPLOYMENT_TARGET="$IOS_DEPLOYMENT_TARGET" \
    -DTH08_IOS_VERSION="$IOS_VERSION" \
    -DTH08_IOS_BUILD="$IOS_BUILD" \
    -DTH08_IOS_DATA_DIR="$DATA_DIR" \
    -DCMAKE_POLICY_VERSION_MINIMUM=3.5 \
    -DCMAKE_C_COMPILER_WORKS=TRUE \
    -DCMAKE_CXX_COMPILER_WORKS=TRUE \
    -DCMAKE_OBJC_COMPILER_WORKS=TRUE \
    -DCMAKE_OBJCXX_COMPILER_WORKS=TRUE \
    $DEPS_ARGS

cmake --build "$BUILD_DIR" --config "$CONFIG" --target th08-modern
APP_PATH=""
for candidate in \
    "$BUILD_DIR/$CONFIG-iphoneos/th08.app" \
    "$BUILD_DIR/$CONFIG-iphoneos/th08-modern.app" \
    "$BUILD_DIR/th08.app" \
    "$BUILD_DIR/th08-modern.app"; do
    if [ -d "$candidate" ]; then APP_PATH="$candidate"; break; fi
done
test -n "$APP_PATH" || { echo "error: no iOS app bundle was produced"; exit 3; }
if [ -x "$APP_PATH/th08-modern" ]; then
    # Xcode names the Mach-O after the CMake target even when OUTPUT_NAME is
    # set.  The Info.plist and IPA contract use the stable `th08` name.
    cp -f "$APP_PATH/th08-modern" "$APP_PATH/th08"
fi
test -x "$APP_PATH/th08" || { echo "error: app was not produced: $APP_PATH"; exit 3; }
rm -f "$APP_PATH/th08-modern"

# Do this after the build as well as through CMake. A reused CMake tree can
# retain an older TH08_IOS_DATA_DIR and silently produce an app without its
# Japanese font, leaving Music Room and dialogue text blank.
for REQUIRED_DATA_FILE in th08.dat thbgm.dat msgothic.ttc; do
    cp -f "$DATA_DIR/$REQUIRED_DATA_FILE" "$APP_PATH/$REQUIRED_DATA_FILE"
    test -s "$APP_PATH/$REQUIRED_DATA_FILE" || {
        echo "error: failed to bundle $REQUIRED_DATA_FILE" >&2
        exit 3
    }
done
echo "Verified game archives and Japanese font in $APP_PATH"

if [ -s "$ROOT_DIR/resources/NotoSansSC-VF.ttf" ]; then
    cp -f "$ROOT_DIR/resources/NotoSansSC-VF.ttf" "$APP_PATH/NotoSansSC-VF.ttf"
    test -s "$APP_PATH/NotoSansSC-VF.ttf" || {
        echo "error: failed to bundle NotoSansSC-VF.ttf" >&2
        exit 3
    }
    echo "Verified bundled CJK font in $APP_PATH"
fi

# A reused build tree can contain an older app under the alternate target
# name. Refuse to package it when its plist does not match this invocation.
ACTUAL_VERSION=$(/usr/libexec/PlistBuddy -c "Print :CFBundleShortVersionString" \
    "$APP_PATH/Info.plist" 2>/dev/null || true)
ACTUAL_BUILD=$(/usr/libexec/PlistBuddy -c "Print :CFBundleVersion" \
    "$APP_PATH/Info.plist" 2>/dev/null || true)
if [ "$ACTUAL_VERSION" != "$IOS_VERSION" ] || [ "$ACTUAL_BUILD" != "$IOS_BUILD" ]; then
    echo "error: stale app selected: $APP_PATH" >&2
    echo "error: expected version $IOS_VERSION ($IOS_BUILD), got $ACTUAL_VERSION ($ACTUAL_BUILD)" >&2
    exit 3
fi

if [ ! -f "$APP_PATH/Assets.car" ] && [ -d "$ROOT_DIR/ios/Assets.xcassets" ]; then
    run_asset_tool actool --platform "$IOS_SDK" --target-device iphone --target-device ipad \
        --app-icon AppIcon --output-format human-readable-text \
        --output-partial-info-plist "$BUILD_DIR/assetcatalog-info.plist" \
        --compile "$APP_PATH" "$ROOT_DIR/ios/Assets.xcassets"
fi
if [ -f "$ROOT_DIR/ios/LaunchScreen.storyboard" ] && [ ! -d "$APP_PATH/LaunchScreen.storyboardc" ]; then
    if [ "$IOS_SDK" = "iphonesimulator" ]; then
        run_asset_tool ibtool --compile "$APP_PATH/LaunchScreen.storyboardc" \
            --minimum-deployment-target "$IOS_DEPLOYMENT_TARGET" \
            "$ROOT_DIR/ios/LaunchScreen.storyboard"
    else
        run_asset_tool ibtool --compile "$APP_PATH/LaunchScreen.storyboardc" \
            --platform iphoneos --minimum-deployment-target "$IOS_DEPLOYMENT_TARGET" \
            "$ROOT_DIR/ios/LaunchScreen.storyboard"
    fi
fi

# CMake's non-Xcode generators copy source bundles verbatim.  The compiled
# products above are what iOS consumes; retaining the source directories only
# increases the IPA and can leave stale resources beside the current catalog.
rm -rf "$APP_PATH/Assets.xcassets"
rm -f "$APP_PATH/LaunchScreen.storyboard"

# Make the bundle metadata independent of Xcode's asset-catalog integration.
# This is required with the Unix Makefiles generator, which does not merge the
# partial plist emitted by actool into the app's Info.plist automatically.
if [ -f "$APP_PATH/Info.plist" ]; then
    /usr/libexec/PlistBuddy -c "Add :CFBundleIconName string AppIcon" "$APP_PATH/Info.plist" 2>/dev/null || \
        /usr/libexec/PlistBuddy -c "Set :CFBundleIconName AppIcon" "$APP_PATH/Info.plist"
fi

codesign --force --deep --sign - "$APP_PATH"
sh "$ROOT_DIR/ios/package_ipa.sh" "$APP_PATH" "$OUTPUT_IPA"
echo "SUCCESS: $OUTPUT_IPA"
