#include "EclManager.hpp"
#include "EclOperands.hpp"
#include "AsciiManager.hpp"
#include "Gui.hpp"
#include "BulletManager.hpp"
#include "GameManager.hpp"
#include "ItemManager.hpp"
#include "Player.hpp"
#include "Spellcard.hpp"
#ifdef TH08_IOS
#include "modern/ios/ios_touch.hpp"
#endif

#include <stdio.h>
#include <string.h>

#define TH08_ECL_RUN_DECLARATIONS_ONLY
#include "EclRunLow.inl"
#include "EclRunHigh.inl"
#undef TH08_ECL_RUN_DECLARATIONS_ONLY

namespace th08
{

// The dispatcher is now source-complete, while target-address service binding
// remains intentionally separate: subsystem owners can replace provisional
// adapters without changing the recovered opcode/control-flow core.
#undef TH08_ECL_CONTEXT_ENEMY
#undef TH08_ECL_CONTEXT_INSTRUCTION
#undef TH08_ECL_CONTEXT_API
#undef TH08_ECL_CONTEXT_CHILD
#define TH08_ECL_CONTEXT_ENEMY(unusedContext) (reinterpret_cast<u8 *>(enemy))
#define TH08_ECL_CONTEXT_INSTRUCTION(unusedContext) \
    (reinterpret_cast<RawInstruction *>(instruction))
#define TH08_ECL_CONTEXT_API(unusedContext) \
    (reinterpret_cast<TargetApi *>(this))
#define TH08_ECL_CONTEXT_CHILD(unusedContext) (activeChildContext)

// FUNCTION: th08 0x004184B0
ZunResult EclManager::RunEcl(Enemy *enemy)
{
    using namespace EclRunLowProposal;
    using namespace EclRunHighProposal;

    EclRawInstruction *instruction;
    i32 activeChildContext = -1;
    i32 lhsInt;
#ifdef TH08_IOS
    static u32 iosRunSerial;
    const u32 runSerial = iosRunSerial++;
    const bool traceRun = runSerial < 24;
    u32 instructionStep = 0;
#endif

#ifdef TH08_IOS
    IosEclPointerField<u8>(enemy, 0x2CA4) = reinterpret_cast<u8 *>(enemy) + 0xA20;
    IosEclPointerField<EnemyEclContext>(enemy, 0x2CA0) =
        reinterpret_cast<EnemyEclContext *>(reinterpret_cast<u8 *>(enemy) + 0x7F8);
    if (traceRun)
    {
        char message[192];
        snprintf(message, sizeof(message),
                 "ecl-run: serial=%u enemy=%p context=%p enter", runSerial,
                 static_cast<void *>(enemy), static_cast<void *>(IosEclCurrentContext(enemy)));
        modern::LogStartup(message);
    }
#else
    *reinterpret_cast<u8 **>(reinterpret_cast<u8 *>(enemy) + 0x2CA4) =
        reinterpret_cast<u8 *>(enemy) + 0xA20;
    *reinterpret_cast<u8 **>(reinterpret_cast<u8 *>(enemy) + 0x2CA0) =
        reinterpret_cast<u8 *>(enemy) + 0x7F8;
#endif
    *reinterpret_cast<i16 *>(reinterpret_cast<u8 *>(enemy) + 0x2CEA) =
        *reinterpret_cast<i16 *>(reinterpret_cast<u8 *>(enemy) + 0x2CE8);

restart_context:
#ifdef TH08_IOS
    if (IosEclCurrentContext(enemy) == NULL)
    {
        modern::LogStartup("ecl-run-error: restart with null context");
        return ZUN_ERROR;
    }
    instruction = IosEclCurrentContext(enemy)->currentInstr.Get();
#else
    instruction = (*reinterpret_cast<EnemyEclContext **>(
        reinterpret_cast<u8 *>(enemy) + 0x2CA0))->currentInstr;
#endif

    for (;;)
    {
#ifdef TH08_IOS
        if (instruction == NULL)
        {
            i16 &pendingSubroutine = *reinterpret_cast<i16 *>(
                reinterpret_cast<u8 *>(enemy) + 0x2D30);
            if (pendingSubroutine >= 0 && pendingSubroutine < 32)
            {
                const i16 mappedSubId = *reinterpret_cast<i16 *>(
                    reinterpret_cast<u8 *>(enemy) + 0x2CF0 +
                    pendingSubroutine * sizeof(i16));
                EnemyEclContext *mainContext = reinterpret_cast<EnemyEclContext *>(
                    reinterpret_cast<u8 *>(enemy) + 0x7F8);
                char message[224];
                snprintf(message, sizeof(message),
                         "ecl-run-recover: enemy=%p pending=%d mapped=%d rawHandle=%u",
                         static_cast<void *>(enemy), static_cast<i32>(pendingSubroutine),
                         static_cast<i32>(mappedSubId),
                         static_cast<unsigned>(mainContext->currentInstr.handle));
                modern::LogStartup(message);

                IosEclPointerField<u8>(enemy, 0x2CA4) =
                    reinterpret_cast<u8 *>(enemy) + 0xA20;
                IosEclPointerField<EnemyEclContext>(enemy, 0x2CA0) = mainContext;
                *reinterpret_cast<i16 *>(reinterpret_cast<u8 *>(enemy) + 0x2CEA) = 0;
                *reinterpret_cast<i16 *>(reinterpret_cast<u8 *>(enemy) + 0x2CE8) = 0;
                pendingSubroutine = -1;
                g_EclManager.CallEclSub(mainContext, mappedSubId);
                instruction = mainContext->currentInstr.Get();
                if (instruction != NULL)
                    goto low_redispatch_instruction;
            }

            char message[192];
            snprintf(message, sizeof(message),
                     "ecl-run-error: enemy=%p pending=%d null current instruction",
                     static_cast<void *>(enemy),
                     static_cast<i32>(pendingSubroutine));
            modern::LogStartup(message);
            pendingSubroutine = -1;
            return ZUN_ERROR;
        }
#endif
        if (*reinterpret_cast<i16 *>(reinterpret_cast<u8 *>(enemy) + 0x2D30) >= 0)
            goto enter_subroutine;

low_redispatch_instruction:
#ifdef TH08_IOS
        if (++instructionStep > 4096)
        {
            char message[192];
            snprintf(message, sizeof(message),
                     "ecl-run-error: serial=%u instruction budget exhausted ptr=%p",
                     runSerial, static_cast<void *>(instruction));
            modern::LogStartup(message);
            if (IosEclCurrentContext(enemy) != NULL)
                IosEclCurrentContext(enemy)->currentInstr = instruction;
            return ZUN_ERROR;
        }
        if (traceRun && instructionStep <= 128)
        {
            char message[224];
            snprintf(message, sizeof(message),
                     "ecl-run: serial=%u step=%u ptr=%p ctxTime=%d insTime=%d op=%d next=%d mask=%02x",
                     runSerial, instructionStep, static_cast<void *>(instruction),
                     IosEclCurrentContext(enemy) != NULL
                         ? static_cast<i32>(IosEclCurrentContext(enemy)->time)
                         : -999999,
                     instruction != NULL ? instruction->time : -999999,
                     instruction != NULL ? static_cast<i32>(instruction->opcode) : -1,
                     instruction != NULL ? static_cast<i32>(instruction->nextOffset) : -1,
                     instruction != NULL ? static_cast<unsigned>(instruction->difficultyMask) : 0U);
            modern::LogStartup(message);
        }
        if (instruction == NULL || IosEclCurrentContext(enemy) == NULL)
        {
            modern::LogStartup("ecl-run-error: null instruction or context");
            return ZUN_ERROR;
        }
#endif
        *reinterpret_cast<D3DXVECTOR3 *>(reinterpret_cast<u8 *>(enemy) + 0x2D88) =
            *reinterpret_cast<D3DXVECTOR3 *>(reinterpret_cast<u8 *>(enemy) + 0x2D34) +
            *reinterpret_cast<D3DXVECTOR3 *>(reinterpret_cast<u8 *>(enemy) + 0x2D40);

#ifdef TH08_IOS
        if ((int)IosEclCurrentContext(enemy)->secondaryTime > 0)
        {
            IosEclCurrentContext(enemy)->secondaryTime--;
            IosEclCurrentContext(enemy)->time--;
            break;
        }
#else
        if ((int)(*reinterpret_cast<EnemyEclContext **>(
                reinterpret_cast<u8 *>(enemy) + 0x2CA0))->secondaryTime > 0)
        {
            (*reinterpret_cast<EnemyEclContext **>(
                reinterpret_cast<u8 *>(enemy) + 0x2CA0))->secondaryTime--;
            (*reinterpret_cast<EnemyEclContext **>(
                reinterpret_cast<u8 *>(enemy) + 0x2CA0))->time--;
            break;
        }
#endif

#ifdef TH08_IOS
        if (IosEclCurrentContext(enemy)->time == instruction->time)
#else
        if ((*reinterpret_cast<EnemyEclContext **>(
                reinterpret_cast<u8 *>(enemy) + 0x2CA0))->time == instruction->time)
#endif
        {
            if ((instruction->difficultyMask &
                 (static_cast<u32>(g_GameManager.difficultyMask) |
                  *reinterpret_cast<u8 *>(reinterpret_cast<u8 *>(enemy) + 0x3330))) !=
                (static_cast<u32>(g_GameManager.difficultyMask) |
                 *reinterpret_cast<u8 *>(reinterpret_cast<u8 *>(enemy) + 0x3330)))
            {
                goto low_advance_instruction;
            }

            {
#define enemy reinterpret_cast<EclOperands::EnemyOverlay *>(enemy)
#ifdef TH08_IOS
#define context (IosEclCurrentContext(enemy))
#else
#define context (*reinterpret_cast<EnemyEclContext **>( \
    reinterpret_cast<u8 *>(enemy) + 0x2CA0))
#endif
#define services (*reinterpret_cast<Services *>(this))
#define ctx unusedContext
#define TH08_ECL_RUN_LOW_BODY
#define TH08_ECL_RUN_HIGH_BODY
#define TH08_ECL_RUN_SHARED_SWITCH
            switch (instruction->opcode)
            {
                {
#include "EclRunLow.inl"
                }
                {
#include "EclRunHigh.inl"
                }
            default:
                break;
            }
#undef TH08_ECL_RUN_SHARED_SWITCH
#undef TH08_ECL_RUN_HIGH_BODY
#undef TH08_ECL_RUN_LOW_BODY
#undef ctx
#undef services
#undef context
#undef enemy
            }

low_advance_instruction:
            instruction = reinterpret_cast<EclRawInstruction *>(
                reinterpret_cast<u8 *>(instruction) + instruction->nextOffset);
            goto low_redispatch_instruction;

            break;
        }

