#include <iostream>
#include <vector>
#include <string>
#include <fstream>
#include <map>
#include <algorithm>
#include <limits>
#include <random>
#ifdef USE_QT
#include <QApplication>
#include "WordleWindow.h"
#endif

// Function to analyze letter frequency by position
std::map<char, int> analyzeLetterFrequencyByPosition(const std::vector<std::string>& words, int position) {
    std::map<char, int> frequency;
    
    // Initialize frequency map with all letters
    for (char c = 'A'; c <= 'Z'; c++) {
        frequency[c] = 0;
    }
    
    // Count letters at the specified position
    for (const std::string& word : words) {
        if (position < word.length()) {
            char letter = word[position];
            frequency[letter]++;
        }
    }
    
    return frequency;
}

// Function to analyze all positions and return a 2D map
std::vector<std::map<char, int>> analyzeAllPositions(const std::vector<std::string>& words) {
    std::vector<std::map<char, int>> positionFrequencies;
    
    // Analyze each position (assuming 5-letter words)
    for (int pos = 0; pos < 5; pos++) {
        positionFrequencies.push_back(analyzeLetterFrequencyByPosition(words, pos));
    }
    
    return positionFrequencies;
}

// Function to print frequency analysis
void printFrequencyAnalysis(const std::vector<std::map<char, int>>& positionFrequencies) {
    std::cout << "Letter Frequency Analysis by Position:" << std::endl;
    std::cout << "=====================================" << std::endl;
    
    for (int pos = 0; pos < positionFrequencies.size(); pos++) {
        std::cout << "\nPosition " << (pos + 1) << ":" << std::endl;
        std::cout << "----------" << std::endl;
        
        // Convert map to vector for sorting
        std::vector<std::pair<char, int>> sortedFreq;
        for (const auto& pair : positionFrequencies[pos]) {
            if (pair.second > 0) {  // Only show letters that appear
                sortedFreq.push_back(pair);
            }
        }
        
        // Sort by frequency (descending)
        std::sort(sortedFreq.begin(), sortedFreq.end(), 
                  [](const auto& a, const auto& b) {
                      return a.second > b.second;
                  });
        
        // Print top 10 most frequent letters
        for (int i = 0; i < std::min(10, (int)sortedFreq.size()); i++) {
            std::cout << sortedFreq[i].first << ": " << sortedFreq[i].second << std::endl;
        }
    }
}

std::vector<std::string> loadWordList(bool printMessage = true) {
    std::vector<std::string> words;
    std::ifstream file("/Users/zak/CLionProjects/Wordle_Calculator/WordList.txt");
    if (!file.is_open()) {
        if (printMessage) {
            std::cout << "Error: Could not open WordList.txt" << std::endl;
            std::cout << "Tried to open: /Users/zak/CLionProjects/Wordle_Calculator/WordList.txt" << std::endl;
        }
        return words;
    }
    std::string word;
    while (std::getline(file, word)) {
        std::transform(word.begin(), word.end(), word.begin(), ::toupper);
        word.erase(std::remove_if(word.begin(), word.end(), ::isspace), word.end());
        if (!word.empty()) {
            words.push_back(word);
        }
    }
    file.close();
    if (printMessage) {
        std::cout << "Loaded " << words.size() << " words from WordList.txt (valid answers)" << std::endl;
    }
    return words;
}

std::vector<std::string> loadAcceptedWordList(bool printMessage = true) {
    std::vector<std::string> words;
    std::ifstream file("/Users/zak/CLionProjects/Wordle_Calculator/AcceptedWordList");
    if (!file.is_open()) {
        if (printMessage) {
            std::cout << "Error: Could not open AcceptedWordList" << std::endl;
            std::cout << "Tried to open: /Users/zak/CLionProjects/Wordle_Calculator/AcceptedWordList" << std::endl;
        }
        return words;
    }
    std::string word;
    while (std::getline(file, word)) {
        std::transform(word.begin(), word.end(), word.begin(), ::toupper);
        word.erase(std::remove_if(word.begin(), word.end(), ::isspace), word.end());
        if (!word.empty()) {
            words.push_back(word);
        }
    }
    file.close();
    if (printMessage) {
        std::cout << "Loaded " << words.size() << " words from AcceptedWordList (valid guesses)" << std::endl;
    }
    return words;
}

// Function to calculate word score based on letter frequency
int calculateWordScore(const std::string& word, const std::vector<std::map<char, int>>& positionFrequencies) {
    int score = 0;
    
    // Score based on letter frequency at each position
    for (int i = 0; i < word.length() && i < positionFrequencies.size(); i++) {
        char letter = word[i];
        score += positionFrequencies[i].at(letter);
    }
    
    // Bonus for unique letters (no repeated letters)
    std::map<char, int> letterCount;
    for (char c : word) {
        letterCount[c]++;
    }
    
    // Penalty for repeated letters
    for (const auto& pair : letterCount) {
        if (pair.second > 1) {
            score -= (pair.second - 1) * 50; // Penalty for repeated letters
        }
    }
    
    return score;
}

