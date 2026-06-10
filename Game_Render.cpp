#include "Game.h"
#include <windows.h>
#include <conio.h>

// Shared helper for the pause and game-over overlay panels.
// Draws one line: a solid border if text is empty, or a centred text string otherwise.
void Game::DrawCenteredOverlay(int yPos, const std::string& text, char borderChar, int startX, int menuWidth) {
    SetCursorPosition(startX, yPos);
    if (text.empty()) {
        std::cout << std::string(menuWidth, borderChar);
    } else {
        int padding = menuWidth - static_cast<int>(text.length());
        int leftPadding = padding / 2;
        int rightPadding = padding - leftPadding;
        std::cout << std::string(leftPadding, ' ') << text << std::string(rightPadding, ' ');
    }
}

// This function draws the "Heads-Up Display" (HUD) on the right side of the board.
void Game::DrawStats() {
    /* Calculating the starting X position based on the board width so the
     text always stays to the right of the game. */
    int startX = (LOGICAL_BOARD_WIDTH * 2) + 4;

    // Draw High Score at the very top of the HUD.
    SetCursorPosition(startX, 0);
    std::cout << "BEST SCORE: " << high_score << "          ";

    // Draw the player's progress and current score.
    SetCursorPosition(startX, 1); std::cout << "LINES CLEARED: " << lines_cleared << "     ";
    SetCursorPosition(startX, 3); std::cout << "LEVEL: " << level << "             ";
    SetCursorPosition(startX, 5); std::cout << "SCORE: " << score << "             ";
    SetCursorPosition(startX, 7); std::cout << "NEXT PIECE:          ";

    // Draw the "Next Piece" preview box.
    for (int y = 0; y < 4; ++y) {
        SetCursorPosition(startX, 8 + y);
        if (show_next_piece) {
            for (int x = 0; x < 4; ++x) {
                if (next_piece.shape[y][x] == 'X') std::cout << "[]"; // Draw piece block.
                else std::cout << "  "; // Draw empty space within preview.
            }
            std::cout << "    ";
        } else {
            // If the player toggled the preview off, draw blank spaces to hide it.
            std::cout << "                ";
        }
    }

    // Draw the control guide at the bottom right.
    int controlsY = 14;
    SetCursorPosition(startX, controlsY);     std::cout << "A(7): LEFT D(9): RIGHT";
    SetCursorPosition(startX, controlsY + 1); std::cout << "W(8): ROTATE";
    SetCursorPosition(startX, controlsY + 3); std::cout << "S(4): SOFT DROP 5: RESET";
    SetCursorPosition(startX, controlsY + 4); std::cout << "1: SHOW NEXT";
    SetCursorPosition(startX, controlsY + 5); std::cout << "0: PAUSE / RESUME";
    SetCursorPosition(startX, controlsY + 6); std::cout << "SPACE - HARD DROP";
}

