#!/bin/sh
set -eu

ROOT_DIR=$(CDPATH= cd -- "$(dirname -- "$0")/.." && pwd)
BUILD_DIR=${BUILD_DIR:-"$ROOT_DIR/build-ios"}
CONFIG=${CONFIG:-Release}
IOS_VERSION=${IOS_VERSION:-0.1.0}
IOS_BUILD=${IOS_BUILD:-1}
DATA_DIR=${TH08_IOS_DATA_DIR:-}
OUTPUT_IPA=${OUTPUT_IPA:-"$BUILD_DIR/th08-ios-${IOS_VERSION}-${IOS_BUILD}.ipa"}
LOG_DIR="$BUILD_DIR/logs"
mkdir -p "$LOG_DIR"
exec > >(tee "$LOG_DIR/build-ios.log") 2>&1

echo "TH08 iOS bring-up $IOS_VERSION ($IOS_BUILD)"
echo "Project: $ROOT_DIR"
echo "Data:    ${DATA_DIR:-not bundled}"

command -v cmake >/dev/null 2>&1 || { echo "error: cmake is required"; exit 2; }
command -v xcodebuild >/dev/null 2>&1 || { echo "error: Xcode command-line tools are required"; exit 2; }
command -v xcrun >/dev/null 2>&1 || { echo "error: xcrun is required"; exit 2; }
SDK_PATH=$(xcrun --sdk iphoneos --show-sdk-path)
test -d "$SDK_PATH" || { echo "error: iPhoneOS SDK not found: $SDK_PATH"; exit 2; }

cmake -S "$ROOT_DIR" -B "$BUILD_DIR" -G Xcode \
    -DCMAKE_SYSTEM_NAME=iOS \
    -DCMAKE_OSX_SYSROOT="$SDK_PATH" \
    -DCMAKE_OSX_ARCHITECTURES=arm64 \
    -DCMAKE_OSX_DEPLOYMENT_TARGET=14.0 \
    -DTH08_IOS_VERSION="$IOS_VERSION" \
    -DTH08_IOS_BUILD="$IOS_BUILD" \
    -DTH08_IOS_DATA_DIR="$DATA_DIR"

cmake --build "$BUILD_DIR" --config "$CONFIG" --target th08-modern -- -quiet
APP_PATH="$BUILD_DIR/$CONFIG-iphoneos/th08.app"
test -x "$APP_PATH/th08" || { echo "error: app was not produced: $APP_PATH"; exit 3; }

if [ ! -f "$APP_PATH/Assets.car" ] && [ -d "$ROOT_DIR/ios/Assets.xcassets" ]; then
    xcrun actool --platform iphoneos --target-device iphone --target-device ipad \
        --app-icon AppIcon --output-format human-readable-text \
        --output-partial-info-plist "$BUILD_DIR/assetcatalog-info.plist" \
        --compile "$APP_PATH" "$ROOT_DIR/ios/Assets.xcassets"
fi

codesign --force --deep --sign - "$APP_PATH"
"$ROOT_DIR/ios/package_ipa.sh" "$APP_PATH" "$OUTPUT_IPA"
echo "SUCCESS: $OUTPUT_IPA"
