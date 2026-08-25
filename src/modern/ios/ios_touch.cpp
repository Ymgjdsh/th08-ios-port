#include "ios_touch.hpp"

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
    float moveX;
    float moveY;
    u16 buttons;
    u8 shotLatch;
    u8 bombLatch;
    u8 menuLatch;
    float sensitivity;
    TouchState()
        : moveFinger(-1), moveX(0), moveY(0), buttons(0), shotLatch(0), bombLatch(0), menuLatch(0), sensitivity(1.0f)
    {
    }
};

TouchState g_state;

void SetDirectional(float dx, float dy)
{
    const float dead = 0.07f;
    g_state.buttons &= (u16)~TH_BUTTON_DIRECTION;
    dx *= g_state.sensitivity;
    dy *= g_state.sensitivity;
    if (dx < -dead) g_state.buttons |= TH_BUTTON_LEFT;
    if (dx > dead) g_state.buttons |= TH_BUTTON_RIGHT;
    if (dy < -dead) g_state.buttons |= TH_BUTTON_UP;
    if (dy > dead) g_state.buttons |= TH_BUTTON_DOWN;
}

void HandleFingerDown(const SDL_TouchFingerEvent &finger)
{
    if (finger.x < 0.48f && finger.y > 0.42f && g_state.moveFinger < 0)
    {
        g_state.moveFinger = finger.fingerId;
        g_state.moveX = finger.x;
        g_state.moveY = finger.y;
        return;
    }

    if (finger.x >= 0.72f && finger.y >= 0.60f)
    {
        if (finger.x >= 0.86f && finger.y >= 0.78f)
            g_state.bombLatch = 1;
        else if (finger.x >= 0.84f)
            g_state.shotLatch = 1;
        else
            g_state.buttons |= TH_BUTTON_FOCUS;
    }
    else if (finger.x >= 0.72f && finger.y < 0.28f)
    {
        g_state.menuLatch = 1;
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
    }
    if (finger.x >= 0.84f && finger.y >= 0.60f)
        g_state.buttons &= (u16)~TH_BUTTON_FOCUS;
}
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
        if (event.key.keysym.sym == SDLK_ESCAPE) g_state.menuLatch = 1;
        if (event.key.keysym.sym == SDLK_z) g_state.shotLatch = 1;
        if (event.key.keysym.sym == SDLK_x) g_state.bombLatch = 1;
    }
    else if (event.type == SDL_KEYUP && event.key.keysym.sym == SDLK_z)
        g_state.shotLatch = 0;
}

u16 PollButtons()
{
    SDL_Event event;
    while (SDL_PollEvent(&event))
        ProcessEvent(event);

    u16 result = g_state.buttons;
    if (g_state.shotLatch) result |= TH_BUTTON_SHOOT;
    if (g_state.bombLatch) result |= TH_BUTTON_BOMB;
    if (g_state.menuLatch) result |= TH_BUTTON_MENU;
    g_state.shotLatch = 0;
    g_state.bombLatch = 0;
    g_state.menuLatch = 0;
    return result;
}

void ResetTouchState()
{
    g_state = TouchState();
}

} // namespace ios
} // namespace modern
} // namespace th08
