// StoryManager.h
#pragma once

#include <string>
#include <vector>
#include <SDL3/SDL.h>       // For SDL_Texture, SDL_FRect, SDL_Event, Uint64, SDL_Color
#include <SDL3_ttf/SDL_ttf.h> // For TTF_Font, TTF_TextEngine, TTF_GetStringSize, TTF_RenderText_Blended_Wrapped

// Include headers for effects and UI functions, as StoryManager will call them
#include "text_effects.h"   // For RenderedWord, text effect declarations
#include "visual_effects.h" // For screen effect declarations
#include "text_ui.h"        // For UI rendering function declarations (drawDialogBoxUI, renderNameBox, renderText)


// --- Data Structures (Moved from main.cpp to be owned by StoryManager) ---

// Choice struct
struct Choice {
    std::string text;
    int nextDialogIndex;
    std::string nextFile;
    SDL_FRect rect;             // Stores the clickable area for the choice box
    SDL_Texture* textTexture;   // Pre-rendered texture for the choice text
    int textWidth;              // Width of the pre-rendered text
    int textHeight;             // Height of the pre-rendered text

    Choice() : textTexture(nullptr), textWidth(0), textHeight(0) {} // Initialize members
};

// DialogLine struct
struct DialogLine {
    std::string speakerName;
    std::string dialogText;
    std::vector<Choice> choices;
    bool hasChoices;

    bool applyJitter;

    bool applyFall;
    bool applyFloat;

    bool applyPulse;
    Uint64 pulseDurationMs;
    float pulseFrequencyHz;
    SDL_Color pulseColor1;
    SDL_Color pulseColor2;

    bool applyShake;
    Uint64 shakeDuration;
    float shakeIntensity;

    bool applyTear;
    Uint32 tearDuration;
    float tearMaxOffsetX;
    float tearLineDensity;

    std::vector<RenderedWord> physicsWords;
    std::vector<RenderedWord> jitterWords;
    bool physicsActive;

    DialogLine() : hasChoices(false), applyJitter(false),
                   applyFall(false), applyFloat(false),
                   applyPulse(false), pulseDurationMs(0), pulseFrequencyHz(0.0f),
                   applyShake(false), shakeDuration(0), shakeIntensity(0.0f),
                   applyTear(false), tearDuration(0), tearMaxOffsetX(0.0f), tearLineDensity(0.0f),
                   physicsActive(false) {}
};


// --- StoryManager Class ---
class StoryManager {
public:
    // Constructor: Takes pointers to global SDL resources
    StoryManager(SDL_Renderer* renderer, TTF_Font* dialogFont, TTF_Font* nameFont, TTF_TextEngine* textEngine);

    // Destructor: Responsible for cleaning up loaded story resources
    ~StoryManager();

    // Loads a story from a text file, parsing lines and choices
    bool loadStory(const std::string& filename);

    // Updates the state of the story (typing animation, physics, effects)
    void update(Uint64 currentTicks, float deltaTime);

    // Handles user input for advancing dialogue and making choices
    void handleInput(const SDL_Event& event);

    // Renders the current dialogue line, name box, and choices
    void render(Uint64 currentTicks);

    // handleWindowResize method in StoryManager
    void handleWindowResize(int newWidth, int newHeight);

private:
    // SDL resources used for rendering
    SDL_Renderer* gRenderer;
    TTF_Font* gDialogFont;
    TTF_Font* gNameFont;
    TTF_TextEngine* gTextEngine;

    // Story state variables
    std::vector<DialogLine> dialogLines;
    size_t currentDialogIndex;
    size_t currentVisibleCharCount;
    float animationDelayMs;
    Uint64 lastCharRevealTime;
    bool animationIsPlaying;
    bool awaitingChoice;
    std::string currentStoryFile;
    size_t prevDialogIndex; // Used for one-time effect triggering on line change

    // Private helper methods
    void advanceStoryLine();
    void activateLineEffects();
    void deactivateActiveEffects(); // Deactivates effects from the *previous* line
    void handleChoiceClick(SDL_FPoint mouseClick);
    void clearAllStoryResources(); // Cleans up all loaded dialog lines (especially choice textures)
};