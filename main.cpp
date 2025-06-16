// main.cpp - FINALIZED VERSION WITH ALL EFFECTS AND PARSING LOGIC
#include "text_ui.h"        // For UI rendering functions (dialog box, name box, generic text)
#include "visual_effects.h" // For screen-wide visual effects (shake)
#include "text_effects.h"   // For text-specific effects (jitter, word physics, color pulse)
#include <iostream>         // For console output (std::cout, std::cerr)
#include <fstream>          // For file input (std::ifstream)
#include <string>           // For std::string
#include <vector>           // For std::vector
#include <algorithm>        // For std::min, std::max
#include <sstream>          // For std::istringstream (to parse numbers from string)
#include <SDL3/SDL_rect.h>  // For SDL_FRect, SDL_PointInRectFloat
#include <SDL3_ttf/SDL_ttf.h>   // For TTF_GetStringSize, TTF_GetError (needed here)
#include <cstdlib>          // For srand, rand
#include <cmath>            // For std::fabs (floating-point absolute value)
#include <ctime>            // For time (to seed rand)

// --- Global Constants (Defined ONCE here, declared extern in text_ui.h) ---
const char* fontstr = "OpenSans-Regular.ttf"; // Make sure you have this font file

// --- Common UI parameters (defined here as they are used by main) ---
const int textPadding = 20; // Padding inside dialog box for text


// --- Data Structures ---

// Choice struct (MUST be defined BEFORE DialogLine if DialogLine uses it)
struct Choice {
    std::string text;
    int nextDialogIndex;
    std::string nextFile;
    SDL_FRect rect; // Stores the clickable area for the choice
};

// DialogLine struct to hold all data and flags for a single line of dialogue
struct DialogLine {
    std::string speakerName;
    std::string dialogText;
    std::vector<Choice> choices;
    bool hasChoices;
    bool applyJitter;
    bool applyFall;
    bool applyFloat;
    bool applyPulse;
    float pulseFrequencyHz;
    Uint64 pulseDurationMs;
    SDL_Color pulseColor1;
    SDL_Color pulseColor2;
    bool applyShake;
    Uint64 shakeDuration;
    float shakeIntensity;
    std::vector<RenderedWord> physicsWords; // For physics effect
    std::vector<RenderedWord> jitterWords;  // For jitter effect
    bool physicsActive; // True when physics simulation is currently running for this line
};


// --- Global SDL Variables (managed by initSDL/closeSDL) ---
SDL_Window* gWindow = nullptr;
SDL_Renderer* gRenderer = nullptr;
TTF_TextEngine* gTextEngine = nullptr; // Global text engine
TTF_Font* gDialogFont = nullptr;
TTF_Font* gNameFont = nullptr;

// --- Colors (Constants) ---
SDL_Color textColorWhite = {255, 255, 255, 255};
SDL_Color dialogBoxBgColor = {50, 50, 50, 200}; // Dark grey, semi-transparent
SDL_Color nameBoxBgColor = {100, 100, 100, 220}; // Lighter grey, semi-transparent
SDL_Color borderColor = {200, 200, 200, 255};   // Light grey, opaque
SDL_Color choiceBgColor = {30, 30, 80, 200};     // Dark blue, semi-transparent
SDL_Color choiceBorderColor = {150, 150, 255, 255}; // Lighter blue, opaque


// --- Dialog State Variables ---
std::vector<DialogLine> dialogLines;
size_t currentDialogIndex = 0;
size_t currentVisibleCharCount = 0;
float animationDelayMs = 40.0f; // Delay per character (milliseconds)
Uint64 lastCharRevealTime = SDL_GetTicks();
bool animationIsPlaying = true;
bool awaitingChoice = false;

// --- Function Declarations ---
bool initSDL();
void closeSDL();
std::vector<DialogLine> loadDialogFromFile(const std::string& filename);


