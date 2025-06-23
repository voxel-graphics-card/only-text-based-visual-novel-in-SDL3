// text_effects.h - Header for text-specific effects like jitter, word physics, and color pulse
#pragma once
#include <SDL3/SDL.h>       // Included for SDL_FRect, SDL_Color, Uint64
#include <SDL3_ttf/SDL_ttf.h> // Included for TTF_Font and TTF_TextEngine types, and TTF_GetStringSize
#include <string>           // For std::string
#include <vector>           // For std::vector

// --- RenderedWord Struct ---
// Represents a single word rendered with its position and physics properties
struct RenderedWord {
    std::string text;
    SDL_FRect rect;         // Current position and size (where it's rendered)
    SDL_FRect originalRect; // Store the word's original, static position and size
    float vx, vy;           // Velocity components
    float ax, ay;           // Acceleration components (e.g., gravity)
    bool active;            // True if word is still actively participating in physics (e.g., falling/floating)
};


// --- Jitter Effect ---
// Initializes words for the jitter effect, calculating their initial positions.
// Renderer and font are needed for text measurement.
std::vector<RenderedWord> initJitterWords(SDL_Renderer* renderer, TTF_Font* font, const std::string& text, int x, int y, int wrapWidth);

// Applies a random jitter offset to the positions of the words, relative to their original positions.
void applyJitter(std::vector<RenderedWord>& words);


// --- Word Physics (Fall/Float) ---
// Initializes words for physics simulation, calculating their initial positions.
// Renderer and font are needed for text measurement.
std::vector<RenderedWord> initPhysicsWords(SDL_Renderer* renderer, TTF_Font* font, const std::string& text, int x, int y, int wrapWidth);

// Applies an initial "pop" force for a falling effect.
void applyfallEffect(std::vector<RenderedWord>& words);

// Applies an initial "pop" force for a floating effect.
void applyfloatEffect(std::vector<RenderedWord>& words);

// Updates the position and velocity of words based on physics (gravity/anti-gravity, drag).
void updatePhysicsWords(std::vector<RenderedWord>& words, float deltaTime);


// --- Text Color Pulse Effect ---
// These are global state variables for the pulse effect, defined in text_effects.cpp
extern Uint64 pulseStartTime;
extern Uint64 pulseDuration;
extern float pulseFrequencyHz;
extern SDL_Color pulseColorStart;
extern SDL_Color pulseColorEnd;
extern bool pulseActive;

// Initializes the text color pulsing effect with a duration, frequency, and two colors to interpolate between.
void initTextColorPulse(Uint64 durationMs, float frequencyHz, SDL_Color startColor, SDL_Color endColor);

// Gets the current interpolated color for the pulsing text based on elapsed time.
SDL_Color getPulsingTextColor(Uint64 currentTicks);

// Checks if the text color pulse effect is currently active.
bool isTextColorPulseActive();

// Deactivates the text color pulse effect and resets its state.
void deactivateTextColorPulse();
