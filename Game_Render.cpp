#include "Game.h"
#include <windows.h>
#include <conio.h>

// This function draws the "Heads-Up Display" (HUD) on the right side of the board.
void Game::DrawStats() {
    /* Calculating the starting X position based on the board width so the
     text always stays to the right of the game. */
    int startX = (LOGICAL_BOARD_WIDTH * 2) + 4;

    // Draw High Score at the very top of the HUD
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
    // --- GAME OVER OVERLAY ---
    if (is_game_over) {
        // Move to the middle of the board
        int centerX = LOGICAL_BOARD_WIDTH / 2;
        int centerY = GAME_BOARD_HEIGHT / 2;

        SetCursorPosition(2, centerY - 1); std::cout << "====================";
        SetCursorPosition(2, centerY);     std::cout << " --- GAME OVER! --- ";
        SetCursorPosition(2, centerY + 1); std::cout << "  Final Score: " << score << "    "; // <--- Spaces erase the []
        SetCursorPosition(2, centerY + 2); std::cout << "  Press 5 to Reset  ";
        SetCursorPosition(2, centerY + 3); std::cout << "====================";
        return; // EXIT the function here so it doesn't draw the board over the message
    }

    // --- 2. PAUSE OVERLAY ---
    if (is_paused) {
        int centerY = GAME_BOARD_HEIGHT / 2;
        SetCursorPosition(2, centerY - 1); std::cout << "********************";
        SetCursorPosition(2, centerY);     std::cout << "   --- PAUSED ---   ";
        SetCursorPosition(2, centerY + 1); std::cout << "  Press 0 to Resume ";
        SetCursorPosition(2, centerY + 2); std::cout << "********************";
        return;
    }

    // This moves the "cursor" to the top-left (0,0) and redraw everything.
    // This is much faster than clearing the screen and prevents flickering.
    SetCursorPosition(0, 0);

    for (int y = 0; y < GAME_BOARD_HEIGHT - 1; ++y) {
        for (int x = 0; x < LOGICAL_BOARD_WIDTH; ++x) {
            std::string displayStr = "  ";
            bool isPieceCell = false;

            // 1. Check if the falling piece is currently over this (x, y) spot.
            if (x >= current_pos.x && x < current_pos.x + 4 && y >= current_pos.y && y < current_pos.y + 4) {
                if (current_piece.shape[y - current_pos.y][x - current_pos.x] == 'X') {
                    displayStr = "[]";
                    isPieceCell = true;
                }
            }

            // 2. If no falling piece is here, check what is stored in the board data.
            if (!isPieceCell) {
                int cellValue = board[y * LOGICAL_BOARD_WIDTH + x];

                // Check for line-clearing animation (flashing).
                if (is_clearing_lines && std::find(lines_to_clear.begin(), lines_to_clear.end(), y) != lines_to_clear.end()) {
                    long long elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(
                        std::chrono::system_clock::now().time_since_epoch()).count() - line_clear_start_time;
                    // Swap between "##" and " ." every 100ms to create a flash effect.
                    displayStr = ((elapsed / 100) % 2 == 0) ? "##" : " .";
                }
                // Draw walls and bottom edges.
                else if (cellValue == 9) {
                    if (y == 0) displayStr = "  "; // Keep top invisible so pieces can spawn.
                    else if (x == 0) displayStr = "<!"; // Left wall.
                    else if (x == LOGICAL_BOARD_WIDTH - 1) displayStr = "!>"; // Right wall.
                }
                // Draw an empty spot.
                else if (cellValue == 0) {
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
    // Draw the fancy floor at the very bottom.
    std::cout << "<!====================!>\n  \\/\\/\\/\\/\\/\\/\\/\\/\\/\\/\n";
}

// This is the heart of the game. It loops forever, handling time and drawing.
void Game::Run() {
    time_point_start = std::chrono::system_clock::now();

    while (true) {
        ProcessInput(); // Check for keys first.

        auto now = std::chrono::system_clock::now();

        if (!is_game_over) {
            // Case A: Here currently playing the "line clear" animation.
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