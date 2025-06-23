#include "StoryManager.h" // Include the header for StoryManager
#include <iostream>       // For std::cerr
#include <fstream>        // For std::ifstream
#include <sstream>        // For std::istringstream
#include <algorithm>      // For std::min, std::max
#include <cmath>          // For std::fabs (already in text_effects.cpp, but good for self-containment)

// --- Global Constants Access ---
// These are declared extern in text_ui.h and defined in main.cpp.
// StoryManager uses them directly.
extern int winWidth;
extern int winHeight;
extern const int textPadding;

// Assuming these color constants are also accessible globally
extern SDL_Color textColorWhite;
extern SDL_Color dialogBoxBgColor;
extern SDL_Color nameBoxBgColor;
extern SDL_Color borderColor;
extern SDL_Color choiceBgColor;
extern SDL_Color choiceBorderColor;


// --- Constructor & Destructor ---
StoryManager::StoryManager(SDL_Renderer* renderer, TTF_Font* dialogFont, TTF_Font* nameFont, TTF_TextEngine* textEngine)
    : gRenderer(renderer), gDialogFont(dialogFont), gNameFont(nameFont), gTextEngine(textEngine),
      currentDialogIndex(0), currentVisibleCharCount(0), animationDelayMs(40.0f),
      lastCharRevealTime(0), animationIsPlaying(true), awaitingChoice(false),
      prevDialogIndex(0), currentStoryFile("") { // Initialize currentStoryFile
    // Constructor initializes internal state and takes SDL pointers
}

StoryManager::~StoryManager() {
    clearAllStoryResources(); // Ensure all loaded resources are freed
}

// --- Resource Management ---
void StoryManager::clearAllStoryResources() {
    for (auto& dialog : dialogLines) {
        if (dialog.hasChoices) {
            for (auto& choice : dialog.choices) {
                if (choice.textTexture) {
                    SDL_DestroyTexture(choice.textTexture);
                    choice.textTexture = nullptr;
                }
            }
        }
        // Also clear jitter/physics words to free their memory if any
        dialog.jitterWords.clear();
        dialog.physicsWords.clear();
    }
    dialogLines.clear(); // Clear the vector itself
}

