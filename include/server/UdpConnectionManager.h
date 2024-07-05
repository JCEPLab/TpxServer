#ifndef UDPCONNECTIONMANAGER_H
#define UDPCONNECTIONMANAGER_H

#include <string>
#include <memory>
#include <fstream>

#include "asio.hpp"
#include "zmq.hpp"

#include "ServerCodes.h"

class UdpThread;

class UdpConnectionManager {

public:
    UdpConnectionManager(UdpThread &thread);
    ~UdpConnectionManager();
    UdpConnectionManager(const UdpConnectionManager &rhs) = delete;

    void poll();

    void attemptConnection(const std::string &host_ip, int host_port);
    void initializeConnection(const asio::error_code &err);

    void handleMessage(const asio::error_code &err, std::size_t bytes_transferred);

    bool setSaveFile(const std::string &path);

    std::string getPublishServerAddress();

    void resetToaRolloverCounter();

private:
    void parseBytes(const std::vector<std::uint8_t> &buffer, std::size_t size);
    void publishRawData(const std::vector<std::uint8_t> &buffer, std::size_t size);

    UdpThread &mThread;

    asio::io_service mAsio {};
    std::unique_ptr<asio::ip::udp::socket> mUdpSocket {nullptr};
    asio::ip::udp::endpoint mUdpEndpoint {};
    std::vector<std::uint8_t> mUdpBuffer {};

    std::ofstream mFile {};

    bool mIsCancelled {false};

    std::vector<std::uint64_t> mTempBuffer {}; // allocated to the same size as mUdpBuffer; this avoids constant malloc's
    std::unique_ptr<zmq::socket_t> mPublishSocket {nullptr};

    std::uint8_t mRolloverCounter {0};
    bool mHalfwayToRollover {false};
    std::uint64_t mLastToA {0};

};

#endif // UDPCONNECTIONMANAGER_H
