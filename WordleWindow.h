#ifndef WORDLEWINDOW_H
#define WORDLEWINDOW_H

#include <QWidget>
#include <QMap>

class QLineEdit;
class QPushButton;
class QLabel;
class QVBoxLayout;
class QTextEdit;
class QGridLayout;

// Main menu window class
class MainMenuWindow : public QWidget {
    Q_OBJECT
public:
    MainMenuWindow(QWidget *parent = nullptr);

private slots:
    void onPlayWordle();
    void onViewStats();
    void onExit();

private:
    QPushButton *playButton;
    QPushButton *statsButton;
    QPushButton *exitButton;
    QVBoxLayout *layout;
};

// Game window class (renamed from WordleWindow)
class WordleGameWindow : public QWidget {
    Q_OBJECT
public:
    WordleGameWindow(QWidget *parent = nullptr);

signals:
    void backToMenuRequested();

private slots:
    void onGuess();
    void onNewGame();
    void onBackToMenu();
    void onShowOptimalGuess();

private:
    void startNewGame();
    QString generateFeedback(const QString &guess);
    void updateKeyboard(const QString &guess, const QString &feedback);
    void setupKeyboard();
    QString findOptimalGuess();
    void updateOptimalGuess();
    
    QString answer;
    int guesses;
    bool optimalGuessMode;
    QLineEdit *input;
    QPushButton *newGameButton;
    QPushButton *backToMenuButton;
    QPushButton *optimalGuessButton;
    QLabel *optimalGuessLabel;
    QLabel *messageLabel;
    QTextEdit *guessesDisplay;
    QVBoxLayout *layout;
    QGridLayout *keyboardLayout;
    QPushButton *keyboardButtons[26];
    QMap<QChar, int> letterStates; // 0=unused, 1=gray, 2=yellow, 3=green
    QSet<QString> validWords;
    QVector<QString> guessHistory;
    QVector<QString> feedbackHistory;
};

// Stats window class
class StatsWindow : public QWidget {
    Q_OBJECT
public:
    StatsWindow(QWidget *parent = nullptr);

signals:
    void backToMenuRequested();

private slots:
    void onBackToMenu();

private:
    void loadAndDisplayStats();
    QPushButton *backToMenuButton;
    QTextEdit *statsDisplay;
    QVBoxLayout *layout;
};

#endif // WORDLEWINDOW_H 