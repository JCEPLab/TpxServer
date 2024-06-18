#ifndef TPXSERVER_STATUSPANEL_H
#define TPXSERVER_STATUSPANEL_H

#include <QGroupBox>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>

class StatusPanel : public QGroupBox {

public:
    StatusPanel(QWidget *parent);

    void setServerRunning(bool running);

private:
    QVBoxLayout *mLayout {nullptr};

    QWidget *mServerRunningWidget {nullptr};
    QHBoxLayout *mServerRunningLayout {nullptr};
    QLabel *mServerRunningLabel {nullptr};
    QLabel *mServerRunningStatus {nullptr};

};

#endif //TPXSERVER_STATUSPANEL_H
