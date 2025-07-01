#include "WordleWindow.h"
#include <QLineEdit>
#include <QPushButton>
#include <QLabel>
#include <QVBoxLayout>
#include <QMessageBox>
#include <QRandomGenerator>
#include <QFile>
#include <QTextStream>
#include <QSet>
#include <QTextEdit>
#include <QApplication>
#include <QGridLayout>
#include <QHBoxLayout>
#include <QMap>
#include <QVector>
#include <QStringList>
#include <algorithm>

// Global function to load word lists
static QStringList loadWordList(const QString &filename) {
    QStringList words;
    QFile file(filename);
    if (file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        QTextStream in(&file);
        while (!in.atEnd()) {
            QString word = in.readLine().trimmed().toUpper();
            if (!word.isEmpty()) words << word;
        }
    }
    return words;
}

// ============================================================================
// MainMenuWindow Implementation
// ============================================================================

MainMenuWindow::MainMenuWindow(QWidget *parent) : QWidget(parent) {
    setWindowTitle("Wordle");
    setFixedSize(400, 300);
    setAttribute(Qt::WA_DeleteOnClose, false);
    
    layout = new QVBoxLayout(this);
    
    // Buttons
    playButton = new QPushButton("Play Wordle", this);
    statsButton = new QPushButton("View Word Stats", this);
    exitButton = new QPushButton("Exit", this);
    
    // Style buttons
    QString buttonStyle = "QPushButton { font-size: 16px; padding: 15px; margin: 5px; background-color: #4CAF50; color: white; border: none; border-radius: 5px; } QPushButton:hover { background-color: #45a049; }";
    playButton->setStyleSheet(buttonStyle);
    statsButton->setStyleSheet(buttonStyle.replace("#4CAF50", "#2196F3").replace("#45a049", "#1976D2"));
    exitButton->setStyleSheet(buttonStyle.replace("#4CAF50", "#f44336").replace("#45a049", "#d32f2f"));
    
    layout->addWidget(playButton);
    layout->addWidget(statsButton);
    layout->addWidget(exitButton);
    
    // Add some spacing
    layout->addStretch();
    
    // Connect signals
    connect(playButton, &QPushButton::clicked, this, &MainMenuWindow::onPlayWordle);
    connect(statsButton, &QPushButton::clicked, this, &MainMenuWindow::onViewStats);
    connect(exitButton, &QPushButton::clicked, this, &MainMenuWindow::onExit);
    
    setLayout(layout);
}

void MainMenuWindow::onPlayWordle() {
    WordleGameWindow *gameWindow = new WordleGameWindow();
    connect(gameWindow, &WordleGameWindow::backToMenuRequested, this, [this, gameWindow]() {
        this->show();
        gameWindow->deleteLater();
    });
    gameWindow->show();
    this->hide();
}

void MainMenuWindow::onViewStats() {
    StatsWindow *statsWindow = new StatsWindow();
    connect(statsWindow, &StatsWindow::backToMenuRequested, this, [this, statsWindow]() {
        this->show();
        statsWindow->deleteLater();
    });
    statsWindow->show();
    this->hide();
}

void MainMenuWindow::onExit() {
    QApplication::quit();
}

// ============================================================================
// WordleGameWindow Implementation
// ============================================================================

