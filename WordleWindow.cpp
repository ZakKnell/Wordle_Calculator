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
#include <QDebug>
#include <QTextEdit>
#include <QApplication>
#include <QGridLayout>
#include <QSpacerItem>
#include <QHBoxLayout>

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

WordleWindow::WordleWindow(QWidget *parent) : QWidget(parent), guesses(0) {
    qDebug() << "WordleWindow constructor starting...";
    setWindowTitle("Wordle (GUI)");
    qDebug() << "Window title set...";
    
    layout = new QVBoxLayout(this);
    input = new QLineEdit(this);
    guessButton = new QPushButton("Guess", this);
    newGameButton = new QPushButton("New Game", this);
    exitButton = new QPushButton("Exit", this);
    feedbackLabel = new QLabel(this);
    messageLabel = new QLabel(this);
    guessesDisplay = new QTextEdit(this);
    guessesDisplay->setMaximumHeight(150);
    guessesDisplay->setReadOnly(true);
    keyboardLayout = new QGridLayout();
    qDebug() << "Widgets created...";
    
    layout->addWidget(new QLabel("Enter your guess:", this));
    layout->addWidget(input);
    layout->addWidget(guessButton);
    layout->addWidget(new QLabel("Previous guesses:", this));
    layout->addWidget(guessesDisplay);
    layout->addWidget(feedbackLabel);
    layout->addWidget(messageLabel);
    layout->addWidget(new QLabel("Keyboard:", this));
    
    // Add keyboard layout
    QWidget *keyboardWidget = new QWidget(this);
    keyboardWidget->setLayout(keyboardLayout);
    layout->addWidget(keyboardWidget);
    
    // Add new game button just above the exit button
    layout->addWidget(newGameButton);
    // Add exit button at the bottom
    layout->addWidget(exitButton);
    
    setLayout(layout);
    qDebug() << "Layout set...";
    
    connect(guessButton, &QPushButton::clicked, this, &WordleWindow::onGuess);
    connect(newGameButton, &QPushButton::clicked, this, &WordleWindow::onNewGame);
    connect(exitButton, &QPushButton::clicked, this, &QApplication::quit);
    connect(input, &QLineEdit::returnPressed, this, &WordleWindow::onGuess);
    qDebug() << "Connections made...";
    
    // Initialize validWords once
    qDebug() << "Loading word lists...";
    QStringList wordList = loadWordList("WordList.txt");
    qDebug() << "WordList.txt loaded, size:" << wordList.size();
    QStringList acceptedList = loadWordList("AcceptedWordList");
    qDebug() << "AcceptedWordList loaded, size:" << acceptedList.size();
    
    validWords = QSet<QString>(wordList.begin(), wordList.end());
    validWords.unite(QSet<QString>(acceptedList.begin(), acceptedList.end()));
    qDebug() << "ValidWords initialized, size:" << validWords.size();
    
    setupKeyboard();
    startNewGame();
    qDebug() << "Constructor completed.";
}

void WordleWindow::setupKeyboard() {
    QString letters = "QWERTYUIOPASDFGHJKLZXCVBNM";
    
    // Create horizontal layouts for each row
    QHBoxLayout *row1Layout = new QHBoxLayout();
    QHBoxLayout *row2Layout = new QHBoxLayout();
    QHBoxLayout *row3Layout = new QHBoxLayout();
    
    for (int i = 0; i < 26; ++i) {
        keyboardButtons[i] = new QPushButton(QString(letters[i]), this);
        keyboardButtons[i]->setFixedSize(30, 30);
        keyboardButtons[i]->setStyleSheet("QPushButton { background-color: white; color: black; border: 1px solid gray; }");
        
        // Layout: QWERTYUIOP (10 letters), ASDFGHJKL (9 letters), ZXCVBNM (7 letters)
        if (i < 10) {
            // First row: QWERTYUIOP
            row1Layout->addWidget(keyboardButtons[i]);
        } else if (i < 19) {
            // Second row: ASDFGHJKL
            row2Layout->addWidget(keyboardButtons[i]);
        } else {
            // Third row: ZXCVBNM
            row3Layout->addWidget(keyboardButtons[i]);
        }
        
        letterStates[letters[i]] = 0; // Initialize as unused
    }
    
    // Add spacers to create brick wall alignment
    // First row: center the 10 letters
    row1Layout->insertStretch(0, 1);
    row1Layout->addStretch(1);
    
    // Second row: add half-space indent to the left
    row2Layout->insertSpacing(0, 15); // Fixed 15px indent for half-space
    row2Layout->addStretch(1);
    
    // Third row: add a full key indent to the left (30px) to shift half a key to the right relative to the second row
    row3Layout->insertSpacing(0, 30); // Full key width
    row3Layout->addStretch(1);
    
    // Add all rows to the keyboard layout
    keyboardLayout->addLayout(row1Layout, 0, 0);
    keyboardLayout->addLayout(row2Layout, 1, 0);
    keyboardLayout->addLayout(row3Layout, 2, 0);
}

