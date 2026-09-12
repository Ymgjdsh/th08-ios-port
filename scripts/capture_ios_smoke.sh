#!/bin/sh
# Run on the Mac with a booted simulator and an installed th08.app.
# Writes only to the chosen evidence directory; preserves the simulator save.
# Usage: sh scripts/capture_ios_smoke.sh musicroom zh 30 /tmp/th08-evidence
set -eu
MODE=${1:?smoke mode required}
LANGUAGE=${2:?ja, zh or en required}
DURATION=${3:-30}
EVIDENCE=${4:?evidence directory required}
BUNDLE=com.th08.sdl2.ios
SIMULATOR=${TH08_SIMULATOR_ID:-booted}
case "$MODE" in *[!a-z-]*|'') exit 2;; esac
case "$LANGUAGE" in ja|zh|en) ;; *) exit 2;; esac
case "$DURATION" in *[!0-9]*|'') exit 2;; esac
mkdir -p "$EVIDENCE"
SIMDATA=$(xcrun simctl get_app_container "$SIMULATOR" "$BUNDLE" data)
LOG="$SIMDATA/Documents/startup.log"
xcrun simctl terminate "$SIMULATOR" "$BUNDLE" >/dev/null 2>&1 || true
OFFSET=0
if [ -f "$LOG" ]; then OFFSET=$(wc -l < "$LOG"); fi
LAUNCH=$(SIMCTL_CHILD_TH08_IOS_LANGUAGE="$LANGUAGE" SIMCTL_CHILD_TH08_IOS_SMOKE_TEST="$MODE" \
    xcrun simctl launch "$SIMULATOR" "$BUNDLE")
printf '%s\n' "$LAUNCH"
APP_PID=${LAUNCH##*: }
ELAPSED=0
while [ "$ELAPSED" -lt "$DURATION" ]; do
    sleep 5
    ELAPSED=$((ELAPSED + 5))
    if ! kill -0 "$APP_PID" 2>/dev/null; then
        tail -n +$((OFFSET + 1)) "$LOG" > "$EVIDENCE/$MODE-$LANGUAGE.log"
        printf 'FAIL: simulator process exited after %s seconds\n' "$ELAPSED" >&2
        exit 1
    fi
    if [ "$ELAPSED" -eq 20 ] || [ "$ELAPSED" -eq 50 ] ||
       { [ "$MODE" = campaign ] && [ $((ELAPSED % 20)) -eq 0 ]; }; then
        xcrun simctl io "$SIMULATOR" screenshot "$EVIDENCE/$MODE-$LANGUAGE-$ELAPSED.png" >/dev/null
    fi
    if [ "$MODE" = campaign ] &&
       tail -n +$((OFFSET + 1)) "$LOG" | grep -q 'smoke-campaign: COMPLETE'; then
        break
    fi
done
xcrun simctl io "$SIMULATOR" screenshot "$EVIDENCE/$MODE-$LANGUAGE-final.png" >/dev/null
tail -n +$((OFFSET + 1)) "$LOG" > "$EVIDENCE/$MODE-$LANGUAGE.log"
# Logs assert control flow; screenshots still require visual inspection.
case "$MODE" in
    language) grep -q 'smoke-language: PASS' "$EVIDENCE/$MODE-$LANGUAGE.log" ;;
    musicroom) grep -q 'music-room: ready' "$EVIDENCE/$MODE-$LANGUAGE.log" ;;
    pause) grep -q 'smoke-pause: moving selection down' "$EVIDENCE/$MODE-$LANGUAGE.log" ;;
    campaign) grep -q 'smoke-campaign: COMPLETE' "$EVIDENCE/$MODE-$LANGUAGE.log" ;;
esac
printf 'Captured %s (%s) after %s seconds\n' "$MODE" "$LANGUAGE" "$ELAPSED"
