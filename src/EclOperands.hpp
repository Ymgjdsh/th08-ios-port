#pragma once

#include "inttypes.hpp"

namespace th08
{
namespace EclOperands
{

struct Vector3
{
    f32 x;
    f32 y;
    f32 z;

    Vector3 operator-(const Vector3 &other) const;
    f32 Length() const;
};
#if defined(TH08_MODERN_WEB) || defined(TH08_MODERN_IOS)
extern Vector3 &g_TargetPlayerPosition017D61AC;
#else
extern Vector3 g_TargetPlayerPosition017D61AC;
#endif

// Private byte overlay for target-observed Enemy offsets. The public Enemy
// layout remains intentionally incomplete until its owning lane proves it.
struct EnemyOverlay
{
    u8 bytes[1];

    f32 ResolveFloat(f32 operand);
    void FUN_0042adb0(i32 value);
};

// The retail Enemy layout stores its linked-list pointers in four-byte slots
// at +0x04/+0x08 and the owning enemy at +0x2DA4.  Native arm64 pointer
// accesses span two adjacent slots and corrupt the list, so all platforms use
// these accessors and iOS translates the slots through IosPtr32.
inline EnemyOverlay *EnemyPointerField(const void *enemy, u32 offset)
{
#if defined(TH08_MODERN_IOS)
    return reinterpret_cast<const IosPtr32<EnemyOverlay> *>(
               reinterpret_cast<const u8 *>(enemy) + offset)
        ->Get();
#else
    return *reinterpret_cast<EnemyOverlay *const *>(
        reinterpret_cast<const u8 *>(enemy) + offset);
#endif
}

inline void SetEnemyPointerField(void *enemy, u32 offset, EnemyOverlay *value)
{
#if defined(TH08_MODERN_IOS)
    *reinterpret_cast<IosPtr32<EnemyOverlay> *>(
        reinterpret_cast<u8 *>(enemy) + offset) = value;
#else
    *reinterpret_cast<EnemyOverlay **>(
        reinterpret_cast<u8 *>(enemy) + offset) = value;
#endif
}

inline EnemyOverlay *EnemyPrevious(const void *enemy)
{
    return EnemyPointerField(enemy, 0x04);
}

inline EnemyOverlay *EnemyNext(const void *enemy)
{
    return EnemyPointerField(enemy, 0x08);
}

inline EnemyOverlay *EnemyOwner(const void *enemy)
{
    return EnemyPointerField(enemy, 0x2DA4);
}

inline void SetEnemyPrevious(void *enemy, EnemyOverlay *value)
{
    SetEnemyPointerField(enemy, 0x04, value);
}

inline void SetEnemyNext(void *enemy, EnemyOverlay *value)
{
    SetEnemyPointerField(enemy, 0x08, value);
}

inline void SetEnemyOwner(void *enemy, EnemyOverlay *value)
{
    SetEnemyPointerField(enemy, 0x2DA4, value);
}

struct TargetEnemyHelpersOverlay
{
    i32 HasParentChain();
    i32 HasAttachedEnemy();
    i32 CountParentChain();
};

i32 __fastcall ResolveInt(EnemyOverlay *enemy, i32 operand);
i32 *__fastcall ResolveIntLValue(EnemyOverlay *enemy, i32 *operand, u16 flags, i32 flagIndex);
f32 *__fastcall ResolveFloatLValue(EnemyOverlay *enemy, f32 *operand, u16 flags, i32 flagIndex);

} // namespace EclOperands

namespace EclRunLowProposal
{
struct EclCallParameterCopy
{
    i32 ints[4];
    f32 floats[4];
};
extern EclCallParameterCopy g_EclCallParameters;
#if defined(TH08_MODERN_WEB)
extern EclOperands::EnemyOverlay *(&g_EclEnemyTableF54CC0)[92];
#elif defined(TH08_MODERN_IOS)
extern IosPtr32<EclOperands::EnemyOverlay> g_EclEnemyTableF54CC0[92];
#else
extern EclOperands::EnemyOverlay *g_EclEnemyTableF54CC0[92];
#endif

inline EclOperands::EnemyOverlay *EclEnemySlot(u32 index)
{
#if defined(TH08_MODERN_IOS)
    return g_EclEnemyTableF54CC0[index].Get();
#else
    return g_EclEnemyTableF54CC0[index];
#endif
}

inline i32 &EclEnemySlotInt(u32 index)
{
#if defined(TH08_MODERN_IOS)
    // The original table consists of 32-bit slots, some used as pointers and
    // some used as integers. IosPtr32 keeps both forms in one four-byte slot.
    return *reinterpret_cast<i32 *>(&g_EclEnemyTableF54CC0[index].handle);
#else
    return reinterpret_cast<i32 *>(g_EclEnemyTableF54CC0)[index];
#endif
}
} // namespace EclRunLowProposal
} // namespace th08
