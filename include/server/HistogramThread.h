#ifndef HISTOGRAMTHREAD_H
#define HISTOGRAMTHREAD_H

#include <memory>

#include "SecondaryThread.h"

class HistogramManager;

class HistogramThread : public SecondaryThread {

public:
    HistogramThread(CommsThread &parent);

    void execute() override;
    void handleCommand(ServerCommand cmd, const DataVec &data) override;

    void sendHistogramServerPath(const DataVec &data);
    void setHistogramInputServer(const DataVec &data);
    void setHistogramOutputPeriod(const DataVec &data);

private:
    std::string mInputAddr {};

    std::unique_ptr<HistogramManager> mHistogramManager {nullptr};

};

#endif // HISTOGRAMTHREAD_H
