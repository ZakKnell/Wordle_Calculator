#ifdef USE_QT
#include <QApplication>
#include "WordleWindow.h"

int main(int argc, char *argv[]) {
    QApplication app(argc, argv);
    MainMenuWindow window;
    window.show();
    return app.exec();
}
#else
#include <iostream>

int main(int argc, char *argv[]) {
    std::cout << "GUI not available. Rebuild with Qt support." << std::endl;
    return 1;
}
#endif