WordleGameWindow::WordleGameWindow(QWidget *parent) : QWidget(parent), guesses(0), optimalGuessMode(false) {
    setWindowTitle("Wordle");
    setFixedSize(500, 600);
    
    layout = new QVBoxLayout(this);
    
    // Game title
    QLabel *titleLabel = new QLabel("Wordle", this);
    titleLabel->setAlignment(Qt::AlignCenter);
    titleLabel->setStyleSheet("QLabel { font-size: 20px; font-weight: bold; margin: 10px; }");
    layout->addWidget(titleLabel);
    
    // Input area
    layout->addWidget(new QLabel("Enter your guess:", this));
    input = new QLineEdit(this);
    input->setMaxLength(5);
    input->setStyleSheet("QLineEdit { font-size: 18px; padding: 10px; }");
    layout->addWidget(input);
    
    // Buttons
    newGameButton = new QPushButton("New Game", this);
    backToMenuButton = new QPushButton("Back to Menu", this);
    optimalGuessButton = new QPushButton("Show Optimal Guess", this);
    optimalGuessButton->setCheckable(true);  // Make it a toggle button
    optimalGuessLabel = new QLabel("", this);
    optimalGuessLabel->setStyleSheet("QLabel { font-size: 16px; font-weight: bold; color: #2196F3; }");
    
    QHBoxLayout *buttonLayout = new QHBoxLayout();
    buttonLayout->addWidget(optimalGuessButton);
    buttonLayout->addWidget(optimalGuessLabel);
    layout->addLayout(buttonLayout);
    
    // Display areas
    QLabel *previousGuessesLabel = new QLabel("Previous guesses:", this);
    previousGuessesLabel->setAlignment(Qt::AlignCenter);
    layout->addWidget(previousGuessesLabel);
    guessesDisplay = new QTextEdit(this);
    guessesDisplay->setMaximumHeight(150);
    guessesDisplay->setReadOnly(true);
    guessesDisplay->setFontPointSize(24);
    guessesDisplay->setAlignment(Qt::AlignCenter);
    layout->addWidget(guessesDisplay);
    
    messageLabel = new QLabel(this);
    messageLabel->setAlignment(Qt::AlignCenter);
    layout->addWidget(messageLabel);
    
    // Keyboard
    layout->addWidget(new QLabel("Keyboard:", this));
    QWidget *keyboardWidget = new QWidget(this);
    keyboardLayout = new QGridLayout();
    keyboardWidget->setLayout(keyboardLayout);
    layout->addWidget(keyboardWidget);
    
    // Bottom buttons
    QHBoxLayout *bottomButtonLayout = new QHBoxLayout();
    bottomButtonLayout->addWidget(newGameButton);
    bottomButtonLayout->addWidget(backToMenuButton);
    layout->addLayout(bottomButtonLayout);
    
    // Connect signals
    connect(newGameButton, &QPushButton::clicked, this, &WordleGameWindow::onNewGame);
    connect(backToMenuButton, &QPushButton::clicked, this, &WordleGameWindow::onBackToMenu);
    connect(optimalGuessButton, &QPushButton::clicked, this, &WordleGameWindow::onShowOptimalGuess);
    connect(input, &QLineEdit::returnPressed, this, &WordleGameWindow::onGuess);
    
    // Load word lists
    QStringList wordList = loadWordList("WordList.txt");
    QStringList acceptedList = loadWordList("AcceptedWordList");
    
    validWords = QSet<QString>(wordList.begin(), wordList.end());
    validWords.unite(QSet<QString>(acceptedList.begin(), acceptedList.end()));
    
    setupKeyboard();
    startNewGame();
}

void WordleGameWindow::setupKeyboard() {
    QString letters = "QWERTYUIOPASDFGHJKLZXCVBNM";
    
    QHBoxLayout *row1Layout = new QHBoxLayout();
    QHBoxLayout *row2Layout = new QHBoxLayout();
    QHBoxLayout *row3Layout = new QHBoxLayout();
    
    for (int i = 0; i < 26; ++i) {
        keyboardButtons[i] = new QPushButton(QString(letters[i]), this);
        keyboardButtons[i]->setFixedSize(30, 30);
        keyboardButtons[i]->setStyleSheet("QPushButton { background-color: white; color: black; border: 1px solid gray; }");
        
        if (i < 10) {
            row1Layout->addWidget(keyboardButtons[i]);
        } else if (i < 19) {
            row2Layout->addWidget(keyboardButtons[i]);
        } else {
            row3Layout->addWidget(keyboardButtons[i]);
        }
        
        letterStates[letters[i]] = 0;
    }
    
    row1Layout->insertStretch(0, 1);
    row1Layout->addStretch(1);
    row2Layout->insertSpacing(0, 15);
    row2Layout->addStretch(1);
    row3Layout->insertSpacing(0, 30);
    row3Layout->addStretch(1);
    
    keyboardLayout->addLayout(row1Layout, 0, 0);
    keyboardLayout->addLayout(row2Layout, 1, 0);
    keyboardLayout->addLayout(row3Layout, 2, 0);
}