// Function to find the best starting words
std::vector<std::pair<std::string, int>> findBestStartingWords(const std::vector<std::string>& words, 
                                                              const std::vector<std::map<char, int>>& positionFrequencies, 
                                                              int topCount = 20) {
    std::vector<std::pair<std::string, int>> wordScores;
    
    for (const std::string& word : words) {
        int score = calculateWordScore(word, positionFrequencies);
        wordScores.push_back({word, score});
    }
    
    // Sort by score (descending)
    std::sort(wordScores.begin(), wordScores.end(), 
              [](const auto& a, const auto& b) {
                  return a.second > b.second;
              });
    
    // Return top words
    if (wordScores.size() > topCount) {
        wordScores.resize(topCount);
    }
    
    return wordScores;
}

// Function to analyze word patterns
void analyzeWordPatterns(const std::vector<std::string>& words) {
    std::map<char, int> overallFrequency;
    std::map<std::string, int> commonPatterns;
    
    // Count overall letter frequency
    for (const std::string& word : words) {
        for (char c : word) {
            overallFrequency[c]++;
        }
        
        // Look for common patterns (first 3 letters)
        if (word.length() >= 3) {
            std::string pattern = word.substr(0, 3);
            commonPatterns[pattern]++;
        }
    }
    
    // Print overall letter frequency
    std::cout << "\nOverall Letter Frequency:" << std::endl;
    std::cout << "========================" << std::endl;
    
    std::vector<std::pair<char, int>> sortedFreq;
    for (const auto& pair : overallFrequency) {
        sortedFreq.push_back(pair);
    }
    
    std::sort(sortedFreq.begin(), sortedFreq.end(), 
              [](const auto& a, const auto& b) {
                  return a.second > b.second;
              });
    
    for (int i = 0; i < std::min(15, (int)sortedFreq.size()); i++) {
        std::cout << sortedFreq[i].first << ": " << sortedFreq[i].second << std::endl;
    }
    
    // Print common 3-letter patterns
    std::cout << "\nMost Common 3-Letter Patterns:" << std::endl;
    std::cout << "=============================" << std::endl;
    
    std::vector<std::pair<std::string, int>> sortedPatterns;
    for (const auto& pair : commonPatterns) {
        sortedPatterns.push_back(pair);
    }
    
    std::sort(sortedPatterns.begin(), sortedPatterns.end(), 
              [](const auto& a, const auto& b) {
                  return a.second > b.second;
              });
    
    for (int i = 0; i < std::min(10, (int)sortedPatterns.size()); i++) {
        std::cout << sortedPatterns[i].first << ": " << sortedPatterns[i].second << std::endl;
    }
}

void scoreWords(const std::vector<std::string>& words, const std::vector<std::map<char, int>>& positionFrequencies) {
    std::cout << "\nWord Scoring Analysis:" << std::endl;
    std::cout << "====================" << std::endl;
    
    // Find best starting words
    std::vector<std::pair<std::string, int>> bestWords = findBestStartingWords(words, positionFrequencies, 10);
    
    std::cout << "\nTop 10 Best Starting Words:" << std::endl;
    std::cout << "-------------------------" << std::endl;
    for (int i = 0; i < bestWords.size(); i++) {
        std::cout << (i + 1) << ". " << bestWords[i].first << " (Score: " << bestWords[i].second << ")" << std::endl;
    }
    
    // Analyze word patterns
    analyzeWordPatterns(words);
    
    // Show some example word scores
    std::cout << "\nExample Word Scores:" << std::endl;
    std::cout << "===================" << std::endl;
    
    std::vector<std::string> exampleWords = {"STARE", "CRANE", "SLATE", "ADIEU", "AUDIO", "RAISE", "ROATE"};
    for (const std::string& word : exampleWords) {
        int score = calculateWordScore(word, positionFrequencies);
        std::cout << word << ": " << score << std::endl;
    }
}

// Function to display the main menu
void displayMenu() {
    std::cout << "\n=== Wordle Calculator ===" << std::endl;
    std::cout << "1. Play Wordle (Terminal)" << std::endl;
    std::cout << "2. Analyze Word Lists" << std::endl;
    std::cout << "3. Launch GUI" << std::endl;
    std::cout << "4. Exit" << std::endl;
    std::cout << "Enter your choice (1-4): ";
}

// Helper function to get a random word from a list
std::string getRandomWord(const std::vector<std::string>& words) {
    static std::random_device rd;
    static std::mt19937 gen(rd());
    std::uniform_int_distribution<> dis(0, words.size() - 1);
    return words[dis(gen)];
}