// --- Story Loading ---
bool StoryManager::loadStory(const std::string& filename) {
    clearAllStoryResources(); // Clear any previously loaded story and its resources

    std::ifstream file(filename);
    if (!file.is_open()) {
        std::cerr << "Failed to open story file: " << filename << std::endl;
        return false;
    }

    currentStoryFile = filename; // Store for relative jumps

    std::string line;
    std::string currentSpeaker = "";
    int rawLineNumber = 0;

    // Temporary flags and parameters for the *next* dialog line to be loaded
    bool nextLineShouldJitter = false;
    bool nextLineShouldFall = false;
    bool nextLineShouldFloat = false;
    bool nextLineShouldPulse = false;
    Uint64 nextLinePulseDuration = 1500;
    float nextLinePulseFrequency = 2.0f;
    SDL_Color nextLinePulseColor1 = {255,255,255,255};
    SDL_Color nextLinePulseColor2 = {255,100,100,255};

    bool nextLineShouldShake = false;
    Uint64 nextLineShakeDuration = 0;
    float nextLineShakeIntensity = 0.0f;

    bool nextLineShouldTear = false;
    Uint32 nextLineTearDuration = 0;
    float nextLineTearMaxOffsetX = 0.0f;
    float nextLineTearLineDensity = 0.0f;


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
            if (dialogLines.empty()) {
                std::cerr << "Warning: Choice block found without preceding dialog on line " << rawLineNumber << " in " << filename << std::endl;
                continue;
            }

            DialogLine& lastDialog = dialogLines.back(); // Choices belong to the most recently added dialog line
            lastDialog.hasChoices = true;

            // Reset all "next line should" flags and parameters for the *next* dialog line (after choices)
            // This ensures effects don't carry over unintentionally if a choice leads to a new dialog line
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
            nextLineShouldTear = false;
            nextLineTearDuration = 0;
            nextLineTearMaxOffsetX = 0.0f;
            nextLineTearLineDensity = 0.0f;

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
                    c.nextDialogIndex = 0;
                    try {
                        size_t end;
                        int val = std::stoi(targetString, &end);
                        if (end == targetString.length()) {
                            c.nextDialogIndex = val;
                            c.nextFile = "";
                        } else {
                            c.nextFile = targetString;
                            c.nextDialogIndex = 0;
                        }
                    }
                    catch (...) {
                        std::cerr << "Invalid choice target in " << filename << " on line " << rawLineNumber << ": " << targetString << std::endl;
                        c.nextDialogIndex = 0;
                        c.nextFile = "";
                    }
                }

                // Pre-render choice text texture here
                // Use the current global winWidth for wrapping when pre-rendering
                TTF_GetStringSize(gDialogFont, c.text.c_str(), c.text.length(), &c.textWidth, &c.textHeight);
                SDL_Surface* textSurface = TTF_RenderText_Blended_Wrapped(gDialogFont, c.text.c_str(), c.text.length(), textColorWhite, winWidth);
                if (textSurface) {
                    c.textTexture = SDL_CreateTextureFromSurface(gRenderer, textSurface);
                    SDL_DestroySurface(textSurface);
                    if (!c.textTexture) {
                         std::cerr << "Failed to create texture for choice text: " << SDL_GetError() << std::endl;
                    }
                } else {
                    std::cerr << "Failed to create surface for choice text: " << SDL_GetError() << std::endl;
                }

                lastDialog.choices.push_back(c);
            }
            continue;
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
            std::string tagStr;
            int r1, g1, b1, a1, r2, g2, b2, a2;
            Uint64 duration;
            float frequency;
            iss >> tagStr;
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
            std::string tagStr;
            Uint64 duration;
            float intensity;
            iss >> tagStr;
            if (iss >> duration >> intensity) {
                 nextLineShouldShake = true;
                 nextLineShakeDuration = duration;
                 nextLineShakeIntensity = intensity;
            } else { std::cerr << "Warning: Malformed [SHAKE] parameters on line " << rawLineNumber << ": " << line << std::endl; }
            continue;
        }
        else if (trimmedLine.rfind("[TEAR", 0) == 0) {
            size_t closeBracketPos = trimmedLine.find(']');
            if (closeBracketPos == std::string::npos) {
                std::cerr << "Warning: Malformed [TEAR] tag on line " << rawLineNumber << ": Missing ']' -> " << line << std::endl; continue;
            }
            std::string tagContent = trimmedLine.substr(0, closeBracketPos + 1);
            std::istringstream iss(tagContent);
            std::string tagStr;
            Uint32 duration;
            float maxOffsetX;
            float density;
            iss >> tagStr;
            if (iss >> duration >> maxOffsetX >> density) {
                 nextLineShouldTear = true;
                 nextLineTearDuration = duration;
                 nextLineTearMaxOffsetX = maxOffsetX;
                 nextLineTearLineDensity = density;
            } else { std::cerr << "Warning: Malformed [TEAR] parameters on line " << rawLineNumber << ": " << line << std::endl; }
            continue;
        }


        size_t speakerQuoteEnd = trimmedLine.find('\'', 1);
        if (trimmedLine.front() == '\'' && speakerQuoteEnd != std::string::npos) {
            currentSpeaker = trimmedLine.substr(1, speakerQuoteEnd - 1);
            trimmedLine = trimmedLine.substr(speakerQuoteEnd + 1);
            size_t recheckedFirstChar = trimmedLine.find_first_not_of(" \t\r\n");
            if (recheckedFirstChar != std::string::npos) trimmedLine = trimmedLine.substr(recheckedFirstChar);
            else trimmedLine = "";
        }

        size_t dialogQuoteEnd = trimmedLine.find('"', 1);
        if (trimmedLine.front() == '"' && dialogQuoteEnd != std::string::npos) {
            DialogLine dl;
            dl.speakerName = currentSpeaker;
            dl.dialogText = trimmedLine.substr(1, dialogQuoteEnd - 1);
            dl.hasChoices = false;

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
            dl.applyTear = nextLineShouldTear;
            dl.tearDuration = nextLineTearDuration;
            dl.tearMaxOffsetX = nextLineTearMaxOffsetX;
            dl.tearLineDensity = nextLineTearLineDensity;
            dl.physicsActive = false;


            const int textStartXForEffectInit = (int)(winWidth * 0.1f + textPadding);
            const int textStartYForEffectInit = (int)(winHeight * 0.55f + textPadding);
            const int textWrapWidthForEffectInit = (int)(winWidth * 0.8f - (2 * textPadding));

            if (dl.applyJitter) {
                dl.jitterWords = initJitterWords(gRenderer, gDialogFont, dl.dialogText,
                                                 textStartXForEffectInit, textStartYForEffectInit,
                                                 textWrapWidthForEffectInit);
            }
            if (dl.applyFall || dl.applyFloat) {
                dl.physicsWords = initPhysicsWords(gRenderer, gDialogFont, dl.dialogText,
                                                   textStartXForEffectInit, textStartYForEffectInit,
                                                   textWrapWidthForEffectInit);
            }

            dialogLines.push_back(dl);

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
            nextLineShouldTear = false;
            nextLineTearDuration = 0;
            nextLineTearMaxOffsetX = 0.0f;
            nextLineTearLineDensity = 0.0f;
        } else {
            std::cerr << "Warning: Unrecognized line format on line " << rawLineNumber << " in " << filename << ": " << line << std::endl;
        }
    }
    file.close();

    currentDialogIndex = 0;
    currentVisibleCharCount = 0;
    animationIsPlaying = true;
    awaitingChoice = false;
    lastCharRevealTime = SDL_GetTicks();
    prevDialogIndex = 0;

    return true;
}

