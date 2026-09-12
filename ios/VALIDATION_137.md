# 0.5.5 (137): text, restart and upstream integration

## Changes and evidence

- Music Room and Result used the animated label color as part of the text
  raster cache key. Fades/highlights could generate a different texture every
  frame. Rasterize white glyphs once and apply the VM color during drawing.
- GDI text reopened the font for each label and rasterized the same string
  five times for its outline. Cache faces by font/size/style and reuse one
  glyph coverage surface. A mutex protects the shared SDL_ttf faces and glyph
  surface; the CP932/UTF-8 mode is local to each thread.
- Keep the 2x text resolution. Limit translation textures to 128 labels and
  an estimated 24 MiB of CPU/GPU pixel backing, with LRU eviction. This limit
  does not represent total application memory or driver allocation overhead.
- Update existing texture storage instead of reallocating it on each pixel
  update. Full pixel-count diagnostics are opt-in. Compile raster, ANM and
  compatibility rendering code with `-O2`; reconstructed gameplay retains
  `-O0`, `-fno-fast-math` and `-fno-strict-aliasing`.
- Shaded HUD rectangles, including boss health, explicitly disable depth,
  fog and alpha testing and use standard alpha blending with diffuse color.
  Restore the previous renderer state afterward. Stage 3 reproduction also
  exposed an independent logic bug: the iOS recovery read the instruction
  pointer at message-VM offset 4 as the message index at offset 8. Correct the
  header fields, check the index offset at compile time, and use the same
  message overlay as the original gauge update and drawing code.
- Commit language changes at a frame boundary. Join the loading worker before
  releasing translation resources, unbind device references, clear freed
  loading/ANM/effect pointers and rebuild game-owned systems. Keep the UIKit
  view and GL context attached. Only finish the restart once the ready title
  menu has been drawn and presented. This is an in-process game restart;
  iOS does not offer a public API to kill and automatically relaunch an app.

## Selected upstream fixes

Reviewed upstream main through
[`a483e1c`](https://github.com/N0zoM1z0/th08/commit/a483e1c17837bc1680906424107cc9118ed64d60).
Do not replace the iOS source tree with upstream files.

| Upstream | Applied to this port |
| --- | --- |
| [`af72ca98`](https://github.com/N0zoM1z0/th08/commit/af72ca98) | Stage-finished background predicate instead of dialogue predicate; correct point/score popup pools; signed random shot spread; Bomb timer parity/frame predicates; Spell Practice retry-menu gate |
| [`63256e0`](https://github.com/N0zoM1z0/th08/commit/63256e0) | Boss name sprites are read from `front.anm`; retain iOS null/bounds checks |
| [`318fec8`](https://github.com/N0zoM1z0/th08/commit/318fec8), [`d9c8f7e`](https://github.com/N0zoM1z0/th08/commit/d9c8f7e) | Reviewed ownership changes; corresponding iOS gauge-bound and effect-owner aliases already exist |

These are reviewed source fixes, not a claim that the arm64 port byte-matches
the original x86 executable. No new upstream laser-renderer fix was identified.

## Reproduction

Build and install the simulator app, then run on the Mac:

```sh
sh scripts/capture_ios_smoke.sh language zh 65 /tmp/th08-137
sh scripts/capture_ios_smoke.sh musicroom zh 35 /tmp/th08-137
SIMCTL_CHILD_TH08_IOS_SMOKE_RELOAD_AT=30 \
  sh scripts/capture_ios_smoke.sh musicroom en 45 /tmp/th08-137/reload
sh scripts/capture_ios_smoke.sh pause zh 30 /tmp/th08-137
sh scripts/capture_ios_smoke.sh dialogue zh 55 /tmp/th08-137
```

The script preserves simulator saves, extracts only each launch's new log,
checks process liveness and captures screenshots. Menu automation follows the
actual cursor so unlocked entries cannot silently redirect a test to a
different page. Inspect the screenshots as well as the logs: a responsive
loop alone is not proof that the correct picture is displayed.
Set `TH08_SIMULATOR_ID` to an explicit simulator UUID when other projects
are being tested on the same Mac. This run uses a separate `TH08 Regression
137` simulator so another app cannot silently replace the captured scene.

## Coverage

Testing uses the iOS 16.0 iPad (9th generation) simulator on an Intel Mac.
The device build targets arm64 with minimum iOS 14.0. Simulator timing is
dominated by presentation on this host and is not a low-end iPad benchmark.
Real-device tests are still needed for memory pressure, sustained frame rate,
repeated language changes and all boss phase/character combinations.

Final run results are recorded below after the build and screenshot checks.
