#include "logger.h"

#include "ui/MainWindow.h"

void logger::log(const std::string &msg) {

    MainWindow::getInstance()->getLogger()->log(msg);

}

void logger::warn(const std::string &msg) {

    MainWindow::getInstance()->getLogger()->warn(msg);

}

void logger::err(const std::string &msg) {

    MainWindow::getInstance()->getLogger()->err(msg);

}