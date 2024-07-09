#ifndef CLUSTERINGMANAGER_H
#define CLUSTERINGMANAGER_H

#include <memory>
#include <vector>
#include <fstream>

#include "ClusterThread.h"

#include "zmq.hpp"

class ClusterList;

class ClusteringManager {

public:
    ClusteringManager(ClusterThread &thread);
    ~ClusteringManager();

    std::string getPublishServerAddress();
    void setRawPacketServerAddress(const std::string &path);
    void setClusterParameters(int max_separation_xy, int max_separation_t, int max_t_sep);

    void poll();
    void flush(); // finishes all open clusters
    void handlePackets(const std::uint64_t *data, std::size_t num_packets);

    bool setSaveFile(const std::string &path);

private:
    ClusterThread &mThread;

    std::unique_ptr<zmq::socket_t> mPublishSocket {nullptr};
    std::unique_ptr<zmq::socket_t> mRawPacketSocket {nullptr};

    std::unique_ptr<ClusterList> mOpenClusters { nullptr };

    std::ofstream mFile {};
    std::uint64_t mSavedClusters {0};

    int mReceivedChunks {0};

};

#endif // CLUSTERINGMANAGER_H
