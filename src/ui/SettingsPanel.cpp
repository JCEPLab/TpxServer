#include "ui/SettingsPanel.h"

#include <algorithm>
#include <fstream>
#include <filesystem>

#include <QSizePolicy>
#include <QFileDialog>
#include <QFileInfo>
#include <QCheckBox>

#include <WinSock2.h>
#include <iphlpapi.h>
#include "toml.hpp"

#include "logger.h"

template<typename T>
void set_or_warn(T &val, const toml::table &table, const std::string &section, const std::string &name) {
    auto view = table[section][name];
    if(view.is_value()) {
        try {
            val = view.value<T>().value();
        } catch (std::bad_optional_access &err) {
            logger::warn("Config file has wrong type for '" + name + "'; using default values.");
        }
    } else {
        logger::warn("Config file has no '" + name + "'; using default values.");
    }
}

template<typename T>
void set_list_or_warn(std::vector<T> &list, const toml::table &table, const std::string &section, const std::string &name) {
    auto view = table[section][name];
    if(view.is_array()) {
        if(list.size() == view.as_array()->size()) {
            //try {
                for (auto ix = 0; ix < list.size(); ++ix) {
                    list[ix] = view[ix].value<T>().value();
                }
            //} catch (std::bad_optional_access &err) {
            //    logger::warn("Config file has wrong type for '" + name + "; using default values.");
            //}
        } else {
            logger::warn("Config file has the wrong number of elements for '" + name + "' (expected " + std::to_string(list.size()) + "); using default values.");
        }
    } else {
        logger::warn("Config file has no '" + name + "'; using default values.");
    }
}

template<typename T>
void load_array_or_warn(std::vector<T> &mat, const std::string &fname) {

    std::vector<T> data;
    data.reserve(256*256);

    std::ifstream input(fname);
    if(!input) {
        logger::warn("Unable to open pixel data file at '" + fname + "'");
        return;
    }

    std::string line;

    std::size_t row = 0;
    while(std::getline(input, line) && row < 256) {

        std::istringstream iss(line);
        T val;
        for(std::size_t ix = 0; ix < 256; ++ix) {
            if(iss >> val) {
                data.push_back(val);
            } else {
                logger::warn("Pixel data at '" + fname + "' has the wrong number of elements; expected a 256x256 array.");
                return;
            }
        }

        ++row;
    }

    if(row != 256) {
        logger::warn("Pixel data at '" + fname + "' has the wrong number of elements; expected a 256x256 array.");
        return;
    }

    mat = data;

}

std::vector<std::string> enumerate_ips_win32() {

    unsigned long num_adaptors;
    GetAdaptersInfo(nullptr, &num_adaptors);
    std::vector<IP_ADAPTER_INFO> adaptors;
    adaptors.resize(num_adaptors);
    auto err = GetAdaptersInfo(adaptors.data(), &num_adaptors);

    if(err != NO_ERROR) {
        return {};
    }

    std::vector<IP_ADDRESS_STRING> addresses;

    for(auto &a : adaptors) {
        auto addr_ll = &a.IpAddressList;
        while(addr_ll) {
            addresses.push_back(addr_ll->IpAddress);
            addr_ll = addr_ll->Next;
        }
    }

    std::vector<std::string> nonempty_addresses;
    for(auto &ip : addresses) {
        std::string addr = ip.String;
        if(!addr.empty() && (addr != "0.0.0.0"))
            nonempty_addresses.push_back(addr);
    }

    return nonempty_addresses;

}

SettingsPanel::SettingsPanel(QWidget *parent) :
    QGroupBox("Server Settings", parent) {

    mLayout = new QGridLayout(this);
    setLayout(mLayout);

    {
        mHostIpSettingLabel = new QLabel("Host IP Address:", this);
        mHostIpSettingEdit = new QComboBox(this);

        mLayout->addWidget(mHostIpSettingLabel, 0, 0);
        mLayout->addWidget(mHostIpSettingEdit, 0, 1);

        updateIpList();
    }

    {
        mHostPortSettingLabel = new QLabel("Host Port ('0'=Auto):", this);
        mHostPortSettingEdit = new QLineEdit("0", this);

        mLayout->addWidget(mHostPortSettingLabel, 1, 0);
        mLayout->addWidget(mHostPortSettingEdit, 1, 1);
    }

    {
        mTimepixIpSettingLabel = new QLabel("Timepix IP Address:", this);
        mTimepixIpSettingEdit = new QLineEdit("192.168.100.10", this);

        mLayout->addWidget(mTimepixIpSettingLabel, 2, 0);
        mLayout->addWidget(mTimepixIpSettingEdit, 2, 1);
    }

    {
        mTimepixPortSettingLabel = new QLabel("Timepix Port:", this);
        mTimepixPortSettingEdit = new QLineEdit("50000", this);

        mLayout->addWidget(mTimepixPortSettingLabel, 3, 0);
        mLayout->addWidget(mTimepixPortSettingEdit, 3, 1);
    }

    {
        mOutgoingPortSettingLabel = new QLabel("Outgoing (Client) Port:", this);
        mOutgoingPortSettingEdit = new QLineEdit("48288", this);

        mLayout->addWidget(mOutgoingPortSettingLabel, 4, 0);
        mLayout->addWidget(mOutgoingPortSettingEdit, 4, 1);
    }

    {
        mAutoRestartLabel = new QLabel("Automatically Restart on Crash:", this);
        mAutoRestartButton = new QCheckBox(this);
        mAutoRestartButton->setChecked(false);

        mLayout->addWidget(mAutoRestartLabel, 5, 0);
        mLayout->addWidget(mAutoRestartButton, 5, 1);
    }

}

void SettingsPanel::setEnabled(bool enabled) {

    mHostIpSettingEdit->setEnabled(enabled);
    mHostPortSettingEdit->setEnabled(enabled);
    mTimepixIpSettingEdit->setEnabled(enabled);
    mTimepixPortSettingEdit->setEnabled(enabled);
    mOutgoingPortSettingEdit->setEnabled(enabled);
    mAutoRestartButton->setEnabled(enabled);

}

CommsSettings SettingsPanel::getSettings() {

    return CommsSettings{
        .host_ip = mHostIpSettingEdit->currentText().toStdString(),
        .host_port = std::stoul(mHostPortSettingEdit->text().toStdString()),
        .timepix_ip = mTimepixIpSettingEdit->text().toStdString(),
        .timepix_port = std::stoul(mTimepixPortSettingEdit->text().toStdString()),
        .outgoing_port = std::stoul(mOutgoingPortSettingEdit->text().toStdString())
    };

}

void SettingsPanel::updateIpList() {

    auto ip_list = enumerate_ips_win32();

    mHostIpSettingEdit->clear();
    for(auto &ip : ip_list)
        mHostIpSettingEdit->addItem(ip.c_str());

}

bool SettingsPanel::autoRestartChecked() const {

    return mAutoRestartButton->isChecked();

}
