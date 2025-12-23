#include <cmath>
#include "Game.h"
#include <conio.h>
#include <windows.h>

// This function listens for when you press a key and decides what to do.
void Game::ProcessInput() {
    /* _kbhit() checks if a key has been pressed so the game doesn't
       stop and wait for you; it just keeps running. */
    if (_kbhit()) {
        char key = _getch(); // Get the actual character pressed.

        // If the game is over, we only care about the '5' key (Reset).
        if (is_game_over) {
            if (key == '5') ResetGame();
            return;
        }

        // If the game is paused, we only care about '0' (Resume) or '5' (Reset).
        if (is_paused) {
            if (key == '0') is_paused = false;
            if (key == '5') ResetGame();
            return;
        }

        // --- Controls ---
        switch (key) {
            case '7': case 'a': case 'A':
                MovePiece(-1, 0); // Move Left.
                break;
            case '9': case 'd': case 'D':
                MovePiece(1, 0);  // Move Right.
                break;
            case '4': case 's': case 'S':
                MovePiece(0, 1);  // Move Down (Soft Drop).
                break;
            case ' ':
                // Hard Drop: Teleport the piece to the bottom instantly.
                while (!CheckCollision(current_piece.shape, current_pos.x, current_pos.y + 1)) {
                    current_pos.y += 1;
                }
                LockPiece(); // Stick it to the board immediately.
                break;
            case '8': case 'w': case 'W':
                TryRotationWithWallKicks(); // Spin the piece.
                break;
            case '5':
                ResetGame(); // Start over.
                break;
            case '1':
                show_next_piece = !show_next_piece; // Hide or show the preview.
                break;
            case '0':
                is_paused = !is_paused; // Pause the action.
                break;
        }
    }
}

// This calculates how fast the piece should fall.
// As your level goes up, the time between drops gets smaller (faster).
int Game::GetFallSpeedMS() const {
    int speed = 500 * std::pow(0.8, level - 1);

    // We set a "speed limit" (50ms) so the game doesn't become impossible.
    return (speed < 50) ? 50 : speed;
}

// This function wipes everything clean to start a fresh game.
void Game::ResetGame() {
    // Use our helper to clear the board and rebuild the walls.
    ResetBoardWithWalls();

    // Reset all our flags and progress markers.
    is_game_over = false;
    is_paused = false;
    is_clearing_lines = false;
    score = 0;
    level = 1;
    lines_cleared = 0;

    // Pick new random pieces and reset the gravity clock.
    InitializeTetrominos();
    time_point_start = std::chrono::system_clock::now();

    // Refresh the console settings.
    SetupConsole();
}