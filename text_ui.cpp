// text_ui.cpp - Implementation for UI rendering functions
#include "text_ui.h" // Include the corresponding header
#include <iostream>  // For std::cerr output

// --- Global Constants (Declared extern in text_ui.h, defined in main.cpp) ---
// These are included from text_ui.h, so they are available here.
// extern const int winWidth;
// extern const int winHeight;
// extern const int textPadding;
// extern SDL_Color textColorWhite; // And other colors


// --- Function Definitions ---

void renderText(SDL_Renderer* renderer, TTF_Font* font, const std::string& text, SDL_Color color, int x, int y, int wrapWidth) {
    if (!font || text.empty() || !renderer) {
        return;
    }

    // SDL3 TTF rendering
    // Create an SDL_Surface for the text (length is required in SDL3 TTF_RenderText_Blended_Wrapped)
    SDL_Surface* textSurface = TTF_RenderText_Blended_Wrapped(font, text.c_str(), text.length(), color, wrapWidth);
    if (!textSurface) {
        std::cerr << "Unable to render text surface! SDL_Error: " << SDL_GetError() << std::endl;
        return;
    }

    // Create texture from surface pixels
    SDL_Texture* textTexture = SDL_CreateTextureFromSurface(renderer, textSurface);
    if (!textTexture) {
        std::cerr << "Unable to create texture from rendered text! SDL_Error: " << SDL_GetError() << std::endl;
        SDL_DestroySurface(textSurface); // Always destroy surface even if texture creation fails
        return;
    }

    // Set up the destination rectangle for rendering the texture
    SDL_FRect dstRect = {(float)x, (float)y, (float)textSurface->w, (float)textSurface->h};

    // Render texture to screen
    SDL_RenderTexture(renderer, textTexture, nullptr, &dstRect);

    // Free the surface and texture as they are no longer needed after rendering
    SDL_DestroyTexture(textTexture);
    SDL_DestroySurface(textSurface);
}

void drawDialogBoxUI(SDL_Renderer* renderer, float x, float y, float w, float h, SDL_Color bgColor, SDL_Color borderColor) {
    if (!renderer) {
        return;
    }

    SDL_FRect bgRect = {x, y, w, h};

    // Fill background
    SDL_SetRenderDrawColor(renderer, bgColor.r, bgColor.g, bgColor.b, bgColor.a);
    SDL_RenderFillRect(renderer, &bgRect);

    // Draw border
    SDL_SetRenderDrawColor(renderer, borderColor.r, borderColor.g, borderColor.b, borderColor.a);
    SDL_RenderRect(renderer, &bgRect);
}

void renderNameBox(SDL_Renderer* renderer, TTF_Font* font, TTF_TextEngine* textEngine, const std::string& name, float x, float y, float w, float h, SDL_Color bgColor, SDL_Color borderColor, SDL_Color textColor) {
    if (!renderer || !font || name.empty() || !textEngine) {
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
    // Use TTF_GetStringSize for text dimensions in SDL3_ttf
    TTF_GetStringSize(font, name.c_str(), name.length(), &textW, &textH);

    // Ensure text does not exceed box width
    int renderWidth = std::min(textW, (int)w); // Prevent text from rendering outside box if too wide

    // Create surface for the name text
    // TTF_RenderText_Blended is suitable for single-line text like names
    SDL_Surface* textSurface = TTF_RenderText_Blended(font, name.c_str(), name.length(), textColor);
    if (!textSurface) {
        std::cerr << "Unable to render name text surface! SDL_Error: " << SDL_GetError() << std::endl;
        return;
    }

    // Create texture from surface
    SDL_Texture* textTexture = SDL_CreateTextureFromSurface(renderer, textSurface);
    if (!textTexture) {
        std::cerr << "Unable to create texture from rendered name! SDL_Error: " << SDL_GetError() << std::endl;
        SDL_DestroySurface(textSurface);
        return;
    }

    // Calculate position to center text within the name box
    SDL_FRect dstRect = {
        x + (w - textW) / 2.0f, // Center horizontally
        y + (h - textH) / 2.0f, // Center vertically
        (float)textW, (float)textH
    };

    SDL_RenderTexture(renderer, textTexture, nullptr, &dstRect);

    // Free resources
    SDL_DestroyTexture(textTexture);
    SDL_DestroySurface(textSurface);
}
