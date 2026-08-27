# TH08 iOS port status

This is an independent mobile-port lane based on the playable reconstruction.
It does not replace the VC7 exact-build lane, and iOS-specific layout adapters
are not claims about the original executable.

## Current implementation

- arm64 application bundle and IPA packaging for iOS 14 or newer;
- SDL2 input, audio, image, mixer, and TrueType font integration;
- an OpenGL ES 2 compatibility renderer for the reconstructed D3D8 calls;
- four-byte pointer handles and side storage for retail structs whose field
  offsets must remain compatible with the original 32-bit data layout;
- touch joystick and configurable action buttons, direct menu touch input,
  keyboard/controller input, developer controls, and performance options;
- iPhone and iPad orientation handling, including a portrait gameplay
  compositor with the HUD above the playfield;
- persistent startup/runtime diagnostics in the app data directory;
- bundled asset-catalog icons and a compiled launch storyboard.

Original game archives are never stored in the repository. A build bundles
only the user-provided `th08.dat`, `thbgm.dat`, and required Japanese font.

## Validation checkpoint

Build `0.3.0 (85)` completed a simulator campaign regression on Normal with
the route `1 -> 2 -> 3 -> 4B -> 5 -> 6A`. The test exercised normal stage and
Boss scripts, player shots, an actual Bomb, dialogue, music transitions,
cross-stage teardown/reload, the ending, result processing, and return to the
title state. The final marker was:

```text
smoke-campaign: COMPLETE - six-stage route and ending returned to title
```

The corresponding fresh device build produced a signed arm64 IPA with
`minos 14.0`, complete asset catalogs, and both required data archives.

Build `0.3.9 (94)` additionally passed focused regressions for Music Room
entry, Stage 1-to-2 resource reconstruction, direct Stage 6 gameplay and Bomb
use. It also suppresses Stage 6 linked familiars and their alignment effects
while their script position is parked at the playfield origin, with
`stage6-corner-familiar` logging for real-device confirmation.

## Remaining coverage

The known reproducible launch and main-route blockers are fixed, but this is
not a claim that every possible P0 defect is eliminated. Additional real-device
coverage is still required for:

- the 4A/6B route and Extra Stage;
- every character/team, shot type, and difficulty;
- Replay, Spell Practice, Music Room, and unusual pause/result transitions;
- repeated background/foreground cycles and memory pressure on older devices;
- controller models and all supported iPhone/iPad aspect ratios.

Report a new permanent hang or crash with the app's `startup.log`, the device
crash report when available, the exact screen/action, device model, iOS version,
and the local time of the failure.
