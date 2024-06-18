#include <QApplication>

#include "ui/MainWindow.h"

#include "asio.hpp"

int main(int argc, char *argv[]) {

    QApplication app(argc, argv);

    auto arguments = QCoreApplication::arguments();
    bool autostart = false;
    for(const auto &arg : arguments) {
        if(arg == "--autostart")
            autostart = true;
    }
    MainWindow window(autostart);

    return QApplication::exec();

}
