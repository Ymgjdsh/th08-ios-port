#include "ios_touch.hpp"
#include "ios_gl_legacy.hpp"

#include "BulletManager.hpp"
#include "GameManager.hpp"
#include "Gui.hpp"
#include "Player.hpp"
#include "Supervisor.hpp"

#include <SDL.h>
#include <math.h>
#include <string.h>

namespace th08
{
namespace modern
{
namespace ios
{
namespace
{
struct TouchState
{
    SDL_FingerID moveFinger;
    SDL_FingerID shotFinger;
    SDL_FingerID bombFinger;
    SDL_FingerID focusFinger;
    SDL_FingerID menuFinger;
    float moveX;
    float moveY;
    float moveDx;
    float moveDy;
    u16 buttons;
    u16 keyboardButtons;
    float sensitivity;
    TouchState()
        : moveFinger(-1), shotFinger(-1), bombFinger(-1), focusFinger(-1), menuFinger(-1),
          moveX(0), moveY(0), moveDx(0), moveDy(0), buttons(0), keyboardButtons(0), sensitivity(1.0f)
    {
    }
};

TouchState g_state;
PresentationLayout g_layout = {640, 480, 0, 0, 640, 480};
bool g_developerOpen = false;
bool g_developerInvincible = false;

struct RectGeometry
{
    float x;
    float y;
    float width;
    float height;
};

struct ControlGeometry
{
    float x;
    float y;
    float radius;
};

ControlGeometry MakeControl(float x, float y, float radiusScale)
{
    const float width = g_layout.drawableWidth > 0 ? static_cast<float>(g_layout.drawableWidth) : 640.0f;
    const float height = g_layout.drawableHeight > 0 ? static_cast<float>(g_layout.drawableHeight) : 480.0f;
    const float scale = width < height ? width : height;
    ControlGeometry geometry = {x * width, y * height, radiusScale * scale};
    return geometry;
}

bool HitControl(const SDL_TouchFingerEvent &finger, const ControlGeometry &control, float hitScale)
{
    const float width = g_layout.drawableWidth > 0 ? static_cast<float>(g_layout.drawableWidth) : 640.0f;
    const float height = g_layout.drawableHeight > 0 ? static_cast<float>(g_layout.drawableHeight) : 480.0f;
    const float dx = finger.x * width - control.x;
    const float dy = finger.y * height - control.y;
    const float radius = control.radius * hitScale;
    return dx * dx + dy * dy <= radius * radius;
}

ControlGeometry JoystickGeometry() { return MakeControl(0.15f, 0.78f, 0.105f); }
ControlGeometry ShotGeometry() { return MakeControl(0.90f, 0.69f, 0.075f); }
ControlGeometry BombGeometry() { return MakeControl(0.81f, 0.84f, 0.065f); }
ControlGeometry FocusGeometry() { return MakeControl(0.74f, 0.66f, 0.060f); }
ControlGeometry MenuGeometry() { return MakeControl(0.92f, 0.12f, 0.052f); }

RectGeometry DeveloperButtonGeometry()
{
    const float width = g_layout.drawableWidth > 0 ? static_cast<float>(g_layout.drawableWidth) : 640.0f;
    const float height = g_layout.drawableHeight > 0 ? static_cast<float>(g_layout.drawableHeight) : 480.0f;
    const float scale = width < height ? width : height;
    const float buttonWidth = scale * 0.16f;
    const float buttonHeight = scale * 0.072f;
    RectGeometry result = {width - buttonWidth - scale * 0.035f,
                           height * 0.225f, buttonWidth, buttonHeight};
    return result;
}

RectGeometry DeveloperPanelGeometry()
{
    const float width = g_layout.drawableWidth > 0 ? static_cast<float>(g_layout.drawableWidth) : 640.0f;
    const float height = g_layout.drawableHeight > 0 ? static_cast<float>(g_layout.drawableHeight) : 480.0f;
    const bool portrait = height > width;
    const float panelWidth = width * (portrait ? 0.88f : 0.60f);
    const float panelHeight = height * (portrait ? 0.76f : 0.84f);
    RectGeometry result = {(width - panelWidth) * 0.5f, (height - panelHeight) * 0.5f,
                           panelWidth, panelHeight};
    return result;
}

RectGeometry DeveloperRowGeometry(int row)
{
    const RectGeometry panel = DeveloperPanelGeometry();
    const float top = panel.y + panel.height * 0.155f;
    const float rowPitch = panel.height * 0.098f;
    RectGeometry result = {panel.x + panel.width * 0.065f, top + row * rowPitch,
                           panel.width * 0.87f, rowPitch * 0.78f};
    return result;
}

bool HitRect(const SDL_TouchFingerEvent &finger, const RectGeometry &rect)
{
    const float width = g_layout.drawableWidth > 0 ? static_cast<float>(g_layout.drawableWidth) : 640.0f;
    const float height = g_layout.drawableHeight > 0 ? static_cast<float>(g_layout.drawableHeight) : 480.0f;
    const float x = finger.x * width;
    const float y = finger.y * height;
    return x >= rect.x && x <= rect.x + rect.width &&
           y >= rect.y && y <= rect.y + rect.height;
}

bool IsGameplayReady()
{
    return g_Supervisor.curState == SupervisorState_GameManager && g_GameManager.globals != NULL;
}

void ClearGameplayTouches()
{
    const u16 keyboardButtons = g_state.keyboardButtons;
    const float sensitivity = g_state.sensitivity;
    g_state = TouchState();
    g_state.keyboardButtons = keyboardButtons;
    g_state.sensitivity = sensitivity;
}

void RefreshDeveloperHud()
{
    g_Gui.flags.lifeDisplayUpdateFrames = 2;
    g_Gui.flags.bombDisplayUpdateFrames = 2;
    g_Gui.flags.powerDisplayUpdateFrames = 2;
    g_Gui.flags.grazeDisplayUpdateFrames = 2;
    g_Gui.flags.pointDisplayUpdateFrames = 2;
    g_Gui.flags.timeDisplayUpdateFrames = 2;
}

void SetMaximumScore()
{
    ZunGlobals *globals = g_GameManager.globals;
    globals->score = 999999999U;
    globals->displayScore = 999999999U;
    globals->displayedHighScore = 999999999U;
    globals->unk0x10 = 0;
}

void SetMaximumItems()
{
    ZunGlobals *globals = g_GameManager.globals;
    globals->graze = 999999;
    globals->grazeInStage = 99999;
    globals->pointItemsCollected = 9999;
    globals->pointItemsCollectedInStage = 9999;
    globals->currentTimeOrbs = 99999;
    globals->totalTimeOrbs = 99999;
    globals->pointItemValue = 999999;
}

void ActivateDeveloperRow(int row)
{
    if (row == 7)
    {
        g_developerOpen = false;
        ClearGameplayTouches();
        modern::LogStartup("developer: panel closed");
        return;
    }

    if (!IsGameplayReady())
    {
        g_developerOpen = false;
        ClearGameplayTouches();
        modern::LogStartup("developer: action ignored outside gameplay");
        return;
    }

    switch (row)
    {
    case 0:
        g_developerInvincible = !g_developerInvincible;
        break;
    case 1:
        SetMaximumScore();
        g_GameManager.UpdateAntiTamper();
        break;
    case 2:
        SetMaximumItems();
        g_GameManager.UpdateAntiTamper();
        break;
    case 3:
        g_GameManager.SetPower(128);
        break;
    case 4:
        g_GameManager.SetLives(8);
        g_GameManager.SetBombCount(8);
        break;
    case 5:
        SetMaximumScore();
        SetMaximumItems();
        g_GameManager.globals->livesRemaining = 8.0f;
        g_GameManager.globals->bombsRemaining = 8.0f;
        g_GameManager.globals->playerPower = 128.0f;
        g_GameManager.UpdateAntiTamper();
        break;
    case 6:
        g_BulletManager.RemoveAllBullets(1);
        break;
    default:
        return;
    }

    RefreshDeveloperHud();
    char message[160];
    SDL_snprintf(message, sizeof(message),
                 "developer: row=%d invincible=%d score=%u graze=%d power=%d lives=%d bombs=%d",
                 row, g_developerInvincible ? 1 : 0, g_GameManager.globals->score,
                 g_GameManager.globals->graze, g_GameManager.GetPower(),
                 g_GameManager.GetLives(), g_GameManager.GetBombsRemaining());
    modern::LogStartup(message);
}

u16 PollSmokeTestButtons()
{
    static bool initialized;
    static Uint32 startedAt;
    static Uint32 lastPulse = static_cast<Uint32>(-1);
    static Uint32 teardownActionAt;
    static int teardownActionStage;
    static bool bombTriggered;
    static bool bombVerified;
    static f32 bombUsageBefore;
    static int developerStage;
    static int musicRoomStep = -1;
    static int invincibilityTestStage;
    static bool finalDeathTriggered;
    static int campaignLastState = -999;
    static int campaignLastStage = -999;
    static unsigned long campaignStageFrames;
    static unsigned long campaignInputFrames;
    static u32 campaignStageMask;
    static bool campaignSawEnding;
    static bool campaignCompleted;
    static bool campaignBombVerified;

    const char *enabled = SDL_getenv("TH08_IOS_SMOKE_TEST");
    if (enabled == NULL || enabled[0] == '\0' || enabled[0] == '0')
        return 0;

    if (!initialized)
    {
        initialized = true;
        startedAt = SDL_GetTicks();
        th08::modern::LogStartup("smoke: automatic menu confirmation enabled");
    }

    const Uint32 elapsed = SDL_GetTicks() - startedAt;
    if (elapsed < 2500)
        return 0;

    const bool testRestart = strcmp(enabled, "restart") == 0;
    const bool testTitle = strcmp(enabled, "title") == 0;
    const bool testBomb = strcmp(enabled, "bomb") == 0;
    const bool testDeveloper = strcmp(enabled, "developer") == 0;
    const bool testMusicRoom = strcmp(enabled, "musicroom") == 0;
    const bool testInvincible = strcmp(enabled, "invincible") == 0;
    const bool testRetry = strcmp(enabled, "retry") == 0;
    const bool testCampaign = strcmp(enabled, "campaign") == 0;

    if (testCampaign)
    {
        if (g_Supervisor.curState != campaignLastState)
        {
            campaignLastState = g_Supervisor.curState;
            char message[128];
            SDL_snprintf(message, sizeof(message), "smoke-campaign: supervisor state=%d",
                         campaignLastState);
            th08::modern::LogStartup(message);
        }

        if (g_Supervisor.curState == SupervisorState_Ending)
        {
            if (!campaignSawEnding)
            {
                char message[192];
                SDL_snprintf(message, sizeof(message),
                             "smoke-campaign: ending reached stages=%08x bomb=%d",
                             static_cast<unsigned>(campaignStageMask),
                             campaignBombVerified ? 1 : 0);
                th08::modern::LogStartup(message);
            }
            campaignSawEnding = true;
        }
        else if (campaignSawEnding && g_Supervisor.curState == SupervisorState_TitleScreen &&
                 !campaignCompleted)
        {
            campaignCompleted = true;
            const bool routeComplete =
                (campaignStageMask & (1u << STAGE1)) != 0 &&
                (campaignStageMask & (1u << STAGE2)) != 0 &&
                (campaignStageMask & (1u << STAGE3)) != 0 &&
                (campaignStageMask & (1u << STAGE5)) != 0 &&
                (campaignStageMask & ((1u << STAGE4A) | (1u << STAGE4B))) != 0 &&
                (campaignStageMask & ((1u << STAGE6A) | (1u << STAGE6B))) != 0;
            if (routeComplete && campaignBombVerified)
                th08::modern::LogStartup(
                    "smoke-campaign: COMPLETE - six-stage route and ending returned to title");
            else
            {
                char message[192];
                SDL_snprintf(message, sizeof(message),
                             "smoke-campaign: FAILED - stages=%08x bomb=%d",
                             static_cast<unsigned>(campaignStageMask),
                             campaignBombVerified ? 1 : 0);
                th08::modern::LogStartup(message);
            }
        }

        if (IsGameplayReady())
        {
            if (g_GameManager.currentStage != campaignLastStage)
            {
                campaignLastStage = g_GameManager.currentStage;
                campaignStageFrames = 0;
                if (campaignLastStage >= 0 && campaignLastStage < 32)
                    campaignStageMask |= 1u << campaignLastStage;
                char message[160];
                SDL_snprintf(message, sizeof(message),
                             "smoke-campaign: stage=%d difficulty=%d shot=%d entered",
                             campaignLastStage, g_GameManager.difficulty,
                             static_cast<int>(g_GameManager.shotType));
                th08::modern::LogStartup(message);
                if (!g_developerInvincible)
                    ActivateDeveloperRow(0);
                ActivateDeveloperRow(5);
            }

            ++campaignStageFrames;
            ++campaignInputFrames;
            if (bombTriggered && !campaignBombVerified &&
                g_GameManager.globals->bombsUsed > bombUsageBefore)
            {
                campaignBombVerified = true;
                th08::modern::LogStartup("smoke-campaign: bomb verified");
            }
            if (campaignStageFrames == 600)
            {
                bombUsageBefore = g_GameManager.globals->bombsUsed;
                bombTriggered = true;
                th08::modern::LogStartup("smoke-campaign: exercising bomb");
                return TH_BUTTON_BOMB;
            }

            // Release Z for one frame at regular intervals. This keeps normal
            // shooting active while providing fresh confirm edges for stage
            // dialogue and post-boss transitions.
            return (campaignInputFrames % 30u) == 0u ? 0 : TH_BUTTON_SHOOT;
        }

        // Title, ending and result screens require edge-triggered confirms.
        // Real-time pacing prevents unbounded smoke runs from stepping through
        // several menus in one animation frame.
        return ((elapsed / 500u) & 1u) != 0u ? TH_BUTTON_SHOOT : 0;
    }

    if (testMusicRoom)
    {
        // A clean save greys out Extra and Spell Practice, so four cursor
        // moves reach Music Room. Wait until the title entry animation and
        // generated help text have finished before sending those edges.
        if (elapsed < 8000)
            return 0;
        const int step = static_cast<int>((elapsed - 8000) / 800);
        if (step <= 4 && step != musicRoomStep)
        {
            musicRoomStep = step;
            if (step < 4)
            {
                th08::modern::LogStartup("smoke-musicroom: moving cursor down");
                return TH_BUTTON_DOWN;
            }
            th08::modern::LogStartup("smoke-musicroom: opening music room");
            return TH_BUTTON_SHOOT;
        }
        return 0;
    }

    if (testDeveloper && elapsed >= 40000 && developerStage == 0 && IsGameplayReady())
    {
        developerStage = 1;
        g_developerOpen = true;
        ClearGameplayTouches();
        th08::modern::LogStartup("smoke-developer: panel opened");
        return 0;
    }
    if (testDeveloper && elapsed >= 55000 && developerStage == 1)
    {
        developerStage = 2;
        ActivateDeveloperRow(5);
        g_developerOpen = false;
        ClearGameplayTouches();
        th08::modern::LogStartup("smoke-developer: max-all applied");
        return 0;
    }

    if (testInvincible && elapsed >= 40000 && invincibilityTestStage == 0 && IsGameplayReady())
    {
        invincibilityTestStage = 1;
        ActivateDeveloperRow(0);
        th08::modern::LogStartup("smoke-invincible: enabled");
        return 0;
    }
    if (testInvincible && elapsed >= 43000 && invincibilityTestStage == 1)
    {
        invincibilityTestStage = 2;
        const i8 stateBefore = g_Player.playerState;
        g_GameManager.RandomizeAntiTamper();
        g_Player.Die();
        const bool passed = g_Player.playerState == stateBefore && !g_GameManager.IsTampered();
        th08::modern::LogStartup(passed
                                     ? "smoke-invincible: death suppressed and integrity valid"
                                     : "smoke-invincible: FAILED");
        return 0;
    }
    if (testRetry && elapsed >= 40000 && !finalDeathTriggered && IsGameplayReady())
    {
        finalDeathTriggered = true;
        g_GameManager.SetLives(0);
        g_GameManager.SetBombCount(0);
        g_GameManager.RandomizeAntiTamper();
        g_Player.Die();
        th08::modern::LogStartup("smoke-retry: forced final death");
        return 0;
    }
    if ((testRestart || testTitle) && elapsed >= 60000)
    {
        if (teardownActionStage == 0)
        {
            teardownActionStage = 1;
            teardownActionAt = elapsed;
            th08::modern::LogStartup(testRestart
                                         ? "smoke-restart: opening pause menu"
                                         : "smoke-title: opening pause menu");
            return TH_BUTTON_MENU;
        }
        if (teardownActionStage == 1 && elapsed - teardownActionAt >= 3000)
        {
            teardownActionStage = 2;
            th08::modern::LogStartup(testRestart
                                         ? "smoke-restart: requesting restart"
                                         : "smoke-title: requesting title");
            return testRestart ? TH_BUTTON_RESET : TH_BUTTON_Q;
        }
        return 0;
    }

    if (testBomb && elapsed >= 40000 && !bombTriggered)
    {
        bombTriggered = true;
        bombUsageBefore = IsGameplayReady() ? g_GameManager.globals->bombsUsed : -1.0f;
        th08::modern::LogStartup("smoke-bomb: requesting player bomb");
        return TH_BUTTON_BOMB;
    }
    if (testBomb && bombTriggered && !bombVerified && IsGameplayReady() &&
        g_GameManager.globals->bombsUsed > bombUsageBefore)
    {
        bombVerified = true;
        th08::modern::LogStartup("smoke-bomb: PASS - bomb usage counter increased");
    }

    // Send a fresh confirmation every 1.5 seconds. This is deliberately
    // slower than the title transitions, so each pulse is observed as a new
    // press by WAS_PRESSED. Once gameplay starts, Z simply becomes shooting.
    const Uint32 pulse = (elapsed - 2500) / 1500;
    if (pulse != lastPulse)
    {
        lastPulse = pulse;
        char message[96];
        SDL_snprintf(message, sizeof(message), "smoke: confirm pulse %u", pulse + 1);
        th08::modern::LogStartup(message);
        return TH_BUTTON_SHOOT;
    }

    // Keep every setup menu edge-triggered, then hold Z once the smoke test is
    // safely inside gameplay so player projectiles exercise the draw chain.
    return elapsed >= 30000 ? TH_BUTTON_SHOOT : 0;
}

void SetDirectional(float dx, float dy)
{
    g_state.buttons &= (u16)~TH_BUTTON_DIRECTION;
    dx *= g_state.sensitivity;
    dy *= g_state.sensitivity;
    g_state.moveDx = dx;
    g_state.moveDy = dy;

    const float width = g_layout.drawableWidth > 0 ? static_cast<float>(g_layout.drawableWidth) : 640.0f;
    const float height = g_layout.drawableHeight > 0 ? static_cast<float>(g_layout.drawableHeight) : 480.0f;
    const float scale = width < height ? width : height;
    const float pixelX = dx * width;
    const float pixelY = dy * height;
    const float length = sqrtf(pixelX * pixelX + pixelY * pixelY);
    if (length < scale * 0.025f)
        return;

    // Divide the stick into eight equal angular sectors. The previous
    // per-axis dead zones left only a narrow corner where diagonals worked.
    const float diagonalThreshold = 0.3826834324f; // sin(22.5 degrees)
    const float normalizedX = pixelX / length;
    const float normalizedY = pixelY / length;
    if (normalizedX <= -diagonalThreshold) g_state.buttons |= TH_BUTTON_LEFT;
    if (normalizedX >= diagonalThreshold) g_state.buttons |= TH_BUTTON_RIGHT;
    if (normalizedY <= -diagonalThreshold) g_state.buttons |= TH_BUTTON_UP;
    if (normalizedY >= diagonalThreshold) g_state.buttons |= TH_BUTTON_DOWN;
}

void HandleFingerDown(const SDL_TouchFingerEvent &finger)
{
    if (g_developerOpen)
    {
        for (int row = 0; row < 8; ++row)
        {
            if (HitRect(finger, DeveloperRowGeometry(row)))
            {
                ActivateDeveloperRow(row);
                return;
            }
        }
        return;
    }

    if (IsGameplayReady() && HitRect(finger, DeveloperButtonGeometry()))
    {
        g_developerOpen = true;
        ClearGameplayTouches();
        modern::LogStartup("developer: panel opened");
        return;
    }

    if (g_state.shotFinger < 0 && HitControl(finger, ShotGeometry(), 1.35f))
    {
        g_state.shotFinger = finger.fingerId;
        g_state.buttons |= TH_BUTTON_SHOOT;
        return;
    }
    if (g_state.bombFinger < 0 && HitControl(finger, BombGeometry(), 1.35f))
    {
        g_state.bombFinger = finger.fingerId;
        g_state.buttons |= TH_BUTTON_BOMB;
        return;
    }
    if (g_state.focusFinger < 0 && HitControl(finger, FocusGeometry(), 1.35f))
    {
        g_state.focusFinger = finger.fingerId;
        g_state.buttons |= TH_BUTTON_FOCUS;
        return;
    }
    if (g_state.menuFinger < 0 && HitControl(finger, MenuGeometry(), 1.45f))
    {
        g_state.menuFinger = finger.fingerId;
        g_state.buttons |= TH_BUTTON_MENU;
        return;
    }

    if (finger.x < 0.48f && finger.y > 0.42f && g_state.moveFinger < 0)
    {
        g_state.moveFinger = finger.fingerId;
        g_state.moveX = finger.x;
        g_state.moveY = finger.y;
        SetDirectional(0.0f, 0.0f);
    }
}

void HandleFingerMotion(const SDL_TouchFingerEvent &finger)
{
    if (finger.fingerId != g_state.moveFinger)
        return;
    SetDirectional(finger.x - g_state.moveX, finger.y - g_state.moveY);
}

void HandleFingerUp(const SDL_TouchFingerEvent &finger)
{
    if (finger.fingerId == g_state.moveFinger)
    {
        g_state.moveFinger = -1;
        g_state.buttons &= (u16)~TH_BUTTON_DIRECTION;
        g_state.moveDx = 0.0f;
        g_state.moveDy = 0.0f;
    }
    if (finger.fingerId == g_state.shotFinger)
    {
        g_state.shotFinger = -1;
        g_state.buttons &= (u16)~TH_BUTTON_SHOOT;
    }
    if (finger.fingerId == g_state.bombFinger)
    {
        g_state.bombFinger = -1;
        g_state.buttons &= (u16)~TH_BUTTON_BOMB;
    }
    if (finger.fingerId == g_state.focusFinger)
    {
        g_state.focusFinger = -1;
        g_state.buttons &= (u16)~TH_BUTTON_FOCUS;
    }
    if (finger.fingerId == g_state.menuFinger)
    {
        g_state.menuFinger = -1;
        g_state.buttons &= (u16)~TH_BUTTON_MENU;
    }
}

void DrawCircle(float x, float y, float radius, GLubyte red, GLubyte green, GLubyte blue, GLubyte alpha)
{
    const int segments = 32;
    glColor4ub(red, green, blue, alpha);
    glBegin(GL_TRIANGLE_FAN);
    glVertex2f(x, y);
    for (int i = 0; i <= segments; ++i)
    {
        const float angle = static_cast<float>(i) * 6.28318530718f / static_cast<float>(segments);
        glVertex2f(x + cosf(angle) * radius, y + sinf(angle) * radius);
    }
    glEnd();
}

void DrawGlyph(char glyph, float x, float y, float radius)
{
    const float left = x - radius * 0.34f;
    const float right = x + radius * 0.34f;
    const float top = y - radius * 0.38f;
    const float bottom = y + radius * 0.38f;
    const float middle = y;
    glColor4ub(255, 255, 255, 235);
    glLineWidth(radius * 0.10f < 2.0f ? 2.0f : radius * 0.10f);
    glBegin(GL_LINES);
    if (glyph == 'Z')
    {
        glVertex2f(left, top); glVertex2f(right, top);
        glVertex2f(right, top); glVertex2f(left, bottom);
        glVertex2f(left, bottom); glVertex2f(right, bottom);
    }
    else if (glyph == 'X')
    {
        glVertex2f(left, top); glVertex2f(right, bottom);
        glVertex2f(right, top); glVertex2f(left, bottom);
    }
    else if (glyph == 'S')
    {
        glVertex2f(right, top); glVertex2f(left, top);
        glVertex2f(left, top); glVertex2f(left, middle);
        glVertex2f(left, middle); glVertex2f(right, middle);
        glVertex2f(right, middle); glVertex2f(right, bottom);
        glVertex2f(right, bottom); glVertex2f(left, bottom);
    }
    glEnd();
}

void DrawMenuGlyph(float x, float y, float radius)
{
    glColor4ub(255, 255, 255, 235);
    glLineWidth(radius * 0.10f < 2.0f ? 2.0f : radius * 0.10f);
    glBegin(GL_LINES);
    for (int row = -1; row <= 1; ++row)
    {
        glVertex2f(x - radius * 0.38f, y + row * radius * 0.24f);
        glVertex2f(x + radius * 0.38f, y + row * radius * 0.24f);
    }
    glEnd();
}

void DrawActionButton(const ControlGeometry &geometry, bool pressed, char glyph)
{
    DrawCircle(geometry.x, geometry.y, geometry.radius,
               pressed ? 242 : 34, pressed ? 112 : 42, pressed ? 132 : 54,
               pressed ? 220 : 150);
    DrawCircle(geometry.x, geometry.y, geometry.radius * 0.82f,
               pressed ? 190 : 92, pressed ? 38 : 20, pressed ? 64 : 38,
               pressed ? 210 : 120);
    if (glyph == 'M')
        DrawMenuGlyph(geometry.x, geometry.y, geometry.radius);
    else
        DrawGlyph(glyph, geometry.x, geometry.y, geometry.radius);
}

void DrawRect(const RectGeometry &rect, GLubyte red, GLubyte green, GLubyte blue, GLubyte alpha)
{
    glColor4ub(red, green, blue, alpha);
    glBegin(GL_TRIANGLES);
    glVertex2f(rect.x, rect.y);
    glVertex2f(rect.x + rect.width, rect.y);
    glVertex2f(rect.x, rect.y + rect.height);
    glVertex2f(rect.x, rect.y + rect.height);
    glVertex2f(rect.x + rect.width, rect.y);
    glVertex2f(rect.x + rect.width, rect.y + rect.height);
    glEnd();
}

void DrawRectOutline(const RectGeometry &rect, float thickness,
                     GLubyte red, GLubyte green, GLubyte blue, GLubyte alpha)
{
    RectGeometry top = {rect.x, rect.y, rect.width, thickness};
    RectGeometry bottom = {rect.x, rect.y + rect.height - thickness, rect.width, thickness};
    RectGeometry left = {rect.x, rect.y, thickness, rect.height};
    RectGeometry right = {rect.x + rect.width - thickness, rect.y, thickness, rect.height};
    DrawRect(top, red, green, blue, alpha);
    DrawRect(bottom, red, green, blue, alpha);
    DrawRect(left, red, green, blue, alpha);
    DrawRect(right, red, green, blue, alpha);
}

const u8 *TextGlyph(char glyph)
{
    static const u8 digits[10][7] = {
        {14,17,19,21,25,17,14},{4,12,4,4,4,4,14},{14,17,1,2,4,8,31},
        {30,1,1,14,1,1,30},{2,6,10,18,31,2,2},{31,16,16,30,1,1,30},
        {6,8,16,30,17,17,14},{31,1,2,4,8,8,8},{14,17,17,14,17,17,14},
        {14,17,17,15,1,2,12}};
    static const u8 letters[26][7] = {
        {14,17,17,31,17,17,17},{30,17,17,30,17,17,30},{15,16,16,16,16,16,15},
        {30,17,17,17,17,17,30},{31,16,16,30,16,16,31},{31,16,16,30,16,16,16},
        {15,16,16,23,17,17,15},{17,17,17,31,17,17,17},{14,4,4,4,4,4,14},
        {7,2,2,2,18,18,12},{17,18,20,24,20,18,17},{16,16,16,16,16,16,31},
        {17,27,21,21,17,17,17},{17,25,25,21,19,19,17},{14,17,17,17,17,17,14},
        {30,17,17,30,16,16,16},{14,17,17,17,21,18,13},{30,17,17,30,20,18,17},
        {15,16,16,14,1,1,30},{31,4,4,4,4,4,4},{17,17,17,17,17,17,14},
        {17,17,17,17,17,10,4},{17,17,17,21,21,21,10},{17,17,10,4,10,17,17},
        {17,17,10,4,4,4,4},{31,1,2,4,8,16,31}};
    static const u8 dash[7] = {0,0,0,31,0,0,0};
    static const u8 slash[7] = {1,2,2,4,8,8,16};
    static const u8 colon[7] = {0,12,12,0,12,12,0};
    static const u8 question[7] = {14,17,1,2,4,0,4};
    if (glyph >= '0' && glyph <= '9') return digits[glyph - '0'];
    if (glyph >= 'a' && glyph <= 'z') glyph = static_cast<char>(glyph - 'a' + 'A');
    if (glyph >= 'A' && glyph <= 'Z') return letters[glyph - 'A'];
    if (glyph == '-') return dash;
    if (glyph == '/') return slash;
    if (glyph == ':') return colon;
    return question;
}

void DrawText(const char *text, float x, float y, float pixel,
              GLubyte red, GLubyte green, GLubyte blue, GLubyte alpha)
{
    glColor4ub(red, green, blue, alpha);
    glBegin(GL_TRIANGLES);
    for (const char *cursor = text; *cursor != '\0'; ++cursor)
    {
        if (*cursor == ' ')
        {
            x += pixel * 6.0f;
            continue;
        }
        const u8 *glyph = TextGlyph(*cursor);
        for (int row = 0; row < 7; ++row)
        {
            for (int column = 0; column < 5; ++column)
            {
                if ((glyph[row] & (1 << (4 - column))) == 0)
                    continue;
                const float x0 = x + column * pixel;
                const float y0 = y + row * pixel;
                const float x1 = x0 + pixel;
                const float y1 = y0 + pixel;
                glVertex2f(x0, y0);
                glVertex2f(x1, y0);
                glVertex2f(x0, y1);
                glVertex2f(x0, y1);
                glVertex2f(x1, y0);
                glVertex2f(x1, y1);
            }
        }
        x += pixel * 6.0f;
    }
    glEnd();
}

float FitTextPixel(const char *text, float requested, float availableWidth)
{
    const size_t length = strlen(text);
    if (length == 0)
        return requested;
    const float maximum = availableWidth / (static_cast<float>(length) * 6.0f);
    return requested < maximum ? requested : maximum;
}

void DrawDeveloperButton()
{
    const RectGeometry button = DeveloperButtonGeometry();
    DrawRect(button, 88, 20, 62, 205);
    DrawRectOutline(button, button.height * 0.055f, 244, 172, 216, 225);
    const float pixel = FitTextPixel("DEV", button.height * 0.105f, button.width * 0.72f);
    const float textWidth = pixel * 18.0f;
    DrawText("DEV", button.x + (button.width - textWidth) * 0.5f,
             button.y + (button.height - pixel * 7.0f) * 0.5f,
             pixel, 255, 232, 248, 250);
}

void DrawDeveloperPanel()
{
    static const char *labels[8] = {
        "INVINCIBLE", "MAX SCORE", "MAX ITEMS", "MAX POWER",
        "FULL STOCK", "MAX ALL", "CLEAR BULLETS", "CLOSE"};
    const RectGeometry panel = DeveloperPanelGeometry();
    DrawRect(panel, 18, 8, 30, 244);
    DrawRectOutline(panel, panel.width * 0.006f, 190, 112, 174, 238);

    RectGeometry title = {panel.x + panel.width * 0.025f, panel.y + panel.height * 0.025f,
                          panel.width * 0.95f, panel.height * 0.095f};
    DrawRect(title, 64, 15, 62, 238);
    const char *titleText = "IMPERISHABLE NIGHT DEV";
    const float titlePixel = FitTextPixel(titleText, title.height * 0.105f, title.width * 0.90f);
    DrawText(titleText, title.x + title.width * 0.05f,
             title.y + (title.height - titlePixel * 7.0f) * 0.5f,
             titlePixel, 255, 210, 239, 255);

    for (int row = 0; row < 8; ++row)
    {
        const RectGeometry line = DeveloperRowGeometry(row);
        DrawRect(line, row == 7 ? 91 : 50, row == 7 ? 14 : 20,
                 row == 7 ? 43 : 69, 230);
        DrawRectOutline(line, line.height * 0.035f, 216, 128, 194, 205);
        const float pixel = FitTextPixel(labels[row], line.height * 0.105f, line.width * 0.72f);
        DrawText(labels[row], line.x + line.width * 0.055f,
                 line.y + (line.height - pixel * 7.0f) * 0.5f,
                 pixel, 255, 226, 246, 250);
        if (row == 0)
        {
            const char *value = g_developerInvincible ? "ON" : "OFF";
            const float valuePixel = FitTextPixel(value, pixel, line.width * 0.16f);
            const float valueWidth = strlen(value) * valuePixel * 6.0f;
            DrawText(value, line.x + line.width - valueWidth - line.width * 0.055f,
                     line.y + (line.height - valuePixel * 7.0f) * 0.5f,
                     valuePixel, g_developerInvincible ? 142 : 255,
                     g_developerInvincible ? 255 : 172, 190, 255);
        }
    }
}
}

void SetPresentationLayout(const PresentationLayout &layout)
{
    g_layout = layout;
}

PresentationLayout GetPresentationLayout()
{
    return g_layout;
}

void ProcessEvent(const SDL_Event &event)
{
    if (event.type == SDL_FINGERDOWN)
        HandleFingerDown(event.tfinger);
    else if (event.type == SDL_FINGERMOTION)
        HandleFingerMotion(event.tfinger);
    else if (event.type == SDL_FINGERUP)
        HandleFingerUp(event.tfinger);
    else if (event.type == SDL_KEYDOWN && event.key.repeat == 0)
    {
        if (g_developerOpen && event.key.keysym.sym == SDLK_ESCAPE)
        {
            g_developerOpen = false;
            ClearGameplayTouches();
            modern::LogStartup("developer: panel closed by keyboard");
            return;
        }
        if (event.key.keysym.sym == SDLK_ESCAPE) g_state.keyboardButtons |= TH_BUTTON_MENU;
        if (event.key.keysym.sym == SDLK_z) g_state.keyboardButtons |= TH_BUTTON_SHOOT;
        if (event.key.keysym.sym == SDLK_x) g_state.keyboardButtons |= TH_BUTTON_BOMB;
        if (event.key.keysym.sym == SDLK_LSHIFT || event.key.keysym.sym == SDLK_RSHIFT)
            g_state.keyboardButtons |= TH_BUTTON_FOCUS;
    }
    else if (event.type == SDL_KEYUP)
    {
        if (event.key.keysym.sym == SDLK_ESCAPE) g_state.keyboardButtons &= (u16)~TH_BUTTON_MENU;
        if (event.key.keysym.sym == SDLK_z) g_state.keyboardButtons &= (u16)~TH_BUTTON_SHOOT;
        if (event.key.keysym.sym == SDLK_x) g_state.keyboardButtons &= (u16)~TH_BUTTON_BOMB;
        if (event.key.keysym.sym == SDLK_LSHIFT || event.key.keysym.sym == SDLK_RSHIFT)
            g_state.keyboardButtons &= (u16)~TH_BUTTON_FOCUS;
    }
    else if (event.type == SDL_WINDOWEVENT && event.window.event == SDL_WINDOWEVENT_FOCUS_LOST)
        ResetTouchState();
}

u16 PollButtons()
{
    SDL_Event event;
    while (SDL_PollEvent(&event))
        ProcessEvent(event);

    const u16 smokeButtons = PollSmokeTestButtons();
    u16 result = g_developerOpen ? 0 : static_cast<u16>(g_state.buttons | g_state.keyboardButtons);
    if (!g_developerOpen)
        result |= smokeButtons;
    return result;
}

void ResetTouchState()
{
    g_state = TouchState();
    g_developerOpen = false;
}

bool IsDeveloperInvincible()
{
    return g_developerInvincible;
}

void DrawVirtualControls()
{
    if (g_layout.drawableWidth <= 0 || g_layout.drawableHeight <= 0)
        return;

    GLint previousProgram = 0;
    GLint previousTexture = 0;
    GLint previousArrayBuffer = 0;
    GLint previousElementBuffer = 0;
    GLint previousViewport[4] = {0, 0, 0, 0};
    GLboolean blendEnabled = glIsEnabled(GL_BLEND);
    GLboolean depthEnabled = glIsEnabled(GL_DEPTH_TEST);
    GLboolean cullEnabled = glIsEnabled(GL_CULL_FACE);
    GLboolean scissorEnabled = glIsEnabled(GL_SCISSOR_TEST);
    GLboolean depthMask = GL_TRUE;
    glGetIntegerv(GL_CURRENT_PROGRAM, &previousProgram);
    glGetIntegerv(GL_TEXTURE_BINDING_2D, &previousTexture);
    glGetIntegerv(GL_ARRAY_BUFFER_BINDING, &previousArrayBuffer);
    glGetIntegerv(GL_ELEMENT_ARRAY_BUFFER_BINDING, &previousElementBuffer);
    glGetIntegerv(GL_VIEWPORT, previousViewport);
    glGetBooleanv(GL_DEPTH_WRITEMASK, &depthMask);

    glViewport(0, 0, g_layout.drawableWidth, g_layout.drawableHeight);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
    glBindTexture(GL_TEXTURE_2D, 0);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glDisable(GL_DEPTH_TEST);
    glDisable(GL_CULL_FACE);
    glDisable(GL_SCISSOR_TEST);
    glDepthMask(GL_FALSE);

    if (g_developerOpen && !IsGameplayReady())
    {
        g_developerOpen = false;
        ClearGameplayTouches();
    }

    if (g_developerOpen)
    {
        DrawDeveloperPanel();
    }
    else
    {
        const u16 pressed = g_state.buttons | g_state.keyboardButtons;
        const ControlGeometry joystick = JoystickGeometry();
        DrawCircle(joystick.x, joystick.y, joystick.radius, 28, 34, 42, 130);
        DrawCircle(joystick.x, joystick.y, joystick.radius * 0.78f, 70, 78, 90, 80);
        const float maxOffset = joystick.radius * 0.42f;
        float dx = g_state.moveDx * static_cast<float>(g_layout.drawableWidth);
        float dy = g_state.moveDy * static_cast<float>(g_layout.drawableHeight);
        const float length = sqrtf(dx * dx + dy * dy);
        if (length > maxOffset && length > 0.0f)
        {
            dx = dx * maxOffset / length;
            dy = dy * maxOffset / length;
        }
        DrawCircle(joystick.x + dx, joystick.y + dy, joystick.radius * 0.39f,
                   230, 235, 242, g_state.moveFinger >= 0 ? 210 : 145);

        DrawActionButton(ShotGeometry(), (pressed & TH_BUTTON_SHOOT) != 0, 'Z');
        DrawActionButton(BombGeometry(), (pressed & TH_BUTTON_BOMB) != 0, 'X');
        DrawActionButton(FocusGeometry(), (pressed & TH_BUTTON_FOCUS) != 0, 'S');
        DrawActionButton(MenuGeometry(), (pressed & TH_BUTTON_MENU) != 0, 'M');
        if (IsGameplayReady())
            DrawDeveloperButton();
    }

    glDepthMask(depthMask);
    if (blendEnabled) glEnable(GL_BLEND); else glDisable(GL_BLEND);
    if (depthEnabled) glEnable(GL_DEPTH_TEST); else glDisable(GL_DEPTH_TEST);
    if (cullEnabled) glEnable(GL_CULL_FACE); else glDisable(GL_CULL_FACE);
    if (scissorEnabled) glEnable(GL_SCISSOR_TEST); else glDisable(GL_SCISSOR_TEST);
    glBindTexture(GL_TEXTURE_2D, static_cast<GLuint>(previousTexture));
    glBindBuffer(GL_ARRAY_BUFFER, static_cast<GLuint>(previousArrayBuffer));
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, static_cast<GLuint>(previousElementBuffer));
    glUseProgram(static_cast<GLuint>(previousProgram));
    glViewport(previousViewport[0], previousViewport[1], previousViewport[2], previousViewport[3]);
}

} // namespace ios
} // namespace modern
} // namespace th08
