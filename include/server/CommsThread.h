#ifndef TPXSERVER_COMMSTHREAD_H
#define TPXSERVER_COMMSTHREAD_H

#include <deque>

#include "asio.hpp"
#include "zmq.hpp"

#include "BgThread.h"

#include "TimepixCommandInfo.h"

namespace io {
    inline std::uint32_t htonl(std::uint32_t x) {
        return asio::detail::socket_ops::host_to_network_long(x);
    }

    inline std::uint32_t ntohl(std::uint32_t x) {
        return asio::detail::socket_ops::network_to_host_long(x);
    }
}

struct CommsSettings {
    std::string host_ip;
    unsigned int host_port;
    std::string timepix_ip;
    unsigned int timepix_port;
    unsigned int outgoing_port;
};

class UdpThread;
class ClusterThread;

class TimepixConnectionManager;
class PythonConnectionManager;

class CommsThread : public BgThread {

public:
    CommsThread(CommsSettings settings);

    void execute() override;

    void resetClientConnection();

    void bindUdpPort(unsigned host_port);

    void startClusterThread();

    void connectThread(BgThread *thread);

    zmq::context_t& getZmq();

    zmq::socket_t* getUdpThreadSocket();
    zmq::socket_t* getClusterThreadSocket();

private:
    CommsSettings mSettings {};

    bool mShouldResetClient {false};

    std::deque<TimepixCommandInfo> mTcpServerCommands {};

    UdpThread *mUdpThread {nullptr};
    std::unique_ptr<zmq::socket_t> mUdpCommandSocket {nullptr};

    ClusterThread *mClusterThread {nullptr};
    std::unique_ptr<zmq::socket_t> mClusterCommandSocket {nullptr};

    zmq::context_t mZmq {};

    std::unique_ptr<TimepixConnectionManager> mTpxManager {nullptr};
    std::unique_ptr<PythonConnectionManager> mClientManager {nullptr};

};

#endif //TPXSERVER_COMMSTHREAD_H
