#ifndef TPXSERVER_DUMMYSPIDR_H
#define TPXSERVER_DUMMYSPIDR_H

#include <string>
#include <utility>
#include <stdint.h>

#include "asio.hpp"

#include "server/TimepixCodes.h"

namespace io {
    inline std::uint32_t htonl(std::uint32_t x) {
        return asio::detail::socket_ops::host_to_network_long(x);
    }

    inline std::uint32_t ntohl(std::uint32_t x) {
        return asio::detail::socket_ops::network_to_host_long(x);
    }
}

class SpidrServer {

public:
    SpidrServer(asio::io_service &service);
    SpidrServer(const SpidrServer &rhs) = delete;

    void acceptTcp();
    void waitForTcpCommand();
    void readTcp(std::size_t bytes);
    void sendTcpResponse(TpxCommand command, const std::vector<uint32_t> &data = {});
    void sendTcpError();
    void handleTcpCommand(TpxCommand command, const std::vector<uint32_t> &data);

    // TCP handlers
    bool getSoftwareVersionHandler(const std::vector<uint32_t> &data);
    bool getFirmwareVersionHandler(const std::vector<uint32_t> &data);
    bool getChipboardIdHandler(const std::vector<uint32_t> &data);
    bool getDeviceIdsHandler(const std::vector<uint32_t> &data);
    bool getRemoteTempHandler(const std::vector<uint32_t> &data);
    bool getLocalTempHandler(const std::vector<uint32_t> &data);
    bool getFpgaTempHandler(const std::vector<uint32_t> &data);
    bool getFanSpeedHandler(const std::vector<uint32_t> &data);
    bool getHumidityHandler(const std::vector<uint32_t> &data);
    bool getPressureHandler(const std::vector<uint32_t> &data);
    bool getBiasVoltageHandler(const std::vector<uint32_t> &data);
    bool setBiasVoltageHandler(const std::vector<uint32_t> &data);

private:
    std::string mIp;
    int mTcpPort;
    int mUdpPort;

    std::unique_ptr<asio::ip::tcp::acceptor> mTcpAcceptor {nullptr};
    std::unique_ptr<asio::ip::tcp::socket> mTcp {nullptr};
    //std::unique_ptr<asio::ip::udp::socket> mUdp {nullptr};

    std::vector<uint8_t> mTcpReceiveBuffer{};
    std::vector<uint8_t> mTcpSendBuffer{};

    // Timepix state
    double mBiasVoltage {12.0};

};

#endif // TPXSERVER_DUMMYSPIDR_H