        break;
    }

    // Target 0x0041E7F8..0x0041ECBD is part of RunEcl itself.  Keep the
    // complete frame tail lexical so VC7 can share RunEcl's locals and emit
    // the observed in-function easing switch and child-context back edge.
    if (TH08_ECL_AT(unusedContext, i32, 0x2DFC) > 0)
    {
#pragma var_order(i, progress, restorePosition, entry, savedPosition)
        i32 i;
        f32 progress;
        i32 restorePosition = 0;
        Interpolator *entry = reinterpret_cast<Interpolator *>(
            TH08_ECL_CURRENT_CONTEXT(unusedContext) + 0x9C);
        Vec3 savedPosition = TH08_ECL_AT(unusedContext, Vec3, 0x2D34);

#ifdef TH08_IOS
        EclContextCallback callback = reinterpret_cast<EclContextCallback>(
            IosEclPointerField<void>(TH08_ECL_CURRENT_CONTEXT(unusedContext), 0x10).Get());
        if (callback != NULL)
            callback(
                enemy,
                IosEclPointerField<void>(TH08_ECL_CURRENT_CONTEXT(unusedContext), 0x14).Get());
#else
        if (*reinterpret_cast<EclContextCallback *>(
                TH08_ECL_CURRENT_CONTEXT(unusedContext) + 0x10))
            (*reinterpret_cast<EclContextCallback *>(
                TH08_ECL_CURRENT_CONTEXT(unusedContext) + 0x10))(
                enemy, *reinterpret_cast<void **>(
                    TH08_ECL_CURRENT_CONTEXT(unusedContext) + 0x14));
#endif

        for (i = 0; i < 8; ++i, ++entry)
        {
            if (entry->callback)
            {
                entry->timer++;
                if (entry->timer >= entry->duration)
                    entry->timer = entry->duration;

                progress = static_cast<f32>(entry->timer) / entry->duration;
                switch (entry->easing)
                {
                case 1: progress = progress * progress; break;
                case 2: progress = progress * progress * progress; break;
                case 3: progress = progress * progress * progress * progress; break;
                case 4:
                    progress = 1.0f - progress;
                    progress = progress * progress;
                    progress = 1.0f - progress;
                    break;
                case 5:
                    progress = 1.0f - progress;
                    progress = progress * progress * progress;
                    progress = 1.0f - progress;
                    break;
                case 6:
                    progress = 1.0f - progress;
                    progress = progress * progress * progress * progress;
                    progress = 1.0f - progress;
                    break;
                }

#ifdef TH08_IOS
                EclInterpolatorCallback callback = entry->callback.Get();
                callback(enemy, entry, progress);
#else
                entry->callback(enemy, entry, progress);
#endif
                if (entry->timer >= entry->duration)
#ifdef TH08_IOS
                    entry->callback = static_cast<EclInterpolatorCallback>(NULL);
#else
                    entry->callback = 0;
#endif

                if (entry->affectedVariable == 10042.0f ||
                    entry->affectedVariable == 10043.0f ||
                    entry->affectedVariable == 10044.0f)
                    restorePosition = 1;
            }
        }

        if (restorePosition)
        {
            TH08_ECL_AT(unusedContext, f32, 0x2D4C) =
                TH08_ECL_AT(unusedContext, f32, 0x2D34) - savedPosition.x;
            TH08_ECL_AT(unusedContext, f32, 0x2D50) =
                TH08_ECL_AT(unusedContext, f32, 0x2D38) - savedPosition.y;
            TH08_ECL_AT(unusedContext, f32, 0x2D94) =
                VectorAngle(
                    *reinterpret_cast<f32 *>(reinterpret_cast<u8 *>(enemy) + 0x2D50),
                    *reinterpret_cast<f32 *>(reinterpret_cast<u8 *>(enemy) + 0x2D4C));
            TH08_ECL_AT(unusedContext, Vec3, 0x2D34) = savedPosition;
        }
    }

    if (activeChildContext == -1)
        TH08_ECL_AT(unusedContext, i16, 0x2CE8) = TH08_ECL_AT(unusedContext, i16, 0x2CEA);
    else