void WordleGameWindow::updateKeyboard(const QString &guess, const QString &feedback) {
    QString letters = "QWERTYUIOPASDFGHJKLZXCVBNM";
    
    for (int i = 0; i < 5; ++i) {
        QChar letter = guess[i];
        int letterIndex = letters.indexOf(letter);
        
        if (letterIndex == -1) continue;
        
        int newState = 0;
        
        if (feedback[i] == 'G') {
            newState = 3;
        } else if (feedback[i] == 'Y') {
            newState = 2;
        } else {
            newState = 1;
        }
        
        if (newState > letterStates[letter]) {
            letterStates[letter] = newState;
            
            QString color;
            if (newState == 3) {
                color = "green";
            } else if (newState == 2) {
                color = "orange";
            } else {
                color = "gray";
            }
            
            keyboardButtons[letterIndex]->setStyleSheet(
                QString("QPushButton { background-color: %1; color: white; border: 1px solid gray; font-weight: bold; }").arg(color)
            );
        }
    }
}

void WordleGameWindow::startNewGame() {
    QStringList answers = loadWordList("WordList.txt");
    if (answers.isEmpty()) {
        QMessageBox::critical(this, "Error", "Could not load WordList.txt");
        close();
        return;
    }
    answer = answers.at(QRandomGenerator::global()->bounded(answers.size()));
    guesses = 0;
    optimalGuessMode = false;
    input->clear();
    guessesDisplay->clear();
    guessHistory.clear();
    feedbackHistory.clear();
    optimalGuessLabel->clear();
    optimalGuessButton->setChecked(false);
    
    for (int i = 0; i < 26; ++i) {
        keyboardButtons[i]->setStyleSheet("QPushButton { background-color: white; color: black; border: 1px solid gray; }");
        letterStates[QChar('A' + i)] = 0;
    }
    
    messageLabel->setText("You have 5 guesses.");
    input->setEnabled(true);
}

void WordleGameWindow::onGuess() {
    QString guess = input->text().toUpper();
    
    if (guess.length() != 5) {
        QMessageBox::warning(this, "Invalid Input", "Please enter a 5-letter word.");
        return;
    }
    
    if (!validWords.contains(guess)) {
        QMessageBox::warning(this, "Invalid Word", "That's not a valid word.");
        return;
    }
    
    guesses++;
    QString feedback = generateFeedback(guess);
    
    // Store guess and feedback history
    guessHistory.append(guess);
    feedbackHistory.append(feedback);
    
    // Add guess to display with colored characters
    QString coloredWord;
    for (int i = 0; i < 5; ++i) {
        if (feedback[i] == 'G') {
            coloredWord += "<span style='color: green; font-weight: bold; font-size:24pt;'>" + QString(guess[i]) + "</span>";
        } else if (feedback[i] == 'Y') {
            coloredWord += "<span style='color: orange; font-weight: bold; font-size:24pt;'>" + QString(guess[i]) + "</span>";
        } else {
            coloredWord += "<span style='color: gray; font-weight: bold; font-size:24pt;'>" + QString(guess[i]) + "</span>";
        }
    }
    guessesDisplay->append("<div style='text-align: center;'>" + coloredWord + "</div>");
    
    updateKeyboard(guess, feedback);
    
    // Update optimal guess if mode is enabled
    if (optimalGuessMode) {
        updateOptimalGuess();
    }
    
    if (guess == answer) {
        messageLabel->setText("Congratulations! You won!");
        input->setEnabled(false);
    } else if (guesses >= 5) {
        messageLabel->setText(QString("Game over! The word was: %1").arg(answer));
        input->setEnabled(false);
    } else {
        messageLabel->setText(QString("You have %1 guesses left.").arg(5 - guesses));
    }
    
    input->clear();
}

