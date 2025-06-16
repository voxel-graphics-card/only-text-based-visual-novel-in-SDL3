// visual_effects.h
#pragma once
#include <SDL3/SDL.h>
// Removed <random> and <chrono> as rand() is now used and seeding is in main.cpp

// --- Screen Shake Effect ---
// Initializes a screen shake effect with a duration in milliseconds and an intensity.
void initScreenShake(Uint32 durationMs, float intensity);

// Updates the screen shake effect state each frame based on currentTicks.
void updateScreenShake(Uint64 currentTicks);

// Gets the current offset for screen shake to apply to rendered elements.
SDL_FPoint getScreenShakeOffset();

// Checks if screen shake is currently active.
bool isScreenShakeActive();