void WordleWindow::updateKeyboard(const QString &guess, const QString &feedback) {
    QString letters = "QWERTYUIOPASDFGHJKLZXCVBNM";
    
    for (int i = 0; i < 5; ++i) {
        QChar letter = guess[i];
        int letterIndex = letters.indexOf(letter);
        
        if (letterIndex == -1) continue; // Skip if letter not found
        
        int newState = 0;
        
        if (feedback[i] == 'G') {
            newState = 3; // Green
        } else if (feedback[i] == 'Y') {
            newState = 2; // Yellow
        } else {
            newState = 1; // Gray
        }
        
        // Only update if the new state is better (higher priority)
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

void WordleWindow::startNewGame() {
    QStringList answers = loadWordList("WordList.txt");
    if (answers.isEmpty()) {
        QMessageBox::critical(this, "Error", "Could not load WordList.txt");
        close();
        return;
    }
    answer = answers.at(QRandomGenerator::global()->bounded(answers.size()));
    guesses = 0;
    input->clear();
    feedbackLabel->clear();
    guessesDisplay->clear();
    
    // Reset keyboard
    for (int i = 0; i < 26; ++i) {
        keyboardButtons[i]->setStyleSheet("QPushButton { background-color: white; color: black; border: 1px solid gray; }");
        letterStates[QChar('A' + i)] = 0;
    }
    
    messageLabel->setText("You have 5 guesses.");
    input->setEnabled(true);
    guessButton->setEnabled(true);
}

void WordleWindow::onGuess() {
    QString guess = input->text().trimmed().toUpper();
    if (guess.length() != 5) {
        messageLabel->setText("Please enter a 5-letter word.");
        return;
    }
    // Use member validWords
    if (!validWords.contains(guess)) {
        messageLabel->setText("Not a valid word.");
        return;
    }
    guesses++;
    
    // Generate feedback for this guess
    QString feedback = generateFeedback(guess);
    
    // Update keyboard with this guess
    updateKeyboard(guess, feedback);
    
    // Add guess to display with colored characters
    QString coloredWord;
    for (int i = 0; i < 5; ++i) {
        if (feedback[i] == 'G') {
            coloredWord += "<span style='color: green; font-weight: bold;'>" + QString(guess[i]) + "</span>";
        } else if (feedback[i] == 'Y') {
            coloredWord += "<span style='color: orange; font-weight: bold;'>" + QString(guess[i]) + "</span>";
        } else {
            coloredWord += "<span style='color: gray; font-weight: bold;'>" + QString(guess[i]) + "</span>";
        }
    }
    
    QString guessText = QString("Guess %1: %2").arg(guesses).arg(coloredWord);
    guessesDisplay->append(guessText);
    
    showFeedback(guess);
    if (guess == answer) {
        messageLabel->setText("Congratulations! You guessed the word!");
        input->setEnabled(false);
        guessButton->setEnabled(false);
    } else if (guesses >= 5) {
        messageLabel->setText("Sorry, you lost! The word was: " + answer);
        input->setEnabled(false);
        guessButton->setEnabled(false);
    } else {
        messageLabel->setText(QString("Guesses left: %1").arg(5 - guesses));
    }
    input->clear();
}

QString WordleWindow::generateFeedback(const QString &guess) {
    QString feedback = "XXXXX";
    QVector<bool> used(5, false);
    
    // First pass: correct position
    for (int i = 0; i < 5; ++i) {
        if (guess[i] == answer[i]) {
            feedback[i] = 'G';
            used[i] = true;
        }
    }
    
    // Second pass: correct letter, wrong position
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

void WordleWindow::showFeedback(const QString &guess) {
    QString feedback = "XXXXX";
    QVector<bool> used(5, false);
    // First pass: correct position
    for (int i = 0; i < 5; ++i) {
        if (guess[i] == answer[i]) {
            feedback[i] = 'G';
            used[i] = true;
        }
    }
    // Second pass: correct letter, wrong position
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
    // Build HTML for display
    QString display;
    for (int i = 0; i < 5; ++i) {
        if (feedback[i] == 'G') display += "<b style='color:green'>G</b>";
        else if (feedback[i] == 'Y') display += "<b style='color:orange'>Y</b>";
        else display += "<b style='color:gray'>X</b>";
    }
    feedbackLabel->setText(display + "  (" + guess + ")");
}

void WordleWindow::onNewGame() {
    startNewGame();
} 