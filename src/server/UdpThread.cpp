#include "server/UdpThread.h"

#include <string>
#include <iostream>
#include <cstring>

UdpThread::UdpThread(CommsThread &parent, std::string host_ip, unsigned int host_port) :
    SecondaryThread(parent),
    mHostIp(host_ip),
    mHostPort(host_port) {

    // do nothing

}

void UdpThread::execute() {

    emit log("Launching UDP server on port " + std::to_string(mHostPort));

    mUdpManager = std::make_unique<UdpConnectionManager>(*this);
    mUdpManager->attemptConnection(mHostIp, mHostPort);

    while(!shouldCancel()) {

        try {
            pollCommands();
            mUdpManager->poll();
        } catch (...) {
            emit err("An unknown error occurred.");
            emit err("UDP server is shutting down.");
            cancel();
        }

    }

    emit log("UDP server shutting down");

}

void UdpThread::handleCommand(ServerCommand cmd, const DataVec &data) {

    switch(cmd) {
    case ServerCommand::SET_RAW_TPX3_PATH:
        setRawTpxPath(data);
        break;

    case ServerCommand::GET_RAW_DATA_SERVER_PATH:
        sendRawDataServerPath(data);
        break;

    default:
        sendError(ServerCommand::UNKNOWN_COMMAND);
        return;
    }

}

void UdpThread::setRawTpxPath(const DataVec &data) {

    std::vector<char> path;
    for(auto x : data) {
        path.push_back(static_cast<char>(x));
    }
    path.push_back('\0');

    std::string s(path.data());

    if(DEBUG_OUTPUT)
        std::cout << "Setting save path for raw *.tpx3 files to: " << s << std::endl;

    if(mUdpManager->setSaveFile(s)) {
        sendResponse(data);
    } else {
        emit warn("Unable to open path \"" + s + "\" for image output");
        sendError(ServerCommand::CANT_OPEN_FILE);
    }

}

void UdpThread::sendRawDataServerPath(const DataVec &data) {

    if(data.size() != 0) {
        sendError(ServerCommand::ERROR_OCCURED);
        return;
    }

    auto server_path = mUdpManager->getPublishServerAddress();
    auto int_size = (server_path.size() / 4) + 1;
    DataVec response(int_size, 0);
    std::memcpy(response.data(), server_path.data(), server_path.size());

    sendResponse(response);

}
