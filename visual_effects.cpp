#include "visual_effects.h"
#include <cmath> // For sinf, cosf, fabs (and M_PI if needed, or define it)
#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif
#include <cstdlib> // For rand
#include <ctime> // For time (to seed rand if not done globally)

// Global state variables for screen shake
static bool shakeactive = false;
static Uint64 shakeStartTime = 0;
static Uint32 shakeDuration = 0;
static float shakeIntensity = 0.0f;
static float shakePhase = 0.0f; // For a smoother, oscillating shake

void initScreenShake(Uint32 durationMs, float intensity) {
    shakeStartTime = SDL_GetTicks();
    shakeactive = true;
    shakeDuration = durationMs;
    shakeIntensity = intensity;
    shakePhase = 0.0f; // Reset phase for new shake
    // Optionally seed rand here if not globally seeded in main or elsewhere
    // srand((unsigned int)time(NULL)); // Better to seed once in main
}

SDL_FPoint getScreenShakeOffset() {
    SDL_FPoint offset = {0.0f, 0.0f};
    if (shakeDuration > 0 && SDL_GetTicks() < (shakeStartTime + shakeDuration)) {
        // Calculate a decreasing intensity over time
        float elapsed = (float)(SDL_GetTicks() - shakeStartTime);
        float progress = elapsed / shakeDuration; // 0.0 to 1.0
        float currentIntensity = shakeIntensity * (1.0f - progress); // Shake fades out

        // Use a sin wave for a smoother, oscillating shake
        // Add random component for less predictable motion
        float angleX = shakePhase + ((float)rand() / RAND_MAX * 2.0f * M_PI); // Random phase offset
        float angleY = shakePhase + ((float)rand() / RAND_MAX * 2.0f * M_PI); // Random phase offset

        offset.x = SDL_sinf(angleX * 20.0f) * currentIntensity; // 20.0f makes it wobble faster
        offset.y = SDL_cosf(angleY * 25.0f) * currentIntensity; // Different frequency for Y for more variation

    } else {
        shakeactive = false; // Ensure shake is deactivated
        shakeDuration = 0; // End the shake if time is up
    }
    return offset;
}

void updateScreenShake(Uint64 currentTicks) {
    if (shakeactive && currentTicks >= (shakeStartTime + shakeDuration)) {
        shakeactive = false; // Shake has ended
        shakeDuration = 0; // Reset
        shakeStartTime = 0; // Reset
        shakeIntensity = 0.0f; // Reset
    }
    // Update phase for continuous oscillation, regardless of active status (can be useful for fading out)
    shakePhase += 0.5f; // Adjust speed of oscillation
}

bool isScreenShakeActive(){
    return shakeactive;
}