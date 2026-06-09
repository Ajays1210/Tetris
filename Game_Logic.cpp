#include "Game.h"
#include <array>
#include <windows.h>

// This function checks if a piece is allowed to be at a certain spot.
// It looks for two things: hitting the walls or hitting other blocks.
bool Game::CheckCollision(const ShapeMatrix& shape, int nextX, int nextY) const {
    for (int y = 0; y < 4; ++y) {
        for (int x = 0; x < 4; ++x) {
            // We only care about the actual solid parts ('X') of the 4x4 piece.
            if (shape[y][x] == 'X') {
                int boardX = nextX + x;
                int boardY = nextY + y;

                // Check if the piece is touching or inside the board boundaries.
                if (boardY < 0 || boardY >= GAME_BOARD_HEIGHT || boardX < 0 || boardX >= LOGICAL_BOARD_WIDTH) {
                    return true;
                }

                // Check if that spot on the board is already taken by a wall or another block.
                if (board[boardY * LOGICAL_BOARD_WIDTH + boardX] != EMPTY_VALUE) {
                    return true;
                }
            }
        }
    }
    return false; // If we get here, the spot is safe!
}

// Moves the piece left, right, or down if the path is clear.
void Game::MovePiece(int deltaX, int deltaY) {
    if (!CheckCollision(current_piece.shape, current_pos.x + deltaX, current_pos.y + deltaY)) {
        current_pos.x += deltaX;
        current_pos.y += deltaY;
    } else if (deltaY > 0) {
        // If we were trying to move DOWN but hit something, the piece has landed.
        LockPiece();
    }
}

// This is a math trick to rotate a 4x4 grid by 90 degrees clockwise.
ShapeMatrix Game::RotatePiece(const ShapeMatrix& current_shape) {
    ShapeMatrix rotated_shape = {"....", "....", "....", "...."};
    for (int y = 0; y < 4; ++y) {
        for (int x = 0; x < 4; ++x) {
            // This formula flips rows into columns to perform the rotation.
            rotated_shape[x][3 - y] = current_shape[y][x];
        }
    }
    return rotated_shape;
}

// This function attempts to rotate the piece. If the rotation hits a wall,
// it tries "kicking" (nudging) the piece to the left or right to make it fit.
bool Game::TryRotationWithWallKicks() {
    ShapeMatrix rotatedShape = RotatePiece(current_piece.shape);
    bool isIPiece = (current_piece.id == 1);

    // We try the original spot, then nudge left/right by 1 and 2 units.
    // This covers both walls cleanly for all standard pieces.
    const std::array<Position, 5> standard_offsets = {{
        {0, 0},   // Test 1: Try original spot
        {-1, 0},  // Test 2: Nudge left 1 unit
        {1, 0},   // Test 3: Nudge right 1 unit
        {-2, 0},  // Test 4: Nudge left 2 units
        {2, 0}    // Test 5: Nudge right 2 units
    }};

    // The 'I' piece is very long, so it needs bigger kicks to clear walls.
    const std::array<Position, 5> i_offsets = {{
        {0, 0},   // Test 1: Try original spot
        {-2, 0},  // Test 2: Nudge left 2 units
        {2, 0},   // Test 3: Nudge right 2 units
        {-1, 0},  // Test 4: Nudge left 1 unit
        {1, 0}    // Test 5: Nudge right 1 unit
    }};

    const auto& offsets = isIPiece ? i_offsets : standard_offsets;

    // Loop through our "nudge" options to see if any spot is empty.
    for (const auto& offset : offsets) {
        int testX = current_pos.x + offset.x;
        int testY = current_pos.y + offset.y;

        if (!CheckCollision(rotatedShape, testX, testY)) {
            // Success! We found a spot that works.
            current_piece.shape = rotatedShape;
            current_pos.x = testX;
            current_pos.y = testY;
            return true;
        }
    }
    return false; // If all 5 spots are blocked, the piece won't rotate.
}

