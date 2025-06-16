// text_effects.cpp
#include "text_effects.h"
#include "text_ui.h" // For winWidth, winHeight, textPadding extern constants - REMOVED, not directly used here, text_ui.h usually includes global constants
#include <cstdlib> // For rand(), srand()
#include <iostream>
#include <sstream> // For std::istringstream
#include <cmath> // For std::fabs, std::sin, M_PI

// Note: srand((unsigned int)time(NULL)); should be called ONCE in main.cpp at the start of the program.
// We will now use rand() directly here.

// --- Jitter Effect ---
std::vector<RenderedWord> initJitterWords(SDL_Renderer* renderer, TTF_Font* font, const std::string& text, int x, int y, int wrapWidth) {
    std::vector<RenderedWord> words;
    std::string currentWord;
    int currentX = x;
    int currentY = y;
    int spaceWidth;
    TTF_GetStringSize(font, " ", 1, &spaceWidth, nullptr); // Get space width for a single space character

    std::istringstream iss(text);
    std::string wordStr;
    while (iss >> wordStr) {
        int wordW, wordH;
        TTF_GetStringSize(font, wordStr.c_str(), wordStr.length(), &wordW, &wordH);

        // Simple wrapping logic (could be more sophisticated)
        if (currentX + wordW > x + wrapWidth && currentX > x) {
            currentX = x;
            currentY += wordH; // Move to next line
        }

        RenderedWord word;
        word.text = wordStr;
        word.rect = {(float)currentX, (float)currentY, (float)wordW, (float)wordH};
        word.vx = 0; word.vy = 0; // Not used for jitter
        word.ax = 0; word.ay = 0; // Not used for jitter
        word.active = true; // Always active for jitter
        words.push_back(word);

        currentX += wordW + spaceWidth;
    }
    return words;
}

void applyJitter(std::vector<RenderedWord>& words) {
    // Reduce this value to decrease the intensity of the jitter
    const float JITTER_MAGNITUDE = 0.1f; // Changed from a higher value (e.g., 2.0f)

    for (auto& word : words) {
        // Calculate random offsets between -JITTER_MAGNITUDE and +JITTER_MAGNITUDE
        float offsetX = ((float)rand() / RAND_MAX * (2 * JITTER_MAGNITUDE)) - JITTER_MAGNITUDE;
        float offsetY = ((float)rand() / RAND_MAX * (2 * JITTER_MAGNITUDE)) - JITTER_MAGNITUDE;

        word.rect.x = word.rect.x + offsetX;
        word.rect.y = word.rect.y + offsetY;
    }
}

// --- Physics Effects (Fall/Float) ---
std::vector<RenderedWord> initPhysicsWords(SDL_Renderer* renderer, TTF_Font* font, const std::string& text, int x, int y, int wrapWidth) {
    std::vector<RenderedWord> words;
    // Similar word splitting logic as initJitterWords
    std::string currentWord;
    int currentX = x;
    int currentY = y;
    int spaceWidth;
    TTF_GetStringSize(font, " ", 1, &spaceWidth, nullptr);

    std::istringstream iss(text);
    std::string wordStr;
    while (iss >> wordStr) {
        int wordW, wordH;
        TTF_GetStringSize(font, wordStr.c_str(), wordStr.length(), &wordW, &wordH);

        if (currentX + wordW > x + wrapWidth && currentX > x) {
            currentX = x;
            currentY += wordH;
        }

        RenderedWord word;
        word.text = wordStr;
        word.rect = {(float)currentX, (float)currentY, (float)wordW, (float)wordH};
        word.vx = 0.0f;
        word.vy = 0.0f;
        word.ax = 0.0f;
        word.ay = 0.0f;
        word.active = true; // Start active
        words.push_back(word);

        currentX += wordW + spaceWidth;
    }
    return words;
}

// Assuming winHeight is accessible globally or via a header
// If winHeight is truly extern from text_ui.h, then that's fine.
// Otherwise, you might need to pass it or make it a global constant here.
// For now, assuming it's available via linkage.
extern const int winHeight; // Declared here for this file to use if from text_ui.h

void applyfallEffect(std::vector<RenderedWord>& words) {
    const float INITIAL_VELOCITY_X_SPREAD = 100.0f; // Spread around 0 for horizontal velocity
    const float INITIAL_VELOCITY_Y_MIN = 200.0f;    // Min initial downward velocity
    const float INITIAL_VELOCITY_Y_MAX = 400.0f;    // Max initial downward velocity

    for (auto& word : words) {
        // Generate random float between -X_SPREAD and X_SPREAD
        word.vx = (float)rand() / RAND_MAX * (2.0f * INITIAL_VELOCITY_X_SPREAD) - INITIAL_VELOCITY_X_SPREAD;
        // Generate random float between Y_MIN and Y_MAX (downwards)
        word.vy = ((float)rand() / RAND_MAX * (INITIAL_VELOCITY_Y_MAX - INITIAL_VELOCITY_Y_MIN)) + INITIAL_VELOCITY_Y_MIN;
        word.ay = 980.0f; // Apply gravity for fall effect
        word.active = true; // Ensure they are active
    }
}

void applyfloatEffect(std::vector<RenderedWord>& words)
{
    const float INITIAL_VELOCITY_X_SPREAD = 100.0f; // Spread around 0 for horizontal velocity
    const float INITIAL_VELOCITY_Y_MIN = 50.0f;    // Min initial upward velocity
    const float INITIAL_VELOCITY_Y_MAX = 100.0f;    // Max initial upward velocity

    for (auto& word : words) {
        // Generate random float between -X_SPREAD and X_SPREAD
        word.vx = (float)rand() / RAND_MAX * (2.0f * INITIAL_VELOCITY_X_SPREAD) - INITIAL_VELOCITY_X_SPREAD;
        // Generate random float between -Y_MAX and -Y_MIN (upwards)
        word.vy = -(((float)rand() / RAND_MAX * (INITIAL_VELOCITY_Y_MAX - INITIAL_VELOCITY_Y_MIN)) + INITIAL_VELOCITY_Y_MIN);
        word.ay = -980.0f; // Apply anti-gravity for float effect (negative for upward acceleration)
        word.active = true; // Ensure they are active
    }
}

