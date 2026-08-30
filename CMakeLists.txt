cmake_minimum_required(VERSION 3.20)

project(th08-modern LANGUAGES CXX)

set(TH08_IOS_VERSION "0.1.0" CACHE STRING "iOS marketing version")
set(TH08_IOS_BUILD "1" CACHE STRING "iOS build number")
set(TH08_IOS_BUNDLE_ID "com.th08.sdl2.ios" CACHE STRING "iOS bundle identifier")
set(TH08_IOS_DATA_DIR "" CACHE PATH "Optional original TH08 data directory to bundle")

if(NOT APPLE AND NOT CMAKE_SIZEOF_VOID_P EQUAL 4)
    message(FATAL_ERROR "TH08 currently requires a 32-bit toolchain.")
endif()

find_package(Python3 REQUIRED COMPONENTS Interpreter)

set(TH08_DX8_SDK_ROOT "${CMAKE_SOURCE_DIR}/scripts/prefix/mssdk" CACHE PATH
    "DirectX 8 SDK root containing include/ and lib/")
if(WIN32 AND NOT EXISTS "${TH08_DX8_SDK_ROOT}/include/d3dx8.h")
    message(FATAL_ERROR
        "DirectX 8 SDK headers were not found. Set TH08_DX8_SDK_ROOT or run the existing prefix setup first.")
endif()

set(TH08_GENERATED_DIR "${CMAKE_BINARY_DIR}/generated")
add_custom_command(
    OUTPUT "${TH08_GENERATED_DIR}/i18n.hpp"
    COMMAND "${CMAKE_COMMAND}" -E make_directory "${TH08_GENERATED_DIR}"
    COMMAND "${Python3_EXECUTABLE}" "${CMAKE_SOURCE_DIR}/scripts/generate_i18n.py"
            -o "${TH08_GENERATED_DIR}/i18n.hpp" --shift-jis
            "${CMAKE_SOURCE_DIR}/config/i18n.csv"
    DEPENDS
        "${CMAKE_SOURCE_DIR}/config/i18n.csv"
        "${CMAKE_SOURCE_DIR}/scripts/generate_i18n.py"
    VERBATIM
)
add_custom_target(th08-modern-generated DEPENDS "${TH08_GENERATED_DIR}/i18n.hpp")

set(TH08_GAME_SOURCES
    src/AnmManager.cpp
    src/AsciiManager.cpp
    src/AsciiManagerBossMarker.cpp
    src/AsciiManagerGauge.cpp
    src/AsciiManagerGuiMode.cpp
    src/AsciiManagerScale.cpp
    src/Background.cpp
    src/BulletManager.cpp
    src/EclDependencies.cpp
    src/EclExIns.cpp
    src/EclGlobals.cpp
    src/EclHelpers.cpp
    src/EclManager.cpp
    src/EclOperandsFloat.cpp
    src/EclOperandsInt.cpp
    src/EclRun.cpp
    src/EffectManager.cpp
    src/Ending.cpp
    src/EnemyManager.cpp
    src/EnemyManagerUpdate.cpp
    src/EnemyTimeline.cpp
    src/GameManager.cpp
    src/Global.cpp
    src/Gui.cpp
    src/ItemManager.cpp
    src/Midi.cpp
    src/MusicRoom.cpp
    src/Player.cpp
    src/PlayerBomb.cpp
    src/ReplayManager.cpp
    src/ResultScreen.cpp
    src/ScoreDat.cpp
    src/ScreenEffect.cpp
    src/SoundPlayer.cpp
    src/Spellcard.cpp
    src/Supervisor.cpp
    src/TextHelper.cpp
    src/TitleScreen.cpp
    src/main.cpp
    src/utils.cpp
    src/zwave.cpp
    src/pbg/Lzss.cpp
    src/pbg/PbgArchive.cpp
    src/pbg/PbgFile.cpp
)

if(WIN32)
    list(APPEND TH08_GAME_SOURCES src/modern/windows_runtime.cpp)
    add_executable(th08-modern WIN32 ${TH08_GAME_SOURCES})