// --- Main Game Loop ---
int main(int argc, char* argv[]) {
    // Seed the random number generator once at the start
    srand((unsigned int)time(NULL));

    if (!initSDL()) {
        std::cerr << "Failed to initialize SDL or TTF!" << std::endl;
        return 1;
    }

    // --- Story Loading ---
    std::string currentStoryFile = "story_test_effects.txt"; // Your test story file
    dialogLines = loadDialogFromFile(currentStoryFile);
    if (dialogLines.empty()) {
        std::cerr << "Initial story file '" << currentStoryFile << "' loaded empty or failed. Exiting." << std::endl;
        closeSDL();
        return 1;
    }

    Uint64 lastFrameTime = SDL_GetTicks();
    bool running = true;
    SDL_Event event;

    size_t prevDialogIndex = 0; // Track previous index for one-time effect triggers

    // --- Main Game Loop ---
    while (running) {
        Uint64 currentTicks = SDL_GetTicks();
        float deltaTime = (currentTicks - lastFrameTime) / 1000.0f; // Delta time in seconds
        lastFrameTime = currentTicks;

        if (deltaTime > 0.05f) deltaTime = 0.05f; // Cap delta time to prevent large jumps on lag

        // Update screen shake every frame
        updateScreenShake(currentTicks); // Function from visual_effects.h

        // --- Event Handling ---
        while (SDL_PollEvent(&event)) {
            if (event.type == SDL_EVENT_QUIT) {
                running = false;
            }
            if (event.type == SDL_EVENT_KEY_DOWN) {
                if (event.key.key == SDLK_SPACE || event.key.key == SDLK_RETURN) {
                    if (awaitingChoice) {
                        // Do nothing, awaiting mouse click for choice
                    } else if (animationIsPlaying) {
                        // Skip text animation to full display
                        currentVisibleCharCount = dialogLines[currentDialogIndex].dialogText.length();
                        animationIsPlaying = false;

                        // Immediately trigger physics or activate choices if animation skipped
                        DialogLine& currentLine = dialogLines[currentDialogIndex];
                        if (currentLine.hasChoices) {
                            awaitingChoice = true;
                        } else if (currentLine.applyFall || currentLine.applyFloat) {
                            currentLine.physicsActive = true;
                            // Call the correct effect based on the flag
                            if (currentLine.applyFall) {
                                applyfallEffect(currentLine.physicsWords);
                            } else if (currentLine.applyFloat) {
                                applyfloatEffect(currentLine.physicsWords);
                            }
                        }

                        // Trigger effects if animation was skipped (they would have triggered on index change otherwise)
                        if (currentLine.applyShake && !isScreenShakeActive()) {
                             initScreenShake(currentLine.shakeDuration, currentLine.shakeIntensity); // Function from visual_effects.h
                        }
                        if (currentLine.applyPulse && !isTextColorPulseActive()) {
                            initTextColorPulse(currentLine.pulseDurationMs, currentLine.pulseFrequencyHz, currentLine.pulseColor1, currentLine.pulseColor2); // Function from text_effects.h
                        }

                    } else { // Animation complete, advance line
                        // If physics is running, wait for it to finish or skip it
                        DialogLine& currentLine = dialogLines[currentDialogIndex];
                        // Check if *any* physics effect is applied and active for the current line
                        if ((currentLine.applyFall || currentLine.applyFloat) && currentLine.physicsActive) {
                            // If user presses space, skip physics and advance
                            currentLine.physicsActive = false; // Stop physics
                            currentLine.physicsWords.clear(); // Clear words (optional, but ensures they don't linger)
                        }

                        // Deactivate effects from the *previous* line before advancing
                        if (currentLine.applyPulse && isTextColorPulseActive()) {
                            deactivateTextColorPulse(); // Function from text_effects.h
                        }
                        // Screen shake naturally fades or is overwritten by new shake

                        currentDialogIndex++; // Advance to next line
                        if (currentDialogIndex < dialogLines.size()) {
                            // Reset state for new line
                            currentVisibleCharCount = 0;
                            animationIsPlaying = true;
                            lastCharRevealTime = currentTicks;
                            awaitingChoice = false;
                        } else {
                            // End of story or loop back
                            std::cout << "End of story. Looping to start." << std::endl;
                            currentDialogIndex = 0; // Loop back for demo
                            currentVisibleCharCount = 0;
                            animationIsPlaying = true;
                            lastCharRevealTime = currentTicks;
                            awaitingChoice = false;
                            deactivateTextColorPulse(); // Ensure pulse is off if looping
                            // No need to reset screen shake, init will handle it for first line
                        }
                    }
                }
            }
            if (event.type == SDL_EVENT_MOUSE_BUTTON_DOWN) {
                if (event.button.button == SDL_BUTTON_LEFT && awaitingChoice) {
                    SDL_FPoint mouseClick = {(float)event.button.x, (float)event.button.y};
                    DialogLine& currentLine = dialogLines[currentDialogIndex];

                    bool choiceMade = false;
                    for (size_t i = 0; i < currentLine.choices.size(); ++i) {
                        Choice& choice = currentLine.choices[i];
                        if (SDL_PointInRectFloat(&mouseClick, &choice.rect)) {
                            choiceMade = true;
                            // Deactivate pulse on choice
                            if (currentLine.applyPulse && isTextColorPulseActive()) {
                                deactivateTextColorPulse();
                            }

                            // Handle choice jump
                            if (!choice.nextFile.empty()) {
                                currentStoryFile = choice.nextFile;
                                dialogLines = loadDialogFromFile(currentStoryFile);
                                currentDialogIndex = choice.nextDialogIndex;
                                if (currentDialogIndex >= dialogLines.size()) { currentDialogIndex = 0; } // Fallback
                            } else {
                                currentDialogIndex = choice.nextDialogIndex;
                                if (currentDialogIndex >= dialogLines.size()) { currentDialogIndex = 0; } // Fallback
                            }

                            // Reset state for new dialog path
                            currentVisibleCharCount = 0;
                            animationIsPlaying = true;
                            awaitingChoice = false;
                            lastCharRevealTime = currentTicks;
                            break;
                        }
                    }
                }
            }
        }

        // --- Update Game State ---
        if (currentDialogIndex < dialogLines.size()) {
            DialogLine& currentLine = dialogLines[currentDialogIndex];

            // Trigger effects when moving to a new line (first time it's displayed)
            if (currentDialogIndex != prevDialogIndex) {
                // Deactivate previous line's pulse if it was active
                if (prevDialogIndex < dialogLines.size() && dialogLines[prevDialogIndex].applyPulse && isTextColorPulseActive()) {
                    deactivateTextColorPulse();
                }

                // Activate effects for the new line
                if (currentLine.applyShake) {
                    initScreenShake(currentLine.shakeDuration, currentLine.shakeIntensity);
                }
                if (currentLine.applyPulse) {
                    initTextColorPulse(currentLine.pulseDurationMs, currentLine.pulseFrequencyHz, currentLine.pulseColor1, currentLine.pulseColor2);
                }
                // Initialize jitter/physics words on line start if not already
                // Note: These calculations use default dialog box dimensions for pre-calculation.
                // Actual rendering offsets (like shake) are applied during draw.
                SDL_FRect tempDialogBoxForWordCalc = {
                    (float)(winWidth * 0.1f), (float)(winHeight * 0.7f),
                    (float)(winWidth * 0.8f), (float)(winHeight * 0.25f)
                };

                if (currentLine.applyJitter && currentLine.jitterWords.empty()) {
                    currentLine.jitterWords = initJitterWords(gRenderer, gDialogFont, currentLine.dialogText,
                                                               (int)(tempDialogBoxForWordCalc.x + textPadding), (int)(tempDialogBoxForWordCalc.y + textPadding),
                                                               (int)(tempDialogBoxForWordCalc.w - (2 * textPadding)));
                }
                 if ((currentLine.applyFall || currentLine.applyFloat) && currentLine.physicsWords.empty()) {
                    currentLine.physicsWords = initPhysicsWords(gRenderer, gDialogFont, currentLine.dialogText,
                                                               (int)(tempDialogBoxForWordCalc.x + textPadding), (int)(tempDialogBoxForWordCalc.y + textPadding),
                                                               (int)(tempDialogBoxForWordCalc.w - (2 * textPadding)));
                }
                prevDialogIndex = currentDialogIndex;
            }

            // Update character typing animation
            if (animationIsPlaying && !awaitingChoice) {
                currentVisibleCharCount = (currentTicks - lastCharRevealTime) / animationDelayMs;
                currentVisibleCharCount = std::min(currentVisibleCharCount, dialogLines[currentDialogIndex].dialogText.length());

                if (currentVisibleCharCount >= dialogLines[currentDialogIndex].dialogText.length()) {
                    animationIsPlaying = false;
                    // Trigger physics/choices here if text finished animating
                    if (currentLine.hasChoices) {
                        awaitingChoice = true;
                    } else if ((currentLine.applyFall || currentLine.applyFloat) && !currentLine.physicsActive) {
                        currentLine.physicsActive = true;
                        // Call the correct effect based on the flag
                        if (currentLine.applyFall) {
                            applyfallEffect(currentLine.physicsWords);
                        } else if (currentLine.applyFloat) {
                            applyfloatEffect(currentLine.physicsWords);
                        }
                    }
                }
            }

            // Update effect states that run continuously
            if (currentLine.applyJitter) {
                applyJitter(currentLine.jitterWords); // Jitter words constantly (from text_effects.h)
            }
            if ((currentLine.applyFall || currentLine.applyFloat) && currentLine.physicsActive) {
                updatePhysicsWords(currentLine.physicsWords, deltaTime); // From text_effects.h
                // Check if all physics words are inactive to stop physics for the line
                bool allWordsInactive = true;
                for(const auto& word : currentLine.physicsWords) {
                    if(word.active) {
                        allWordsInactive = false;
                        break;
                    }
                }
                if(allWordsInactive) {
                    currentLine.physicsActive = false;
                }
            }
        }

        // --- Rendering ---
        SDL_SetRenderDrawColor(gRenderer, 0x20, 0x20, 0x20, 0xFF); // Dark background
        SDL_RenderClear(gRenderer);

        SDL_FPoint shakeOffset = getScreenShakeOffset(); // Get current shake offset (from visual_effects.h)

        // Dialog box position (apply shake)
        // Adjust Y position to move it higher and leave more space for choices
        SDL_FRect dialogBoxRect = {
            (float)(winWidth * 0.1f + shakeOffset.x), (float)(winHeight * 0.55f + shakeOffset.y), // Moved up to 0.55f
            (float)(winWidth * 0.8f), (float)(winHeight * 0.25f)
        };

        // Name box position (apply shake)
        SDL_FRect nameBoxRect = {
            (float)(dialogBoxRect.x), (float)(dialogBoxRect.y - 40), // Adjusted to be relative to dialog box
            150.0f, 30.0f
        };

        // Draw dialog box background and border (from text_ui.h)
        drawDialogBoxUI(gRenderer, dialogBoxRect.x, dialogBoxRect.y, dialogBoxRect.w, dialogBoxRect.h, dialogBoxBgColor, borderColor);


        // Draw name box (from text_ui.h)
        DialogLine& currentLine = dialogLines[currentDialogIndex];
        renderNameBox(gRenderer, gNameFont, gTextEngine, currentLine.speakerName
            , nameBoxRect.x, nameBoxRect.y, nameBoxRect.w, nameBoxRect.h
            , nameBoxBgColor, nameBoxBgColor, textColorWhite);

        // Get current text color (applies pulse if active)
        SDL_Color currentTextColor = isTextColorPulseActive() ? getPulsingTextColor(currentTicks) : textColorWhite;

        // Render main dialog text based on active effects
        if (currentLine.applyJitter && !animationIsPlaying && !(currentLine.applyFall || currentLine.applyFloat)) { // Jitter active, full text, NOT physics
            for (const auto& word : currentLine.jitterWords) {
                renderText(gRenderer, gDialogFont, word.text, currentTextColor,
                           (int)(word.rect.x + shakeOffset.x), (int)(word.rect.y + shakeOffset.y), 0);
            }
        } else if ((currentLine.applyFall || currentLine.applyFloat) && currentLine.physicsActive) { // Physics active
            for (const auto& word : currentLine.physicsWords) {
                if (word.active) { // Only render active words
                    renderText(gRenderer, gDialogFont, word.text, currentTextColor,
                               (int)(word.rect.x + shakeOffset.x), (int)(word.rect.y + shakeOffset.y), 0);
                }
            }
        } else { // Normal typing or static display
            std::string textToDisplay = currentLine.dialogText.substr(0, currentVisibleCharCount);
            renderText(gRenderer, gDialogFont, textToDisplay, currentTextColor,
                       (int)(dialogBoxRect.x + textPadding), (int)(dialogBoxRect.y + textPadding),
                       (int)(dialogBoxRect.w - (2 * textPadding)));
        }

        // Render Choices
        if (awaitingChoice && currentLine.hasChoices) {
            // --- Layout Constants (Adjust these values to fine-tune appearance) ---
            const float CHOICES_GAP_ABOVE_DIALOG = 20.0f; // Minimum gap between dialog box top and choices block bottom
            const float CHOICE_HEIGHT = 40.0f;             // Fixed height for each choice box
            const float HORIZONTAL_CHOICE_SPACING = 30.0f; // Horizontal spacing between choices in a row
            const float VERTICAL_ROW_SPACING = 15.0f;     // Vertical spacing between rows of choices
            const float CHOICE_PADDING_X = 20.0f;         // Horizontal padding inside each choice box for text
            const float LAYOUT_HORIZONTAL_MARGIN = 50.0f; // Margin from screen sides for choice layout area
            // Calculate the actual usable width for choices, considering screen margins
            const float LAYOUT_AREA_WIDTH = (float)winWidth - (2 * LAYOUT_HORIZONTAL_MARGIN);

            // --- Step 1: Pre-calculate metrics for each choice (text width, calculated box width) ---
            struct ChoiceLayoutInfo {
                size_t originalIndex; // To link back to currentLine.choices
                int textWidth;
                float boxWidth;
            };
            std::vector<ChoiceLayoutInfo> layoutInfos;
            for (size_t i = 0; i < currentLine.choices.size(); ++i) {
                int textW, textH;
                TTF_GetStringSize(gDialogFont, currentLine.choices[i].text.c_str(), currentLine.choices[i].text.length(), &textW, &textH);
                layoutInfos.push_back({i, textW, (float)textW + (2 * CHOICE_PADDING_X)});
            }

            // --- Step 2: Determine row breaks and calculate total width for each row ---
            std::vector<std::vector<size_t>> rowChoiceIndices; // Stores indices from layoutInfos for each row
            std::vector<float> rowWidths;                       // Stores actual rendered width of each row (including spacing)

            rowChoiceIndices.emplace_back(); // Start with the first row
            rowWidths.emplace_back(0.0f);
            size_t currentRow = 0;
            float currentXInRow = 0.0f; // Tracks width used in the current row

            for (size_t i = 0; i < layoutInfos.size(); ++i) {
                float potentialNextElementTotalWidth = layoutInfos[i].boxWidth;
                if (currentXInRow > 0) { // If not the very first element in the current row, add spacing
                    potentialNextElementTotalWidth += HORIZONTAL_CHOICE_SPACING;
                }

                // Check if adding this choice (and its leading spacing if applicable) would exceed row width
                if (currentXInRow + potentialNextElementTotalWidth > LAYOUT_AREA_WIDTH && currentXInRow > 0) {
                    // This element won't fit in the current row, start a new row
                    currentRow++;
                    rowChoiceIndices.emplace_back();
                    rowWidths.emplace_back(0.0f);
                    currentXInRow = 0.0f; // Reset for the new row
                    // The first element of the new row doesn't have leading spacing
                    currentXInRow += layoutInfos[i].boxWidth;
                } else {
                    // Element fits, add its width (and spacing if applicable) to current row
                    currentXInRow += potentialNextElementTotalWidth;
                }
                rowChoiceIndices[currentRow].push_back(i); // Store index from layoutInfos for current row
                rowWidths[currentRow] = currentXInRow;     // Update current row's total calculated width
            }

            // --- Step 3: Calculate total height of the entire block of choices and its final Y position ---
            float totalChoicesBlockHeight = (float)rowChoiceIndices.size() * CHOICE_HEIGHT;
            if (rowChoiceIndices.size() > 1) { // Add vertical spacing only if there's more than one row
                totalChoicesBlockHeight += (float)(rowChoiceIndices.size() - 1) * VERTICAL_ROW_SPACING;
            }

            // Determine the final starting Y for the entire block of choices
            // Shifts the block up if multiple rows are created
            float finalChoicesRenderY = dialogBoxRect.y - totalChoicesBlockHeight - CHOICES_GAP_ABOVE_DIALOG;

            // --- Step 4: Render choices based on calculated rows and positions ---
            float currentRenderY = finalChoicesRenderY; // Starting Y for the first row
            for (size_t r = 0; r < rowChoiceIndices.size(); ++r) {
                // Calculate starting X for this specific row to center it
                float rowStartX = ((float)winWidth - rowWidths[r]) / 2.0f;
                float currentRenderX = rowStartX;

                for (size_t layoutInfoIndex : rowChoiceIndices[r]) {
                    const auto& info = layoutInfos[layoutInfoIndex]; // Get pre-calculated metrics
                    Choice& choice = currentLine.choices[info.originalIndex]; // Get the actual Choice object

                    SDL_FRect choiceRect = {
                        currentRenderX + shakeOffset.x, // Apply shake offset
                        currentRenderY + shakeOffset.y, // Apply shake offset
                        info.boxWidth,                  // Use pre-calculated box width
                        CHOICE_HEIGHT                   // Fixed choice box height
                    };
                    choice.rect = choiceRect; // Store for click detection

                    // Draw choice box background
                    SDL_SetRenderDrawColor(gRenderer, choiceBgColor.r, choiceBgColor.g, choiceBgColor.b, choiceBgColor.a);
                    SDL_RenderFillRect(gRenderer, &choiceRect);
                    SDL_SetRenderDrawColor(gRenderer, choiceBorderColor.r, choiceBorderColor.g, choiceBorderColor.b, choiceBorderColor.a);
                    SDL_RenderRect(gRenderer, &choiceRect);

                    // Render choice text centered within its box
                    int dummyTextW, textH; // dummyTextW is not used, as info.textWidth holds the actual width
                    TTF_GetStringSize(gDialogFont, choice.text.c_str(), choice.text.length(), &dummyTextW, &textH);

                    renderText(gRenderer, gDialogFont, choice.text, textColorWhite,
                               (int)(choiceRect.x + (choiceRect.w - info.textWidth) / 2), // Center text horizontally
                               (int)(choiceRect.y + (choiceRect.h - textH) / 2),           // Center text vertically
                               (int)choiceRect.w); // Pass the box width for potential text wrapping

                    // Move X position for the next choice in this row
                    currentRenderX += info.boxWidth + HORIZONTAL_CHOICE_SPACING;
                }
                // Move Y position down for the start of the next row
                currentRenderY += CHOICE_HEIGHT + VERTICAL_ROW_SPACING;
            }
        }

        SDL_RenderPresent(gRenderer);
    }

    closeSDL();
    return 0;
}

