#include "ui/StatusPanel.h"

StatusPanel::StatusPanel(QWidget *parent) :
    QGroupBox("Status", parent) {

    mLayout = new QVBoxLayout(this);
    setLayout(mLayout);

    {
        mServerRunningWidget = new QWidget(this);
        mServerRunningLayout = new QHBoxLayout(mServerRunningWidget);
        mServerRunningWidget->setLayout(mServerRunningLayout);

        mServerRunningLabel = new QLabel("Server:", mServerRunningWidget);
        mServerRunningStatus = new QLabel("", mServerRunningWidget);
        setServerRunning(false);

        mServerRunningLayout->addWidget(mServerRunningLabel);
        mServerRunningLayout->addWidget(mServerRunningStatus);

        mLayout->addWidget(mServerRunningWidget);
    }

}

void StatusPanel::setServerRunning(bool running) {

    if(running) {

        mServerRunningStatus->setText("Running");
        mServerRunningStatus->setStyleSheet("QLabel { color: green; }");

    } else {

        mServerRunningStatus->setText("Not Running");
        mServerRunningStatus->setStyleSheet("QLabel { color: red; }");

    }

}