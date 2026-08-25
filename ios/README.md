# Building the TH08 iOS bring-up

On the Mac, clone this repository and point the build at a directory containing
the legally obtained original `th08.dat`, `thbgm.dat`, and (for Japanese text)
`msgothic.ttc`:

```sh
git clone https://github.com/Ymgjdsh/th08-ios-port.git
cd th08-ios-port
chmod +x ios/*.sh
TH08_IOS_DATA_DIR="$HOME/TH08-data" \
  IOS_VERSION=0.1.0 IOS_BUILD=1 \
  ./ios/build_ios.sh
```

The IPA is written under `build-ios/`. CMake downloads pinned SDL2 sources on
the first configure, so the initial build needs network access. No original
executable or data archive is committed to this repository.

This is currently a bring-up build, not a release claim. See
`PORTING_STATUS.md` for the arm64 layout, renderer, and fixed-address work that
still has to be completed before device gameplay is expected.
