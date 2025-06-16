// text_ui.cpp
#include "text_ui.h"
#include <iostream>

// These are definitions for extern constants if they are not defined in main.cpp
// However, since we define them in main.cpp, we don't need these here.
// If you uncommented these, it would cause redefinition errors.
// const int winWidth = 1280;
// const int winHeight = 720;
// const int textPadding = 20;

void renderText(SDL_Renderer* renderer, TTF_Font* font, const std::string& text, SDL_Color color, int x, int y, int wrapWidth) {
    if (!font || text.empty()) {
        return;
    }

    // SDL3 TTF rendering
    // Create an SDL_Surface for the text
    SDL_Surface* textSurface = TTF_RenderText_Blended_Wrapped(font, text.c_str(), text.size(), color, wrapWidth);
    if (!textSurface) {
        std::cerr << "Unable to render text surface! TTF_Error: " << SDL_GetError() << std::endl;
        return;
    }

    // Create texture from surface pixels
    SDL_Texture* textTexture = SDL_CreateTextureFromSurface(renderer, textSurface);
    if (!textTexture) {
        std::cerr << "Unable to create texture from rendered text! SDL_Error: " << SDL_GetError() << std::endl;
        SDL_DestroySurface(textSurface);
        return;
    }

    // Set rendering space and render to screen
    SDL_FRect renderQuad = {(float)x, (float)y, (float)textSurface->w, (float)textSurface->h};
    SDL_RenderTexture(renderer, textTexture, nullptr, &renderQuad);

    // Get rid of old surface and texture
    SDL_DestroyTexture(textTexture);
    SDL_DestroySurface(textSurface);
}

void drawDialogBoxUI(SDL_Renderer* renderer, float x, float y, float w, float h, SDL_Color bgColor, SDL_Color borderColor) {
    // Fill background
    SDL_FRect bgRect = {x, y, w, h};
    SDL_SetRenderDrawColor(renderer, bgColor.r, bgColor.g, bgColor.b, bgColor.a);
    SDL_RenderFillRect(renderer, &bgRect);

    // Draw border
    SDL_SetRenderDrawColor(renderer, borderColor.r, borderColor.g, borderColor.b, borderColor.a);
    SDL_RenderRect(renderer, &bgRect);
}

void renderNameBox(SDL_Renderer* renderer, TTF_Font* font, TTF_TextEngine* textEngine, const std::string& name, float x, float y, float w, float h, SDL_Color bgColor, SDL_Color borderColor, SDL_Color textColor) {
    if (name.empty()) {
        return;
    }

    // Draw background
    SDL_FRect bgRect = {x, y, w, h};
    SDL_SetRenderDrawColor(renderer, bgColor.r, bgColor.g, bgColor.b, bgColor.a);
    SDL_RenderFillRect(renderer, &bgRect);

    // Draw border
    SDL_SetRenderDrawColor(renderer, borderColor.r, borderColor.g, borderColor.b, borderColor.a);
    SDL_RenderRect(renderer, &bgRect);

    // Render name text centered within the box
    int textW, textH;
    // Changed TTF_SizeUTF8 to TTF_GetStringSize
    TTF_GetStringSize(font, name.c_str(), name.length(), &textW, &textH);

    // Calculate position to center the text
    int textX = (int)(x + (w - textW) / 2);
    int textY = (int)(y + (h - textH) / 2);

    renderText(renderer, font, name, textColor, textX, textY);
}