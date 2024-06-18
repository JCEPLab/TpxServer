#include "ui/LogWindow.h"

#include "BgThread.h"

LogWindow::LogWindow(QWidget *parent) :
        QPlainTextEdit(parent) {

    setWindowFlags(Qt::Window | Qt::WindowMinimizeButtonHint | Qt::WindowMaximizeButtonHint);
    setWindowTitle("Timepix3 Server Log");

    setReadOnly(true);
    setMinimumSize(700, 400);

}

void LogWindow::log(const std::string &str) {

    std::string html_str = "<b>Log:</b> " + str;
    appendHtml(html_str.c_str());

}

void LogWindow::warn(const std::string &str) {

    std::string html_str = "<b style=\"color:orange\">Warning:</b> " + str;
    appendHtml(html_str.c_str());

}

void LogWindow::err(const std::string &str) {

    std::string html_str = "<b style=\"color:red\">Error:</b> " + str;
    appendHtml(html_str.c_str());

}

void LogWindow::connectToThread(BgThread *thread) {

    // signals allowing the thread to log to the main window
    connect(thread, &BgThread::log, this, &LogWindow::log);
    connect(thread, &BgThread::warn, this, &LogWindow::warn);
    connect(thread, &BgThread::err, this, &LogWindow::err);

}
