// text_effects.cpp - Implementation for text-specific effects
#include "text_effects.h" // Include the corresponding header
#include <cstdlib> // For rand(), srand()
#include <iostream> // For std::cerr
#include <sstream> // For std::istringstream
#include <cmath>   // For std::fabs, std::sin, M_PI, std::pow (for drag)
#include <algorithm> // For std::min, std::max (not directly used here, but common)

// --- Global Constants (Declared extern in text_ui.h, defined in main.cpp) ---
// We need winHeight here for physics collision detection.
// Assuming it is accessible globally via linkage from main.cpp through text_ui.h include.
extern const int winHeight;


// --- Jitter Effect Implementations ---
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

        // Simple wrapping logic
        if (currentX + wordW > x + wrapWidth && currentX > x) {
            currentX = x;
            currentY += wordH; // Move to next line
        }

        RenderedWord word;
        word.text = wordStr;
        word.rect = {(float)currentX, (float)currentY, (float)wordW, (float)wordH};
        word.originalRect = word.rect; // Store the initial position for jitter effect
        word.vx = 0; word.vy = 0;
        word.ax = 0; word.ay = 0;
        word.active = true;
        words.push_back(word);

        currentX += wordW + spaceWidth;
    }
    return words;
}

void applyJitter(std::vector<RenderedWord>& words) {
    // This value controls the intensity of the jitter. Lower value for less jitter.
    const float JITTER_MAGNITUDE = 0.1f;

    for (auto& word : words) {
        // Calculate random offsets between -JITTER_MAGNITUDE and +JITTER_MAGNITUDE
        // Offsets are applied relative to the word's original (non-jittered) position
        float offsetX = ((float)rand() / RAND_MAX * (2 * JITTER_MAGNITUDE)) - JITTER_MAGNITUDE;
        float offsetY = ((float)rand() / RAND_MAX * (2 * JITTER_MAGNITUDE)) - JITTER_MAGNITUDE;

        word.rect.x = word.originalRect.x + offsetX;
        word.rect.y = word.originalRect.y + offsetY;
    }
}


// --- Word Physics (Fall/Float) Implementations ---
std::vector<RenderedWord> initPhysicsWords(SDL_Renderer* renderer, TTF_Font* font, const std::string& text, int x, int y, int wrapWidth) {
    std::vector<RenderedWord> words;
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
        word.originalRect = word.rect; // Store original for reference if needed, not strictly used in physics update
        word.vx = 0.0f;
        word.vy = 0.0f;
        word.ax = 0.0f;
        word.ay = 0.0f;
        word.active = true; // Start active for physics
        words.push_back(word);

        currentX += wordW + spaceWidth;
    }
    return words;
}

void applyfallEffect(std::vector<RenderedWord>& words) {
    const float INITIAL_VELOCITY_X_SPREAD = 100.0f; // Spread around 0 for horizontal velocity
    const float INITIAL_VELOCITY_Y_MIN = 200.0f;    // Min initial downward velocity
    const float INITIAL_VELOCITY_Y_MAX = 400.0f;    // Max initial downward velocity
    const float GRAVITY = 980.0f;                   // Standard gravity (pixels/sec^2)

    for (auto& word : words) {
        word.vx = (float)rand() / RAND_MAX * (2.0f * INITIAL_VELOCITY_X_SPREAD) - INITIAL_VELOCITY_X_SPREAD;
        word.vy = ((float)rand() / RAND_MAX * (INITIAL_VELOCITY_Y_MAX - INITIAL_VELOCITY_Y_MIN)) + INITIAL_VELOCITY_Y_MIN;
        word.ay = GRAVITY; // Apply gravity for fall effect
        word.active = true;
    }
}

void applyfloatEffect(std::vector<RenderedWord>& words) {
    const float INITIAL_VELOCITY_X_SPREAD = 100.0f;
    const float INITIAL_VELOCITY_Y_MIN = 200.0f;
    const float INITIAL_VELOCITY_Y_MAX = 400.0f;
    const float ANTI_GRAVITY = -980.0f; // Negative for upward acceleration

    for (auto& word : words) {
        word.vx = (float)rand() / RAND_MAX * (2.0f * INITIAL_VELOCITY_X_SPREAD) - INITIAL_VELOCITY_X_SPREAD;
        word.vy = -(((float)rand() / RAND_MAX * (INITIAL_VELOCITY_Y_MAX - INITIAL_VELOCITY_Y_MIN)) + INITIAL_VELOCITY_Y_MIN);
        word.ay = ANTI_GRAVITY; // Apply anti-gravity for float effect
        word.active = true;
    }
}

