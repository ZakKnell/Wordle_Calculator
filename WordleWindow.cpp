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

// Forward declaration for shared optimal guess function
QString findOptimalGuessShared(const QSet<QString> &answerWords, const QSet<QString> &acceptedWords, const QList<QPair<QString, QString>> &guessFeedbackPairs, const QMap<int, QChar> &greenLetters, const QMap<QChar, QSet<int>> &yellowPositions, const QSet<QChar> &yellowLetters, const QSet<QChar> &grayLetters);
QString findOptimalGuessWithConstraints(const QSet<QString>& answerWords, const QSet<QString>& acceptedWords, const QMap<int, QChar>& greenLetters, const QMap<QChar, QSet<int>>& yellowPositions, const QSet<QChar>& yellowLetters, const QSet<QChar>& grayLetters);

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

// Add this function after the includes:
QVector<QPair<QString, int>> getBestStartingWords(const QSet<QString>& answerWords, const QSet<QString>& acceptedWords, int topN = 10) {
    // Combine all valid words
    QSet<QString> allWords = answerWords;
    for (const QString& w : acceptedWords) allWords.insert(w);
    // Precompute letter frequencies by position from answer words
    QVector<QMap<QChar, int>> posFreq(5);
    for (const QString& word : answerWords) {
        for (int i = 0; i < word.size() && i < 5; ++i) {
            posFreq[i][word[i]]++;
        }
    }
    QVector<QPair<QString, int>> scored;
    for (const QString& word : allWords) {
        QSet<QChar> uniqueLetters;
        for (QChar c : word) uniqueLetters.insert(c);
        if (uniqueLetters.size() < 5) continue; // Only 5-unique-letter words
        int score = 0;
        for (int i = 0; i < word.size() && i < 5; ++i) {
            score += posFreq[i].value(word[i], 0);
        }
        score += uniqueLetters.size() * 2000;
        score -= (word.length() - uniqueLetters.size()) * 10000;
        scored.append(qMakePair(word, score));
    }
    std::sort(scored.begin(), scored.end(), [](const QPair<QString, int>& a, const QPair<QString, int>& b) {
        return a.second > b.second;
    });
    if (scored.size() > topN) scored.resize(topN);
    return scored;
}

// Function to get all valid words that match constraints
QVector<QPair<QString, int>> getAllValidWordsWithConstraints(const QSet<QString>& answerWords, const QSet<QString>& acceptedWords, const QMap<int, QChar>& greenLetters, const QMap<QChar, QSet<int>>& yellowPositions, const QSet<QChar>& yellowLetters, const QSet<QChar>& grayLetters) {
    QVector<QMap<QChar, int>> posFreq(5);
    for (const QString& word : answerWords) {
        for (int i = 0; i < word.size() && i < 5; ++i) {
            posFreq[i][word[i]]++;
        }
    }
    QSet<QString> allWords = answerWords;
    for (const QString& w : acceptedWords) allWords.insert(w);
    QVector<QPair<QString, int>> validWords;
    // Build required counts and forbidden positions
    QMap<QChar, int> requiredCount; // letter -> min count (only green positions count)
    QMap<QChar, QSet<int>> forbiddenPositions = yellowPositions; // letter -> set of forbidden positions (yellow)
    QMap<QChar, QSet<int>> greenPositions; // letter -> set of green positions
    for (auto it = greenLetters.begin(); it != greenLetters.end(); ++it) {
        requiredCount[it.value()]++;
        greenPositions[it.value()].insert(it.key());
    }
    // Yellow positions are only forbidden, not counted toward required instances
    for (const QString& word : allWords) {
        bool valid = true;
        QMap<QChar, int> wordCount;
        // Green check
        for (auto it = greenLetters.begin(); it != greenLetters.end(); ++it) {
            if (word[it.key()] != it.value()) {
                valid = false;
                break;
            }
        }
        if (!valid) continue;
        // Yellow check (must not be in forbidden positions)
        for (auto it = yellowPositions.begin(); it != yellowPositions.end(); ++it) {
            QChar yellow = it.key();
            for (int pos : it.value()) {
                if (word[pos] == yellow) {
                    valid = false;
                    break;
                }
            }
            if (!valid) break;
        }
        if (!valid) continue;
        // Count letters in word and track positions
        for (int i = 0; i < 5; ++i) {
            wordCount[word[i]]++;
        }
        // Required count check (only green positions count)
        for (auto it = requiredCount.begin(); it != requiredCount.end(); ++it) {
            if (wordCount[it.key()] < it.value()) {
                valid = false;
                break;
            }
        }
        if (!valid) continue;
        // Gray logic: forbid gray letters only in positions not green/yellow for that letter
        for (QChar gray : grayLetters) {
            bool isGreenOrYellow = requiredCount.contains(gray) || yellowPositions.contains(gray);
            if (!isGreenOrYellow) {
                // If not green/yellow anywhere, must not appear at all
                if (wordCount[gray] > 0) {
                    valid = false;
                    break;
                }
            } else {
                // If green/yellow somewhere, must not appear in any other positions
                for (int i = 0; i < 5; ++i) {
                    bool isGreen = greenLetters.value(i, QChar()) == gray;
                    bool isYellow = yellowPositions.value(gray).contains(i);
                    if (!isGreen && !isYellow && word[i] == gray) {
                        valid = false;
                        break;
                    }
                }
                if (!valid) break;
            }
        }
        if (!valid) continue;
        // Calculate score for this word
        QSet<QChar> uniqueLetters;
        int score = 0;
        for (int i = 0; i < word.size() && i < 5; ++i) {
            score += posFreq[i].value(word[i], 0);
            uniqueLetters.insert(word[i]);
        }
        
        // Bonus for using yellow letters (confirmed to be in solution)
        int yellowLettersUsed = 0;
        for (QChar yellow : yellowLetters) {
            if (word.contains(yellow)) {
                yellowLettersUsed++;
            }
        }
        score += yellowLettersUsed * 5000; // Significant bonus for using yellow letters
        
        // Reduced penalty for repeated letters when yellow letters are involved
        int repeatedLetters = word.length() - uniqueLetters.size();
        if (yellowLettersUsed > 0) {
            // If using yellow letters, reduce the penalty for repeated letters
            score -= repeatedLetters * 1000; // Much smaller penalty
        } else {
            // If not using yellow letters, keep the original penalty
            score -= repeatedLetters * 10000;
        }
        
        score += uniqueLetters.size() * 2000;
        validWords.append(qMakePair(word, score));
    }
    return validWords;
}