#ifdef TH08_IOS
        *(i16 *)(IosEclChildContextField(enemy, activeChildContext).Get() + 6) =
            TH08_ECL_AT(unusedContext, i16, 0x2CEA);
#else
        *(i16 *)(TH08_ECL_AT(unusedContext, u8 *, 0x3384 + activeChildContext * 4) + 6) =
            TH08_ECL_AT(unusedContext, i16, 0x2CEA);
#endif

#ifdef TH08_IOS
    IosEclCurrentContext(enemy)->currentInstr =
        reinterpret_cast<EclRawInstruction *>(instruction);
#else
    *(RawInstruction **)TH08_ECL_CURRENT_CONTEXT(unusedContext) =
        reinterpret_cast<RawInstruction *>(instruction);
#endif
    reinterpret_cast<ZunTimer *>(TH08_ECL_CURRENT_CONTEXT(unusedContext) + 4)->operator++(0);

low_select_next_context:
    for (i32 next = activeChildContext + 1; next < 4; ++next)
    {
#ifdef TH08_IOS
        if (IosEclChildContextField(enemy, next).Get())
        {
            u8 *childContext = IosEclChildContextField(enemy, next).Get();
            IosEclPointerField<u8>(enemy, 0x2CA4) = childContext + 0x230;
            IosEclPointerField<EnemyEclContext>(enemy, 0x2CA0) =
                reinterpret_cast<EnemyEclContext *>(childContext + 8);
            instruction = IosEclCurrentContext(enemy)->currentInstr.Get();
            *(i32 *)(reinterpret_cast<u8 *>(IosEclCurrentContext(enemy)) + 0x220) = next + 1;
            TH08_ECL_AT(unusedContext, i16, 0x2CEA) = *(i16 *)(childContext + 6);
            activeChildContext = next;
            goto low_redispatch_instruction;
        }
#else
        if (TH08_ECL_AT(unusedContext, u8 *, 0x3384 + next * 4))
        {
            u8 *childContext = TH08_ECL_AT(unusedContext, u8 *, 0x3384 + next * 4);
            TH08_ECL_AT(unusedContext, u8 *, 0x2CA4) =
                childContext + 0x230;
            TH08_ECL_AT(unusedContext, u8 *, 0x2CA0) =
                childContext + 8;
            instruction = reinterpret_cast<EclRawInstruction *>(
                *(RawInstruction **)TH08_ECL_CURRENT_CONTEXT(unusedContext));
            *(i32 *)(TH08_ECL_CURRENT_CONTEXT(unusedContext) + 0x220) = next + 1;
            TH08_ECL_AT(unusedContext, i16, 0x2CEA) =
                *(i16 *)(childContext + 6);
            activeChildContext = next;
            goto low_redispatch_instruction;
        }
#endif
    }

#ifdef TH08_IOS
    IosEclPointerField<u8>(enemy, 0x2CA4) = reinterpret_cast<u8 *>(enemy) + 0xA20;
    IosEclPointerField<EnemyEclContext>(enemy, 0x2CA0) =
        reinterpret_cast<EnemyEclContext *>(reinterpret_cast<u8 *>(enemy) + 0x7F8);
#else
    TH08_ECL_AT(unusedContext, u8 *, 0x2CA4) =
        reinterpret_cast<u8 *>(enemy) + 0xA20;
    TH08_ECL_AT(unusedContext, u8 *, 0x2CA0) =
        reinterpret_cast<u8 *>(enemy) + 0x7F8;
#endif
    enemy->FUN_00422c40();
    enemy->FUN_00423150();

#ifdef TH08_IOS
    if (traceRun)
        modern::LogStartup("ecl-run: return success");
#endif
    return ZUN_SUCCESS;
}

#undef TH08_ECL_CONTEXT_ENEMY
#undef TH08_ECL_CONTEXT_INSTRUCTION
#undef TH08_ECL_CONTEXT_API
#undef TH08_ECL_CONTEXT_CHILD

} // namespace th08