elseif(UNIX AND NOT APPLE)
    list(APPEND TH08_GAME_SOURCES
        src/modern/linux/d3d8_compat.cpp
        src/modern/linux/d3dx8_compat.cpp
        src/modern/linux/linux_compat.cpp
        src/modern/linux/linux_runtime.cpp
    )
    add_executable(th08-modern ${TH08_GAME_SOURCES})
elseif(APPLE)
    enable_language(OBJC)
    enable_language(OBJCXX)
    set(TH08_IOS_DIALOGUE_GENERATED "${TH08_GENERATED_DIR}/ios_dialogue_i18n.hpp")
    file(GLOB_RECURSE TH08_IOS_DIALOGUE_DATA CONFIGURE_DEPENDS
        "${CMAKE_SOURCE_DIR}/resources/localization/th08/*.jdiff")
    add_custom_command(
        OUTPUT "${TH08_IOS_DIALOGUE_GENERATED}"
        COMMAND "${CMAKE_COMMAND}" -E make_directory "${TH08_GENERATED_DIR}"
        COMMAND "${Python3_EXECUTABLE}"
                "${CMAKE_SOURCE_DIR}/scripts/generate_ios_dialogue_i18n.py"
                --root "${CMAKE_SOURCE_DIR}/resources/localization/th08"
                --output "${TH08_IOS_DIALOGUE_GENERATED}"
        DEPENDS
            "${CMAKE_SOURCE_DIR}/scripts/generate_ios_dialogue_i18n.py"
            ${TH08_IOS_DIALOGUE_DATA}
        VERBATIM
    )
    add_custom_target(th08-ios-dialogue-generated DEPENDS "${TH08_IOS_DIALOGUE_GENERATED}")
    list(APPEND TH08_GAME_SOURCES
        src/modern/ios/ios_compat.cpp
        src/modern/ios/ios_runtime.cpp
        src/modern/ios/ios_d3d8_compat.cpp
        src/modern/ios/ios_d3dx8_compat.cpp
        src/modern/ios/ios_gl_legacy.cpp
        src/modern/ios/ios_localization.cpp
        src/modern/ios/ios_touch.cpp
        src/modern/ios/ios_cheat.mm)
    set_source_files_properties(src/modern/ios/ios_cheat.mm PROPERTIES
        COMPILE_DEFINITIONS "TH08_IOS_CHEAT_OBJC=1;__OBJC_BOOL_IS_BOOL=0")
    add_executable(th08-modern MACOSX_BUNDLE ${TH08_GAME_SOURCES})
else()
    message(FATAL_ERROR "The modern target currently supports Windows and Linux.")
endif()
add_dependencies(th08-modern th08-modern-generated)
if(APPLE)
    add_dependencies(th08-modern th08-ios-dialogue-generated)
endif()
target_compile_features(th08-modern PRIVATE cxx_std_98)
target_compile_definitions(th08-modern PRIVATE
    TH08_MODERN_PORT
    WIN32_LEAN_AND_MEAN
)
target_include_directories(th08-modern PRIVATE
    "${CMAKE_SOURCE_DIR}/src"
    "${TH08_GENERATED_DIR}"
)

