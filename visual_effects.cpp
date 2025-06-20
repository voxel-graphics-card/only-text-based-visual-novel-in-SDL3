#include "visual_effects.h"
#include <cmath> // For sinf, cosf, fabs (and M_PI if needed, or define it)
#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif
#include <cstdlib> // For rand
#include <ctime> // For time (to seed rand if not done globally)

// Global state variables for screen shake
static bool shakeActive = false;
static Uint64 shakeStartTime = 0;
static Uint32 shakeDuration = 0;
static float shakeIntensity = 0.0f;
static SDL_FPoint currentShakeOffset = {0.0f, 0.0f};

// Global state variables for screen tear (NEW)
static bool tearActive = false;
static Uint64 tearStartTime = 0;
static Uint32 tearDuration = 0;
static float tearMaxOffsetX = 0.0f;
static float tearLineDensity = 1.0f; // How quickly the tear line changes
static float currentTearLineY = 0.0f; // The Y-coordinate where the tear occurs
static float currentTearOffsetX = 0.0f; // The X-offset to apply below the tear line

// --- Screen Shake Effect Implementations ---
void initScreenShake(Uint32 durationMs, float intensity) {
    shakeStartTime = SDL_GetTicks();
    shakeActive = true;
    shakeDuration = durationMs;
    shakeIntensity = intensity;
    currentShakeOffset = {0.0f, 0.0f}; // Reset
}

void updateScreenShake(Uint64 currentTicks) {
    if (!shakeActive || currentTicks >= shakeStartTime + shakeDuration) {
        currentShakeOffset = {0.0f, 0.0f}; // Stop shaking
        shakeActive = false; // Ensure it's off
        return;
    }

    float elapsedRatio = (float)(currentTicks - shakeStartTime) / shakeDuration;
    float currentIntensity = shakeIntensity * (1.0f - elapsedRatio); // Intensity fades out
    currentIntensity = std::max(0.0f, currentIntensity);

    // Random offset based on current intensity
    currentShakeOffset.x = (float)rand() / RAND_MAX * (2.0f * currentIntensity) - currentIntensity;
    currentShakeOffset.y = (float)rand() / RAND_MAX * (2.0f * currentIntensity) - currentIntensity;
}

SDL_FPoint getScreenShakeOffset() {
    return currentShakeOffset;
}

bool isScreenShakeActive() {
    return shakeActive;
}

// --- Screen Tear Effect Implementations (NEW) ---
void initScreenTear(Uint32 durationMs, float maxOffsetX, float density) {
    tearStartTime = SDL_GetTicks();
    tearActive = true;
    tearDuration = durationMs;
    tearMaxOffsetX = maxOffsetX;
    tearLineDensity = density;
    // Initialize tear line and offset to a random state
    currentTearLineY = (float)rand() / RAND_MAX * 720.0f; // Random Y within window height (assuming 720)
    currentTearOffsetX = (float)rand() / RAND_MAX * (2.0f * tearMaxOffsetX) - tearMaxOffsetX;
}

void updateScreenTear(Uint64 currentTicks) {
    if (!tearActive || currentTicks >= tearStartTime + tearDuration) {
        tearActive = false; // Deactivate tear
        currentTearLineY = 0.0f; // Reset to default
        currentTearOffsetX = 0.0f; // Reset to default
        return;
    }

    // Adjust tear line and offset over time for dynamic effect
    float elapsedRatio = (float)(currentTicks - tearStartTime) / tearDuration;

    // Make the tear offset decay over time
    float dynamicMaxOffset = tearMaxOffsetX * (1.0f - elapsedRatio);
    dynamicMaxOffset = std::max(0.0f, dynamicMaxOffset); // Ensure it doesn't go negative

    // Randomly update the tear line and offset based on density
    // Higher density means more frequent and larger jumps
    // We'll use a simple threshold to decide when to change
    if ((float)rand() / RAND_MAX < tearLineDensity * 0.1f) { // 10% chance per frame to change tear parameters
        // Randomize the tear line Y-position within a reasonable screen range
        // Assuming typical window height, adjust as needed.
        currentTearLineY = (float)rand() / RAND_MAX * 720.0f; // Example: Random Y from 0 to 720

        // Randomize the offset within the dynamic max offset
        currentTearOffsetX = (float)rand() / RAND_MAX * (2.0f * dynamicMaxOffset) - dynamicMaxOffset;
    }
}

float getScreenTearXOffset(float yPosition) {
    if (!tearActive) {
        return 0.0f;
    }
    // If the element's Y position is below the current tear line, apply the offset
    if (yPosition >= currentTearLineY) {
        return currentTearOffsetX;
    }
    return 0.0f; // No offset if above the tear line
}

bool isScreenTearActive() {
    return tearActive;
}
