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

class WordleWindow : public QWidget {
    Q_OBJECT
public:
    WordleWindow(QWidget *parent = nullptr);

private slots:
    void onGuess();
    void onNewGame();

private:
    void startNewGame();
    void showFeedback(const QString &guess);
    QString generateFeedback(const QString &guess);
    void updateKeyboard(const QString &guess, const QString &feedback);
    void setupKeyboard();
    
    QString answer;
    int guesses;
    QLineEdit *input;
    QPushButton *guessButton;
    QPushButton *newGameButton;
    QPushButton *exitButton;
    QLabel *feedbackLabel;
    QLabel *messageLabel;
    QTextEdit *guessesDisplay;
    QVBoxLayout *layout;
    QGridLayout *keyboardLayout;
    QPushButton *keyboardButtons[26];
    QMap<QChar, int> letterStates; // 0=unused, 1=gray, 2=yellow, 3=green
    QSet<QString> validWords;
};

#endif // WORDLEWINDOW_H 