if(MINGW)
    # MinGW ships compatible D3D8/DirectInput/DirectSound headers and import
    # libraries, but not D3DX8. Search the legacy SDK only after the toolchain
    # headers so its old Windows SDK does not shadow modern system headers.
    set(TH08_DX8_SANITIZED_INCLUDE "${CMAKE_BINARY_DIR}/dx8-include")
    file(COPY "${TH08_DX8_SDK_ROOT}/include/" DESTINATION "${TH08_DX8_SANITIZED_INCLUDE}")
    file(GLOB TH08_DX8_HEADERS "${TH08_DX8_SANITIZED_INCLUDE}/*")
    string(ASCII 26 TH08_DOS_EOF)
    foreach(TH08_DX8_HEADER IN LISTS TH08_DX8_HEADERS)
        if(NOT IS_DIRECTORY "${TH08_DX8_HEADER}")
            file(READ "${TH08_DX8_HEADER}" TH08_DX8_HEADER_CONTENT)
            string(REPLACE "${TH08_DOS_EOF}" "" TH08_DX8_HEADER_CONTENT "${TH08_DX8_HEADER_CONTENT}")
            file(WRITE "${TH08_DX8_HEADER}" "${TH08_DX8_HEADER_CONTENT}")
        endif()
    endforeach()

    target_compile_options(th08-modern PRIVATE
        # The reconstructed source preserves the original VC7 /Od boundaries;
        # cross-TU inline bodies are not available for modern optimization.
        -O0
        -fpermissive
        -mms-bitfields
        -Wno-write-strings
        "-include${CMAKE_SOURCE_DIR}/src/modern/windows_compat.hpp"
        "-idirafter${TH08_DX8_SANITIZED_INCLUDE}"
    )
    target_link_options(th08-modern PRIVATE
        -static
        -static-libgcc
        -static-libstdc++
        # Some reconstructed data references still use target absolute VAs.
        # Keep the original image base until every such owner is symbolic.
        -Wl,--disable-dynamicbase
    )
    target_link_libraries(th08-modern PRIVATE
        # The retail D3DX8 library is a static MSVC C++ archive and cannot be
        # mixed with MinGW's C++ ABI. The SDK's debug DLL exposes the same C
        # API through an import library, which is suitable for bring-up builds.
        "${TH08_DX8_SDK_ROOT}/lib/d3dx8d.lib"
        dxguid dinput8 dsound d3d8 winmm
        kernel32 user32 gdi32 advapi32 shell32 ole32 oleaut32 uuid dbghelp
    )
    set(TH08_D3DX8_DEBUG_DLL
        "${TH08_DX8_SDK_ROOT}/samples/multimedia/support/d3dx8d.dll")
    if(EXISTS "${TH08_D3DX8_DEBUG_DLL}")
        add_custom_command(TARGET th08-modern POST_BUILD
            COMMAND "${CMAKE_COMMAND}" -E copy_if_different
                    "${TH08_D3DX8_DEBUG_DLL}" "$<TARGET_FILE_DIR:th08-modern>"
            VERBATIM
        )
    else()
        message(WARNING "d3dx8d.dll was not found; th08-modern.exe will build but needs that SDK DLL to start.")
    endif()
elseif(MSVC)
    target_include_directories(th08-modern SYSTEM PRIVATE "${TH08_DX8_SDK_ROOT}/include")
    target_link_directories(th08-modern PRIVATE "${TH08_DX8_SDK_ROOT}/lib")
    target_compile_options(th08-modern PRIVATE /W3)
    target_link_options(th08-modern PRIVATE /DYNAMICBASE:NO)
    target_link_libraries(th08-modern PRIVATE
        dxguid d3dx8 dinput8 dsound d3d8 winmm
        kernel32 user32 gdi32 advapi32 shell32 ole32 oleaut32 uuid dbghelp
    )
