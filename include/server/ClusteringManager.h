#ifndef CLUSTERINGMANAGER_H
#define CLUSTERINGMANAGER_H

#include <memory>
#include <vector>

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

private:
    ClusterThread &mThread;

    std::unique_ptr<zmq::socket_t> mPublishSocket {nullptr};
    std::unique_ptr<zmq::socket_t> mRawPacketSocket {nullptr};

    // clustering parameters
    //int mMaxSeparationXY {5};
    //int mMaxSeparationT {100};
    //int mMaxRange {20};

    std::unique_ptr<ClusterList> mOpenClusters { nullptr };

    // reusable arrays
    //std::vector<std::size_t> mIndices {};
    //std::vector<std::size_t> mUniqueIndices {};
    //std::vector<double> mAccumX {};
    //std::vector<double> mAccumY {};
    //std::vector<double> mAccumT {};
    //std::vector<double> mAccumToT {};
    //std::vector<std::size_t> mInvertedIndices {};
    //std::vector<unsigned long long> mCentroids {};



};

#endif // CLUSTERINGMANAGER_H
