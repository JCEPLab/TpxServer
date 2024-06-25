#include <QApplication>

#include "ui/MainWindow.h"

#include <chrono>
#include <thread>

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

    auto retval = QApplication::exec();

    std::this_thread::sleep_for(std::chrono::milliseconds(1000));

    return retval;

}
