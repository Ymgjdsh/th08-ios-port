#pragma once

#include "Global.hpp"
#include "AnmManager.hpp"
#include "Supervisor.hpp"
#include "ZunResult.hpp"

#include <stddef.h>

namespace th08
{

struct Enemy;
struct AnmVm;

struct Effect
{
    AnmVm vm;
    Float3 vector0;
    Float3 vector1;
    Float3 vector2;
    Float3 vector3;
    Float3 vector4;
    Float3 vector5;
    Float3 vector6;
    Float3 vector7;
    Float3 vector8;
    unknown_fields(0x310, 0x28);
    ZunTimer timer;
    unknown_fields(0x344, 0x1c);

    Effect();
};
C_ASSERT(sizeof(Effect) == 0x360);

#ifdef TH08_IOS
typedef i32 (__fastcall *IosEffectUpdateCallback)(AnmVm *effect);
typedef i32 (__fastcall *IosEffectDrawCallback)(Effect *effect);

// Retail effects store four pointers in adjacent 32-bit fields.  Keep those
// pointers in native-width side storage on arm64 so one field cannot overwrite
// the next effect record.
void SetIosEffectUpdateCallback(AnmVm *effect, IosEffectUpdateCallback callback);
void SetIosEffectDrawCallback(AnmVm *effect, IosEffectDrawCallback callback);
void *GetIosEffectResource(AnmVm *effect);
void SetIosEffectResource(AnmVm *effect, void *resource);
ZunBool RunIosEffectDrawCallback(AnmVm *effect);
#endif

struct EffectManager
{
    EffectManager();

    i32 unknown0;
    i32 unknown4;
    i32 activeCount;
    f32 scaleX;
    f32 scaleY;
    f32 scaleZ;
    f32 scaleW;
    Effect effects[654];
    Effect sentinel0;
    Effect sentinel1;
    Effect sentinel2;
    Effect sentinel3;
    Effect sentinel4;
    unknown_fields(0x8b03c, 0x20);