// This is the core visual engine. It draws every block and empty space on the grid.
void Game::DrawBoard() {
    int centerY = GAME_BOARD_HEIGHT / 2;

    // --- 1. PAUSE OVERLAY (early exit - nothing on the board is moving) ---
    if (is_paused) {
        int menuWidth = (LOGICAL_BOARD_WIDTH - 2) * 2;
        int startX = 2;

        DrawCenteredOverlay(centerY - 1, "", '=', startX, menuWidth);
        DrawCenteredOverlay(centerY,     "--- PAUSED ---", ' ', startX, menuWidth);
        DrawCenteredOverlay(centerY + 1, "0 to Resume Game", ' ', startX, menuWidth);
        DrawCenteredOverlay(centerY + 2, "", '=', startX, menuWidth);
        return;
    }

    // --- 2. BOARD RENDERING (runs for both normal gameplay AND game over) ---
    // The old code returned early on game over after drawing only 5 overlay
    // lines in the middle of the screen. This left the previous frame's pixels
    // at rows 0-9 untouched, so the player saw a stale "floating piece" with a
    // gap below it and assumed the game over was a false positive.
    //
    // Fix: always redraw the full board before any overlay. During game over we
    // suppress the active piece so the player sees the real locked-board state,
    // making it clear which blocks are blocking the spawn area. The overlay is
    // then drawn on top of the freshly rendered board.
    SetCursorPosition(0, 0);

    for (int y = 0; y < GAME_BOARD_HEIGHT; ++y) {
        for (int x = 0; x < LOGICAL_BOARD_WIDTH; ++x) {
            std::string displayStr = "  ";
            bool isPieceCell = false;

            // Draw the active falling piece - suppressed on game over so the
            // player can see the actual board state that triggered the end.
            if (!is_game_over &&
                x >= current_pos.x && x < current_pos.x + 4 &&
                y >= current_pos.y && y < current_pos.y + 4) {
                if (current_piece.shape[y - current_pos.y][x - current_pos.x] == 'X') {
                    displayStr = "[]";
                    isPieceCell = true;
                }
            }

            // If no falling piece is here, check what is stored in the board data.
            if (!isPieceCell) {
                int cellValue = board[y * LOGICAL_BOARD_WIDTH + x];

                // Check for line-clearing animation (flashing).
                // Wall cells are excluded so the border characters don't get
                // overwritten by the flash effect.
                if (is_clearing_lines &&
                    cellValue != WALL_VALUE &&
                    std::find(lines_to_clear.begin(), lines_to_clear.end(), y) != lines_to_clear.end()) {
                    long long elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(
                        std::chrono::system_clock::now().time_since_epoch()).count() - line_clear_start_time;
                    displayStr = ((elapsed / 100) % 2 == 0) ? "##" : " .";
                }
                // Draw walls and bottom edges dynamically using the board data values.
                else if (cellValue == WALL_VALUE) {
                    if (y == 0) {
                        displayStr = "  "; // Keep top invisible so pieces can spawn.
                    }
                    else if (y == GAME_BOARD_HEIGHT - 1) {
                        if (x == 0) displayStr = "<!"; // Bottom Left Corner
                        else if (x == LOGICAL_BOARD_WIDTH - 1) displayStr = "!>"; // Bottom Right Corner
                        else displayStr = "=="; // Floor piece that stretches automatically.
                    }
                    else if (x == 0) {
                        displayStr = "<!"; // Left wall.
                    }
                    else if (x == LOGICAL_BOARD_WIDTH - 1) {
                        displayStr = "!>"; // Right wall.
                    }
                }
                // Draw an empty spot.
                else if (cellValue == EMPTY_VALUE) {
                    displayStr = " .";
                }
                // Draw a locked block that landed previously.
                else {
                    displayStr = "[]";
                }
            }
            std::cout << displayStr;
        }
        std::cout << "\n";
    }

    // Decorative accents printed underneath the board.
    // Start at character position 2 (skipping the left wall '<!').
    SetCursorPosition(2, GAME_BOARD_HEIGHT);

    // Loop through the inner width of the board and print matching jagged segments.
    for (int i = 0; i < LOGICAL_BOARD_WIDTH - 2; ++i) {
        std::cout << "\\/";
    }

    // --- 3. GAME OVER OVERLAY (drawn on top of the refreshed board) ---
    if (is_game_over) {
        int menuWidth = (LOGICAL_BOARD_WIDTH - 2) * 2;
        int startX = 2;

        DrawCenteredOverlay(centerY - 1, "", '=', startX, menuWidth);
        DrawCenteredOverlay(centerY,     "--- GAME OVER! ---", ' ', startX, menuWidth);
        DrawCenteredOverlay(centerY + 1, "Score: " + std::to_string(score), ' ', startX, menuWidth);
        DrawCenteredOverlay(centerY + 2, "5 to Reset Game", ' ', startX, menuWidth);
        DrawCenteredOverlay(centerY + 3, "", '=', startX, menuWidth);
    }
}

// This is the heart of the game. It loops forever, handling time and drawing.
void Game::Run() {
    time_point_start = std::chrono::system_clock::now();

    while (true) {
        ProcessInput(); // Check for keys first.

        auto now = std::chrono::system_clock::now();

        if (!is_game_over) {
            // Case A: Currently playing the "line clear" animation.
            if (is_clearing_lines) {
                long long currentTime = std::chrono::duration_cast<std::chrono::milliseconds>(now.time_since_epoch()).count();
                // Wait for the animation delay to finish before dropping blocks.
                if (currentTime - line_clear_start_time >= LINE_CLEAR_DELAY_MS) {
                    ShiftLinesDown();
                }
            }
            // Case B: Normal gameplay (not paused).
            else if (!is_paused) {
                // Check if enough time has passed for gravity to pull the piece down.
                if (std::chrono::duration_cast<std::chrono::milliseconds>(now - time_point_start).count() >= GetFallSpeedMS()) {
                    MovePiece(0, 1);
                    time_point_start = now; // Reset gravity timer.
                }
            }
            // Case C: Game is paused.
            else {
                // Keep the gravity timer "resetting" so the piece doesn't fall immediately after unpausing.
                time_point_start = now;
            }
        }

        // Refresh the screen.
        DrawBoard();
        DrawStats();

        // A tiny sleep to stop the game from using 100% of the CPU power.
        Sleep(10);
    }
}