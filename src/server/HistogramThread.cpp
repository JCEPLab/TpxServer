#include "server/HistogramThread.h"

#include "server/HistogramManager.h"

HistogramThread::HistogramThread(CommsThread &parent) :
    SecondaryThread(parent),
    mInputAddr() {

    // do nothing

}

void HistogramThread::execute() {

    emit log("Histogram server started");

    mHistogramManager = std::make_unique<HistogramManager>(*this);

    while(!shouldCancel()) {

        try {
            pollCommands();
            mHistogramManager->poll();
        } catch (...) {
            emit err("An unknown error occurred.");
            emit err("Histogram server is shutting down.");
            cancel();
        }

    }

    emit log("Histogram server shutting down");

}

void HistogramThread::handleCommand(ServerCommand cmd, const DataVec &data) {

    switch(cmd) {
    case ServerCommand::GET_HISTOGRAM_SERVER_PATH:
        sendHistogramServerPath(data);
        break;

    case ServerCommand::SET_HISTOGRAM_INPUT_SERVER:
        setHistogramInputServer(data);
        break;

    case ServerCommand::SET_HISTOGRAM_OUTPUT_PERIOD:
        setHistogramOutputPeriod(data);
        break;

    default:
        sendError(ServerCommand::UNKNOWN_COMMAND);
        break;
    }

}

void HistogramThread::sendHistogramServerPath(const DataVec &data) {

    if(data.size() != 0) {
        sendError(ServerCommand::INVALID_COMMAND_DATA);
        return;
    }

    auto server_path = mHistogramManager->getPublishServerAddress();
    auto int_size = (server_path.size() / 4) + 1;
    DataVec response(int_size, 0);
    std::memcpy(response.data(), server_path.data(), server_path.size());

    sendResponse(response);

}

void HistogramThread::setHistogramInputServer(const DataVec &data) {

    std::vector<char> path;
    for(auto x : data) {
        path.push_back(static_cast<char>(x));
    }
    path.push_back('\0');

    std::string s(path.data());

    DEBUG("Histogramming clicks from server: " + s);

    try {
        mHistogramManager->setInputServerAddress(s);
        sendResponse(data);
        DEBUG("Histogram server has connected to server at " + s);
    } catch (...) {
        DEBUG("An error occured while connecting to the histogram input server");
        getParentThread().sendLog("An error occured while connecting to the histogram input server");
        sendError(ServerCommand::ERROR_OCCURED);
    }

}

void HistogramThread::setHistogramOutputPeriod(const DataVec &data) {

    if(data.size() != 1) {
        sendError(ServerCommand::INVALID_COMMAND_DATA);
        return;
    }

    int period = data[0];
    mHistogramManager->setOutputPeriod(period);

    sendResponse(data);

}
