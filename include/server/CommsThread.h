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

// TODO: remove - unused
// values here are defaults
struct TimepixConfig {
    // acquisition
    bool polarityPositive           = true;
    std::string acqMode             = "AUTOTRIGSTART_TIMERSTOP";
    int acqCount                    = 1;
    double time                     = 0.0;
    double frequency                = 140;

    // hardware
    std::vector<int> VDD            = { 1524, 198, 302 };
    std::vector<int> AVDD           = { 1512, 484, 730 };
    double biasAdjust               = 40;

    // DACs
    int Vthreshold_fine_relative    = 225;
    int Ibias_Preamp_ON             = 128;
    int Ibias_Preamp_OFF            = 8;
    int VPreamp_NCAS                = 128;
    int Ibias_Ikrum                 = 10;
    int Vfbk                        = 128;
    int Vthreshold_fine             = 225;
    int Vthreshold_coarse           = 4;
    int Ibias_DiscS1_ON             = 128;
    int Ibias_DiscS1_OFF            = 8;
    int Ibias_DiscS2_ON             = 128;
    int Ibias_DiscS2_OFF            = 8;
    int Ibias_PixelDAC              = 126;
    int Ibias_TPbufferIn            = 128;
    int Ibias_TPbufferOut           = 128;
    int VTP_coarse                  = 128;
    int VTP_fine                    = 256;
    int Ibias_CP_PLL                = 128;
    int PLL_Vcntrl                  = 128;

    // pixels
    std::vector<int> maskFile       = std::vector<int>(256*256, 0);
    std::vector<int> testFile       = std::vector<int>(256*256, 0);
    std::vector<int> thlAdjFile     = std::vector<int>(256*256, 0);
};

struct CommsSettings {
    std::string host_ip;
    unsigned int host_port;
    std::string timepix_ip;
    unsigned int timepix_port;
    unsigned int outgoing_port;

    TimepixConfig config;
};

class UdpThread;

class TimepixConnectionManager;
class PythonConnectionManager;

class CommsThread : public BgThread {

public:
    CommsThread(CommsSettings settings);

    void execute() override;

    void resetClientConnection();

    void bindUdpPort(unsigned host_port);

    void connectThread(BgThread *thread);

    zmq::context_t& getZmq();

    zmq::socket_t* getUdpThreadSocket();

private:
    CommsSettings mSettings {};

    bool mShouldResetClient {false};

    std::deque<TimepixCommandInfo> mTcpServerCommands {};

    UdpThread *mUdpThread {nullptr};
    std::unique_ptr<zmq::socket_t> mUdpCommandSocket {nullptr};

    zmq::context_t mZmq {};

    std::unique_ptr<TimepixConnectionManager> mTpxManager {nullptr};
    std::unique_ptr<PythonConnectionManager> mClientManager {nullptr};

};

#endif //TPXSERVER_COMMSTHREAD_H
