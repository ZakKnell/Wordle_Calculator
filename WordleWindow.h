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
class QHBoxLayout;

// Main menu window class
class MainMenuWindow : public QWidget {
    Q_OBJECT
public:
    MainMenuWindow(QWidget *parent = nullptr);

private slots:
    void onPlayWordle();
    void onViewStats();
    void onSolver();
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

// Solver window class
class SolverWindow : public QWidget {
    Q_OBJECT
public:
    SolverWindow(QWidget *parent = nullptr);

signals:
    void backToMenuRequested();

private slots:
    void onBackToMenu();
    void onUpdateGuesses();
    void onClearAll();

private:
    void updateGuessesDisplay();
    void updateLetterStates();
    QVector<QPair<QString, int>> findTopGuesses(int count);
    void loadWordLists();
    
    QPushButton *backToMenuButton;
    QPushButton *updateGuessesButton;
    QPushButton *clearAllButton;
    QLineEdit *greenBoxes[5];
    QLineEdit *yellowBoxes[5];
    QLineEdit *grayInput;
    QLabel *optimalGuessLabel;
    QLabel *possibleAnswersBox;
    QTextEdit *guessesDisplay;
    QVBoxLayout *layout;
    QHBoxLayout *feedbackLayout;
    QHBoxLayout *buttonLayout;
    QSet<QString> answerWords;
    QSet<QString> acceptedWords;
    QVector<QPair<QString, QString>> guessFeedbackPairs; // guess and feedback pairs
    QMap<QChar, int> letterStates; // 0=unused, 1=gray, 2=yellow, 3=green
};

QString findOptimalGuessShared(const QSet<QString> &answerWords, const QSet<QString> &acceptedWords, const QList<QPair<QString, QString>> &guessFeedbackPairs, const QMap<int, QChar> &greenLetters, const QMap<QChar, QSet<int>> &yellowPositions, const QSet<QChar> &yellowLetters, const QSet<QChar> &grayLetters);
QString findOptimalGuessWithConstraints(const QSet<QString>& answerWords, const QSet<QString>& acceptedWords, const QMap<int, QChar>& greenLetters, const QMap<QChar, QSet<int>>& yellowPositions, const QSet<QChar>& yellowLetters, const QSet<QChar>& grayLetters);
QVector<QPair<QString, int>> getAllValidWordsWithConstraints(const QSet<QString>& answerWords, const QSet<QString>& acceptedWords, const QMap<int, QChar>& greenLetters, const QMap<QChar, QSet<int>>& yellowPositions, const QSet<QChar>& yellowLetters, const QSet<QChar>& grayLetters);

#endif // WORDLEWINDOW_H 