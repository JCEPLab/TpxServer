#include "ui/MainWindow.h"

#include "logger.h"

#include <iostream>
#include <stdexcept>
#include <chrono>
#include <thread>

#include <QThreadPool>

MainWindow* sMainWindow {nullptr};

MainWindow::MainWindow(bool autostart) :
    QMainWindow() {

    if(sMainWindow)
        throw std::runtime_error("Attempted to open two MainWindows at once");
    sMainWindow = this;

    setWindowTitle("Timepix3 Server");

    QThreadPool::globalInstance()->setMaxThreadCount(QThread::idealThreadCount());

    mCentralWidget = new QWidget(this);
    mCentralLayout = new QVBoxLayout(mCentralWidget);

    setCentralWidget(mCentralWidget);

    initializeActions();

    {
        mLogWindow = new LogWindow(this);
    }

    {
        mSettingsPanel = new SettingsPanel(this);
        mCentralLayout->addWidget(mSettingsPanel);
    }

    {
        mStatusPanel = new StatusPanel(this);
        mCentralLayout->addWidget(mStatusPanel);
    }

    {
        mLaunchServerButton = new QPushButton("Launch Server");
        mCentralLayout->addWidget(mLaunchServerButton);
        connect(mLaunchServerButton, &QPushButton::clicked, mActions.launchServerAction, &QAction::trigger);
    }

    {
        mStopServerButton = new QPushButton("Stop Server");
        mStopServerButton->setEnabled(false);
        mCentralLayout->addWidget(mStopServerButton);
        connect(mStopServerButton, &QPushButton::clicked, mActions.stopServerAction, &QAction::trigger);
    }

    show();

    mLogWindow->move({this->pos().x() + this->size().width() + 20, this->pos().y()});
    mLogWindow->resize(400, 400);
    mLogWindow->show();

    if(autostart)
        launchServer();

}

void MainWindow::initializeActions() {

    mActions = {
            .launchServerAction = new QAction(this),
            .stopServerAction = new QAction(this)
    };

    connect(mActions.launchServerAction, &QAction::triggered, this, &MainWindow::launchServer);
    connect(mActions.stopServerAction, &QAction::triggered, this, &MainWindow::stopServer);

}

LogWindow* MainWindow::getLogger() {

    return mLogWindow;

}

MainWindow* MainWindow::getInstance() {

    return sMainWindow;

}

void MainWindow::launchServer() {

    mManuallyCancelled = false;

    mSettingsPanel->setEnabled(false);
    mLaunchServerButton->setEnabled(false);
    mStopServerButton->setEnabled(true);

    mStatusPanel->setServerRunning(true);

    logger::log("Starting server...");

    if(mActiveThread)
        throw std::runtime_error("Attempted to start a new server thread while one already exists");

    auto settings = mSettingsPanel->getSettings();

    mActiveThread = new CommsThread(settings);

    mLogWindow->connectToThread(mActiveThread);
    connect(mActiveThread, &CommsThread::threadDone, this, &MainWindow::serverDone);

    QThreadPool::globalInstance()->start(mActiveThread);

}

void MainWindow::stopServer() {

    logger::log("Stopping server...");

    if(DEBUG_OUTPUT)
        std::cout << "Terminating server from main window" << std::endl;

    mManuallyCancelled = true;

    if(mActiveThread)
        mActiveThread->cancel();

}

void MainWindow::serverDone() {

    mSettingsPanel->setEnabled(true);
    mLaunchServerButton->setEnabled(true);
    mStopServerButton->setEnabled(false);

    mActiveThread = nullptr;

    mStatusPanel->setServerRunning(false);

    if(!mManuallyCancelled && mSettingsPanel->autoRestartChecked()) {
        logger::log("Restarting server...");
        if(DEBUG_OUTPUT)
            std::cout << "Auto-restarting server" << std::endl;
        std::this_thread::sleep_for(std::chrono::milliseconds(5000));
        launchServer();
    }

}

void MainWindow::closeEvent(QCloseEvent *event) {

    stopServer();
    QMainWindow::closeEvent(event);

}
