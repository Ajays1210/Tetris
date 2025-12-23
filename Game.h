#ifndef TETRIS_GAME_H
#define TETRIS_GAME_H

#include <iostream>
#include <vector>
#include <string>
#include <chrono>
#include <fstream>
#include <cstdlib>
#include <ctime>
#include <algorithm>

// Using a vector of strings to represent the 4x4 grid of a piece.
// 'X' is a solid block, and '.' is empty space.
using ShapeMatrix = std::vector<std::string>;

// Keeping the shapes here so they don't clutter up our logic functions.
// These are the "Blueprints" for every piece in the game.
const std::vector<ShapeMatrix> TETROMINO_TEMPLATES = {
    {"....", "XXXX", "....", "...."}, // I
    {"....", ".X..", ".XXX", "...."}, // J
    {"....", "..X.", ".XXX", "...."}, // L
    {"....", ".XX.", ".XX.", "...."}, // O
    {"....", "..XX", ".XX.", "...."}, // S
    {"....", ".XXX", "..X.", "...."}, // T
    {"....", ".XX.", "..XX", "...."}  // Z
};

// These constants define the size of our game world.
const int LOGICAL_BOARD_WIDTH = 12; // 10 columns for play + 2 for side walls.
const int GAME_BOARD_HEIGHT = 22;   // 20 rows for play + 1 top wall + 1 bottom wall.

// A simple structure to hold a piece's shape and its color/type ID.
struct Piece {
    ShapeMatrix shape;
    int id;
};

// A simple structure to keep track of where a piece is on the grid.
struct Position {
    int x;
    int y;
};

class Game {
public:
    // This sets up everything when you first start the program.
    Game();

    // This is the main engine that keeps the game running.
    void Run();

private:
    // --- Game World Data ---
    std::vector<int> board;      // The 2D grid stored as a 1D list for speed.
    Piece current_piece;         // The piece the player is controlling.
    Position current_pos;        // The current (x, y) location of that piece.
    Piece next_piece;            // The "Preview" piece shown on the side.

    // --- Game State Flags ---
    bool show_next_piece = true; // Can be toggled to hide/show the preview.
    bool is_paused = false;      // Stops the game logic when true.
    bool is_game_over = false;   // Set to true when the stack reaches the top.

    // --- Line Clearing & Animation ---
    std::vector<int> lines_to_clear;     // List of rows that are currently full.
    bool is_clearing_lines = false;      // True when we are playing the "flash" effect.
    long long line_clear_start_time = 0; // Helps us time the animation delay.
    const int LINE_CLEAR_DELAY_MS = 300; // How long the clear animation lasts.

    // --- Scoring & Leveling ---
    int score = 0;
    int level = 1;
    int lines_cleared = 0;

    // --- Timing ---
    // This tracks the last time the piece moved down due to gravity.
    std::chrono::time_point<std::chrono::system_clock> time_point_start;

    // --- Movement & Physics (Game_Logic.cpp) ---
    int GetFallSpeedMS() const;
    bool CheckCollision(const ShapeMatrix& shape, int nextX, int nextY) const;
    void MovePiece(int deltaX, int deltaY);
    void LockPiece();
    bool TryRotationWithWallKicks();
    ShapeMatrix RotatePiece(const ShapeMatrix& current_shape);
    void ClearLines();
    void ShiftLinesDown();

    // --- High-Score (Game_Logic.cpp) ---
    int high_score = 0;              // To store the record
    const std::string HIGH_SCORE_FILE = "highscore.txt";

    void LoadHighScore();            // Read from disk
    void SaveHighScore();            // Write to disk
    void UpdateHighScore();          // Logic to check if a new record is set

    // --- Input & Control (Game_Input.cpp) ---
    void ProcessInput();
    void ResetGame();

    // --- Initialization (Game_Init.cpp) ---
    void SetupConsole();
    void InitializeTetrominos();
    void ResetBoardWithWalls();

    // --- Visuals (Game_Render.cpp) ---
    void SetCursorPosition(int x, int y);
    void DrawBoard();
    void DrawStats();
};

#endif