// Shared, optimized optimal guess function
QString findOptimalGuessWithConstraints(const QSet<QString>& answerWords, const QSet<QString>& acceptedWords, const QMap<int, QChar>& greenLetters, const QMap<QChar, QSet<int>>& yellowPositions, const QSet<QChar>& yellowLetters, const QSet<QChar>& grayLetters) {
    QVector<QMap<QChar, int>> posFreq(5);
    for (const QString& word : answerWords) {
        for (int i = 0; i < word.size() && i < 5; ++i) {
            posFreq[i][word[i]]++;
        }
    }
    QSet<QString> allWords = answerWords;
    for (const QString& w : acceptedWords) allWords.insert(w);
    QString bestWord;
    int bestScore = -1;
    // Build required counts and forbidden positions
    QMap<QChar, int> requiredCount; // letter -> min count (only green positions count)
    QMap<QChar, QSet<int>> forbiddenPositions = yellowPositions; // letter -> set of forbidden positions (yellow)
    QMap<QChar, QSet<int>> greenPositions; // letter -> set of green positions
    for (auto it = greenLetters.begin(); it != greenLetters.end(); ++it) {
        requiredCount[it.value()]++;
        greenPositions[it.value()].insert(it.key());
    }
    // Yellow positions are only forbidden, not counted toward required instances
    for (const QString& word : allWords) {
        bool valid = true;
        QMap<QChar, int> wordCount;
        // Green check
        for (auto it = greenLetters.begin(); it != greenLetters.end(); ++it) {
            if (word[it.key()] != it.value()) {
                valid = false;
                break;
            }
        }
        if (!valid) continue;
        // Yellow check (must not be in forbidden positions)
        for (auto it = yellowPositions.begin(); it != yellowPositions.end(); ++it) {
            QChar yellow = it.key();
            for (int pos : it.value()) {
                if (word[pos] == yellow) {
                    valid = false;
                    break;
                }
            }
            if (!valid) break;
        }
        if (!valid) continue;
        // Count letters in word and track positions
        for (int i = 0; i < 5; ++i) {
            wordCount[word[i]]++;
        }
        // Required count check (only green positions count)
        for (auto it = requiredCount.begin(); it != requiredCount.end(); ++it) {
            if (wordCount[it.key()] < it.value()) {
                valid = false;
                break;
            }
        }
        if (!valid) continue;
        // Gray logic: forbid gray letters only in positions not green/yellow for that letter
        for (QChar gray : grayLetters) {
            bool isGreenOrYellow = requiredCount.contains(gray) || yellowPositions.contains(gray);
            if (!isGreenOrYellow) {
                // If not green/yellow anywhere, must not appear at all
                if (wordCount[gray] > 0) {
                    valid = false;
                    break;
                }
            } else {
                // If green/yellow somewhere, must not appear in any other positions
                for (int i = 0; i < 5; ++i) {
                    bool isGreen = greenLetters.value(i, QChar()) == gray;
                    bool isYellow = yellowPositions.value(gray).contains(i);
                    if (!isGreen && !isYellow && word[i] == gray) {
                        valid = false;
                        break;
                    }
                }
                if (!valid) break;
            }
        }
        if (!valid) continue;
        // Calculate score for this word
        QSet<QChar> uniqueLetters;
        int score = 0;
        for (int i = 0; i < word.size() && i < 5; ++i) {
            score += posFreq[i].value(word[i], 0);
            uniqueLetters.insert(word[i]);
        }
        
        // Bonus for using yellow letters (confirmed to be in solution)
        int yellowLettersUsed = 0;
        for (QChar yellow : yellowLetters) {
            if (word.contains(yellow)) {
                yellowLettersUsed++;
            }
        }
        score += yellowLettersUsed * 5000; // Significant bonus for using yellow letters
        
        // Reduced penalty for repeated letters when yellow letters are involved
        int repeatedLetters = word.length() - uniqueLetters.size();
        if (yellowLettersUsed > 0) {
            // If using yellow letters, reduce the penalty for repeated letters
            score -= repeatedLetters * 1000; // Much smaller penalty
        } else {
            // If not using yellow letters, keep the original penalty
            score -= repeatedLetters * 10000;
        }
        
        score += uniqueLetters.size() * 2000;
        if (score > bestScore) {
            bestScore = score;
            bestWord = word;
        }
    }
    return bestWord;
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
    QPushButton *solverButton = new QPushButton("Wordle Solver", this);
    exitButton = new QPushButton("Exit", this);
    
    // Style buttons
    QString buttonStyle = "QPushButton { font-size: 16px; padding: 15px; margin: 5px; background-color: #4CAF50; color: white; border: none; border-radius: 5px; } QPushButton:hover { background-color: #45a049; }";
    playButton->setStyleSheet(buttonStyle);
    statsButton->setStyleSheet(buttonStyle.replace("#4CAF50", "#2196F3").replace("#45a049", "#1976D2"));
    solverButton->setStyleSheet(buttonStyle.replace("#4CAF50", "#FF9800").replace("#45a049", "#F57C00"));
    exitButton->setStyleSheet(buttonStyle.replace("#4CAF50", "#f44336").replace("#45a049", "#d32f2f"));
    
    layout->addWidget(playButton);
    layout->addWidget(statsButton);
    layout->addWidget(solverButton);
    layout->addWidget(exitButton);
    
    // Add some spacing
    layout->addStretch();
    
    // Connect signals
    connect(playButton, &QPushButton::clicked, this, &MainMenuWindow::onPlayWordle);
    connect(statsButton, &QPushButton::clicked, this, &MainMenuWindow::onViewStats);
    connect(solverButton, &QPushButton::clicked, this, &MainMenuWindow::onSolver);
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

void MainMenuWindow::onSolver() {
    SolverWindow *solverWindow = new SolverWindow();
    connect(solverWindow, &SolverWindow::backToMenuRequested, this, [this, solverWindow]() {
        this->show();
        solverWindow->deleteLater();
    });
    solverWindow->show();
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
    input->clear();
    guessesDisplay->clear();
    guessHistory.clear();
    feedbackHistory.clear();
    optimalGuessLabel->clear();
    input->setReadOnly(false);
    input->setPlaceholderText("");
    for (int i = 0; i < 26; ++i) {
        keyboardButtons[i]->setStyleSheet("QPushButton { background-color: white; color: black; border: 1px solid gray; }");
        letterStates[QChar('A' + i)] = 0;
    }
    messageLabel->setText("You have 5 guesses.");
    input->setEnabled(true);
    if (optimalGuessButton->isChecked()) {
        updateOptimalGuess();
    }
}

void WordleGameWindow::onGuess() {
    if (input->isReadOnly()) {
        startNewGame();
        return;
    }
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
    
    // Update optimal guess if button is checked
    if (optimalGuessButton->isChecked()) {
        updateOptimalGuess();
    }
    
    if (guess == answer) {
        messageLabel->setText("Congratulations! You won!");
        input->setReadOnly(true);
        input->setPlaceholderText("Press Enter to start a new game");
    } else if (guesses >= 5) {
        messageLabel->setText(QString("Game over! The word was: %1").arg(answer));
        input->setReadOnly(true);
        input->setPlaceholderText("Press Enter to start a new game");
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
    if (guessHistory.isEmpty()) {
        QStringList answerList = loadWordList("WordList.txt");
        QStringList acceptedList = loadWordList("AcceptedWordList");
        QSet<QString> answerSet(answerList.begin(), answerList.end());
        QSet<QString> acceptedSet(acceptedList.begin(), acceptedList.end());
        QVector<QPair<QString, int>> best = getBestStartingWords(answerSet, acceptedSet, 1);
        if (!best.isEmpty()) {
            optimalGuessLabel->setText(QString("Optimal: %1").arg(best[0].first));
        } else {
            optimalGuessLabel->setText("No optimal guess found");
        }
        return;
    }
    // Reconstruct constraints from guessHistory and feedbackHistory
    QMap<int, QChar> greenLetters;
    QMap<QChar, QSet<int>> yellowPositions;
    QSet<QChar> yellowLetters;
    QSet<QChar> grayLetters;
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
    QString optimal = findOptimalGuessWithConstraints(validWords, validWords, greenLetters, yellowPositions, yellowLetters, grayLetters);
    if (!optimal.isEmpty()) {
        optimalGuessLabel->setText(QString("Optimal: %1").arg(optimal));
    } else {
        optimalGuessLabel->setText("No optimal guess found");
    }
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
    
    stats += "\n=== TOP STARTING WORDS (CONSISTENT ALGORITHM) ===\n\n";
    QSet<QString> answerSet = QSet<QString>(answerWords.begin(), answerWords.end());
    QSet<QString> acceptedSet = QSet<QString>(acceptedWords.begin(), acceptedWords.end());
    QVector<QPair<QString, int>> bestWords = getBestStartingWords(answerSet, acceptedSet, 10);
    stats += "Rank  Word    Score\n";
    stats += "----------------------\n";
    for (int i = 0; i < bestWords.size(); ++i) {
        stats += QString("%1     %2    %3\n").arg(i+1, 4).arg(bestWords[i].first, 4).arg(bestWords[i].second, 8);
    }
    
    statsDisplay->setPlainText(stats);
}

void StatsWindow::onBackToMenu() {
    emit backToMenuRequested();
    this->close();
}

// ============================================================================
// SolverWindow Implementation
// ============================================================================

SolverWindow::SolverWindow(QWidget *parent) : QWidget(parent) {
    setWindowTitle("Wordle Solver");
    setFixedSize(600, 500);
    
    layout = new QVBoxLayout(this);
    
    // Set dark background for the window
    this->setStyleSheet("background-color: #2D2D2D;");

    // Title
    QLabel *titleLabel = new QLabel("Wordle Solver", this);
    titleLabel->setAlignment(Qt::AlignCenter);
    titleLabel->setStyleSheet("QLabel { font-size: 28px; font-weight: bold; color: white; margin: 10px; }");
    layout->addWidget(titleLabel);
    
    // Feedback input area
    feedbackLayout = new QHBoxLayout();
    
    // Green letters (correct position)
    for (int i = 0; i < 5; ++i) {
        greenBoxes[i] = new QLineEdit(this);
        greenBoxes[i]->setMaxLength(1);
        greenBoxes[i]->setFixedWidth(40);
        greenBoxes[i]->setStyleSheet("QLineEdit { font-size: 18px; padding: 10px; border: 2px solid #4CAF50; background: black; color: white; }");
        greenBoxes[i]->setPlaceholderText("");
        feedbackLayout->addWidget(greenBoxes[i]);
        
        // Auto-tab to next green box when a letter is entered
        connect(greenBoxes[i], &QLineEdit::textChanged, [this, i](const QString &text) {
            if (text.length() == 1 && i < 4) {
                greenBoxes[i + 1]->setFocus();
            }
        });
    }
    
    feedbackLayout->addSpacing(20);
    
    // Yellow letters (wrong position)
    for (int i = 0; i < 5; ++i) {
        yellowBoxes[i] = new QLineEdit(this);
        yellowBoxes[i]->setMaxLength(5);
        yellowBoxes[i]->setFixedWidth(40);
        yellowBoxes[i]->setStyleSheet("QLineEdit { font-size: 18px; padding: 10px; border: 2px solid orange; background: black; color: white; }");
        yellowBoxes[i]->setPlaceholderText("");
        feedbackLayout->addWidget(yellowBoxes[i]);
    }
    layout->addLayout(feedbackLayout);

    // Gray letters (not in word) - placed below
    QHBoxLayout *grayLayout = new QHBoxLayout();
    grayInput = new QLineEdit(this);
    grayInput->setMaxLength(26);
    grayInput->setFixedWidth(220);
    grayInput->setStyleSheet("QLineEdit { font-size: 18px; padding: 10px; border: 2px solid #9E9E9E; background: black; color: white; }");
    grayInput->setPlaceholderText("");
    grayLayout->addWidget(grayInput);
    grayLayout->addStretch();
    layout->addLayout(grayLayout);

    // Buttons
    buttonLayout = new QHBoxLayout();
    QString mainButtonStyle = "QPushButton { font-size: 16px; padding: 15px; margin: 5px; background-color: #444; color: white; border: none; border-radius: 5px; } QPushButton:hover { background-color: #666; }";
    QString clearButtonStyle = "QPushButton { font-size: 16px; padding: 15px; margin: 5px; background-color: #444; color: white; border: none; border-radius: 5px; } QPushButton:hover { background-color: #666; }";
    QString backButtonStyle = "QPushButton { font-size: 14px; padding: 10px; background-color: #444; color: white; border: none; border-radius: 5px; } QPushButton:hover { background-color: #666; }";
    updateGuessesButton = new QPushButton("Update Top Guesses", this);
    updateGuessesButton->setStyleSheet(mainButtonStyle);
    clearAllButton = new QPushButton("Clear All", this);
    clearAllButton->setStyleSheet(clearButtonStyle);
    buttonLayout->addWidget(updateGuessesButton);
    buttonLayout->addWidget(clearAllButton);
    layout->addLayout(buttonLayout);

    // Back button
    backToMenuButton = new QPushButton("Back to Menu", this);
    backToMenuButton->setStyleSheet(backButtonStyle);
    layout->addWidget(backToMenuButton);
    
    // Connect signals
    connect(updateGuessesButton, &QPushButton::clicked, this, &SolverWindow::onUpdateGuesses);
    connect(clearAllButton, &QPushButton::clicked, this, &SolverWindow::onClearAll);
    connect(backToMenuButton, &QPushButton::clicked, this, &SolverWindow::onBackToMenu);
    connect(grayInput, &QLineEdit::returnPressed, this, &SolverWindow::onUpdateGuesses);
    
    setLayout(layout);
    
    // Load word lists
    loadWordLists();
    
    // Initialize letter states
    QString letters = "ABCDEFGHIJKLMNOPQRSTUVWXYZ";
    for (QChar c : letters) {
        letterStates[c] = 0;
    }

    // Add optimal guess label above possibleAnswersBox
    optimalGuessLabel = new QLabel("Optimal Guess: ", this);
    optimalGuessLabel->setStyleSheet("QLabel { font-size: 16px; font-weight: bold; color: white; margin-bottom: 4px; }");
    layout->addWidget(optimalGuessLabel);

    // possibleAnswersBox as before
    possibleAnswersBox = new QLabel("", this);
    possibleAnswersBox->setAlignment(Qt::AlignLeft | Qt::AlignTop);
    possibleAnswersBox->setStyleSheet("QLabel { font-size: 16px; font-weight: bold; color: white; background: transparent; min-height: 120px; max-height: 120px; }");
    possibleAnswersBox->setMinimumHeight(120);
    possibleAnswersBox->setMaximumHeight(120);
    possibleAnswersBox->setWordWrap(true);
    layout->addWidget(possibleAnswersBox);
}

void SolverWindow::loadWordLists() {
    QStringList answerList = loadWordList("WordList.txt");
    QStringList acceptedList = loadWordList("AcceptedWordList");
    
    answerWords = QSet<QString>(answerList.begin(), answerList.end());
    acceptedWords = QSet<QString>(acceptedList.begin(), acceptedList.end());
}

void SolverWindow::onUpdateGuesses() {
    updateLetterStates();
    QVector<QPair<QString, int>> topGuesses = findTopGuesses(10); // get top 10 possible answers
    if (topGuesses.isEmpty()) {
        optimalGuessLabel->setText("Optimal Guess: None");
        possibleAnswersBox->setText("No valid words found with current constraints");
    } else {
        optimalGuessLabel->setText(QString("Optimal Guess: %1").arg(topGuesses[0].first));
        int perRow = 5; // Reduced from 8 to 5 for better readability
        QString display;
        for (int i = 0; i < topGuesses.size(); ++i) {
            display += topGuesses[i].first;
            if ((i + 1) % perRow == 0)
                display += "\n";
            else
                display += "     "; // Increased spacing from 4 to 5 spaces
        }
        possibleAnswersBox->setText(display);
    }
}

void SolverWindow::onClearAll() {
    guessFeedbackPairs.clear();
    letterStates.clear();
    
    // Reset letter states
    QString letters = "ABCDEFGHIJKLMNOPQRSTUVWXYZ";
    for (QChar c : letters) {
        letterStates[c] = 0;
    }
    
    // Clear all input boxes
    for (int i = 0; i < 5; ++i) {
        greenBoxes[i]->clear();
        yellowBoxes[i]->clear();
    }
    grayInput->clear();
    
    possibleAnswersBox->clear();
}

void SolverWindow::updateLetterStates() {
    // Reset letter states
    QString letters = "ABCDEFGHIJKLMNOPQRSTUVWXYZ";
    for (QChar c : letters) {
        letterStates[c] = 0;
    }
    
    // Process green letters
    for (int i = 0; i < 5; ++i) {
        QString greenLetter = greenBoxes[i]->text().trimmed().toUpper();
        if (!greenLetter.isEmpty() && greenLetter.length() == 1 && greenLetter[0].isLetter()) {
            letterStates[greenLetter[0]] = 3; // Green
        }
    }
    
    // Process yellow letters
    for (int i = 0; i < 5; ++i) {
        QString yellowLetters = yellowBoxes[i]->text().trimmed().toUpper();
        for (QChar c : yellowLetters) {
            if (c.isLetter() && letterStates[c] < 2) {
                letterStates[c] = 2; // Yellow
            }
        }
    }
    
    // Process gray letters
    QString grayLetters = grayInput->text().trimmed().toUpper();
    for (QChar c : grayLetters) {
        if (c.isLetter() && letterStates[c] == 0) {
            letterStates[c] = 1; // Gray
        }
    }
}

QVector<QPair<QString, int>> SolverWindow::findTopGuesses(int count) {
    // If all feedback boxes are empty, use getBestStartingWords
    bool allEmpty = true;
    for (int i = 0; i < 5; ++i) {
        if (!greenBoxes[i]->text().trimmed().isEmpty() || !yellowBoxes[i]->text().trimmed().isEmpty()) {
            allEmpty = false;
            break;
        }
    }
    if (allEmpty && grayInput->text().trimmed().isEmpty()) {
        QVector<QPair<QString, int>> best = getBestStartingWords(answerWords, acceptedWords, count);
        return best;
    }
    
    // Build constraints from UI
    QMap<int, QChar> greenLetters;
    QMap<QChar, QSet<int>> yellowPositions;
    QSet<QChar> yellowLetters;
    QSet<QChar> grayLetters;
    
    for (int i = 0; i < 5; ++i) {
        QString green = greenBoxes[i]->text().trimmed().toUpper();
        if (!green.isEmpty() && green[0].isLetter()) greenLetters[i] = green[0];
        
        QString yellow = yellowBoxes[i]->text().trimmed().toUpper();
        for (QChar c : yellow) {
            if (c.isLetter()) {
                yellowLetters.insert(c);
                yellowPositions[c].insert(i);
            }
        }
    }
    
    QString gray = grayInput->text().trimmed().toUpper();
    for (QChar c : gray) if (c.isLetter()) grayLetters.insert(c);
    
    // Get all valid words that match constraints
    QVector<QPair<QString, int>> validWords = getAllValidWordsWithConstraints(answerWords, acceptedWords, greenLetters, yellowPositions, yellowLetters, grayLetters);
    
    // Sort by score (highest first) and return top count
    std::sort(validWords.begin(), validWords.end(), [](const QPair<QString, int>& a, const QPair<QString, int>& b) {
        return a.second > b.second;
    });
    
    if (validWords.size() > count) {
        validWords.resize(count);
    }
    
    return validWords;
}

void SolverWindow::onBackToMenu() {
    emit backToMenuRequested();
    this->close();
} 