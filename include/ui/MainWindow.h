#ifndef TPXSERVER_MAINWINDOW_H
#define TPXSERVER_MAINWINDOW_H

#include <QMainWindow>
#include <QWidget>
#include <QVBoxLayout>
#include <QPushButton>
#include <QAction>

#include "ui/LogWindow.h"
#include "ui/SettingsPanel.h"
#include "ui/StatusPanel.h"

#include "server/CommsThread.h"

struct AppActions {
    QAction *launchServerAction {nullptr};
    QAction *stopServerAction {nullptr};
};

class MainWindow : public QMainWindow {

public:
    MainWindow(bool autostart);
    ~MainWindow() = default;

    LogWindow* getLogger();

    static MainWindow* getInstance();

    void closeEvent(QCloseEvent*) override;

private:
    void initializeActions();

    void launchServer();
    void stopServer();
    void serverDone();

    QWidget *mCentralWidget {nullptr};
    QVBoxLayout *mCentralLayout {nullptr};

    LogWindow *mLogWindow {nullptr};

    SettingsPanel *mSettingsPanel {nullptr};
    StatusPanel *mStatusPanel {nullptr};

    QPushButton *mLaunchServerButton {nullptr};
    QPushButton *mStopServerButton {nullptr};

    AppActions mActions {};

    CommsThread* mActiveThread {nullptr};

    bool mManuallyCancelled {false};

};

#endif // TPXSERVER_MAINWINDOW_H