// --- Update Logic ---
void StoryManager::update(Uint64 currentTicks, float deltaTime) {
    if (currentDialogIndex >= dialogLines.size()) {
        return;
    }

    DialogLine& currentLine = dialogLines[currentDialogIndex];

    if (currentDialogIndex != prevDialogIndex) {
        deactivateActiveEffects(); // Correctly scoped
        activateLineEffects();     // Correctly scoped
        prevDialogIndex = currentDialogIndex;
    }

    if (animationIsPlaying && !awaitingChoice) {
        currentVisibleCharCount = (currentTicks - lastCharRevealTime) / animationDelayMs;
        currentVisibleCharCount = std::min(currentVisibleCharCount, currentLine.dialogText.length());

        if (currentVisibleCharCount >= currentLine.dialogText.length()) {
            animationIsPlaying = false;
            if (currentLine.hasChoices) {
                awaitingChoice = true;
            } else if ((currentLine.applyFall || currentLine.applyFloat) && !currentLine.physicsActive) {
                currentLine.physicsActive = true;
                if (currentLine.applyFall) {
                    applyfallEffect(currentLine.physicsWords);
                } else if (currentLine.applyFloat) {
                    applyfloatEffect(currentLine.physicsWords);
                }
            }
        }
    }

    if (currentLine.applyJitter) {
        applyJitter(currentLine.jitterWords);
    }
    if ((currentLine.applyFall || currentLine.applyFloat) && currentLine.physicsActive) {
        updatePhysicsWords(currentLine.physicsWords, deltaTime);
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

// --- Input Handling ---
void StoryManager::handleInput(const SDL_Event& event) {
    if (currentDialogIndex >= dialogLines.size()) {
        return;
    }

    if (event.type == SDL_EVENT_KEY_DOWN) {
        if (event.key.key == SDLK_SPACE || event.key.key == SDLK_RETURN) {
            if (awaitingChoice) {
                // Do nothing, awaiting mouse click for choice
            } else if (animationIsPlaying) {
                currentVisibleCharCount = dialogLines[currentDialogIndex].dialogText.length();
                animationIsPlaying = false;
                DialogLine& currentLine = dialogLines[currentDialogIndex];
                if (currentLine.hasChoices) {
                    awaitingChoice = true;
                } else if (currentLine.applyFall || currentLine.applyFloat) {
                    currentLine.physicsActive = true;
                    if (currentLine.applyFall) {
                        applyfallEffect(currentLine.physicsWords);
                    } else if (currentLine.applyFloat) {
                        applyfloatEffect(currentLine.physicsWords);
                    }
                }
                activateLineEffects(); // Correctly scoped
            } else {
                DialogLine& currentLine = dialogLines[currentDialogIndex];
                if ((currentLine.applyFall || currentLine.applyFloat) && currentLine.physicsActive) {
                    currentLine.physicsActive = false;
                    currentLine.physicsWords.clear();
                }
                advanceStoryLine(); // Correctly scoped
            }
        }
    }
    if (event.type == SDL_EVENT_MOUSE_BUTTON_DOWN) {
        if (event.button.button == SDL_BUTTON_LEFT && awaitingChoice) {
            handleChoiceClick({(float)event.button.x, (float)event.button.y}); // Correctly scoped
        }
    }
}

// --- Rendering ---
void StoryManager::render(Uint64 currentTicks) {
    if (currentDialogIndex >= dialogLines.size()) {
        return;
    }

    DialogLine& currentLine = dialogLines[currentDialogIndex];

    SDL_FPoint shakeOffset = getScreenShakeOffset();

    float dialogBoxBaseX = (float)(winWidth * 0.1f + shakeOffset.x);
    float dialogBoxBaseY = (float)(winHeight * 0.55f + shakeOffset.y);
    float dialogBoxTearOffsetX = getScreenTearXOffset(dialogBoxBaseY);
    SDL_FRect dialogBoxRect = {
        dialogBoxBaseX + dialogBoxTearOffsetX,
        dialogBoxBaseY,
        (float)(winWidth * 0.8f), (float)(winHeight * 0.25f)
    };

    float nameBoxBaseX = (float)(dialogBoxRect.x);
    float nameBoxBaseY = (float)(dialogBoxRect.y - 40);
    float nameBoxTearOffsetX = getScreenTearXOffset(nameBoxBaseY);
    SDL_FRect nameBoxRect = {
        nameBoxBaseX + nameBoxTearOffsetX,
        nameBoxBaseY + shakeOffset.y,
        150.0f, 30.0f
    };

    drawDialogBoxUI(gRenderer, dialogBoxRect.x, dialogBoxRect.y, dialogBoxRect.w, dialogBoxRect.h, dialogBoxBgColor, borderColor);

    renderNameBox(gRenderer, gNameFont, gTextEngine, currentLine.speakerName
        , nameBoxRect.x, nameBoxRect.y, nameBoxRect.w, nameBoxRect.h
        , nameBoxBgColor, nameBoxBgColor, textColorWhite);

    SDL_Color currentTextColor = isTextColorPulseActive() ? getPulsingTextColor(currentTicks) : textColorWhite;

    float textRenderBaseX = (float)(dialogBoxRect.x + textPadding);
    float textRenderBaseY = (float)(dialogBoxRect.y + textPadding);

    if (currentLine.applyJitter && !animationIsPlaying && !(currentLine.applyFall || currentLine.applyFloat)) {
        for (const auto& word : currentLine.jitterWords) {
            renderText(gRenderer, gDialogFont, word.text, currentTextColor,
                       (int)(word.rect.x + shakeOffset.x + getScreenTearXOffset(word.rect.y + shakeOffset.y)),
                       (int)(word.rect.y + shakeOffset.y), 0);
        }
    } else if ((currentLine.applyFall || currentLine.applyFloat) && currentLine.physicsActive) {
        for (const auto& word : currentLine.physicsWords) {
            if (word.active) {
                renderText(gRenderer, gDialogFont, word.text, currentTextColor,
                           (int)(word.rect.x + shakeOffset.x + getScreenTearXOffset(word.rect.y + shakeOffset.y)),
                           (int)(word.rect.y + shakeOffset.y), 0);
            }
        }
    } else {
        std::string textToDisplay = currentLine.dialogText.substr(0, currentVisibleCharCount);
        float currentTextTearOffsetX = getScreenTearXOffset(textRenderBaseY);
        renderText(gRenderer, gDialogFont, textToDisplay, currentTextColor,
                   (int)(textRenderBaseX + currentTextTearOffsetX),
                   (int)textRenderBaseY,
                   (int)(dialogBoxRect.w - (2 * textPadding)));
    }

    if (awaitingChoice && currentLine.hasChoices) {
        const float CHOICES_GAP_ABOVE_DIALOG = 20.0f;
        const float CHOICE_HEIGHT = 40.0f;
        const float HORIZONTAL_CHOICE_SPACING = 30.0f;
        const float VERTICAL_ROW_SPACING = 15.0f;
        const float CHOICE_PADDING_X = 20.0f;
        const float LAYOUT_HORIZONTAL_MARGIN = 50.0f;
        const float LAYOUT_AREA_WIDTH = (float)winWidth - (2 * LAYOUT_HORIZONTAL_MARGIN);

        struct ChoiceLayoutInfo {
            size_t originalIndex;
            int textWidth;
            float boxWidth;
        };
        std::vector<ChoiceLayoutInfo> layoutInfos;
        for (size_t i = 0; i < currentLine.choices.size(); ++i) {
            layoutInfos.push_back({i, currentLine.choices[i].textWidth, (float)currentLine.choices[i].textWidth + (2 * CHOICE_PADDING_X)});
        }

        std::vector<std::vector<size_t>> rowChoiceIndices;
        std::vector<float> rowWidths;

        rowChoiceIndices.emplace_back();
        rowWidths.emplace_back(0.0f);
        size_t currentRow = 0;
        float currentXInRow = 0.0f;

        for (size_t i = 0; i < layoutInfos.size(); ++i) {
            float potentialNextElementTotalWidth = layoutInfos[i].boxWidth;
            if (currentXInRow > 0) {
                potentialNextElementTotalWidth += HORIZONTAL_CHOICE_SPACING;
            }

            if (currentXInRow + potentialNextElementTotalWidth > LAYOUT_AREA_WIDTH && currentXInRow > 0) {
                currentRow++;
                rowChoiceIndices.emplace_back();
                rowWidths.emplace_back(0.0f);
                currentXInRow = 0.0f;
                currentXInRow += layoutInfos[i].boxWidth;
            } else {
                currentXInRow += potentialNextElementTotalWidth;
            }
            rowChoiceIndices[currentRow].push_back(i);
            rowWidths[currentRow] = currentXInRow;
        }

        float totalChoicesBlockHeight = (float)rowChoiceIndices.size() * CHOICE_HEIGHT;
        if (rowChoiceIndices.size() > 1) {
            totalChoicesBlockHeight += (float)(rowChoiceIndices.size() - 1) * VERTICAL_ROW_SPACING;
        }

        float finalChoicesRenderY = dialogBoxRect.y - totalChoicesBlockHeight - CHOICES_GAP_ABOVE_DIALOG;

        float currentRenderY = finalChoicesRenderY;
        for (size_t r = 0; r < rowChoiceIndices.size(); ++r) {
            float rowStartX = ((float)winWidth - rowWidths[r]) / 2.0f;
            float currentRenderX = rowStartX;

            for (size_t layoutInfoIndex : rowChoiceIndices[r]) {
                const auto& info = layoutInfos[layoutInfoIndex];
                Choice& choice = currentLine.choices[info.originalIndex];

                float choiceBaseX = currentRenderX + shakeOffset.x;
                float choiceBaseY = currentRenderY + shakeOffset.y;

                float choiceBoxTearOffsetX = getScreenTearXOffset(choiceBaseY);

                SDL_FRect choiceRect = {
                    choiceBaseX + choiceBoxTearOffsetX,
                    choiceBaseY,
                    info.boxWidth,
                    CHOICE_HEIGHT
                };
                choice.rect = choiceRect;

                SDL_SetRenderDrawColor(gRenderer, choiceBgColor.r, choiceBgColor.g, choiceBgColor.b, choiceBgColor.a);
                SDL_RenderFillRect(gRenderer, &choiceRect);
                SDL_SetRenderDrawColor(gRenderer, choiceBorderColor.r, choiceBorderColor.g, choiceBorderColor.b, choiceBorderColor.a);
                SDL_RenderRect(gRenderer, &choiceRect);

                if (choice.textTexture) {
                     float textX = choiceRect.x + (choiceRect.w - choice.textWidth) / 2.0f;
                     float textY = choiceRect.y + (choiceRect.h - choice.textHeight) / 2.0f;
                     SDL_FRect textDstRect = { textX, textY, (float)choice.textWidth, (float)choice.textHeight };
                     SDL_RenderTexture(gRenderer, choice.textTexture, NULL, &textDstRect);
                } else {
                    renderText(gRenderer, gDialogFont, choice.text, textColorWhite,
                               (int)(choiceRect.x + (choiceRect.w - info.textWidth) / 2),
                               (int)(choiceRect.y + (choiceRect.h - choice.textHeight) / 2),
                               (int)choiceRect.w);
                }

                currentRenderX += info.boxWidth + HORIZONTAL_CHOICE_SPACING;
            }
            currentRenderY += CHOICE_HEIGHT + VERTICAL_ROW_SPACING;
        }
    }
}

// --- Private Helper Methods (for internal use by StoryManager) ---

// All these methods now have the correct 'StoryManager::' scope qualification.

void StoryManager::advanceStoryLine() { // Corrected: Added StoryManager::
    deactivateActiveEffects();

    currentDialogIndex++;
    if (currentDialogIndex < dialogLines.size()) {
        currentVisibleCharCount = 0;
        animationIsPlaying = true;
        awaitingChoice = false;
        lastCharRevealTime = SDL_GetTicks();
    } else {
        std::cout << "End of story. Looping to start." << std::endl;
        currentDialogIndex = 0;
        currentVisibleCharCount = 0;
        animationIsPlaying = true;
        awaitingChoice = false;
        lastCharRevealTime = SDL_GetTicks();
    }
}

void StoryManager::handleWindowResize(int newWidth, int newHeight) { // CORRECTED: Added StoryManager::
    // Re-render all choice textures as their wrapWidth (winWidth) has changed.
    for (auto& dialog : dialogLines) {
        if (dialog.hasChoices) {
            for (auto& choice : dialog.choices) {
                if (choice.textTexture) {
                    SDL_DestroyTexture(choice.textTexture); // Destroy old texture
                    choice.textTexture = nullptr;
                }
                // Re-create texture using the new window width for wrapping
                TTF_GetStringSize(gDialogFont, choice.text.c_str(), choice.text.length(), &choice.textWidth, &choice.textHeight);
                SDL_Surface* textSurface = TTF_RenderText_Blended_Wrapped(gDialogFont, choice.text.c_str(), choice.text.length(), textColorWhite, newWidth);
                if (textSurface) {
                    choice.textTexture = SDL_CreateTextureFromSurface(gRenderer, textSurface);
                    SDL_DestroySurface(textSurface);
                    if (!choice.textTexture) {
                         std::cerr << "Failed to re-create texture for choice text after resize: " << SDL_GetError() << std::endl;
                    }
                } else {
                    std::cerr << "Failed to re-create surface for choice text after resize: " << SDL_GetError() << std::endl;
                }
            }
        }

        // Clear and reset word-based effects (jitter, physics) for each dialog line.
        // This forces them to be re-initialized with new dimensions when that line becomes active.
        if (dialog.applyJitter) {
            dialog.jitterWords.clear();
        }
        if (dialog.applyFall || dialog.applyFloat) {
            dialog.physicsWords.clear();
            dialog.physicsActive = false; // Also reset physics active state
        }
    }
}

void StoryManager::activateLineEffects() { // Corrected: Added StoryManager::
    if (currentDialogIndex >= dialogLines.size()) return;

    DialogLine& currentLine = dialogLines[currentDialogIndex];

    // Important: Effects need to be re-initialized if their parameters were changed
    // or if they were deactivated by the previous line or window resize.
    if (currentLine.applyShake) {
        initScreenShake(currentLine.shakeDuration, currentLine.shakeIntensity);
    }
    if (currentLine.applyPulse) {
        initTextColorPulse(currentLine.pulseDurationMs, currentLine.pulseFrequencyHz, currentLine.pulseColor1, currentLine.pulseColor2);
    }
    if (currentLine.applyTear) {
        initScreenTear(currentLine.tearDuration, currentLine.tearMaxOffsetX, currentLine.tearLineDensity);
    }

    const int textStartXForEffectInit = (int)(winWidth * 0.1f + textPadding);
    const int textStartYForEffectInit = (int)(winHeight * 0.55f + textPadding);
    const int textWrapWidthForEffectInit = (int)(winWidth * 0.8f - (2 * textPadding));

    // Re-initialize jitter/physics words if they are currently empty (e.g., after clearAllStoryResources or resize)
    // or if the effect is applied for this line and they haven't been set up yet.
    if (currentLine.applyJitter && currentLine.jitterWords.empty()) {
        currentLine.jitterWords = initJitterWords(gRenderer, gDialogFont, currentLine.dialogText,
                                                 textStartXForEffectInit, textStartYForEffectInit,
                                                 textWrapWidthForEffectInit);
    }
     if ((currentLine.applyFall || currentLine.applyFloat) && currentLine.physicsWords.empty()) {
        currentLine.physicsWords = initPhysicsWords(gRenderer, gDialogFont, currentLine.dialogText,
                                                   textStartXForEffectInit, textStartYForEffectInit,
                                                   textWrapWidthForEffectInit);
    }
}

void StoryManager::deactivateActiveEffects() { // Corrected: Added StoryManager::
    // These functions from text_effects.h and visual_effects.h modify their internal state to turn off effects
    deactivateTextColorPulse(); // From text_effects.h

    // Force deactivate screen-wide effects if they are active, or let them fade
    initScreenShake(0, 0.0f); // Setting duration to 0 effectively turns it off
    initScreenTear(0, 0.0f, 0.0f); // Setting duration to 0 effectively turns it off
}

void StoryManager::handleChoiceClick(SDL_FPoint mouseClick) { // Corrected: Added StoryManager::
    DialogLine& currentLine = dialogLines[currentDialogIndex];
    for (size_t i = 0; i < currentLine.choices.size(); ++i) {
        Choice& choice = currentLine.choices[i];
        if (SDL_PointInRectFloat(&mouseClick, &choice.rect)) {
            // Deactivate effects before changing state
            deactivateActiveEffects();

            // Handle choice jump (potentially loading new file)
            if (!choice.nextFile.empty()) {
                loadStory(choice.nextFile); // Load new story file (this will also clear old resources)
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
            lastCharRevealTime = SDL_GetTicks();
            prevDialogIndex = currentDialogIndex; // Reset prevDialogIndex for new path to trigger effects

            break; // Exit loop after a choice is made
        }
    }
}