// text_ui.h
#pragma once
#include <SDL3/SDL.h>
#include <SDL3_ttf/SDL_ttf.h> // Ensure this path is correct for your SDL3_ttf installation
#include <string>
#include <vector>

// --- Global Constants (Declared extern, defined once in main.cpp) ---
const int winWidth = 1000; // Window width - DEFINED HERE
const int winHeight = 600; // Window height - DEFINED HERE
extern const char* fontstr;

// --- Common UI parameters (Declared extern, defined once in main.cpp or text_ui.cpp) ---
// Declaring extern here if it's defined in main.cpp or another .cpp file
extern const int textPadding;

// --- Forward Declaration of RenderedWord for DialogLine if needed (but DialogLine is in main.cpp) ---
// As DialogLine and Choice are now defined in main.cpp, they don't strictly need forward declarations here.
// However, RenderedWord is still used by DialogLine, so it's defined in text_effects.h

// --- Function Declarations ---
// Renders text to the renderer. wrapWidth=0 means no wrapping.
void renderText(SDL_Renderer* renderer, TTF_Font* font, const std::string& text, SDL_Color color, int x, int y, int wrapWidth = 0);

// Draws a generic dialog box background and border.
void drawDialogBoxUI(SDL_Renderer* renderer, float x, float y, float w, float h, SDL_Color bgColor, SDL_Color borderColor);

// Renders a name tag box with centered text.
void renderNameBox(SDL_Renderer* renderer, TTF_Font* font, TTF_TextEngine* textEngine, const std::string& name, float x, float y, float w, float h, SDL_Color bgColor, SDL_Color borderColor, SDL_Color textColor);