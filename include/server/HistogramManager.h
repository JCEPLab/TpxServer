#ifndef HISTOGRAMMANAGER_H
#define HISTOGRAMMANAGER_H

#include <memory>
#include <vector>

#include "HistogramThread.h"

#include "zmq.hpp"

class HistogramManager {

public:
    HistogramManager(HistogramThread &thread);
    ~HistogramManager();

    std::string getPublishServerAddress();
    void setInputServerAddress(const std::string &path);
    void setOutputPeriod(int period_ms);

    void poll();
    void handlePackets(const std::uint64_t *data, std::size_t num_packets);

private:
    HistogramThread &mThread;

    std::unique_ptr<zmq::socket_t> mPublishSocket {nullptr};
    std::unique_ptr<zmq::socket_t> mInputSocket {nullptr};

    int mOutputPeriod {1000};

    std::array<std::uint16_t, 256*256> mHistogram;

    std::chrono::high_resolution_clock::time_point mLastOutputTime;

};

#endif // HISTOGRAMMANAGER_H
