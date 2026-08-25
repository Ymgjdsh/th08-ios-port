#!/bin/sh
set -eu
if [ "$#" -ne 2 ]; then echo "usage: $0 APP_PATH OUTPUT_IPA" >&2; exit 2; fi
APP_PATH=$1
IPA_PATH=$2
for required in th08 Info.plist th08.dat thbgm.dat; do
    if [ ! -s "$APP_PATH/$required" ]; then
        echo "error: app bundle is missing $required; set TH08_IOS_DATA_DIR when building" >&2
        exit 3
    fi
done
WORK_DIR=$(mktemp -d "${TMPDIR:-/tmp}/th08-ipa.XXXXXX")
trap 'rm -rf "$WORK_DIR"' EXIT INT TERM
mkdir -p "$WORK_DIR/Payload"
cp -R "$APP_PATH" "$WORK_DIR/Payload/th08.app"
rm -f "$IPA_PATH"
(cd "$WORK_DIR" && /usr/bin/zip -qry "$IPA_PATH" Payload)
echo "created $IPA_PATH"
