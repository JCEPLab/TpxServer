#include "server/ClusterThread.h"

#include "server/ClusteringManager.h"

ClusterThread::ClusterThread(CommsThread &parent) :
    SecondaryThread(parent),
    mRawPacketAddr() {

    // do nothing

}

void ClusterThread::execute() {

    emit log("Clustering server started");

    mClusterManager = std::make_unique<ClusteringManager>(*this);

    while(!shouldCancel()) {

        try {
            pollCommands();
            mClusterManager->poll();
        } catch (...) {
            emit err("An unknown error occurred.");
            emit err("Clustering server is shutting down.");
            cancel();
        }

    }

    emit log("Clustering server shutting down");

}

void ClusterThread::handleCommand(ServerCommand cmd, const DataVec &data) {

    switch(cmd) {
    case ServerCommand::GET_CLUSTER_SERVER_PATH:
        sendClusterServerPath(data);
        break;

    case ServerCommand::SET_CLUSTER_INPUT_SERVER:
        setClusterInputServer(data);
        break;

    case ServerCommand::SET_CLUSTER_PARAMETERS:
        setClusterParameters(data);
        break;

    case ServerCommand::FLUSH_CLUSTERS:
        flushClusters(data);

    default:
        sendError(ServerCommand::UNKNOWN_COMMAND);
        break;
    }

}

void ClusterThread::sendClusterServerPath(const DataVec &data) {

    if(data.size() != 0) {
        sendError(ServerCommand::INVALID_COMMAND_DATA);
        return;
    }

    auto server_path = mClusterManager->getPublishServerAddress();
    auto int_size = (server_path.size() / 4) + 1;
    DataVec response(int_size, 0);
    std::memcpy(response.data(), server_path.data(), server_path.size());

    sendResponse(response);

}

void ClusterThread::setClusterInputServer(const DataVec &data) {

    std::vector<char> path;
    for(auto x : data) {
        path.push_back(static_cast<char>(x));
    }
    path.push_back('\0');

    std::string s(path.data());

    DEBUG("Clustering raw packets from server: " + s);

    try {
        mClusterManager->setRawPacketServerAddress(s);
        sendResponse(data);
        DEBUG("Cluster server has connected to raw packet server at " + s);
    } catch (...) {
        DEBUG("An error occured while connecting to the raw packet server");
        getParentThread().sendLog("An error occured while connecting to the raw packet server");
        sendError(ServerCommand::ERROR_OCCURED);
    }

}

void ClusterThread::setClusterParameters(const DataVec &data) {

    mClusterManager->setClusterParameters(data[0], data[1], data[2]);

    sendResponse(data);

}

void ClusterThread::flushClusters(const DataVec &data) {

    if(data.size() != 0) {
        sendError(ServerCommand::INVALID_COMMAND_DATA);
        return;
    }

    mClusterManager->flush();

    sendResponse({});

}
