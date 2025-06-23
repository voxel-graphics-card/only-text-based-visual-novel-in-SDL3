// text_ui.h - Header for UI rendering functions and global constants
#pragma once
#include <SDL3/SDL.h>       // Included for SDL_Color
#include <SDL3_ttf/SDL_ttf.h> // Included for TTF_Font and TTF_TextEngine types, and TTF_GetStringSize
#include <string>           // For std::string
#include <vector>           // For std::vector (though not directly used in this specific header, good practice)

// --- Global Constants (Declared extern, defined once in main.cpp) ---
extern int winWidth;      // Window width
extern int winHeight;     // Window height
extern const char* fontstr;     // Font file path
extern const int textPadding;   // Padding inside dialog box for text

// Assuming color constants are also defined globally in main.cpp
extern SDL_Color textColorWhite;
extern SDL_Color dialogBoxBgColor;
extern SDL_Color nameBoxBgColor;
extern SDL_Color borderColor;
extern SDL_Color choiceBgColor;
extern SDL_Color choiceBorderColor;


// --- Function Declarations (These are implemented in text_ui.cpp) ---

// Renders generic text to the renderer.
// text: The string to render.
// color: The color of the text.
// x, y: Top-left coordinates for the text.
// wrapWidth: Maximum width for text wrapping. 0 means no wrapping.
void renderText(SDL_Renderer* renderer, TTF_Font* font, const std::string& text, SDL_Color color, int x, int y, int wrapWidth = 0);

// Draws a generic dialog box background and border.
// x, y, w, h: Position and dimensions of the box.
// bgColor: Background color of the box.
// borderColor: Color of the box's border.
void drawDialogBoxUI(SDL_Renderer* renderer, float x, float y, float w, float h, SDL_Color bgColor, SDL_Color borderColor);

// Renders a name tag box with centered text.
// name: The speaker's name.
// x, y, w, h: Position and dimensions of the name box.
// bgColor: Background color of the name box.
// borderColor: Color of the name box's border.
// textColor: Color of the name text.
void renderNameBox(SDL_Renderer* renderer, TTF_Font* font, TTF_TextEngine* textEngine, const std::string& name, float x, float y, float w, float h, SDL_Color bgColor, SDL_Color borderColor, SDL_Color textColor);
