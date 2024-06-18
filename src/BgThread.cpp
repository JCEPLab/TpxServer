#include "BgThread.h"

BgThread::BgThread() :
        QObject(),
        QRunnable() {

    // Do nothing

}

void BgThread::run() {

    try {
        execute();
        emit threadDone();
    } catch (...) {
        emit err("An unknown exception has occurred. Shutting down server.");
        emit threadDone();
    }

}

bool BgThread::shouldCancel() {

    return mShouldCancel;

}

void BgThread::cancel() {

    mShouldCancel = true;

}

void BgThread::sendLog(const std::string &str) {
    emit log(str);
}

void BgThread::sendWarn(const std::string &str) {
    emit warn(str);
}

void BgThread::sendErr(const std::string &str) {
    emit err(str);
}
