#pragma once

#ifndef TH08_MODERN_IOS
#error "This header is only for the modern iOS build."
#endif

#include <strings.h>
#include <stdint.h>
#include <windows.h>
#include <mmsystem.h>
#include <d3d8.h>

// The reconstructed structs retain 32-bit retail offsets. iOS is arm64, so
// these assertions are intentionally disabled until the portable-layout pass
// replaces the remaining target-offset views with symbolic storage.
#undef C_ASSERT
#define C_ASSERT(e)

#define TH08_IOS_EXPERIMENTAL_LAYOUT 1

#define _snwprintf swprintf
#define D3D_WRAPPER 0
#define VK_SHIFT 0x10
#define VK_CONTROL 0x11
#define VK_RETURN 0x0d
#define VK_ESCAPE 0x1b
#define VK_HOME 0x24
#define VK_LEFT 0x25
#define VK_UP 0x26
#define VK_RIGHT 0x27
#define VK_DOWN 0x28
#define VK_NUMPAD1 0x61
#define VK_NUMPAD2 0x62
#define VK_NUMPAD3 0x63
#define VK_NUMPAD4 0x64
#define VK_NUMPAD6 0x66
#define VK_NUMPAD7 0x67
#define VK_NUMPAD8 0x68
#define VK_NUMPAD9 0x69

#define TRANSPARENT 1
#define BI_RGB 0
#define DIB_RGB_COLORS 0
#define FW_NORMAL 400
#define FW_BOLD 700
#define FW_SEMIBOLD 600
#define DEFAULT_CHARSET 1
#define SHIFTJIS_CHARSET 128
#define OUT_DEFAULT_PRECIS 0
#define CLIP_DEFAULT_PRECIS 0
#define ANTIALIASED_QUALITY 4
#define DEFAULT_PITCH 0
#define FIXED_PITCH 1
#define FF_ROMAN 16

typedef LPDIRECT3D8 PDIRECT3D8;
typedef LPDIRECT3DDEVICE8 PDIRECT3DDEVICE8;

// The retail data layouts store pointers in four-byte slots.  Keeping native
// arm64 pointers in those slots shifts every following fixed-offset field.
// IosPtr32 stores a process-local handle while preserving the original size.
DWORD TH08IosStorePointer(uintptr_t pointerValue);
uintptr_t TH08IosLoadPointer(DWORD handle);
void TH08IosForgetPointer(uintptr_t pointerValue);

// Retail text is CP932. Localized dialogue and replacement menu labels opt in
// to UTF-8 only while their text texture is being generated.
BOOL TH08IosSetGdiTextUtf8(BOOL enabled);
BOOL TH08IosGetGdiTextUtf8();

// Rasterize a localized UTF-8 label directly into an RGBA8 destination.
// This bypasses the retail A1R5G5B5 GDI path, whose alpha convention is not
// representable reliably by the GLES2 compatibility layer.
BOOL TH08IosRasterizeTextUtf8(const char *text, int pixelSize, int width, int height,
                              COLORREF textColor, COLORREF shadowColor,
                              BYTE *destination, int destinationPitch);

// Mark a texture whose locked surface was filled by the iOS UTF-8 rasterizer.
// D3DFMT_A8R8G8B8 normally stores legacy BGRA bytes, while the rasterizer
// writes canonical RGBA bytes. The GLES upload path uses this marker to avoid
// decoding the text surface a second time.
void TH08IosMarkTextureRgba(IDirect3DTexture8 *texture);

template <typename T> struct IosPtr32
{
    DWORD handle;

    // Raw 32-bit fields are used by a few on-disk formats.  They must not be
    // passed through the process-local pointer registry until the owner has
    // added its file base and explicitly relocates them.
    DWORD RawHandle() const { return this->handle; }
    void SetRawHandle(DWORD value) { this->handle = value; }

    void SetPointer(T *pointer)
    {
        this->handle = TH08IosStorePointer(reinterpret_cast<uintptr_t>(pointer));
    }

    IosPtr32 &operator=(T *pointer)
    {
        this->SetPointer(pointer);
        return *this;
    }

    T *Get() const
    {
        return reinterpret_cast<T *>(TH08IosLoadPointer(this->handle));
    }

    operator T *() const
    {
        return this->Get();
    }

    T *operator->() const
    {
        return this->Get();
    }

    void operator()() const
    {
        T *function = this->Get();
        (*function)();
    }
};

typedef char TH08IosPtr32SizeCheck[(sizeof(IosPtr32<void>) == 4) ? 1 : -1];