// When a piece lands, it is "glued" to the board and converted into numbers.
void Game::LockPiece() {
    for (int y = 0; y < 4; ++y) {
        for (int x = 0; x < 4; ++x) {
            if (current_piece.shape[y][x] == 'X') {
                board[(current_pos.y + y) * LOGICAL_BOARD_WIDTH + (current_pos.x + x)] = current_piece.id;
            }
        }
    }

    ClearLines();

    // Update the high score here so any points scored on this move (including
    // a hard drop bonus added just before this call) are captured immediately,
    // even if no lines were cleared and ShiftLinesDown never runs this turn.
    UpdateHighScore();

    // Prepare the next piece.
    current_piece = next_piece;
    current_pos.x = LOGICAL_BOARD_WIDTH / 2 - 2;
    current_pos.y = 0;

    // Only pick a new piece for the "Next" slot.
    // We do NOT call InitializeTetrominos() here because it would overwrite current_piece.
    std::uniform_int_distribution<int> dist(0, static_cast<int>(TETROMINO_TEMPLATES.size()) - 1);
    int next_index = dist(rng);
    next_piece.shape = TETROMINO_TEMPLATES[next_index];
    next_piece.id = next_index + 1;

    // Check if the new piece immediately hits something (board is full).
    // IMPORTANT: only do this check if no lines are pending clearance. If lines
    // are about to be removed, the board still contains those full rows right now,
    // which would make CheckCollision return a false positive and end the game
    // even though clearing the rows might free up the spawn area.
    // ShiftLinesDown() repeats this check once the board is actually clean.
    if (!is_clearing_lines && CheckCollision(current_piece.shape, current_pos.x, current_pos.y)) {
        is_game_over = true;
    }
}

// Scans the board to see if any rows are completely full of blocks.
void Game::ClearLines() {
    lines_to_clear.clear();
    for (int y = GAME_BOARD_HEIGHT - 2; y >= 1; --y) {
        bool is_full = true;
        for (int x = 1; x < LOGICAL_BOARD_WIDTH - 1; ++x) {
            if (board[y * LOGICAL_BOARD_WIDTH + x] == EMPTY_VALUE) {
                is_full = false;
                break;
            }
        }
        if (is_full) {
            lines_to_clear.push_back(y);
        }
    }

    // Start the animation timer if we have lines to clear.
    if (!lines_to_clear.empty()) {
        is_clearing_lines = true;
        line_clear_start_time = std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::system_clock::now().time_since_epoch()).count();
    }
}

// Deletes the full lines and shifts all the blocks above them downward.
void Game::ShiftLinesDown() {
    std::sort(lines_to_clear.begin(), lines_to_clear.end());
    int count = lines_to_clear.size();

    lines_cleared += count;
    // Official Tetris Guideline scoring progression array (0, Single, Double, Triple, Tetris)
    static const int LINE_SCORES[] = {0, 100, 300, 500, 800};

    // Protect against indexing out of bounds, then apply the disproportionate reward multiplier.
    if (count >= 1 && count <= 4) {
        score += static_cast<int64_t>(LINE_SCORES[count]) * level;
    }
    UpdateHighScore();
    // Use the named constant here while safely maintaining the level 20 cap.
    level = std::min((lines_cleared / LINES_PER_LEVEL) + 1, 20);

    int dropDistance = 0;
    for (int y = GAME_BOARD_HEIGHT - 2; y >= 1; --y) {
        if (std::find(lines_to_clear.begin(), lines_to_clear.end(), y) != lines_to_clear.end()) {
            dropDistance++;
        } else if (dropDistance > 0) {
            for (int x = 1; x < LOGICAL_BOARD_WIDTH - 1; ++x) {
                board[(y + dropDistance) * LOGICAL_BOARD_WIDTH + x] = board[y * LOGICAL_BOARD_WIDTH + x];
                board[y * LOGICAL_BOARD_WIDTH + x] = EMPTY_VALUE;
            }
        }
    }
    lines_to_clear.clear();
    is_clearing_lines = false;

    // Now that the rows are gone and everything has shifted down, run the
    // game over check that was deferred in LockPiece. The spawn area may
    // genuinely be blocked even after clearing — this is the correct moment
    // to know for sure.
    if (CheckCollision(current_piece.shape, current_pos.x, current_pos.y)) {
        is_game_over = true;
    }
}

void Game::LoadHighScore() {
    std::ifstream inFile(HIGH_SCORE_FILE);
    if (inFile.is_open()) {
        inFile >> high_score;
        inFile.close();
    }
}

void Game::SaveHighScore() {
    std::ofstream outFile(HIGH_SCORE_FILE);
    if (outFile.is_open()) {
        outFile << high_score;
        outFile.close();
    }
}

void Game::UpdateHighScore() {
    if (score > high_score) {
        high_score = score;
        SaveHighScore();
    }
}