#ifndef TPXSERVER_LOGWINDOW_H
#define TPXSERVER_LOGWINDOW_H

#include <string>

#include <QPlainTextEdit>

class BgThread;

class LogWindow : public QPlainTextEdit {
Q_OBJECT

public:
    explicit LogWindow(QWidget *parent);

    void connectToThread(BgThread *thread);

public slots:
    void log(const std::string &str); // copy necessary to avoid concurrent access
    void warn(const std::string &str);
    void err(const std::string &str);

private:

};

#endif //TPXSERVER_LOGWINDOW_H