elseif(UNIX AND NOT APPLE)
    find_package(PkgConfig REQUIRED)
    pkg_check_modules(SDL2 REQUIRED IMPORTED_TARGET sdl2)
    pkg_check_modules(SDL2_IMAGE REQUIRED IMPORTED_TARGET SDL2_image)
    pkg_check_modules(SDL2_TTF REQUIRED IMPORTED_TARGET SDL2_ttf)
    find_package(Fontconfig REQUIRED)
    # Prefer the broadly deployed libGL.so.1 runtime ABI over GLVND's separate
    # libOpenGL.so.0 dispatch library for the 32-bit portable build.
    set(OpenGL_GL_PREFERENCE LEGACY)
    find_package(OpenGL REQUIRED)
    find_package(Threads REQUIRED)

    target_compile_definitions(th08-modern PRIVATE
        TH08_MODERN_LINUX
        _FILE_OFFSET_BITS=64
        _TIME_BITS=64
    )
    target_compile_options(th08-modern PRIVATE
        # Keep the original /Od-style TU boundaries in every CMake build type.
        -O0
        -fpermissive
        -fdata-sections
        -fno-omit-frame-pointer
        -fno-pie
        -Wno-write-strings
        "-include${CMAKE_SOURCE_DIR}/src/modern/linux/linux_compat.hpp"
    )
    target_link_options(th08-modern PRIVATE
        -no-pie
        -rdynamic
        "-Wl,-T,${CMAKE_SOURCE_DIR}/src/modern/linux/th08-layout.ld"
    )
    set_property(TARGET th08-modern APPEND PROPERTY LINK_DEPENDS
        "${CMAKE_SOURCE_DIR}/src/modern/linux/th08-layout.ld"
    )
    target_include_directories(th08-modern BEFORE PRIVATE
        "${CMAKE_SOURCE_DIR}/src/modern/linux/include"
    )
    target_link_libraries(th08-modern PRIVATE
        PkgConfig::SDL2
        PkgConfig::SDL2_IMAGE
        PkgConfig::SDL2_TTF
        Fontconfig::Fontconfig
        OpenGL::GL
        Threads::Threads
        dl
    )
    add_custom_command(TARGET th08-modern POST_BUILD
        COMMAND "${CMAKE_COMMAND}" -E copy_if_different
                "${CMAKE_SOURCE_DIR}/resources/modern-icon.png"
                "$<TARGET_FILE_DIR:th08-modern>/th08-modern.png"
                VERBATIM
    )
