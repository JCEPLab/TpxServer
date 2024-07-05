#include "server/CommsThread.h"

#include <string>
#include <utility>
#include <functional>
#include <iostream>

#include <QThreadPool>

#include "asio.hpp"
#include "zmq.hpp"

#include "server/UdpThread.h"
#include "server/ClusterThread.h"
#include "server/HistogramThread.h"

#include "server/TimepixConnectionManager.h"
#include "server/PythonConnectionManager.h"

CommsThread::CommsThread(CommsSettings settings) :
        BgThread() {

    mSettings = std::move(settings);

}

void CommsThread::execute() {

    std::string startup_text = "Timepix server has started"
                                "<br>Host: " + mSettings.host_ip + ":" + std::to_string(mSettings.host_port)
                                + "<br>Timepix: " + mSettings.timepix_ip + ":" + std::to_string(mSettings.timepix_port)
                                + "<br>Outgoing data: tcp://localhost:" + std::to_string(mSettings.outgoing_port);

    emit log(startup_text);

    mTpxManager = std::make_unique<TimepixConnectionManager>(*this);
    mTpxManager->attemptConnection(mSettings.host_ip, mSettings.host_port, mSettings.timepix_ip, mSettings.timepix_port);

    mClientManager = std::make_unique<PythonConnectionManager>(*this, *mTpxManager.get());
    mClientManager->open(mSettings.outgoing_port);

    startClusterThread();
    startHistogramThread();

    while(!shouldCancel()) {

        if(mShouldResetClient) {
            mClientManager.reset();
            mClientManager = std::make_unique<PythonConnectionManager>(*this, *mTpxManager.get());
            mClientManager->open(mSettings.outgoing_port);
            mShouldResetClient = false;
        }

        try {
            mTpxManager->poll();

            if(!mTpxManager->isExecutingCommand())
                mClientManager->poll();
        } catch (asio::system_error &ex) {
            emit err("A network error occurred while communicating with the Timepix (errcode=" + std::to_string(ex.code().value()) + ")");
            emit err(ex.what());
            emit err("Server is shutting down.");
            DEBUG("Terminating TCP thread due to a network error: [" + std::to_string(ex.code().value()) + "]: " + ex.what());
            break;
        } catch (const std::exception &ex) {
            emit err("An unknown error occurred:");
            emit err(ex.what());
            emit err("Server is shutting down.");
            DEBUG("Terminating TCP thread due to an unknown error: " + std::string(ex.what()));
            cancel();
            break;
        }

    }

    mTpxManager->clearAnyClientRequest();
    mTpxManager->terminateConnection();

    if(mUdpThread) {
        mUdpThread->cancel();
        mUdpThread = nullptr; // should auto-delete once run() is finished
    }
    if(mUdpCommandSocket)
        mUdpCommandSocket->close();

    if(mClusterThread) {
        mClusterThread->cancel();
        mClusterThread = nullptr; // should auto-delete once run() is finished
    }
    if(mClusterCommandSocket)
        mClusterCommandSocket->close();

    if(mHistogramThread) {
        mHistogramThread->cancel();
        mHistogramThread = nullptr; // should auto-delete once run() is finished
    }
    if(mHistogramCommandSocket)
        mHistogramCommandSocket->close();

    DEBUG("TCP thread terminated");

    emit log("Server thread shutting down");

}

void CommsThread::resetClientConnection() {

    mShouldResetClient = true;

    if(mUdpThread)
        mUdpThread->cancel();    if(mUdpThread) {
        mUdpThread->cancel();
        mUdpThread = nullptr; // should auto-delete once run() is finished
    }

}

void CommsThread::connectThread(BgThread *thread) {

    connect(thread, &BgThread::log, this, &BgThread::log);
    connect(thread, &BgThread::warn, this, &BgThread::warn);
    connect(thread, &BgThread::err, this, &BgThread::err);

}

void CommsThread::bindUdpPort(unsigned host_port) {

    if(mUdpThread) {
        mUdpThread->cancel();
        mUdpThread = nullptr; // should auto-delete once run() is finished
    }

    mUdpThread = new UdpThread(*this, mSettings.host_ip, host_port);
    mUdpCommandSocket = mUdpThread->getCommandClient();
    QThreadPool::globalInstance()->start(mUdpThread);

}

void CommsThread::startClusterThread() {

    if(mClusterThread)
        return;

    mClusterThread = new ClusterThread(*this);
    mClusterCommandSocket = mClusterThread->getCommandClient();
    QThreadPool::globalInstance()->start(mClusterThread);

}

void CommsThread::startHistogramThread() {

    if(mHistogramThread)
        return;

    mHistogramThread = new HistogramThread(*this);
    mHistogramCommandSocket = mHistogramThread->getCommandClient();
    QThreadPool::globalInstance()->start(mHistogramThread);

}

zmq::context_t& CommsThread::getZmq() {

    return mZmq;

}

zmq::socket_t* CommsThread::getUdpThreadSocket() {

    return mUdpCommandSocket.get();

}

zmq::socket_t* CommsThread::getClusterThreadSocket() {

    return mClusterCommandSocket.get();

}

zmq::socket_t* CommsThread::getHistogramThreadSocket() {

    return mHistogramCommandSocket.get();

}
