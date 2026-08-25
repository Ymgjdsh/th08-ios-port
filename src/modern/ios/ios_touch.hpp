#pragma once

#include "Global.hpp"

namespace th08
{
namespace modern
{
namespace ios
{

// Polls SDL touch events and returns the current Touhou button bitset. The
// virtual controls use the same 640x480 coordinate space as the game, so the
// gameplay code remains independent of device orientation and safe-area size.
u16 PollButtons();
void ResetTouchState();

} // namespace ios
} // namespace modern
} // namespace th08