// --- SDL Initialization and Teardown Functions ---
bool initSDL() {
    if (SDL_Init(SDL_INIT_VIDEO) < 0) {
        std::cerr << "SDL could not initialize! SDL_Error: " << SDL_GetError() << std::endl;
        return false;
    }

    gWindow = SDL_CreateWindow("Visual Novel Demo", winWidth, winHeight, SDL_WINDOW_RESIZABLE);
    if (gWindow == nullptr) {
        std::cerr << "Window could not be created! SDL_Error: " << SDL_GetError() << std::endl;
        return false;
    }

    // Use SDL3 specific renderer flags
    gRenderer = SDL_CreateRenderer(gWindow, nullptr); // Added flags back
    if (gRenderer == nullptr) {
        std::cerr << "Renderer could not be created! SDL_Error: " << SDL_GetError() << std::endl;
        return false;
    }

    SDL_SetRenderDrawBlendMode(gRenderer, SDL_BLENDMODE_BLEND);

    if (TTF_Init() == -1) {
        std::cerr << "SDL_ttf could not initialize! TTF_Error: " << SDL_GetError() << std::endl; // Corrected TTF_GetError
        return false;
    }

    // Create a TTF_TextEngine
    gTextEngine = TTF_CreateRendererTextEngine(gRenderer);
    if (gTextEngine == nullptr) {
        std::cerr << "Failed to create TTF_TextEngine: " << SDL_GetError() << std::endl;
        return false;
    }

    gDialogFont = TTF_OpenFont(fontstr, 18); // Main dialog font size
    if (gDialogFont == nullptr) {
        std::cerr << "Failed to load dialog font! TTF_Error: " << SDL_GetError() << std::endl; // Corrected TTF_GetError
        return false;
    }

    gNameFont = TTF_OpenFont(fontstr, 18); // Name font size
    if (gNameFont == nullptr) {
        std::cerr << "Failed to load name font! TTF_Error: " << SDL_GetError() << std::endl; // Corrected TTF_GetError
        return false;
    }

    return true;
}

