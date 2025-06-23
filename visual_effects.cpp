#include "visual_effects.h" // Include the corresponding header
#include <cmath>    // For sinf, cosf, fabs, M_PI (for trigonometric functions)
#ifndef M_PI        // Define M_PI if it's not already defined by cmath or other headers
#define M_PI 3.14159265358979323846
#endif
#include <cstdlib>  // For rand()
#include <algorithm> // For std::max (used in fading intensity calculations)


// --- Global State Variables for Screen Shake ---
static bool shakeActive = false;        // Is the shake effect currently active?
static Uint64 shakeStartTime = 0;       // SDL_GetTicks() when the shake started
static Uint32 shakeDuration = 0;        // How long the shake should last (milliseconds)
static float shakeIntensity = 0.0f;     // Maximum displacement intensity of the shake
static SDL_FPoint currentShakeOffset = {0.0f, 0.0f}; // The current (x,y) offset to apply to screen elements

// --- Global State Variables for Screen Tear ---
static bool tearActive = false;         // Is the tear effect currently active?
static Uint64 tearStartTime = 0;        // SDL_GetTicks() when the tear started
static Uint32 tearDuration = 0;         // How long the tear should last (milliseconds)
static float tearMaxOffsetX = 0.0f;     // The maximum horizontal displacement for the torn part
static float tearLineDensity = 1.0f;    // How often the tear line changes (higher for more chaotic)
static float currentTearLineY = 0.0f;   // The Y-coordinate on screen where the tear visually occurs
static float currentTearOffsetX = 0.0f; // The X-offset to apply to elements below the tear line


// --- Screen Shake Effect Implementations ---
void initScreenShake(Uint32 durationMs, float intensity) {
    shakeStartTime = SDL_GetTicks();
    shakeActive = true;
    shakeDuration = durationMs;
    shakeIntensity = intensity;
    currentShakeOffset = {0.0f, 0.0f}; // Reset offset when starting new shake
}

void updateScreenShake(Uint64 currentTicks) {
    // If the shake is not active or its duration has passed, deactivate and reset
    if (!shakeActive || currentTicks >= shakeStartTime + shakeDuration) {
        currentShakeOffset = {0.0f, 0.0f}; // Ensure offset is zero when inactive
        shakeActive = false;
        return;
    }

    // Calculate how far along the shake duration we are (0.0 to 1.0)
    float elapsedRatio = (float)(currentTicks - shakeStartTime) / shakeDuration;
    // Make the intensity fade out over time
    float currentIntensity = shakeIntensity * (1.0f - elapsedRatio);
    currentIntensity = std::max(0.0f, currentIntensity); // Ensure intensity doesn't go negative

    // Apply random offsets based on the current fading intensity
    // Using rand() for simple randomness, normalized to -currentIntensity to +currentIntensity
    currentShakeOffset.x = ((float)rand() / RAND_MAX * (2.0f * currentIntensity)) - currentIntensity;
    currentShakeOffset.y = ((float)rand() / RAND_MAX * (2.0f * currentIntensity)) - currentIntensity;
}

SDL_FPoint getScreenShakeOffset() {
    return currentShakeOffset;
}

bool isScreenShakeActive() {
    return shakeActive;
}


// --- Screen Tear Effect Implementations ---
void initScreenTear(Uint32 durationMs, float maxOffsetX, float density) {
    tearStartTime = SDL_GetTicks();
    tearActive = true;
    tearDuration = durationMs;
    tearMaxOffsetX = maxOffsetX;
    tearLineDensity = density; // Controls choppiness/frequency of tear line changes

    // Initialize tear line and offset to a random state
    // Assumes a window height of around 720. Adjust if your winHeight is drastically different
    // For proper generalization, winHeight should be passed in or accessed via extern.
    // For now, using a common default or assuming a suitable range.
    currentTearLineY = (float)rand() / RAND_MAX * 720.0f; // Random Y within a typical screen height (0-720)
    currentTearOffsetX = ((float)rand() / RAND_MAX * (2.0f * tearMaxOffsetX)) - tearMaxOffsetX;
}

void updateScreenTear(Uint64 currentTicks) {
    // If tear is not active or its duration has passed, deactivate and reset
    if (!tearActive || currentTicks >= tearStartTime + tearDuration) {
        tearActive = false;
        currentTearLineY = 0.0f; // Reset tear line to top (no visible tear)
        currentTearOffsetX = 0.0f; // Reset offset to zero
        return;
    }

    // Calculate how far along the tear duration we are (0.0 to 1.0)
    float elapsedRatio = (float)(currentTicks - tearStartTime) / tearDuration;

    // Make the max offset decay over time, so the tear fades out
    float dynamicMaxOffset = tearMaxOffsetX * (1.0f - elapsedRatio);
    dynamicMaxOffset = std::max(0.0f, dynamicMaxOffset); // Ensure it doesn't go negative

    // Randomly update the tear line and offset based on density
    // Higher density (e.g., 1.0) makes it change more often and erratically.
    // 0.1f is a base factor to convert density into a probability per frame.
    if (((float)rand() / RAND_MAX) < (tearLineDensity * 0.1f)) {
        // Randomize the tear line Y-position. Assumes a common screen range.
        currentTearLineY = (float)rand() / RAND_MAX * 720.0f;

        // Randomize the offset within the *dynamic* (fading) max offset range
        currentTearOffsetX = ((float)rand() / RAND_MAX * (2.0f * dynamicMaxOffset)) - dynamicMaxOffset;
    }
}

float getScreenTearXOffset(float yPosition) {
    if (!tearActive) {
        return 0.0f; // No offset if tear effect is inactive
    }
    // If the element's Y position is below or at the current tear line, apply the offset
    if (yPosition >= currentTearLineY) {
        return currentTearOffsetX;
    }
    return 0.0f; // No offset if above the tear line
}

bool isScreenTearActive() {
    return tearActive;
}
