// text_effects.h - Header for text-specific effects like jitter, word physics, and color pulse
#pragma once
#include <SDL3/SDL.h>
#include <SDL3_ttf/SDL_ttf.h> // Included for TTF_Font and TTF_TextEngine types, and TTF_GetStringSize
#include <string>
#include <vector>

// Removed <random> and <chrono> as rand() is now used and seeding is in main.cpp

// --- RenderedWord Struct ---
// Represents a single word rendered with its position and physics properties
struct RenderedWord {
    std::string text;
    SDL_FRect rect; // Position and size of the word
    float vx, vy;   // Velocity components
    float ax, ay;   // Acceleration components (e.g., gravity)
    bool active;    // True if word is still actively participating in physics (e.g., falling/floating)
};

// Note: DialogLine struct and Choice struct are defined in main.cpp,
// so they are not declared here to avoid circular dependencies or redefinitions
// if this header were included in files that don't need them.


// --- Jitter Effect ---
// Initializes words for the jitter effect, calculating their initial positions.
// Renderer and font are needed for text measurement.
std::vector<RenderedWord> initJitterWords(SDL_Renderer* renderer, TTF_Font* font, const std::string& text, int x, int y, int wrapWidth);

// Applies a random jitter offset to the positions of the words.
void applyJitter(std::vector<RenderedWord>& words);

// --- Word Physics (Fall/Float) ---
// Initializes words for physics simulation, calculating their initial positions.
// Renderer and font are needed for text measurement.
std::vector<RenderedWord> initPhysicsWords(SDL_Renderer* renderer, TTF_Font* font, const std::string& text, int x, int y, int wrapWidth);

// Applies an initial "pop" force to words, useful for starting fall/float effects.
void applyfallEffect(std::vector<RenderedWord>& words);

// Applies an float effect
void applyfloatEffect(std::vector<RenderedWord>& words);

// Updates the position and velocity of words based on physics (gravity/anti-gravity, drag).
void updatePhysicsWords(std::vector<RenderedWord>& words, float deltaTime);

// --- Text Color Pulse Effect ---
// Initializes the text color pulsing effect with a duration and two colors to interpolate between.
void initTextColorPulse(Uint64 durationMs, float frequencyHz, SDL_Color startColor, SDL_Color endColor);

// Gets the current interpolated color for the pulsing text based on elapsed time.
SDL_Color getPulsingTextColor(Uint64 currentTicks);

// Checks if the text color pulse effect is currently active.
bool isTextColorPulseActive();

// Deactivates the text color pulse effect.
void deactivateTextColorPulse();