elseif(APPLE)
    enable_language(OBJC)
    enable_language(OBJCXX)

    # iOS uses a static SDL2 stack. Fetching the pinned releases at configure
    # time keeps the source repository free of third-party build artifacts.
    include(FetchContent)
    set(BUILD_SHARED_LIBS OFF CACHE BOOL "Build static libraries" FORCE)
    set(SDL_SHARED OFF CACHE BOOL "" FORCE)
    set(SDL_STATIC ON CACHE BOOL "" FORCE)
    set(SDL_TESTS OFF CACHE BOOL "" FORCE)
    set(SDL_TEST_LIBRARY OFF CACHE BOOL "" FORCE)
    set(SDL_EXAMPLES OFF CACHE BOOL "" FORCE)
    set(SDL_INSTALL OFF CACHE BOOL "" FORCE)
    set(SDL_GPU OFF CACHE BOOL "" FORCE)
    set(SDL_RENDER ON CACHE BOOL "" FORCE)
    set(SDL_METAL OFF CACHE BOOL "" FORCE)
    # The game uses the OpenGL ES compatibility path.  Do not compile SDL's
    # optional Metal renderer; it is unused and its iOS view-tag code is not
    # available in the pinned SDL release with Xcode's current SDK headers.
    set(SDL_RENDER_METAL OFF CACHE BOOL "" FORCE)
    set(SDL_VULKAN OFF CACHE BOOL "" FORCE)
    set(SDL_OPENGLES ON CACHE BOOL "" FORCE)
    set(SDL2IMAGE_VENDORED ON CACHE BOOL "" FORCE)
    set(SDL2IMAGE_BACKEND_STB ON CACHE BOOL "" FORCE)
    set(SDL2IMAGE_BACKEND_IMAGEIO OFF CACHE BOOL "" FORCE)
    set(SDL2IMAGE_PNG ON CACHE BOOL "" FORCE)
    set(SDL2IMAGE_JPG ON CACHE BOOL "" FORCE)
    set(SDL2IMAGE_BMP OFF CACHE BOOL "" FORCE)
    set(SDL2IMAGE_TIF OFF CACHE BOOL "" FORCE)
    set(SDL2IMAGE_WEBP OFF CACHE BOOL "" FORCE)
    set(SDL2IMAGE_INSTALL OFF CACHE BOOL "" FORCE)
    set(SDL2MIXER_INSTALL OFF CACHE BOOL "" FORCE)
    set(SDL2MIXER_VENDORED ON CACHE BOOL "" FORCE)
    set(SDL2MIXER_MIDI OFF CACHE BOOL "" FORCE)
    set(SDL2MIXER_MOD OFF CACHE BOOL "" FORCE)
    set(SDL2MIXER_MP3 OFF CACHE BOOL "" FORCE)
    set(SDL2MIXER_FLAC OFF CACHE BOOL "" FORCE)
    set(SDL2MIXER_OPUS OFF CACHE BOOL "" FORCE)
    set(SDL2MIXER_WAVE ON CACHE BOOL "" FORCE)
    set(SDL2MIXER_WAVPACK OFF CACHE BOOL "" FORCE)
    set(SDL2MIXER_VORBIS STB CACHE STRING "" FORCE)
    set(SDL2TTF_VENDORED ON CACHE BOOL "" FORCE)
    set(SDL2TTF_SAMPLES OFF CACHE BOOL "" FORCE)
    set(SDL2TTF_TESTS OFF CACHE BOOL "" FORCE)
    set(SDL2TTF_INSTALL OFF CACHE BOOL "" FORCE)
    FetchContent_Declare(SDL2
        GIT_REPOSITORY https://github.com/libsdl-org/SDL.git
        GIT_TAG release-2.30.9)
    FetchContent_Declare(SDL2_image
        GIT_REPOSITORY https://github.com/libsdl-org/SDL_image.git
        GIT_TAG release-2.8.2)
    FetchContent_Declare(SDL2_mixer
        GIT_REPOSITORY https://github.com/libsdl-org/SDL_mixer.git
        GIT_TAG release-2.8.0)
    FetchContent_Declare(SDL2_ttf
        GIT_REPOSITORY https://github.com/libsdl-org/SDL_ttf.git
        GIT_TAG release-2.22.0)
    FetchContent_MakeAvailable(SDL2 SDL2_image SDL2_mixer SDL2_ttf)
    if(TARGET SDL2-static AND NOT TARGET SDL2::SDL2-static)
        add_library(SDL2::SDL2-static ALIAS SDL2-static)
    endif()
    if(TARGET SDL2_image AND NOT TARGET SDL2_image::SDL2_image)
        add_library(SDL2_image::SDL2_image ALIAS SDL2_image)
    endif()
    if(TARGET SDL2_mixer AND NOT TARGET SDL2_mixer::SDL2_mixer)
        add_library(SDL2_mixer::SDL2_mixer ALIAS SDL2_mixer)
    endif()
    if(TARGET SDL2_ttf AND NOT TARGET SDL2_ttf::SDL2_ttf)
        add_library(SDL2_ttf::SDL2_ttf ALIAS SDL2_ttf)
    endif()

    configure_file("${CMAKE_SOURCE_DIR}/ios/Info.plist.in"
        "${CMAKE_BINARY_DIR}/th08-Info.plist" @ONLY)
    set_target_properties(th08-modern PROPERTIES
        OUTPUT_NAME "th08"
        MACOSX_BUNDLE TRUE
        MACOSX_BUNDLE_INFO_PLIST "${CMAKE_BINARY_DIR}/th08-Info.plist"
        XCODE_ATTRIBUTE_PRODUCT_BUNDLE_IDENTIFIER "${TH08_IOS_BUNDLE_ID}"
        XCODE_ATTRIBUTE_MARKETING_VERSION "${TH08_IOS_VERSION}"
        XCODE_ATTRIBUTE_CURRENT_PROJECT_VERSION "${TH08_IOS_BUILD}"
        XCODE_ATTRIBUTE_IPHONEOS_DEPLOYMENT_TARGET "14.0"
        XCODE_ATTRIBUTE_TARGETED_DEVICE_FAMILY "1,2"
        XCODE_ATTRIBUTE_CLANG_CXX_LANGUAGE_STANDARD "c++98"
        XCODE_ATTRIBUTE_CLANG_CXX_LIBRARY "libc++"
        XCODE_ATTRIBUTE_CODE_SIGNING_ALLOWED "NO"
        XCODE_ATTRIBUTE_CODE_SIGN_IDENTITY ""
        XCODE_ATTRIBUTE_ASSETCATALOG_COMPILER_APPICON_NAME "AppIcon")
    target_compile_definitions(th08-modern PRIVATE
        TH08_MODERN_PORT TH08_MODERN_IOS TH08_IOS
        TH08_IOS_VERSION="${TH08_IOS_VERSION}"
        TH08_IOS_BUILD="${TH08_IOS_BUILD}")
    target_compile_options(th08-modern PRIVATE
        -O0 -fno-fast-math -fno-strict-aliasing -Wno-deprecated-declarations
        "-include${CMAKE_SOURCE_DIR}/src/modern/ios/ios_compat.hpp")
    target_include_directories(th08-modern BEFORE PRIVATE
        "${CMAKE_SOURCE_DIR}/src/modern/linux/include"
        "${CMAKE_SOURCE_DIR}/src/modern/linux"
        "${CMAKE_SOURCE_DIR}/src/modern/ios"
        "${SDL2_SOURCE_DIR}/include")
    target_link_libraries(th08-modern PRIVATE
        SDL2::SDL2main SDL2::SDL2-static
        SDL2_image::SDL2_image SDL2_mixer::SDL2_mixer
        SDL2_ttf::SDL2_ttf
        "-framework OpenGLES" "-framework UIKit" "-framework Foundation"
        "-framework CoreFoundation" "-framework CoreGraphics" "-framework QuartzCore" "-framework AudioToolbox"
        "-framework AVFoundation" "-framework CoreAudio")
    if(EXISTS "${CMAKE_SOURCE_DIR}/ios/LaunchScreen.storyboard")
        target_sources(th08-modern PRIVATE ios/LaunchScreen.storyboard)
        set_source_files_properties(ios/LaunchScreen.storyboard PROPERTIES
            MACOSX_PACKAGE_LOCATION Resources)
    endif()
    set(TH08_IOS_LAUNCH_ICON
        "${CMAKE_SOURCE_DIR}/ios/Assets.xcassets/AppIcon.appiconset/Icon-1024.png")
    if(EXISTS "${TH08_IOS_LAUNCH_ICON}")
        target_sources(th08-modern PRIVATE "${TH08_IOS_LAUNCH_ICON}")
        set_source_files_properties("${TH08_IOS_LAUNCH_ICON}" PROPERTIES
            MACOSX_PACKAGE_LOCATION Resources)
    endif()
    if(EXISTS "${CMAKE_SOURCE_DIR}/ios/Assets.xcassets")
        target_sources(th08-modern PRIVATE ios/Assets.xcassets)
        set_source_files_properties(ios/Assets.xcassets PROPERTIES
            MACOSX_PACKAGE_LOCATION Resources)
    endif()
    foreach(TH08_DATA_FILE th08.dat thbgm.dat msgothic.ttc)
        if(TH08_IOS_DATA_DIR AND EXISTS "${TH08_IOS_DATA_DIR}/${TH08_DATA_FILE}")
            target_sources(th08-modern PRIVATE "${TH08_IOS_DATA_DIR}/${TH08_DATA_FILE}")
            set_source_files_properties("${TH08_IOS_DATA_DIR}/${TH08_DATA_FILE}"
                PROPERTIES MACOSX_PACKAGE_LOCATION Resources)
        endif()
    endforeach()
    # Localized Chinese labels use a CJK-capable font. Keep it as a project
    # resource so the same rasterization path works on every iOS device; the
    # Japanese retail font remains separately bundled from TH08_IOS_DATA_DIR.
    set(TH08_IOS_CJK_FONT "${CMAKE_SOURCE_DIR}/resources/NotoSansSC-VF.ttf")
    if(EXISTS "${TH08_IOS_CJK_FONT}")
        target_sources(th08-modern PRIVATE "${TH08_IOS_CJK_FONT}")
        set_source_files_properties("${TH08_IOS_CJK_FONT}"
            PROPERTIES MACOSX_PACKAGE_LOCATION Resources)
    endif()
else()
    message(FATAL_ERROR "The modern Windows target currently supports MSVC and MinGW.")
endif()

if(NOT IOS)
    set_target_properties(th08-modern PROPERTIES OUTPUT_NAME "th08-modern")
endif()
