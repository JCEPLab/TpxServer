#ifndef TPXSERVER_SETTINGSPANEL_H
#define TPXSERVER_SETTINGSPANEL_H

#include <QGroupBox>
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QPushButton>
#include <QLabel>
#include <QComboBox>
#include <QLineEdit>
#include <QCheckBox>

#include "server/CommsThread.h"

#include "toml.hpp"

struct AppActions;

class SettingsPanel : public QGroupBox {

public:
    SettingsPanel(QWidget *parent);

    void setEnabled(bool enabled);

    CommsSettings getSettings();

    void updateIpList();

    bool autoRestartChecked() const;

private:
    QGridLayout *mLayout {nullptr};

    QLabel *mHostIpSettingLabel {nullptr};
    QComboBox *mHostIpSettingEdit {nullptr};

    QLabel *mHostPortSettingLabel {nullptr};
    QLineEdit *mHostPortSettingEdit {nullptr};

    QLabel *mTimepixIpSettingLabel {nullptr};
    QLineEdit *mTimepixIpSettingEdit {nullptr};

    QLabel *mTimepixPortSettingLabel {nullptr};
    QLineEdit *mTimepixPortSettingEdit {nullptr};

    QLabel *mOutgoingPortSettingLabel {nullptr};
    QLineEdit *mOutgoingPortSettingEdit {nullptr};

    QLabel *mAutoRestartLabel {nullptr};
    QCheckBox *mAutoRestartButton {nullptr};

    TimepixConfig mCurrentConfig {};

};

#endif //TPXSERVER_SETTINGSPANEL_H