    void ResetEffects();
    static ChainCallbackResult OnUpdate(EffectManager *effectManager);
    static ChainCallbackResult OnDraw(EffectManager *effectManager);
    static ZunResult AddedCallback(EffectManager *effectManager);
    static ZunResult DeletedCallback(EffectManager *effectManager);
    static ZunResult RegisterChain();
    static void CutChain();
    AnmVm *SpawnEffect00425B70(i32 id, D3DXVECTOR3 *position, i32 count, i32 color);
    AnmVm *SpawnEffect(i32 id, D3DXVECTOR3 *position, i32 count, i32 color);
    AnmVm *SpawnEffectAngle(i32 id, D3DXVECTOR3 *position, D3DXVECTOR3 *velocity, i32 count, i32 color);
    AnmVm *FUN_004253e0(i32 index);
    AnmVm *FUN_00425870(i32 id, D3DXVECTOR3 *position, i32 slotIndex, i32 unused, i32 color);
    AnmVm *FUN_004259e0(i32 id, D3DXVECTOR3 *position, D3DXVECTOR3 *velocity, i32 slotIndex, i32 unused, i32 color);
    i32 DrawUnkTypeEffects();
    i32 FUN_004281e0();
};
C_ASSERT(sizeof(EffectManager) == 0x8b05c);
extern EffectManager g_EffectManager;
void __fastcall FUN_00426d10(Float3 *delta);
DIFFABLE_EXTERN(ChainElem, g_EffectManagerCalcChain);
DIFFABLE_EXTERN(ChainElem, g_EffectManagerDrawChain);

// A dword published by an ECL enemy callback from the target object's +0x18
// field.  On relocatable targets these are views into their owning objects,
// rather than independent globals.
#if defined(TH08_MODERN_WEB) || defined(TH08_MODERN_IOS)
extern i32 &g_EclCallbackPublishedEnemyField24;
extern i32 &g_EclCallbackPublishedEnemyField56;
#else
DIFFABLE_EXTERN(i32, g_EclCallbackPublishedEnemyField24);
DIFFABLE_EXTERN(i32, g_EclCallbackPublishedEnemyField56);
#endif
struct EclExBarrierRenderState
{
    i32 mode;
    i32 unknown4;
    AnmVm vm0;
    AnmVm vm1;
};
C_ASSERT(offsetof(EclExBarrierRenderState, vm0) == 0x8);
C_ASSERT(offsetof(EclExBarrierRenderState, vm1) == 0x2ac);
#if defined(TH08_MODERN_WEB) || defined(TH08_MODERN_IOS)
extern i8 &g_EclScriptedGlobalUpdateFreeze;
extern f32 &g_EclGameTimeScale;
extern u32 &g_EclGameTimeScaleFlags;
#else
DIFFABLE_EXTERN(i8, g_EclScriptedGlobalUpdateFreeze);
DIFFABLE_EXTERN(f32, g_EclGameTimeScale);
DIFFABLE_EXTERN(u32, g_EclGameTimeScaleFlags);
#endif
struct EclRawInstruction
{
    i32 time;
    i16 opcode;
    i16 nextOffset;
    u8 unknown08;
    u8 difficultyMask;
    u16 operandFlags;
    u8 operands[1];
};

// Only the target-observed fixed header is named here. Both pointer tables are
// stored as file-relative offsets and are rebased in place by Load.
struct EclRawHeader
{
    u32 version;
    i16 subCount;
    i16 unknown06;
    u32 timelineOffsets[16];
    u32 subOffsets[1];
};
typedef char EclRawHeaderSubTableOffsetCheck[offsetof(EclRawHeader, subOffsets) == 0x48 ? 1 : -1];

// Target-observed TH08 per-enemy ECL context. Unknown spans deliberately keep
// the public ABI narrower than the still-unrecovered interpreter state.
struct EnemyEclContext
{
#ifdef TH08_IOS
    IosPtr32<EclRawInstruction> currentInstr; // +0x000
#else
    EclRawInstruction *currentInstr; // +0x000
#endif
    ZunTimer time;                   // +0x004
    u8 unknown10[0x80];
    ZunTimer secondaryTime; // +0x090
    u8 unknown9C[0x188];
    i16 subId; // +0x224
    u8 unknown226[2];
};
typedef char EnemyEclContextSecondaryTimerOffsetCheck[offsetof(EnemyEclContext, secondaryTime) == 0x90 ? 1 : -1];
typedef char EnemyEclContextSubIdOffsetCheck[offsetof(EnemyEclContext, subId) == 0x224 ? 1 : -1];
typedef char EnemyEclContextSizeCheck[sizeof(EnemyEclContext) == 0x228 ? 1 : -1];

#ifdef TH08_IOS
// ECL runtime state contains several adjacent retail pointer fields. Accessing
// them as native arm64 pointers overwrites the following field, so every iOS
// access must go through the same four-byte handle representation used by the
// reconstructed structures.
template <typename T>
inline IosPtr32<T> &IosEclPointerField(void *base, size_t offset)
{
    return *reinterpret_cast<IosPtr32<T> *>(reinterpret_cast<u8 *>(base) + offset);
}

inline EnemyEclContext *IosEclCurrentContext(void *enemy)
{
    return IosEclPointerField<EnemyEclContext>(enemy, 0x2CA0).Get();
}

inline u8 *IosEclCurrentVariables(void *enemy)
{
    return IosEclPointerField<u8>(enemy, 0x2CA4).Get();
}

inline IosPtr32<u8> &IosEclChildContextField(void *enemy, i32 index)
{
    return IosEclPointerField<u8>(enemy, 0x3384 + index * 4);
}
#endif

struct EclTimelineState
{
    EclTimelineState();

    u8 unknown000[0x100];
    D3DXVECTOR3 vectors[8];
};

struct EclManager
{
    EclManager();
    ZunResult Load(char *path);
    void Unload();
    ZunResult CallEclSub(EnemyEclContext *context, i16 subId);
    ZunResult RunEcl(Enemy *enemy);
    i32 GetTimelineCount();
    uintptr_t GetTimeline(i32 index);

#ifdef TH08_IOS
    IosPtr32<EclRawHeader> eclFile;  // +0x000
    IosPtr32<u32> subTable;          // +0x004
#else
    EclRawHeader *eclFile;             // +0x000
    u32 *subTable;                     // +0x004
#endif
    EclTimelineState timelineState;    // +0x008

    EclRawHeader *GetEclFile() const
    {
        return this->eclFile;
    }

    u32 *GetSubTable() const
    {
        return this->subTable;
    }
};
C_ASSERT(sizeof(EclManager) == 0x168);
#ifdef TH08_IOS
typedef char IosEclManagerSizeCheck[(sizeof(EclManager) == 0x168) ? 1 : -1];
#endif

DIFFABLE_EXTERN(EclManager, g_EclManager);

} // namespace th08
