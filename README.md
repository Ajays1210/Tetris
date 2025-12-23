Console Tetris (C++)

A fully functional, high-performance Tetris clone built from scratch using C++ and the Windows API. This project demonstrates advanced console manipulation, state-based game logic, and object-oriented programming (OOP).

Key Features:
- Flicker-Free Rendering: Optimized visual engine using SetConsoleCursorPosition to avoid the "blinking" effect of system("cls").

- Super Rotation System (SRS): Custom-built "Wall Kick" logic that allows pieces to rotate even when pressed against walls or the floor.

- Persistent High Scores: Automatically saves and loads your best performance from a local text file.

- Responsive Input: Utilizes non-blocking keyboard input for smooth, real-time gameplay.

- Dynamic Difficulty: Leveling system that increases the gravity speed as you clear more lines.

How to Play:
- Download the Tetris.exe from the Releases section.

- Run the executable in a Windows terminal.

Controls:

| Key | Action |
| --- | --- |
| A / 7 | Move Left |
| D / 9 | Move Right |
| W / 8 | Rotate Piece |
| S / 4 | Soft Drop (Speed up) |
| SPACE | Hard Drop (Instant) |
| 0 | Pause / Resume |
| 5 | Reset Game |
| 1 | Toggle Next Piece Preview |

Technical Implementation:
- Separation of Concerns: The project is modularized into dedicated files for Logic, Input, Rendering, and Initialization.

- Collision Detection: Matrix-based collision checking to ensure pixel-perfect (or rather, character-perfect) movement.

- Delta Timing: Gravity is handled using std::chrono to ensure consistent speed regardless of CPU performance.

How to Build:
If you want to compile the source code yourself:

- Clone the repository.

- Open the project in CLion or any C++ IDE.

- Use the provided CMakeLists.txt to build.

- Or use the terminal: g++ *.cpp -o Tetris.exe