void updatePhysicsWords(std::vector<RenderedWord>& words, float deltaTime) {
    const float DRAG_FACTOR = 0.98f; // Reduced drag for smoother motion
    const float MIN_VELOCITY_DEACTIVATE = 5.0f; // Stop small movements
    const float BOUNCE_FACTOR = 0.7f; // How much velocity is retained after bounce (0.0 to 1.0)

    // Collision boundaries relative to window height
    const int BOTTOM_COLLISION_Y = winHeight - 50; // A bit above the bottom of the window
    const int TOP_COLLISION_Y = 0;                 // Top of the window

    for (auto& word : words) {
        if (word.active) {
            // Apply acceleration
            word.vx += word.ax * deltaTime;
            word.vy += word.ay * deltaTime;

            // Apply drag, adjusting for deltaTime (e.g., 60.0f for a typical 60 FPS update base)
            word.vx *= std::pow(DRAG_FACTOR, deltaTime * 60.0f);
            word.vy *= std::pow(DRAG_FACTOR, deltaTime * 60.0f);

            // Update position
            word.rect.x += word.vx * deltaTime;
            word.rect.y += word.vy * deltaTime;

            // Bounce logic
            if (word.ay > 0) { // Falling (positive acceleration due to gravity)
                if (word.rect.y + word.rect.h > BOTTOM_COLLISION_Y) {
                    word.rect.y = BOTTOM_COLLISION_Y - word.rect.h; // Clamp to boundary
                    word.vy *= -BOUNCE_FACTOR; // Reverse velocity and damp
                    if (std::fabs(word.vy) < MIN_VELOCITY_DEACTIVATE) { // If velocity is very small after bounce, stop it
                        word.vy = 0;
                        word.ax = 0; // Stop horizontal movement if settling
                        word.ay = 0; // Stop gravity
                        word.active = false; // Deactivate if it has settled
                    }
                }
            } else if (word.ay < 0) { // Floating (negative acceleration due to anti-gravity)
                if (word.rect.y < TOP_COLLISION_Y) {
                    word.rect.y = TOP_COLLISION_Y; // Clamp to boundary
                    word.vy *= -BOUNCE_FACTOR; // Reverse velocity and damp
                    if (std::fabs(word.vy) < MIN_VELOCITY_DEACTIVATE) { // If velocity is very small after bounce, stop it
                        word.vy = 0;
                        word.ax = 0; // Stop horizontal movement if settling
                        word.ay = 0; // Stop anti-gravity
                        word.active = false; // Deactivate if it has settled
                    }
                }
            } else { // No active acceleration (e.g., after initial pop, or a settled bounce with no gravity/anti-gravity)
                if (std::fabs(word.vx) < MIN_VELOCITY_DEACTIVATE && std::fabs(word.vy) < MIN_VELOCITY_DEACTIVATE) {
                    word.active = false; // Deactivate if nearly stopped
                }
            }
        }
    }
}


// --- Text Color Pulse Effect Implementations ---
// Define global state variables (declared extern in text_effects.h)
Uint64 pulseStartTime = 0;
Uint64 pulseDuration = 0;
float pulseFrequencyHz = 2.0f; // Default frequency (e.g., 2 cycles per second)
SDL_Color pulseColorStart = {255, 255, 255, 255}; // Default white
SDL_Color pulseColorEnd = {255, 100, 100, 255};   // Default reddish
bool pulseActive = false;

void initTextColorPulse(Uint64 durationMs, float frequencyHz, SDL_Color c1, SDL_Color c2) {
    pulseStartTime = SDL_GetTicks();
    pulseDuration = durationMs;
    pulseFrequencyHz = frequencyHz;
    pulseColorStart = c1;
    pulseColorEnd = c2;
    pulseActive = true;
    getPulsingTextColor(pulseStartTime);
}

SDL_Color getPulsingTextColor(Uint64 currentTicks) {
    if (!pulseActive) {
        return pulseColorStart; // Return the start color if not active
    }

    Uint64 elapsed = currentTicks - pulseStartTime;
    // Check if pulse has ended
    if (pulseDuration > 0 && elapsed >= pulseDuration) {
        pulseActive = false; // Deactivate
        // Return the end color or start color here, based on desired final state after pulse completes.
        // Returning pulseColorStart is common if it's meant to revert.
        return pulseColorStart;
    }

    float timeInSeconds = (float)elapsed / 1000.0f; // Corrected: use 1000.0f for seconds

    // Corrected phase calculation for continuous pulsing starting at 0.0f
    // sin(x - PI/2) starts at -1 when x=0. Adding 1 and dividing by 2 maps to 0..1.
    float phase = (std::sin(timeInSeconds * pulseFrequencyHz * 2 * M_PI - M_PI / 2.0f) + 1.0f) / 2.0f;

    SDL_Color interpolatedColor;
    // Linear interpolation: color = color1 * (1-phase) + color2 * phase
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
    pulseFrequencyHz = 2.0f; // Reset to default frequency
    // Colors are not reset here, they'll be set again by initTextColorPulse
}
