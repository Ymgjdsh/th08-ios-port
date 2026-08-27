#include "th_pch.h"

#include "EnemyManager.hpp"
#include "EclManager.hpp"
#include "EclOperands.hpp"
#include "Gui.hpp"
#include "GameManager.hpp"
#include "ReplayManager.hpp"
#ifdef TH08_IOS
#include "modern/ios/ios_touch.hpp"
#endif

#include <stdio.h>

namespace th08
{


// FUNCTION: th08 0x42a4e0
#pragma var_order(i, enemy, this)
void *EnemyManager::SpawnEnemy1(i32 type, const D3DXVECTOR3 *position, i32 a, i32 b, i32 c, i32 flags)
{
    struct EnemySpawnCopy
    {
        u32 words[sizeof(Enemy) / sizeof(u32)];
    };
    struct EnemySpawnFlags
    {
        u32 active : 1;
        u32 pad1_17 : 17;
        u32 spawnVariant : 1;
        u32 pad19_31 : 13;
    };
    i32 i;
    Enemy *enemy;
#ifdef TH08_IOS
    static u32 spawnSerial;
    const u32 currentSpawnSerial = spawnSerial++;
    if (currentSpawnSerial < 32)
    {
        char message[192];
        snprintf(message, sizeof(message),
                 "enemy-spawn: serial=%u type=%d pos=(%.2f,%.2f) flags=%d enter",
                 currentSpawnSerial, type, position->x, position->y, flags);
        modern::LogStartup(message);
    }
#endif

    enemy = &this->enemies[0];
    g_ReplayManager->flags |= 0x1000;
    for (i = 0; i < 480; i++, enemy++)
    {
        if ((*reinterpret_cast<u32 *>(reinterpret_cast<u8 *>(enemy) + 0x3324) & 1) != 0)
            continue;

        *reinterpret_cast<EnemySpawnCopy *>(enemy) =
            *reinterpret_cast<const EnemySpawnCopy *>(&this->firstEnemy);
        *reinterpret_cast<i32 *>(reinterpret_cast<u8 *>(enemy) + 0x2e0c) = i;
        reinterpret_cast<EnemySpawnFlags *>(reinterpret_cast<u8 *>(enemy) + 0x3324)->spawnVariant = flags;
        if (a >= 0)
            *reinterpret_cast<i32 *>(reinterpret_cast<u8 *>(enemy) + 0x2dfc) = a;
        enemy->vector2d34 = *reinterpret_cast<const Float3 *>(position);
#ifdef TH08_IOS
        if (currentSpawnSerial < 32)
            modern::LogStartup("enemy-spawn: CallEclSub enter");
#endif
        g_EclManager.CallEclSub(
            reinterpret_cast<EnemyEclContext *>(reinterpret_cast<u8 *>(enemy) + 0x7f8), (i16)type);
#ifdef TH08_IOS
        if (currentSpawnSerial < 32)
            modern::LogStartup("enemy-spawn: CallEclSub returned; RunEcl enter");
#endif
        if (g_EclManager.RunEcl(enemy) == ZUN_ERROR)
        {
            reinterpret_cast<EnemySpawnFlags *>(reinterpret_cast<u8 *>(enemy) + 0x3324)->active = 0;
            i = 480;
        }
        else
        {
            *reinterpret_cast<i32 *>(reinterpret_cast<u8 *>(enemy) + 0x2e20) =
                *reinterpret_cast<i32 *>(reinterpret_cast<u8 *>(enemy) + 0x1fc);
            *reinterpret_cast<i32 *>(reinterpret_cast<u8 *>(enemy) + 0x3304) = (i8)b;
            if (c >= 0)
                *reinterpret_cast<i32 *>(reinterpret_cast<u8 *>(enemy) + 0x2e08) = c;
            *reinterpret_cast<i32 *>(reinterpret_cast<u8 *>(enemy) + 0x2e00) =
                *reinterpret_cast<i32 *>(reinterpret_cast<u8 *>(enemy) + 0x2dfc);
            *reinterpret_cast<i32 *>(reinterpret_cast<u8 *>(enemy) + 0x2e04) =
                *reinterpret_cast<i32 *>(reinterpret_cast<u8 *>(enemy) + 0x2e00);
        }
#ifdef TH08_IOS
        if (currentSpawnSerial < 32)
        {
            AnmVm *vm = reinterpret_cast<AnmVm *>(
                reinterpret_cast<u8 *>(enemy) + 0xC);
            char stateMessage[320];
            snprintf(stateMessage, sizeof(stateMessage),
                     "enemy-spawn: RunEcl returned flags1=%08x flags2=%08x dir=%u scripts=%d,%d,%d,%d,%d,%d vmVisible=%u vmScript=%d vmSprite=%d vmBegin=%p vmCurrent=%p vmLoaded=%p vmAnm=%p common=%p stage=%p",
                     static_cast<unsigned>(*reinterpret_cast<u32 *>(reinterpret_cast<u8 *>(enemy) + 0x3324)),
                     static_cast<unsigned>(*reinterpret_cast<u32 *>(reinterpret_cast<u8 *>(enemy) + 0x3328)),
                     static_cast<unsigned>(*reinterpret_cast<u8 *>(reinterpret_cast<u8 *>(enemy) + 0x332E)),
                     static_cast<int>(*reinterpret_cast<i16 *>(reinterpret_cast<u8 *>(enemy) + 0x3332)),
                     static_cast<int>(*reinterpret_cast<i16 *>(reinterpret_cast<u8 *>(enemy) + 0x3338)),
                     static_cast<int>(*reinterpret_cast<i16 *>(reinterpret_cast<u8 *>(enemy) + 0x333A)),
                     static_cast<int>(*reinterpret_cast<i16 *>(reinterpret_cast<u8 *>(enemy) + 0x3334)),
                     static_cast<int>(*reinterpret_cast<i16 *>(reinterpret_cast<u8 *>(enemy) + 0x3336)),
                     static_cast<int>(*reinterpret_cast<i16 *>(reinterpret_cast<u8 *>(enemy) + 0x333C)),
                     static_cast<unsigned>(vm->visible),
                     static_cast<int>(vm->scriptIndex),
                     static_cast<int>(vm->activeSpriteIndex),
                     vm->beginningOfScript.Get(), vm->currentInstruction.Get(),
                     vm->loadedSprite.Get(), vm->anmFile.Get(),
                     EnemyManagerCommonAnm(&g_EnemyManager),
                     EnemyManagerStageAnm(&g_EnemyManager));
            modern::LogStartup(stateMessage);
        }
#endif
        break;
    }
    *reinterpret_cast<i32 *>(reinterpret_cast<u8 *>(this) + 0x9dcef8) = (i == 480);
    return enemy;
}

// FUNCTION: th08 0x42a680
#pragma var_order(i, enemy, this)
void *EnemyManager::SpawnEnemy2(i32 type, const D3DXVECTOR3 *position, i32 a, i32 b, i32 c, i32 *contextInts)
{
    struct EnemySpawnCopy
    {
        u32 words[sizeof(Enemy) / sizeof(u32)];
    };
    struct EnemySpawnFlags
    {
        u32 active : 1;
        u32 pad1_31 : 31;
    };
    struct EnemyContextCopy
    {
        u32 words[0x78 / sizeof(u32)];
    };
    i32 i;
    Enemy *enemy;

    enemy = &this->enemies[0];
    g_ReplayManager->flags |= 0x1000;
    for (i = 0; i < 480; i++, enemy++)
    {
        if ((*reinterpret_cast<u32 *>(reinterpret_cast<u8 *>(enemy) + 0x3324) & 1) != 0)
            continue;

        *reinterpret_cast<EnemySpawnCopy *>(enemy) =
            *reinterpret_cast<const EnemySpawnCopy *>(&this->firstEnemy);
        *reinterpret_cast<i32 *>(reinterpret_cast<u8 *>(enemy) + 0x2e0c) = i;
        if (a >= 0)
            *reinterpret_cast<i32 *>(reinterpret_cast<u8 *>(enemy) + 0x2dfc) = a;
        enemy->vector2d34 = *reinterpret_cast<const Float3 *>(position);
        g_EclManager.CallEclSub(
            reinterpret_cast<EnemyEclContext *>(reinterpret_cast<u8 *>(enemy) + 0x7f8), (i16)type);
        *reinterpret_cast<EnemyContextCopy *>(reinterpret_cast<u8 *>(enemy) + 0x810) =
            *reinterpret_cast<const EnemyContextCopy *>(contextInts);
        if (g_EclManager.RunEcl(enemy) == ZUN_ERROR)
        {
            reinterpret_cast<EnemySpawnFlags *>(reinterpret_cast<u8 *>(enemy) + 0x3324)->active = 0;
            i = 480;
        }
        else
        {
            *reinterpret_cast<i32 *>(reinterpret_cast<u8 *>(enemy) + 0x2e20) =
                *reinterpret_cast<i32 *>(reinterpret_cast<u8 *>(enemy) + 0x1fc);
            *reinterpret_cast<i32 *>(reinterpret_cast<u8 *>(enemy) + 0x3304) = (i8)b;
            if (a >= 0)
                *reinterpret_cast<i32 *>(reinterpret_cast<u8 *>(enemy) + 0x2dfc) = a;
            if (c >= 0)
                *reinterpret_cast<i32 *>(reinterpret_cast<u8 *>(enemy) + 0x2e08) = c;
            *reinterpret_cast<i32 *>(reinterpret_cast<u8 *>(enemy) + 0x2e00) =
                *reinterpret_cast<i32 *>(reinterpret_cast<u8 *>(enemy) + 0x2dfc);
            *reinterpret_cast<i32 *>(reinterpret_cast<u8 *>(enemy) + 0x2e04) =
                *reinterpret_cast<i32 *>(reinterpret_cast<u8 *>(enemy) + 0x2e00);
        }
        break;
    }
    *reinterpret_cast<i32 *>(reinterpret_cast<u8 *>(this) + 0x9dcef8) = (i == 480);
    return enemy;
}


#pragma var_order(variant, args1, position1, args15, position15, args11, spawned11, position11, argsRange, positionRange, argsPlay, positionPlay, i, matchCount, j)
// FUNCTION: th08 0x42a8a0
void EclTimeline::Run()
{
    i32 variant;
#ifdef TH08_IOS
    static u32 runSerial;
    const u32 currentRunSerial = runSerial++;
    u32 instructionBudget = 0;
    if (currentRunSerial < 32)
    {
        char message[192];
        snprintf(message, sizeof(message), "timeline-run: serial=%u timer=%d ptr=%p enter",
                 currentRunSerial, static_cast<i32>(this->timer),
                 static_cast<void *>(this->instruction.Get()));
        modern::LogStartup(message);
    }
#endif

    variant = 0;
#ifdef TH08_IOS
    for (;;)
    {
        EclTimelineInstruction *currentInstruction = this->instruction.Get();
        if (currentInstruction == NULL)
        {
            modern::LogStartup("timeline-error: null instruction");
            goto done;
        }
        if (currentInstruction->time < 0)
            break;
        if (++instructionBudget > 1024)
        {
            modern::LogStartup("timeline-error: instruction budget exhausted");
            goto done;
        }
        if (currentRunSerial < 32 && instructionBudget <= 64)
        {
            char message[224];
            snprintf(message, sizeof(message),
                     "timeline-run: serial=%u step=%u timer=%d ptr=%p time=%d op=%d size=%u mask=%02x",
                     currentRunSerial, instructionBudget, static_cast<i32>(this->timer),
                     static_cast<void *>(currentInstruction), currentInstruction->time,
                     static_cast<i32>(currentInstruction->opcode),
                     static_cast<unsigned>(currentInstruction->size),
                     static_cast<unsigned>(currentInstruction->difficultyMask));
            modern::LogStartup(message);
        }
#else
    while (this->instruction->time >= 0)
    {
#endif
        if (this->timer == this->instruction->time)
        {
            variant = 0;
            if ((this->instruction->difficultyMask & g_GameManager.difficultyMask) == 0)
                goto advance_instruction;

            switch (this->instruction->opcode)
            {
            case 16:
                g_GameManager.showRetryMenu = 1;
                break;

            case 1:
                variant = 1;
            case 0:
                if (!g_Gui.IsBossPresent() && EclRunLowProposal::g_EclEnemyTableF54CC0[91] == NULL)
                {
#ifdef TH08_IOS
                    if (currentRunSerial < 32)
                        modern::LogStartup("timeline-run: spawn opcode 0/1 enter");
#endif
                    i32 *args1 = this->instruction->args.ints;
                    D3DXVECTOR3 position1;
                    position1.x = reinterpret_cast<f32 *>(args1)[1];
                    position1.y = reinterpret_cast<f32 *>(args1)[2];
                    position1.z = 0.0f;
                    g_EnemyManager.SpawnEnemy1(args1[0], &position1, args1[3], args1[4], args1[5], variant);
#ifdef TH08_IOS
                    if (currentRunSerial < 32)
                        modern::LogStartup("timeline-run: spawn opcode 0/1 returned");
#endif
                }
                break;

            case 15:
            {
                i32 *args15 = this->instruction->args.ints;
                D3DXVECTOR3 position15;
                position15.x = reinterpret_cast<f32 *>(args15)[1];
                position15.y = reinterpret_cast<f32 *>(args15)[2];
                position15.z = 0.0f;
                g_EnemyManager.SpawnEnemy1(args15[0], &position15, args15[3], args15[4], args15[5], variant);
                break;
            }

            case 12:
                variant = 1;
            case 11:
                if (!g_Gui.IsBossPresent() && EclRunLowProposal::g_EclEnemyTableF54CC0[91] == NULL)
                {
                    struct SpecialSpawnLocals
                    {
                        Enemy *spawned11;
                        i32 *args11;
                    } locals;
                    locals.args11 = this->instruction->args.ints;
                    D3DXVECTOR3 position11;
                    position11.x = reinterpret_cast<f32 *>(locals.args11)[1];
                    position11.y = reinterpret_cast<f32 *>(locals.args11)[2];
                    position11.z = 0.0f;
                    locals.spawned11 = static_cast<Enemy *>(g_EnemyManager.SpawnEnemy1(
                        locals.args11[0], &position11, locals.args11[3], -1, locals.args11[6], variant));
                    *reinterpret_cast<i32 *>(reinterpret_cast<u8 *>(locals.spawned11) + 0x3308) = locals.args11[4];
                    *reinterpret_cast<i32 *>(reinterpret_cast<u8 *>(locals.spawned11) + 0x330c) = locals.args11[5];
                }
                break;

            case 4:
                variant = 1;
            case 2:
                if (!g_Gui.IsBossPresent() && EclRunLowProposal::g_EclEnemyTableF54CC0[91] == NULL)
                {
                    i32 *argsRange = this->instruction->args.ints;
                    D3DXVECTOR3 positionRange;
                    positionRange.x = g_Rng.GetRandomF32InRange(
                        reinterpret_cast<f32 *>(argsRange)[2] - reinterpret_cast<f32 *>(argsRange)[1]) +
                        reinterpret_cast<f32 *>(argsRange)[1];
                    positionRange.y = reinterpret_cast<f32 *>(argsRange)[3];
                    positionRange.z = 0.0f;
                    g_EnemyManager.SpawnEnemy1(
                        argsRange[0], &positionRange, argsRange[4], argsRange[5], argsRange[6], variant);
                }
                break;

            case 5:
                variant = 1;
            case 3:
                if (!g_Gui.IsBossPresent() && EclRunLowProposal::g_EclEnemyTableF54CC0[91] == NULL)
                {
                    i32 *argsPlay = this->instruction->args.ints;
                    {
                        D3DXVECTOR3 positionPlay;
                        positionPlay.x = g_Rng.GetRandomF32InRange(384.0f);
                        positionPlay.y = reinterpret_cast<f32 *>(argsPlay)[1];
                        positionPlay.z = 0.0f;
                        g_EnemyManager.SpawnEnemy1(
                            argsPlay[0], &positionPlay, argsPlay[2], argsPlay[3], argsPlay[4], variant);
                    }
                }
                break;

            case 6:
                g_Gui.FUN_00439810(this->instruction->args.ints[0]);
                break;

            case 7:
                if (g_Gui.MsgWait())
                {
                    this->timer--;
                    goto done;
                }
                break;

            case 8:
#if defined(TH08_MODERN_IOS)
            {
                const i32 slot = this->instruction->args.ints[0];
                EclOperands::EnemyOverlay *target =
                    (slot >= 0 && slot < 92)
                        ? EclRunLowProposal::EclEnemySlot(static_cast<u32>(slot))
                        : NULL;
                if (target != NULL &&
                    (*reinterpret_cast<u32 *>(target->bytes + 0x3324) & 1U) != 0)
                {
                    EnemyEclContext *context = IosEclCurrentContext(target);
                    EclRawInstruction *current =
                        context != NULL ? context->currentInstr.Get() : NULL;
                    char message[224];
                    snprintf(message, sizeof(message),
                             "timeline-signal: slot=%d target=%p sub=%d context=%p current=%p",
                             slot, static_cast<void *>(target),
                             this->instruction->args.ints[1],
                             static_cast<void *>(context), static_cast<void *>(current));
                    modern::LogStartup(message);
                    *reinterpret_cast<i16 *>(target->bytes + 0x2D30) =
                        static_cast<i16>(this->instruction->args.ints[1]);
                }
                else
                {
                    char message[160];
                    snprintf(message, sizeof(message),
                             "timeline-signal-skip: slot=%d target=%p",
                             slot, static_cast<void *>(target));
                    modern::LogStartup(message);
                }
                break;
            }
#else
                *reinterpret_cast<i16 *>(reinterpret_cast<u8 *>(
                    EclRunLowProposal::g_EclEnemyTableF54CC0[this->instruction->args.ints[0]]) + 0x2d30) =
                    static_cast<i16>(this->instruction->args.ints[1]);
                break;
#endif

            case 9:
                g_GameManager.SetPower(this->instruction->args.ints[0]);
                break;

            case 10:
                if (EclRunLowProposal::g_EclEnemyTableF54CC0[this->instruction->args.ints[0]] != NULL &&
                    (*reinterpret_cast<u32 *>(reinterpret_cast<u8 *>(
#if defined(TH08_MODERN_IOS)
                         EclRunLowProposal::EclEnemySlot(this->instruction->args.ints[0])) + 0x3324) & 1) != 0)
#else
                         EclRunLowProposal::g_EclEnemyTableF54CC0[this->instruction->args.ints[0]]) + 0x3324) & 1) != 0)
#endif
                {
                    this->timer--;
                    goto done;
                }
                break;

            case 13:
            {
                i32 matchCount = 0;
                for (u32 i = 0; i < 4; i++)
                {
#if defined(TH08_MODERN_IOS)
                    if (EclRunLowProposal::EclEnemySlotInt(87 + i) ==
#else
                    if (reinterpret_cast<i32 *>(EclRunLowProposal::g_EclEnemyTableF54CC0)[87 + i] ==
#endif
                        this->instruction->args.ints[0])
                    {
                        matchCount++;
#if defined(TH08_MODERN_IOS)
                        EclRunLowProposal::EclEnemySlotInt(87 + i) = -1;
#else
                        reinterpret_cast<i32 *>(EclRunLowProposal::g_EclEnemyTableF54CC0)[87 + i] = -1;
#endif
                    }
                }
                if (matchCount == 0)
                {
                    this->timer--;
                    goto done;
                }
                break;
            }

            case 14:
            {
                for (u32 j = 0; j < 4; j++)
                {
#if defined(TH08_MODERN_IOS)
                    if (EclRunLowProposal::EclEnemySlotInt(87 + j) >= 0)
#else
                    if (reinterpret_cast<i32 *>(EclRunLowProposal::g_EclEnemyTableF54CC0)[87 + j] >= 0)
#endif
                        continue;
#if defined(TH08_MODERN_IOS)
                    EclRunLowProposal::EclEnemySlotInt(87 + j) =
#else
                    reinterpret_cast<i32 *>(EclRunLowProposal::g_EclEnemyTableF54CC0)[87 + j] =
#endif
                        this->instruction->args.ints[0];
                }
                break;
            }
            }
        }
        else if (this->timer < this->instruction->time)
        {
            break;
        }

advance_instruction:
#ifdef TH08_IOS
        if (this->instruction->size < 8)
        {
            char message[160];
            snprintf(message, sizeof(message),
                     "timeline-error: invalid instruction size=%u ptr=%p",
                     static_cast<unsigned>(this->instruction->size),
                     static_cast<void *>(this->instruction.Get()));
            modern::LogStartup(message);
            goto done;
        }
        this->instruction = reinterpret_cast<EclTimelineInstruction *>(
            reinterpret_cast<u8 *>(this->instruction.Get()) + this->instruction->size);
#else
        this->instruction = reinterpret_cast<EclTimelineInstruction *>(
            reinterpret_cast<u8 *>(this->instruction) + this->instruction->size);
#endif
    }

done:
    this->timer++;
#ifdef TH08_IOS
    if (currentRunSerial < 32)
        modern::LogStartup("timeline-run: return");
#endif
}


} // namespace th08