void updatePhysicsWords(std::vector<RenderedWord>& words, float deltaTime) {
    const float DRAG_FACTOR = 0.98f; // Reduced drag for smoother motion
    const float MIN_VELOCITY_DEACTIVATE = 5.0f; // Stop small movements
    const float BOUNCE_FACTOR = 0.7f; // How much velocity is retained after bounce (0.0 to 1.0)

    // Boundaries are based on winHeight, which is extern from text_ui.h
    const int BOTTOM_COLLISION_Y = winHeight - 50; // A bit above the bottom of the window
    const int TOP_COLLISION_Y = 0; // Top of the window

    for (auto& word : words) {
        if (word.active) {
            // Apply acceleration
            word.vx += word.ax * deltaTime;
            word.vy += word.ay * deltaTime;

            // Apply drag
            word.vx *= std::pow(DRAG_FACTOR, deltaTime * 60.0f);
            word.vy *= std::pow(DRAG_FACTOR, deltaTime * 60.0f);

            // Update position
            word.rect.x += word.vx * deltaTime;
            word.rect.y += word.vy * deltaTime;

            // Bounce logic
            if (word.ay > 0) { // Falling (positive acceleration)
                if (word.rect.y + word.rect.h > BOTTOM_COLLISION_Y) {
                    word.rect.y = BOTTOM_COLLISION_Y - word.rect.h; // Clamp to boundary
                    word.vy *= -BOUNCE_FACTOR; // Reverse velocity and damp
                    // If velocity is very small after bounce, stop it
                    if (std::fabs(word.vy) < MIN_VELOCITY_DEACTIVATE) {
                        word.vy = 0;
                        word.ay = 0; // Stop gravity
                        word.active = false; // Deactivate if it has settled
                    }
                }
            } else if (word.ay < 0) { // Floating (negative acceleration)
                if (word.rect.y < TOP_COLLISION_Y) {
                    word.rect.y = TOP_COLLISION_Y; // Clamp to boundary
                    word.vy *= -BOUNCE_FACTOR; // Reverse velocity and damp
                    // If velocity is very small after bounce, stop it
                    if (std::fabs(word.vy) < MIN_VELOCITY_DEACTIVATE) {
                        word.vy = 0;
                        word.ay = 0; // Stop anti-gravity
                        word.active = false; // Deactivate if it has settled
                    }
                }
            } else { // No active acceleration (e.g., after initial pop, or a settled bounce)
                if (std::fabs(word.vx) < MIN_VELOCITY_DEACTIVATE && std::fabs(word.vy) < MIN_VELOCITY_DEACTIVATE) {
                    word.active = false; // Deactivate if nearly stopped
                }
            }
        }
    }
}


// --- Text Color Pulse Effect ---
static Uint64 pulseStartTime = 0;
static Uint64 pulseDuration = 0;
static float pulseFrequencyHz = 0.0f; // Initialize with a default value, e.g., 2.0f
static SDL_Color pulseColorStart;
static SDL_Color pulseColorEnd;
static bool pulseActive = false;

// Corrected: durationMs is Uint64 to match main.cpp and SDL_GetTicks()
void initTextColorPulse(Uint64 durationMs, float frequencyHz, SDL_Color c1, SDL_Color c2) {
    pulseStartTime = SDL_GetTicks();
    pulseDuration = durationMs;
    pulseFrequencyHz = frequencyHz;
    pulseColorStart = c1;
    pulseColorEnd = c2;
    pulseActive = true;
}

SDL_Color getPulsingTextColor(Uint64 currentTicks) {
    if (!pulseActive) {
        return pulseColorStart; // Return the start color if not active
    }

    Uint64 elapsed = currentTicks - pulseStartTime;
    // Check if pulse has ended
    if (pulseDuration > 0 && elapsed >= pulseDuration) {
        pulseActive = false;
        return pulseColorStart; // Return final color state or start, based on desired behavior
    }

    float timeInSeconds = (float)elapsed / 500.0f;

    // Corrected phase calculation: starts at 0.0f and oscillates up to 1.0f and back
    float phase = (std::sin(timeInSeconds * pulseFrequencyHz * 2 * M_PI - M_PI / 2.0f) + 1.0f) / 2.0f;

    SDL_Color interpolatedColor;
    // Correct linear interpolation formula: color = color1 * (1-phase) + color2 * phase
    interpolatedColor.r = (Uint8)(pulseColorStart.r * (1.0f - phase) + pulseColorEnd.r * phase);
    interpolatedColor.g = (Uint8)(pulseColorStart.g * (1.0f - phase) + pulseColorEnd.g * phase);
    interpolatedColor.b = (Uint8)(pulseColorStart.b * (1.0f - phase) + pulseColorEnd.b * phase);
    interpolatedColor.a = (Uint8)(pulseColorStart.a * (1.0f - phase) + pulseColorEnd.a * phase);

    return interpolatedColor;
}

bool isTextColorPulseActive() {
    return pulseActive;
}

void deactivateTextColorPulse() {
    pulseActive = false;
    // Reset state variables to their defaults when deactivated
    pulseStartTime = 0;
    pulseDuration = 0;
    pulseFrequencyHz = 0.0f; // Or your desired default frequency if it should not be 0
}