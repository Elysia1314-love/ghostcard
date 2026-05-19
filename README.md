# Ghost Card Game

A simple Uno-like card game implemented in C++ using SFML.

## Prerequisites

- C++17 compatible compiler (e.g., Visual Studio 2019+, GCC 7+, Clang 5+)
- CMake 3.10 or higher
- SFML 2.5 or higher

## Installing SFML

### Windows (using vcpkg)
1. Install vcpkg: `git clone https://github.com/Microsoft/vcpkg.git`
2. Run `.\vcpkg\bootstrap-vcpkg.bat`
3. Install SFML: `.\vcpkg\vcpkg install sfml`
4. Set environment: `set CMAKE_TOOLCHAIN_FILE=C:\path\to\vcpkg\scripts\buildsystems\vcpkg.cmake`

### Or download precompiled binaries from https://www.sfml-dev.org/download.php

## Building

1. Open the project in VS Code
2. Run the "build" task (Ctrl+Shift+P > Tasks: Run Task > build)
3. Run the "compile" task to build the executable

## Running

1. Ensure assets/card_*.png files exist (placeholders needed)
2. Run the executable from the build directory
3. Enter number of players (3-10) in console
4. Human player is player 0, others are AI

## Game Rules

- Players take turns playing cards that match the color or number of the top card on the discard pile.
- Special cards: Skip, Reverse, Draw Two, Wild, Wild Draw Four.
- First player to empty their hand wins.

## Features

- Supports 3+ players
- One human player, others AI
- Basic graphical interface with SFML
- Turn-based gameplay