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

    inline std::uint64_t htonll(std::uint64_t x) {
        static bool ordered = htonl(1)==1; // cache this to make branch prediction easier
        if(ordered) {
            return x;
        } else {
            return static_cast<std::uint64_t>(htonl((x >> 32) & 0xFFFFFFFF))
                   | (static_cast<std::uint64_t>(htonl(x & 0xFFFFFFFF)) << 32);
        }
    }

    inline std::uint64_t ntohll(std::uint64_t x) {
        return htonll(x); // swaps if it needs to be swapped, otherwise keep the same value
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
class HistogramThread;

class TimepixConnectionManager;
class PythonConnectionManager;

class CommsThread : public BgThread {

public:
    CommsThread(CommsSettings settings);

    void execute() override;

    void resetClientConnection();

    void bindUdpPort(unsigned host_port);

    void startClusterThread();
    void startHistogramThread();

    void connectThread(BgThread *thread);

    zmq::context_t& getZmq();

    zmq::socket_t* getUdpThreadSocket();
    zmq::socket_t* getClusterThreadSocket();
    zmq::socket_t* getHistogramThreadSocket();

private:
    CommsSettings mSettings {};

    bool mShouldResetClient {false};

    std::deque<TimepixCommandInfo> mTcpServerCommands {};

    UdpThread *mUdpThread {nullptr};
    std::unique_ptr<zmq::socket_t> mUdpCommandSocket {nullptr};

    ClusterThread *mClusterThread {nullptr};
    std::unique_ptr<zmq::socket_t> mClusterCommandSocket {nullptr};

    HistogramThread *mHistogramThread {nullptr};
    std::unique_ptr<zmq::socket_t> mHistogramCommandSocket {nullptr};

    zmq::context_t mZmq {};

    std::unique_ptr<TimepixConnectionManager> mTpxManager {nullptr};
    std::unique_ptr<PythonConnectionManager> mClientManager {nullptr};

};

#endif //TPXSERVER_COMMSTHREAD_H
