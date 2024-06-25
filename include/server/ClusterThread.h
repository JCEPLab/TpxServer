#ifndef CLUSTERTHREAD_H
#define CLUSTERTHREAD_H

#include <memory>

#include "SecondaryThread.h"

class ClusteringManager;

class ClusterThread : public SecondaryThread {

public:
    ClusterThread(CommsThread &parent);

    void execute() override;
    void handleCommand(ServerCommand cmd, const DataVec &data) override;

    void sendClusterServerPath(const DataVec &data);
    void setClusterInputServer(const DataVec &data);
    void setClusterParameters(const DataVec &data);
    void flushClusters(const DataVec &data);

private:
    std::string mRawPacketAddr {};

    std::unique_ptr<ClusteringManager> mClusterManager {nullptr};

};

#endif // CLUSTERTHREAD_H