// Helper function to check if a word is in either list
bool isValidGuess(const std::string& guess, const std::vector<std::string>& acceptedWords, const std::vector<std::string>& answerWords) {
    return std::find(acceptedWords.begin(), acceptedWords.end(), guess) != acceptedWords.end() ||
           std::find(answerWords.begin(), answerWords.end(), guess) != answerWords.end();
}

// Helper function to print feedback for a guess
void printWordleFeedback(const std::string& guess, const std::string& answer) {
    std::string feedback = "";
    std::string answerCopy = answer;
    std::vector<char> result(guess.size(), 'X');
    std::vector<bool> used(answer.size(), false);

    // First pass: check for correct position (G)
    for (size_t i = 0; i < guess.size(); ++i) {
        if (guess[i] == answer[i]) {
            result[i] = 'G';
            used[i] = true;
        }
    }
    // Second pass: check for correct letter, wrong position (Y)
    for (size_t i = 0; i < guess.size(); ++i) {
        if (result[i] == 'G') continue;
        for (size_t j = 0; j < answer.size(); ++j) {
            if (!used[j] && guess[i] == answer[j]) {
                result[i] = 'Y';
                used[j] = true;
                break;
            }
        }
    }
    // Print feedback
    for (char c : result) std::cout << c;
    std::cout << "  (" << guess << ")" << std::endl;
}

void playWordle() {
    std::cout << "\n=== Wordle Game Mode ===" << std::endl;
    std::vector<std::string> answerWords = loadWordList(false);
    std::vector<std::string> acceptedWords = loadAcceptedWordList(false);
    if (answerWords.empty() || acceptedWords.empty()) {
        std::cout << "Word lists not loaded. Cannot play." << std::endl;
        return;
    }
    std::string answer = getRandomWord(answerWords);
    int maxGuesses = 5;
    int wordLength = answer.length();
    std::cout << "Guess the " << wordLength << "-letter word! You have " << maxGuesses << " guesses." << std::endl;
    for (int attempt = 1; attempt <= maxGuesses; ++attempt) {
        std::string guess;
        std::cout << "\nGuess " << attempt << ": ";
        std::cin >> guess;
        std::transform(guess.begin(), guess.end(), guess.begin(), ::toupper);
        if (guess.length() != wordLength) {
            std::cout << "Please enter a " << wordLength << "-letter word." << std::endl;
            --attempt;
            continue;
        }
        if (!isValidGuess(guess, acceptedWords, answerWords)) {
            std::cout << "Not a valid word. Try again." << std::endl;
            --attempt;
            continue;
        }
        printWordleFeedback(guess, answer);
        if (guess == answer) {
            std::cout << "\nCongratulations! You guessed the word!" << std::endl;
            return;
        }
    }
    std::cout << "\nSorry, you lost! The word was: " << answer << std::endl;
}

// Function to handle word list analysis
void analyzeWordLists() {
    std::cout << "\n=== Word List Analysis ===" << std::endl;
    
    // Load the answer word list (for frequency analysis)
    std::vector<std::string> answerWords = loadWordList(true);
    
    if (answerWords.empty()) {
        std::cout << "No answer words loaded. Exiting." << std::endl;
        return;
    }
    
    // Load the accepted word list (for scoring potential guesses)
    std::vector<std::string> acceptedWords = loadAcceptedWordList(true);
    
    if (acceptedWords.empty()) {
        std::cout << "No accepted words loaded. Exiting." << std::endl;
        return;
    }
    
    // Analyze letter frequency by position using ONLY answer words
    std::vector<std::map<char, int>> positionFrequencies = analyzeAllPositions(answerWords);
    
    // Print the analysis
    printFrequencyAnalysis(positionFrequencies);
    
    // Score words using accepted words but frequency data from answer words
    scoreWords(acceptedWords, positionFrequencies);
}

int main(int argc, char *argv[]) {
    int choice;
    do {
        displayMenu();
        std::cin >> choice;
        switch (choice) {
            case 1:
                playWordle();
                break;
            case 2:
                analyzeWordLists();
                break;
            case 3:
            #ifdef USE_QT
                {
                    QApplication app(argc, argv);
                    WordleWindow window;
                    window.show();
                    app.exec();
                }
            #else
                std::cout << "GUI not available. Rebuild with Qt support." << std::endl;
            #endif
                break;
            case 4:
                std::cout << "Goodbye!" << std::endl;
                break;
            default:
                std::cout << "Invalid choice. Please enter 1, 2, 3, or 4." << std::endl;
                break;
        }
        if (choice != 4) {
            std::cout << "\nPress Enter to continue...";
            std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
            std::cin.get();
        }
    } while (choice != 4);
    return 0;
}