void WordleGameWindow::onNewGame() {
    startNewGame();
}

void WordleGameWindow::onBackToMenu() {
    emit backToMenuRequested();
    this->close();
}

void WordleGameWindow::onShowOptimalGuess() {
    optimalGuessMode = optimalGuessButton->isChecked();
    
    if (optimalGuessMode) {
        optimalGuessButton->setText("Hide Optimal Guess");
        updateOptimalGuess();
    } else {
        optimalGuessButton->setText("Show Optimal Guess");
        optimalGuessLabel->clear();
    }
}

void WordleGameWindow::updateOptimalGuess() {
    QString optimalGuess = findOptimalGuess();
    if (!optimalGuess.isEmpty()) {
        optimalGuessLabel->setText(QString("Optimal: %1").arg(optimalGuess));
    } else {
        optimalGuessLabel->setText("No optimal guess found");
    }
}

QString WordleGameWindow::findOptimalGuess() {
    // Load word lists
    QStringList answerWords = loadWordList("WordList.txt");
    QStringList acceptedWords = loadWordList("AcceptedWordList");
    
    // For first guess, always use the best starting word
    if (guessHistory.isEmpty()) {
        // Use the best starting word from analysis
        return "STARE";  // This should be replaced with the actual best word from your analysis
    }
    
    // Collect information from previous guesses
    QSet<QChar> grayLetters;  // Letters that are not in the word
    QSet<QChar> yellowLetters;  // Letters that are in the word but wrong position
    QMap<int, QChar> greenLetters;  // Letters in correct positions
    QMap<QChar, QSet<int>> yellowPositions;  // Positions where yellow letters cannot be
    
    for (int i = 0; i < guessHistory.size(); ++i) {
        QString guess = guessHistory[i];
        QString feedback = feedbackHistory[i];
        
        for (int j = 0; j < 5; ++j) {
            QChar letter = guess[j];
            if (feedback[j] == 'G') {
                greenLetters[j] = letter;
            } else if (feedback[j] == 'Y') {
                yellowLetters.insert(letter);
                yellowPositions[letter].insert(j);
            } else if (feedback[j] == 'X') {
                // Only add to gray letters if it's not also yellow or green
                bool isAlsoYellowOrGreen = false;
                for (int k = 0; k < 5; ++k) {
                    if (k != j && guess[k] == letter && feedback[k] != 'X') {
                        isAlsoYellowOrGreen = true;
                        break;
                    }
                }
                if (!isAlsoYellowOrGreen) {
                    grayLetters.insert(letter);
                }
            }
        }
    }
    
    // Filter words based on previous guesses - only consider answer words
    QSet<QString> possibleWords(answerWords.begin(), answerWords.end());
    QSet<QString> newPossibleWords;
    
    for (const QString &word : possibleWords) {
        bool isValid = true;
        
        // Check green letters (must be in correct position)
        for (auto it = greenLetters.begin(); it != greenLetters.end(); ++it) {
            if (word[it.key()] != it.value()) {
                isValid = false;
                break;
            }
        }
        
        if (!isValid) continue;
        
        // Check yellow letters (must be in word but not in forbidden positions)
        for (QChar yellowLetter : yellowLetters) {
            bool found = false;
            QSet<int> forbiddenPositions = yellowPositions.value(yellowLetter);
            
            for (int pos = 0; pos < 5; ++pos) {
                if (word[pos] == yellowLetter && !forbiddenPositions.contains(pos)) {
                    found = true;
                    break;
                }
            }
            
            if (!found) {
                isValid = false;
                break;
            }
        }
        
        if (!isValid) continue;
        
        // Check gray letters (should not be in word)
        for (QChar grayLetter : grayLetters) {
            if (word.contains(grayLetter)) {
                isValid = false;
                break;
            }
        }
        
        if (isValid) {
            newPossibleWords.insert(word);
        }
    }
    
    possibleWords = newPossibleWords;
    
    // Calculate letter frequency for remaining possible words
    QMap<QChar, int> letterFreq;
    for (const QString &word : possibleWords) {
        for (QChar c : word) {
            letterFreq[c]++;
        }
    }
    
    // If we have very few possible words, prioritize actual possible answers
    if (possibleWords.size() <= 5 && !possibleWords.isEmpty()) {
        // Among possible answers, find the one with best letter frequency
        QString bestPossibleWord;
        int bestScore = -1;
        
        for (const QString &word : possibleWords) {
            int score = 0;
            for (QChar c : word) {
                score += letterFreq.value(c, 0);
            }
            if (score > bestScore) {
                bestScore = score;
                bestPossibleWord = word;
            }
        }
        return bestPossibleWord;
    }
    
    // Score words based on letter frequency and constraints - only consider answer words
    QString bestWord;
    int bestScore = -1;
    
    // Prioritize possible answers when we have few left
    bool prioritizePossibleAnswers = (possibleWords.size() <= 10);
    
    for (const QString &word : answerWords) {
        int score = 0;
        QSet<QChar> uniqueLetters;
        
        // Check if word violates any constraints
        bool violatesConstraints = false;
        
        // Check gray letters
        for (QChar grayLetter : grayLetters) {
            if (word.contains(grayLetter)) {
                violatesConstraints = true;
                break;
            }
        }
        
        if (violatesConstraints) continue;
        
        // Check yellow letters (must be moved to different positions)
        for (QChar yellowLetter : yellowLetters) {
            QSet<int> forbiddenPositions = yellowPositions.value(yellowLetter);
            bool hasYellowLetter = false;
            bool inValidPosition = false;
            
            for (int pos = 0; pos < 5; ++pos) {
                if (word[pos] == yellowLetter) {
                    hasYellowLetter = true;
                    if (!forbiddenPositions.contains(pos)) {
                        inValidPosition = true;
                        break;
                    }
                }
            }
            
            if (hasYellowLetter && !inValidPosition) {
                violatesConstraints = true;
                break;
            }
        }
        
        if (violatesConstraints) continue;
        
        // Score based on letter frequency in remaining possible words
        for (QChar c : word) {
            score += letterFreq.value(c, 0);
            uniqueLetters.insert(c);
        }
        
        // Bonus for using yellow letters in new positions
        for (QChar c : word) {
            if (yellowLetters.contains(c)) {
                score += 100;  // Bonus for using yellow letters
            }
        }
        
        // Extremely strong bonus for unique letters and penalty for repeated letters
        score += uniqueLetters.size() * 1000;  // Very strong bonus for more unique letters
        score -= (word.length() - uniqueLetters.size()) * 5000;  // Very strong penalty for repeated letters
        
        // Heavy bonus for possible answers when we're getting close
        if (prioritizePossibleAnswers && possibleWords.contains(word)) {
            score += 10000;  // Massive bonus for possible answers
        }
        
        if (score > bestScore) {
            bestScore = score;
            bestWord = word;
        }
    }
    
    // If no good answer word found, try accepted words as fallback
    if (bestWord.isEmpty()) {
        for (const QString &word : acceptedWords) {
            int score = 0;
            QSet<QChar> uniqueLetters;
            
            // Check if word violates any constraints
            bool violatesConstraints = false;
            
            // Check gray letters
            for (QChar grayLetter : grayLetters) {
                if (word.contains(grayLetter)) {
                    violatesConstraints = true;
                    break;
                }
            }
            
            if (violatesConstraints) continue;
            
            // Check yellow letters (must be moved to different positions)
            for (QChar yellowLetter : yellowLetters) {
                QSet<int> forbiddenPositions = yellowPositions.value(yellowLetter);
                bool hasYellowLetter = false;
                bool inValidPosition = false;
                
                for (int pos = 0; pos < 5; ++pos) {
                    if (word[pos] == yellowLetter) {
                        hasYellowLetter = true;
                        if (!forbiddenPositions.contains(pos)) {
                            inValidPosition = true;
                            break;
                        }
                    }
                }
                
                if (hasYellowLetter && !inValidPosition) {
                    violatesConstraints = true;
                    break;
                }
            }
            
            if (violatesConstraints) continue;
            
            // Score based on letter frequency
            for (QChar c : word) {
                score += letterFreq.value(c, 0);
                uniqueLetters.insert(c);
            }
            
            // Bonus for using yellow letters
            for (QChar c : word) {
                if (yellowLetters.contains(c)) {
                    score += 100;
                }
            }
            
            // Strong bonus for unique letters
            score += uniqueLetters.size() * 1000;
            score -= (word.length() - uniqueLetters.size()) * 5000;
            
            if (score > bestScore) {
                bestScore = score;
                bestWord = word;
            }
        }
    }
    
    return bestWord;
}

