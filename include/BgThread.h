#ifndef TPXSERVER_BGTHREAD_H
#define TPXSERVER_BGTHREAD_H

#include <atomic>

#include <QObject>
#include <QRunnable>
#include <QProgressDialog>

class LogWindow;

// Handles common functionality of background long-running threads
class BgThread : public QObject, public QRunnable {
Q_OBJECT

public:
    BgThread();

    void run() final;
    virtual void execute() = 0; // override this in the child classes
    [[nodiscard]] bool shouldCancel();

    void sendLog(const std::string &str);
    void sendWarn(const std::string &str);
    void sendErr(const std::string &str);

signals:
    void log(std::string str);
    void warn(std::string str);
    void err(std::string str);

    void threadDone(); // emitted when the thread finishes

public slots:
    void cancel();

private:
    std::atomic<bool> mShouldCancel {false};
};

#endif //TPXSERVER_BGTHREAD_H
