#ifndef UDPTHREAD_H
#define UDPTHREAD_H

#include <cstdint>
#include <memory>

#include "SecondaryThread.h"

#include "UdpConnectionManager.h"

class UdpThread : public SecondaryThread {

public:
    UdpThread(CommsThread &parent, std::string host_ip, unsigned int host_port);

    void execute() override;

    void handleCommand(ServerCommand cmd, const DataVec &data) override;

    void setRawTpxPath(const DataVec &data);
    void sendRawDataServerPath(const DataVec &data);
    void resetToaRolloverCounter(const DataVec &data);

private:
    std::string mHostIp;
    unsigned mHostPort;

    std::unique_ptr<UdpConnectionManager> mUdpManager {nullptr};

};

#endif // UDPTHREAD_H
