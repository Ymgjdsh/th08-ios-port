#!/bin/sh
set -eu

ROOT_DIR=$(CDPATH= cd -- "$(dirname -- "$0")/.." && pwd)
BUILD_DIR=${BUILD_DIR:-"$ROOT_DIR/build-ios"}
CONFIG=${CONFIG:-Release}
IOS_VERSION=${IOS_VERSION:-0.1.0}
IOS_BUILD=${IOS_BUILD:-1}
DATA_DIR=${TH08_IOS_DATA_DIR:-}
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
command -v xcodebuild >/dev/null 2>&1 || { echo "error: Xcode command-line tools are required"; exit 2; }
command -v xcrun >/dev/null 2>&1 || { echo "error: xcrun is required"; exit 2; }
SDK_PATH=$(xcrun --sdk iphoneos --show-sdk-path)
test -d "$SDK_PATH" || { echo "error: iPhoneOS SDK not found: $SDK_PATH"; exit 2; }

DEPS_ARGS=""
if [ -n "$DEPS_DIR" ]; then
    DEPS_ARGS="-DFETCHCONTENT_SOURCE_DIR_SDL2=$DEPS_DIR/SDL-release-2.30.9 -DFETCHCONTENT_SOURCE_DIR_SDL2_IMAGE=$DEPS_DIR/SDL_image-release-2.8.2 -DFETCHCONTENT_SOURCE_DIR_SDL2_MIXER=$DEPS_DIR/SDL_mixer-release-2.8.0 -DFETCHCONTENT_SOURCE_DIR_SDL2_TTF=$DEPS_DIR/SDL_ttf-release-2.22.0"
fi

cmake -S "$ROOT_DIR" -B "$BUILD_DIR" -G Xcode \
    -DCMAKE_SYSTEM_NAME=iOS \
    -DCMAKE_OSX_SYSROOT="$SDK_PATH" \
    -DCMAKE_OSX_ARCHITECTURES=arm64 \
    -DCMAKE_OSX_DEPLOYMENT_TARGET=14.0 \
    -DTH08_IOS_VERSION="$IOS_VERSION" \
    -DTH08_IOS_BUILD="$IOS_BUILD" \
    -DTH08_IOS_DATA_DIR="$DATA_DIR" \
    -DCMAKE_POLICY_VERSION_MINIMUM=3.5 \
    $DEPS_ARGS

cmake --build "$BUILD_DIR" --config "$CONFIG" --target th08-modern
APP_PATH="$BUILD_DIR/$CONFIG-iphoneos/th08-modern.app"
if [ ! -x "$APP_PATH/th08" ] && [ -x "$APP_PATH/th08-modern" ]; then
    # Xcode names the Mach-O after the CMake target even when OUTPUT_NAME is
    # set.  The Info.plist and IPA contract use the stable `th08` name.
    cp "$APP_PATH/th08-modern" "$APP_PATH/th08"
fi
test -x "$APP_PATH/th08" || { echo "error: app was not produced: $APP_PATH"; exit 3; }
rm -f "$APP_PATH/th08-modern"

if [ ! -f "$APP_PATH/Assets.car" ] && [ -d "$ROOT_DIR/ios/Assets.xcassets" ]; then
    xcrun actool --platform iphoneos --target-device iphone --target-device ipad \
        --app-icon AppIcon --output-format human-readable-text \
        --output-partial-info-plist "$BUILD_DIR/assetcatalog-info.plist" \
        --compile "$APP_PATH" "$ROOT_DIR/ios/Assets.xcassets"
fi

codesign --force --deep --sign - "$APP_PATH"
sh "$ROOT_DIR/ios/package_ipa.sh" "$APP_PATH" "$OUTPUT_IPA"
echo "SUCCESS: $OUTPUT_IPA"