QString WordleGameWindow::generateFeedback(const QString &guess) {
    QString feedback = "XXXXX";
    QString answerCopy = answer;
    QVector<bool> used(5, false);
    
    // Check for correct position (G)
    for (int i = 0; i < 5; ++i) {
        if (guess[i] == answer[i]) {
            feedback[i] = 'G';
            used[i] = true;
        }
    }
    
    // Check for correct letter, wrong position (Y)
    for (int i = 0; i < 5; ++i) {
        if (feedback[i] == 'G') continue;
        for (int j = 0; j < 5; ++j) {
            if (!used[j] && guess[i] == answer[j]) {
                feedback[i] = 'Y';
                used[j] = true;
                break;
            }
        }
    }
    
    return feedback;
}

// ============================================================================
// StatsWindow Implementation
// ============================================================================

StatsWindow::StatsWindow(QWidget *parent) : QWidget(parent) {
    setWindowTitle("Word Statistics");
    setFixedSize(600, 500);
    
    layout = new QVBoxLayout(this);
    
    // Title
    QLabel *titleLabel = new QLabel("Word Statistics", this);
    titleLabel->setAlignment(Qt::AlignCenter);
    titleLabel->setStyleSheet("QLabel { font-size: 20px; font-weight: bold; margin: 10px; }");
    layout->addWidget(titleLabel);
    
    // Stats display
    statsDisplay = new QTextEdit(this);
    statsDisplay->setReadOnly(true);
    statsDisplay->setFontFamily("Courier New"); // Monospaced font for alignment
    layout->addWidget(statsDisplay);
    
    // Back button
    backToMenuButton = new QPushButton("Back to Menu", this);
    backToMenuButton->setStyleSheet("QPushButton { font-size: 14px; padding: 10px; background-color: #2196F3; color: white; border: none; border-radius: 5px; } QPushButton:hover { background-color: #1976D2; }");
    layout->addWidget(backToMenuButton);
    
    connect(backToMenuButton, &QPushButton::clicked, this, &StatsWindow::onBackToMenu);
    
    setLayout(layout);
    
    loadAndDisplayStats();
}

