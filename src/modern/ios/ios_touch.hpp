#pragma once

#include "Global.hpp"
#include <SDL.h>

namespace th08
{
namespace modern
{
void LogStartup(const char *phase);

namespace ios
{

struct PresentationLayout
{
    int drawableWidth;
    int drawableHeight;
    int viewportX;
    int viewportY;
    int viewportWidth;
    int viewportHeight;
};

void SetPresentationLayout(const PresentationLayout &layout);
PresentationLayout GetPresentationLayout();

// Polls SDL touch events and returns the current Touhou button bitset. The
// virtual controls use the same 640x480 coordinate space as the game, so the
// gameplay code remains independent of device orientation and safe-area size.
u16 PollButtons();
void ProcessEvent(const SDL_Event &event);
void ResetTouchState();
void DrawVirtualControls();
bool IsDeveloperInvincible();

} // namespace ios
} // namespace modern
} // namespace th08