void closeSDL() {
    if (gDialogFont) TTF_CloseFont(gDialogFont);
    if (gNameFont) TTF_CloseFont(gNameFont);
    if (gTextEngine) TTF_DestroyRendererTextEngine(gTextEngine); // Destroy the text engine
    if (gRenderer) SDL_DestroyRenderer(gRenderer);
    if (gWindow) SDL_DestroyWindow(gWindow);

    TTF_Quit();
    SDL_Quit();
}

// --- Dialog File Loading ---
std::vector<DialogLine> loadDialogFromFile(const std::string& filename) {
    std::vector<DialogLine> loadedDialogs; // Use a local vector to build
    std::ifstream file(filename);
    if (!file.is_open()) {
        std::cerr << "Failed to open dialog file: " << filename << std::endl;
        // Return an empty vector, error message is already printed
        return loadedDialogs;
    }

    std::string line;
    std::string currentSpeaker = "";
    int rawLineNumber = 0;

    // Temporary flags and parameters for the *next* dialog line to be loaded
    bool nextLineShouldJitter = false;
    bool nextLineShouldFall = false;
    bool nextLineShouldFloat = false;
    bool nextLineShouldPulse = false;
    Uint64 nextLinePulseDuration = 1500;           // Default pulse duration
    float nextLinePulseFrequency = 2.0f;
    SDL_Color nextLinePulseColor1 = {255,255,255,255};  // Default white
    SDL_Color nextLinePulseColor2 = {255,100,100,255};  // Default reddish
    bool nextLineShouldShake = false;
    Uint64 nextLineShakeDuration = 0;   // Default duration for shake
    float nextLineShakeIntensity = 0.0f; // Default intensity for shake

    while (std::getline(file, line)) {
        rawLineNumber++;
        size_t firstChar = line.find_first_not_of(" \t\r\n");
        if (firstChar == std::string::npos) {
            continue; // Skip empty or whitespace-only lines
        }
        std::string trimmedLine = line.substr(firstChar);

        // Skip lines that start with a '#' comment character
        if (trimmedLine.rfind("#", 0) == 0) {
            continue;
        }

        // --- Handle Special Tags ---
        if (trimmedLine == "[") { // Start of a choice block
            if (loadedDialogs.empty()) {
                std::cerr << "Warning: Choice block found without preceding dialog on line " << rawLineNumber << " in " << filename << std::endl;
                continue;
            }

            DialogLine& lastDialog = loadedDialogs.back(); // Choices belong to the most recently added dialog line
            lastDialog.hasChoices = true;

            // Reset all "next line should" flags for the dialog line *after* choices
            // (These flags apply to the *next* dialog line, not the one with choices)
            nextLineShouldJitter = false;
            nextLineShouldFall = false;
            nextLineShouldFloat = false;
            nextLineShouldPulse = false;
            nextLinePulseDuration = 1500;
            nextLinePulseColor1 = {255,255,255,255};
            nextLinePulseColor2 = {255,100,100,255};
            nextLineShouldShake = false;
            nextLineShakeDuration = 0;
            nextLineShakeIntensity = 0.0f;

            while (std::getline(file, line)) {
                rawLineNumber++;
                size_t choiceLineFirstChar = line.find_first_not_of(" \t\r\n");
                if (choiceLineFirstChar == std::string::npos) continue;
                trimmedLine = line.substr(choiceLineFirstChar);

                if (trimmedLine == "]") break; // End of choice block
                if (trimmedLine.rfind("#", 0) == 0) continue; // Skip comments in choice block

                size_t firstQuote = trimmedLine.find('"');
                size_t secondQuote = trimmedLine.find('"', firstQuote + 1);
                size_t arrow = trimmedLine.find("->");

                if (firstQuote == std::string::npos || secondQuote == std::string::npos || arrow == std::string::npos ||
                    firstQuote >= secondQuote || secondQuote >= arrow) {
                    std::cerr << "Warning: Malformed choice line " << rawLineNumber << " in " << filename << ": " << line << std::endl;
                    continue;
                }

                Choice c;
                c.text = trimmedLine.substr(firstQuote + 1, secondQuote - (firstQuote + 1));
                std::string targetString = trimmedLine.substr(arrow + 2);
                size_t colonPos = targetString.find(':');

                if (colonPos != std::string::npos) {
                    c.nextFile = targetString.substr(0, colonPos);
                    try { c.nextDialogIndex = std::stoi(targetString.substr(colonPos + 1)); }
                    catch (...) { std::cerr << "Invalid choice index in " << filename << " on line " << rawLineNumber << std::endl; c.nextDialogIndex = 0; }
                } else {
                    c.nextDialogIndex = 0; // Default to 0 if no index specified
                    try {
                        // Check if targetString is a number, if not, it's a file name
                        size_t end;
                        int val = std::stoi(targetString, &end);
                        if (end == targetString.length()) { // It's purely a number
                            c.nextDialogIndex = val;
                            c.nextFile = ""; // No file change
                        } else { // It's likely a file name
                            c.nextFile = targetString;
                            c.nextDialogIndex = 0; // Default to line 0 of new file
                        }
                    }
                    catch (...) {
                        std::cerr << "Invalid choice target in " << filename << " on line " << rawLineNumber << ": " << targetString << std::endl;
                        c.nextDialogIndex = 0;
                        c.nextFile = "";
                    }
                }
                lastDialog.choices.push_back(c);
            }
            continue; // Move to next line after processing choice block
        }
        else if (trimmedLine == "[JITTER]") { nextLineShouldJitter = true; continue; }
        else if (trimmedLine == "[FALL]") { nextLineShouldFall = true; continue; }
        else if (trimmedLine == "[FLOAT]") { nextLineShouldFloat = true; continue; }
        else if (trimmedLine.rfind("[PULSE", 0) == 0) {
            size_t closeBracketPos = trimmedLine.find(']');
            if (closeBracketPos == std::string::npos) {
                std::cerr << "Warning: Malformed [PULSE] tag on line " << rawLineNumber << ": Missing ']' -> " << line << std::endl; continue;
            }
            std::string tagContent = trimmedLine.substr(0, closeBracketPos + 1);
            std::istringstream iss(tagContent);
            std::string tagStr; // Will read "[PULSE]"
            int r1, g1, b1, a1, r2, g2, b2, a2;
            Uint64 duration;
            float frequency;
            // Expecting "[PULSE duration R1 G1 B1 A1 R2 G2 B2 A2]"
            // Need to skip the "[PULSE]" part first
            iss >> tagStr; // Consume "[PULSE]"
            if (iss >> duration >> frequency >> r1 >> g1 >> b1 >> a1 >> r2 >> g2 >> b2 >> a2) {
                 nextLineShouldPulse = true;
                 nextLinePulseDuration = duration;
                 nextLinePulseFrequency = frequency;
                 nextLinePulseColor1 = {(Uint8)r1, (Uint8)g1, (Uint8)b1, (Uint8)a1};
                 nextLinePulseColor2 = {(Uint8)r2, (Uint8)g2, (Uint8)b2, (Uint8)a2};
            } else { std::cerr << "Warning: Malformed [PULSE] parameters on line " << rawLineNumber << ": " << line << std::endl; }
            continue;
        }
        else if (trimmedLine.rfind("[SHAKE", 0) == 0) {
            size_t closeBracketPos = trimmedLine.find(']');
            if (closeBracketPos == std::string::npos) {
                std::cerr << "Warning: Malformed [SHAKE] tag on line " << rawLineNumber << ": Missing ']' -> " << line << std::endl; continue;
            }
            std::string tagContent = trimmedLine.substr(0, closeBracketPos + 1);
            std::istringstream iss(tagContent);
            std::string tagStr; // Will read "[SHAKE]"
            Uint64 duration;
            float intensity;
            iss >> tagStr; // Consume "[SHAKE]"
            if (iss >> duration >> intensity) {
                 nextLineShouldShake = true;
                 nextLineShakeDuration = duration;
                 nextLineShakeIntensity = intensity;
            } else { std::cerr << "Warning: Malformed [SHAKE] parameters on line " << rawLineNumber << ": " << line << std::endl; }
            continue;
        }

        // --- Parse Actual Dialog Line ---
        size_t speakerQuoteEnd = trimmedLine.find('\'', 1);
        if (trimmedLine.front() == '\'' && speakerQuoteEnd != std::string::npos) {
            currentSpeaker = trimmedLine.substr(1, speakerQuoteEnd - 1);
            trimmedLine = trimmedLine.substr(speakerQuoteEnd + 1); // Rest of the line
            firstChar = trimmedLine.find_first_not_of(" \t\r\n"); // Recalculate first char for dialog
            if (firstChar != std::string::npos) trimmedLine = trimmedLine.substr(firstChar);
            else trimmedLine = ""; // Only speaker, no dialog
        }

        size_t dialogQuoteEnd = trimmedLine.find('"', 1);
        if (trimmedLine.front() == '"' && dialogQuoteEnd != std::string::npos) {
            DialogLine dl;
            dl.speakerName = currentSpeaker;
            dl.dialogText = trimmedLine.substr(1, dialogQuoteEnd - 1);
            dl.hasChoices = false; // Default, can be set true by choice block

            // Apply flags/params for this dialog line
            dl.applyJitter = nextLineShouldJitter;
            dl.applyFall = nextLineShouldFall;
            dl.applyFloat = nextLineShouldFloat;
            dl.applyPulse = nextLineShouldPulse;
            dl.pulseDurationMs = nextLinePulseDuration;
            dl.pulseFrequencyHz = nextLinePulseFrequency;
            dl.pulseColor1 = nextLinePulseColor1;
            dl.pulseColor2 = nextLinePulseColor2;
            dl.applyShake = nextLineShouldShake;
            dl.shakeDuration = nextLineShakeDuration;
            dl.shakeIntensity = nextLineShakeIntensity;
            dl.physicsActive = false; // Physics always starts inactive for new line


            // For pre-calculating word positions for jitter/physics,
            // we use the approximate screen area where text *would* appear
            // without considering runtime shake offset or precise dialog box padding.
            // The actual rendering will then offset these pre-calculated positions.
            const int textStartX = (int)(winWidth * 0.1f + textPadding);
            const int textStartY = (int)(winHeight * 0.55f + textPadding); // Based on dialog box Y
            const int textWrapWidth = (int)(winWidth * 0.8f - (2 * textPadding));

            // Initialize words for effects that need pre-calculation (jitter, physics)
            if (dl.applyJitter) {
                dl.jitterWords = initJitterWords(gRenderer, gDialogFont, dl.dialogText,
                                                 textStartX, textStartY, textWrapWidth);
            }
            if (dl.applyFall || dl.applyFloat) {
                dl.physicsWords = initPhysicsWords(gRenderer, gDialogFont, dl.dialogText,
                                                   textStartX, textStartY, textWrapWidth);
            }

            loadedDialogs.push_back(dl);

            // Reset "next line should" flags and parameters for the *next* iteration
            nextLineShouldJitter = false;
            nextLineShouldFall = false;
            nextLineShouldFloat = false;
            nextLineShouldPulse = false;
            nextLinePulseDuration = 1500;
            nextLinePulseFrequency = 2.0f;
            nextLinePulseColor1 = {255,255,255,255};
            nextLinePulseColor2 = {255,100,100,255};
            nextLineShouldShake = false;
            nextLineShakeDuration = 0;
            nextLineShakeIntensity = 0.0f;
        } else {
            // If it's not a tag, and not a recognized dialog format, warn
            std::cerr << "Warning: Unrecognized line format on line " << rawLineNumber << " in " << filename << ": " << line << std::endl;
        }
    }
    file.close();
    return loadedDialogs;
}