void StatsWindow::loadAndDisplayStats() {
    QStringList answerWords = loadWordList("WordList.txt");
    QStringList acceptedWords = loadWordList("AcceptedWordList");
    
    if (answerWords.isEmpty() || acceptedWords.isEmpty()) {
        statsDisplay->setPlainText("Error: Could not load word lists.");
        return;
    }
    
    QString stats;
    stats += "=== WORD STATISTICS ===\n\n";
    stats += QString("Total answer words: %1\n").arg(answerWords.size());
    stats += QString("Total accepted words: %1\n\n").arg(acceptedWords.size());
    
    // Letter frequency by position
    QVector<QMap<QChar, int>> positionFrequencies(5);
    
    for (const QString &word : answerWords) {
        for (int i = 0; i < word.length() && i < 5; ++i) {
            positionFrequencies[i][word[i]]++;
        }
    }
    
    stats += "=== LETTER FREQUENCY BY POSITION ===\n\n";
    
    // Gather top 10 for each position
    QVector<QVector<QPair<QChar, int>>> topLetters(5);
    for (int pos = 0; pos < 5; ++pos) {
        QVector<QPair<QChar, int>> sortedFreq;
        for (auto it = positionFrequencies[pos].begin(); it != positionFrequencies[pos].end(); ++it) {
            if (it.value() > 0) {
                sortedFreq.append(qMakePair(it.key(), it.value()));
            }
        }
        std::sort(sortedFreq.begin(), sortedFreq.end(), 
                  [](const QPair<QChar, int> &a, const QPair<QChar, int> &b) {
                      return a.second > b.second;
                  });
        for (int i = 0; i < qMin(10, sortedFreq.size()); ++i) {
            topLetters[pos].append(sortedFreq[i]);
        }
    }
    // Header
    stats += "Rank    Pos1         Pos2         Pos3         Pos4         Pos5\n";
    stats += "---------------------------------------------------------------------\n";
    // Rows
    for (int rank = 0; rank < 10; ++rank) {
        stats += QString("%1   ").arg(rank+1, 2, 10, QChar(' '));
        for (int pos = 0; pos < 5; ++pos) {
            if (rank < topLetters[pos].size()) {
                QString entry = QString("%1 : %2").arg(QString(topLetters[pos][rank].first), 2, QChar(' ')).arg(topLetters[pos][rank].second, 4, 10, QChar(' '));
                stats += entry.leftJustified(12, ' ');
            } else {
                stats += QString("").leftJustified(12, ' ');
            }
        }
        stats += "\n";
    }
    stats += "\n";
    
    // Overall letter frequency
    QMap<QChar, int> overallFrequency;
    for (const QString &word : answerWords) {
        for (QChar c : word) {
            overallFrequency[c]++;
        }
    }
    
    // 3-letter combinations analysis
    QMap<QString, int> threeLetterCombos;
    for (const QString &word : answerWords) {
        if (word.length() >= 3) {
            QString combo = word.left(3);
            threeLetterCombos[combo]++;
        }
    }
    
    QVector<QPair<QChar, int>> sortedOverall;
    for (auto it = overallFrequency.begin(); it != overallFrequency.end(); ++it) {
        sortedOverall.append(qMakePair(it.key(), it.value()));
    }
    std::sort(sortedOverall.begin(), sortedOverall.end(), 
              [](const QPair<QChar, int> &a, const QPair<QChar, int> &b) {
                  return a.second > b.second;
              });
    
    QVector<QPair<QString, int>> sortedCombos;
    for (auto it = threeLetterCombos.begin(); it != threeLetterCombos.end(); ++it) {
        sortedCombos.append(qMakePair(it.key(), it.value()));
    }
    std::sort(sortedCombos.begin(), sortedCombos.end(), 
              [](const QPair<QString, int> &a, const QPair<QString, int> &b) {
                  return a.second > b.second;
              });
    
    stats += "OVERALL LETTER FREQUENCY   | MOST COMMON 3-LETTER COMBINATIONS\n";
    int maxRows = 26;
    for (int i = 0; i < maxRows; ++i) {
        QString left = (i < sortedOverall.size()) ? QString("%1: %2").arg(sortedOverall[i].first).arg(sortedOverall[i].second, 4) : "";
        QString right = (i < sortedCombos.size()) ? QString("%1: %2").arg(sortedCombos[i].first, 3).arg(sortedCombos[i].second, 4) : "";
        stats += left.leftJustified(26, ' ') + " |  " + right + "\n";
    }
    
    statsDisplay->setPlainText(stats);
}

void StatsWindow::onBackToMenu() {
    emit backToMenuRequested();
    this->close();
} 