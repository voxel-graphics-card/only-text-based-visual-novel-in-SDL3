// visual_effects.h - Header for screen-wide visual effects
#pragma once
#include <SDL3/SDL.h> // Included for SDL_FPoint, Uint32, Uint64

// --- Screen Shake Effect ---
// Initializes a screen shake effect with a duration in milliseconds and an intensity.
// durationMs: How long the shake lasts.
// intensity: How strong the shake is (e.g., 0.0f for none, 10.0f for moderate, 20.0f for strong).
void initScreenShake(Uint32 durationMs, float intensity);

// Updates the screen shake effect state each frame based on currentTicks.
void updateScreenShake(Uint64 currentTicks);

// Gets the current offset for screen shake to apply to rendered elements.
// Returns a 2D float point representing the (x, y) offset.
SDL_FPoint getScreenShakeOffset();

// Checks if screen shake is currently active.
bool isScreenShakeActive();


// --- Screen Tear Effect ---
// Initializes a screen tear effect.
// durationMs: How long the tear effect lasts.
// maxOffsetX: The maximum horizontal displacement for the "torn" part of the screen in pixels.
// tearLineDensity: A float value influencing how rapidly the tear line and offset change (e.g., 0.5 for smoother, 2.0 for choppy).
void initScreenTear(Uint32 durationMs, float maxOffsetX, float tearLineDensity);

// Updates the screen tear effect state each frame based on currentTicks.
void updateScreenTear(Uint64 currentTicks);

// Gets the current horizontal offset for elements at a given Y position due to screen tear.
// yPosition: The vertical position of the element being rendered.
// Returns a float representing the X offset. Returns 0.0f if the effect is inactive or if the yPosition is above the tear line.
float getScreenTearXOffset(float yPosition);

// Checks if the screen tear effect is currently active.
bool isScreenTearActive();
