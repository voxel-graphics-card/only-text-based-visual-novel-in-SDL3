// main.cpp - The main application entry point, now simplified with StoryManager
#include <iostream>         // For console output (std::cout, std::cerr)
#include <string>           // For std::string
#include <vector>           // For std::vector
#include <algorithm>        // For std::min, std::max
#include <sstream>          // For std::istringstream
#include <SDL3/SDL.h>       // For core SDL functions
#include <SDL3/SDL_rect.h>  // For SDL_FRect
#include <SDL3_ttf/SDL_ttf.h> // For TTF_Init, TTF_OpenFont, TTF_CloseFont, TTF_TextEngine, etc.
#include <cstdlib>          // For srand, rand
#include <cmath>            // For std::fabs
#include <ctime>            // For time (to seed rand)

// --- Include your custom modules ---
#include "StoryManager.h"   // The new story management class
#include "visual_effects.h" // For global screen effect update functions
#include "text_ui.h"        // For global constants and common UI functions (if still needed directly)
                            // Note: renderText, drawDialogBoxUI, renderNameBox are now used by StoryManager,
                            // but constants like winWidth, winHeight, textPadding are still here.

// --- Global Constants (Defined ONCE here, declared extern in text_ui.h) ---
int winWidth = 500; // Window width
int winHeight = 500; // Window height
const char* fontstr = "OpenSans-Regular.ttf"; // Make sure you have this font file

// --- Common UI parameters (defined here as they are used by main) ---
const int textPadding = 20; // Padding inside dialog box for text


// --- Global SDL Variables (managed by initSDL/closeSDL) ---
// These are still global, but StoryManager now takes pointers to them.
// main.cpp remains responsible for their lifecycle.
SDL_Window* gWindow = nullptr;
SDL_Renderer* gRenderer = nullptr;
TTF_TextEngine* gTextEngine = nullptr;
TTF_Font* gDialogFont = nullptr;
TTF_Font* gNameFont = nullptr;

// --- Colors (Constants - still global for simplicity, could be moved to a Theme struct/class) ---
SDL_Color textColorWhite = {255, 255, 255, 255};
SDL_Color dialogBoxBgColor = {50, 50, 50, 200}; // Dark grey, semi-transparent
SDL_Color nameBoxBgColor = {100, 100, 100, 220}; // Lighter grey, semi-transparent
SDL_Color borderColor = {200, 200, 200, 255};   // Light grey, opaque
SDL_Color choiceBgColor = {30, 30, 80, 200};     // Dark blue, semi-transparent
SDL_Color choiceBorderColor = {150, 150, 255, 255}; // Lighter blue, opaque


// --- Function Declarations ---
bool initSDL();
void closeSDL();


// --- Main Application Entry Point ---
int main(int argc, char* argv[]) {
    // Seed the random number generator ONCE at the start of the program
    srand((unsigned int)time(NULL));

    // 1. Initialize SDL and its extensions
    if (!initSDL()) {
        std::cerr << "Failed to initialize SDL or TTF!" << std::endl;
        return 1;
    }

    // 2. Create the StoryManager instance
    // Pass the globally initialized SDL pointers to the StoryManager
    StoryManager storyManager(gRenderer, gDialogFont, gNameFont, gTextEngine);

    // 3. Load the initial story file
    if (!storyManager.loadStory("story_test_effects.txt")) {
        std::cerr << "Failed to load initial story file. Exiting." << std::endl;
        closeSDL();
        return 1;
    }

    Uint64 lastFrameTime = SDL_GetTicks();
    bool running = true;
    SDL_Event event;

    // --- Main Game Loop ---
    while (running) {
        Uint64 currentTicks = SDL_GetTicks();
        float deltaTime = (currentTicks - lastFrameTime) / 1000.0f; // Delta time in seconds
        lastFrameTime = currentTicks;

        // Cap delta time to prevent large jumps on lag, improving physics stability
        if (deltaTime > 0.05f) deltaTime = 0.05f;

        // --- Event Handling ---
        while (SDL_PollEvent(&event)) {
            if (event.type == SDL_EVENT_QUIT) {
                running = false; // User requested quit
            }
            if (event.type == SDL_EVENT_WINDOW_RESIZED) {
                int newW, newH;
                SDL_GetWindowSize(gWindow, &newW, &newH);
                winWidth = newW; // Update global width
                winHeight = newH; // Update global height
                // Inform StoryManager about the resize so it can re-render textures etc.
                storyManager.handleWindowResize(newW, newH);
            }
            // Delegate input handling to StoryManager
            storyManager.handleInput(event);
        }

        // --- Update Game State ---
        // Update global screen-wide effects first
        updateScreenShake(currentTicks);
        updateScreenTear(currentTicks);

        // Delegate story update logic to StoryManager
        storyManager.update(currentTicks, deltaTime);

        // --- Rendering ---
        // Clear the screen
        SDL_SetRenderDrawColor(gRenderer, 0x20, 0x20, 0x20, 0xFF); // Dark background
        SDL_RenderClear(gRenderer);

        // Delegate rendering of story elements to StoryManager
        storyManager.render(currentTicks);

        // Present the rendered frame to the screen
        SDL_RenderPresent(gRenderer);
    }

    // 4. Clean up SDL resources when the game loop ends
    closeSDL();

    return 0;
}


