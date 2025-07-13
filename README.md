# Wordle Calculator

A C++ Qt-based Wordle solver and game with advanced analysis features.

## Features

- **Game Mode**: Play Wordle against the computer with official word lists
- **Solver Mode**: Get optimal guesses based on your feedback (green/yellow/gray letters)
- **Stats Mode**: View letter frequency analysis and optimal first guesses
- **Smart Algorithm**: Advanced scoring that prioritizes yellow letters and considers letter frequency

## Downloads

### Windows
1. Download the `Wordle_Calculator_Windows` artifact from the latest GitHub Actions run
2. Extract the ZIP file
3. Run `Wordle_Calculator.exe`

### macOS
1. Download the `Wordle_Calculator_macOS` artifact from the latest GitHub Actions run
2. Extract the ZIP file to get `Wordle_Calculator.app`
3. **Important**: Right-click the app and select "Open" (or go to System Preferences → Security & Privacy → General and click "Open Anyway")
4. The app will run normally after the first time

**Note**: macOS may show a "damaged" warning because the app isn't code-signed. This is normal for open-source applications. The app is safe to run.

## Building from Source

### Prerequisites
- CMake 3.31+
- Qt6
- C++20 compatible compiler

### macOS
```bash
brew install qt@6
cmake -B build
cmake --build build
```

### Windows
```bash
# Install Qt6 and set CMAKE_PREFIX_PATH to Qt installation
cmake -B build
cmake --build build --config Release
```

## How to Use

### Game Mode
- Click "New Game" to start
- Type your guess and press Enter
- Use the feedback to guide your next guess
- Track your statistics

### Solver Mode
- Input your Wordle feedback:
  - **Green letters**: Type the letter in the green box
  - **Yellow letters**: Type the letter in the yellow box  
  - **Gray letters**: Type the letter in the gray box
- Click "Get Optimal Guess" for the best next word
- View all possible remaining words

### Stats Mode
- See letter frequency analysis
- View optimal first guesses
- Analyze word patterns

## License

This project is open source. Feel free to contribute or modify for your own use. 