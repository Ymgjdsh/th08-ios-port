# Building the TH08 iOS port

Use a Mac with Xcode, CMake, Python 3, and legally obtained copies of
`th08.dat`, `thbgm.dat`, and `msgothic.ttc`. Put all three required files in
one directory:

```sh
git clone https://github.com/Ymgjdsh/th08-ios-port.git
cd th08-ios-port
chmod +x ios/*.sh

TH08_IOS_DATA_DIR="$HOME/TH08-data" \
  IOS_VERSION=0.5.6 IOS_BUILD=139 \
  ./ios/build_ios.sh
```

The initial configure downloads the pinned SDL2 family sources and therefore
needs network access. `msgothic.ttc` is required for Japanese dialogue, menus,
and Music Room text and is never stored in this repository. For repeatable
offline builds, set `TH08_IOS_DEPS_DIR`
to a directory containing these source trees:

```text
SDL-release-2.30.9
SDL_image-release-2.8.2
SDL_mixer-release-2.8.0
SDL_ttf-release-2.22.0
```

Useful overrides are `BUILD_DIR`, `OUTPUT_IPA`, `IOS_SDK`, `IOS_ARCH`, and
`IOS_DEPLOYMENT_TARGET`. Their defaults produce a Release arm64 device build
with a minimum deployment target of iOS 14.0. The IPA is written below
`build-ios/` unless `OUTPUT_IPA` is set.

The packaging step compiles the asset catalog and launch storyboard, applies
an ad-hoc signature, and rejects a stale app bundle whose version does not
match the requested `IOS_VERSION` and `IOS_BUILD`. It never downloads or
commits original game data.

See [PORTING_STATUS.md](PORTING_STATUS.md) for the implemented mobile features,
the latest complete-campaign checkpoint, and remaining test coverage.
