# TH08 iOS port status

This is an independent mobile-port lane based on the playable Linux
reconstruction. The original VC7 target and the Linux build remain untouched.

The first bring-up adds:

- an Xcode/iOS CMake target for arm64 and iOS 14;
- static SDL2, SDL2_image, and SDL2_mixer dependencies fetched at configure;
- an IPA packaging script that bundles only user-provided `th08.dat`,
  `thbgm.dat`, and an optional Japanese font;
- SDL touch input mapped to the game's existing Touhou button bitset;
- a diagnostics log under the app's data directory.

Known blockers before this can be called a playable release:

1. The reconstructed gameplay structs preserve 32-bit retail offsets while an
   iOS device is arm64. The experimental build disables layout assertions only
   to expose compiler errors; it is not a correctness claim.
2. The Linux D3D8 compatibility renderer uses desktop fixed-function OpenGL.
   `ios_gl_legacy.hpp` currently isolates those calls as a temporary compile
   bridge. It must be replaced with an OpenGL ES 2 renderer before shipping.
3. Fixed target-address globals must be moved to symbolic portable storage;
   iOS does not permit mapping the original 32-bit address image.

The build scripts therefore intentionally label this as a bring-up package.
Each blocker is isolated so the Mac build can report the next concrete source
or linker failure instead of producing an unexplained black-screen IPA.
