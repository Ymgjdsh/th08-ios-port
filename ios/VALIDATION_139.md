# 0.5.6 (139): dialogue translation coverage

## Causes and changes

- `msg1a.dat`, script 0: Wriggle's two-line interruption is grouped under
  `63_1` in the translation table, but the retail VM displays its second row
  at tick 124. Requiring identical ticks missed that row and discarded the
  first translated row, exposing Japanese/stale text in the shared atlas.
  Match exact CP932 text within the active file and route when timestamps
  differ; reject ambiguous matches instead of choosing an unrelated row.
- Several speeches share the same tick. Remove the time-only translation
  cache and reset the display when a new first row arrives. Preserve the
  preceding translated row when the second row arrives later.
- Reset the implicit line index before translating opcode 0x10 when the
  speaker changes. Preserve delayed second-row timing; only immediately wrap
  a translation into two rows if its original consists of a single row.
- Final A/B message script IDs 0 and 1 share the same bytecode offsets.
  Resolve aliases from each loaded retail offset table. Previously, script 1
  bypassed translations recorded under script 0.
- Add the missing `msg4ac.dat/0/120_0` Chinese and English entry, using the
  existing adjacent translations of those sentences. Reject missing/empty
  translations or more than two rows during generation rather than silently
  producing Japanese fallbacks or truncating rows.

## Automated verification

`scripts/test_ios_dialogue.py` parses the private retail message files and
compiles the production `ios_localization.cpp` with a stub language provider.
All 32 files, including duplicate-offset routes, supply 2,768 text instructions.
Both Chinese and English lookup/row-identity checks pass: 5,536 queries, zero failures.
Additional assertions cover Wriggle's delayed second row, unknown text at a
known timestamp, localization reset, and Japanese bypass.

The test operates on real retail bytecode, not only on translation JSON keys.
It verifies lookup coverage; it is not an exhaustive visual playthrough or a
new quality review of every translation.

Reproduce on the Mac using an isolated simulator:

1. Launch the game with `TH08_IOS_DUMP_MESSAGES=1` and enter a stage. This
   opt-in tool extracts the 32 files to the test app's private
   `Library/Application Support/th08/dialogue-audit` directory.
2. Run `python3 scripts/test_ios_dialogue.py --messages "<that directory>"`.
   Original message files must not be committed.
3. For screenshots, launch with `TH08_IOS_SMOKE_TEST=dialogue`,
   `TH08_IOS_LANGUAGE=zh` (or `en`), `TH08_IOS_DIALOGUE_DIAGNOSTICS=1` and
   `TH08_IOS_DIALOGUE_HOLD_TIME=124`. The probe advances with distinct button
   presses and holds at the reported Wriggle scene. Environment switches are
   absent during ordinary release launches.

## Visual/device coverage

The isolated iOS 16 iPad simulator displays both Chinese rows of the reported
Wriggle scene, including `我可是在的啊！`, without the unrelated Extra label.
The same delayed-row scene was captured in English, displaying
`I've been here the whole time!` alongside the preceding translated row.
Device builds target arm64 and iOS 14.0. Real-device confirmation remains
necessary; lookup coverage does not establish performance or layout on every
device, language, and route.