// --- SDL Initialization ---
// This function remains in main.cpp as it sets up global SDL resources.
bool initSDL() {
    // Initialize SDL video subsystem
    if (SDL_Init(SDL_INIT_VIDEO) < 0) {
        std::cerr << "SDL could not initialize! SDL_Error: " << SDL_GetError() << std::endl;
        return false;
    }

    // Create the main window
    gWindow = SDL_CreateWindow("Visual Novel Demo", winWidth, winHeight, SDL_WINDOW_RESIZABLE);
    if (gWindow == nullptr) {
        std::cerr << "Window could not be created! SDL_Error: " << SDL_GetError() << std::endl;
        return false;
    }

    // Create the renderer (accelerated with VSync for smooth animation)
    gRenderer = SDL_CreateRenderer(gWindow, nullptr);
    if (gRenderer == nullptr) {
        std::cerr << "Renderer could not be created! SDL_Error: " << SDL_GetError() << std::endl;
        return false;
    }

    // Enable blend mode for semi-transparent UI elements
    SDL_SetRenderDrawBlendMode(gRenderer, SDL_BLENDMODE_BLEND);

    // Initialize SDL_ttf for font rendering
    if (TTF_Init() == -1) {
        std::cerr << "SDL_ttf could not initialize! TTF_Error: " << SDL_GetError() << std::endl;
        return false;
    }

    // Create a TTF_TextEngine (required for TTF_RenderText_Blended_Wrapped in SDL3_ttf)
    gTextEngine = TTF_CreateRendererTextEngine(gRenderer);
    if (gTextEngine == nullptr) {
        std::cerr << "Failed to create TTF_TextEngine: " << SDL_GetError() << std::endl;
        return false;
    }

    // Load fonts (fontstr defined globally in main.cpp)
    gDialogFont = TTF_OpenFont(fontstr, 24); // Main dialog font size
    if (gDialogFont == nullptr) {
        std::cerr << "Failed to load dialog font! TTF_Error: " << SDL_GetError() << std::endl;
        return false;
    }

    gNameFont = TTF_OpenFont("Nasa21.ttf", 20); // Name font size
    if (gNameFont == nullptr) {
        std::cerr << "Failed to load name font! TTF_Error: " << SDL_GetError() << std::endl;
        return false;
    }

    return true; // All initializations successful
}

// --- SDL Teardown ---
// This function also remains in main.cpp to destroy global SDL resources.
void closeSDL() {
    // Destroy fonts
    if (gDialogFont) TTF_CloseFont(gDialogFont);
    if (gNameFont) TTF_CloseFont(gNameFont);

    // Destroy TTF TextEngine
    if (gTextEngine) TTF_DestroyRendererTextEngine(gTextEngine);

    // Destroy renderer and window
    if (gRenderer) SDL_DestroyRenderer(gRenderer);
    if (gWindow) SDL_DestroyWindow(gWindow);

    // Quit SDL_ttf and SDL subsystems
    TTF_Quit();
    SDL_Quit();

    // Note: StoryManager's destructor will handle its internal resource cleanup (e.g., choice textures)
}
