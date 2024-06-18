#include "server/UdpConnectionManager.h"

#include "common_defs.h"
#include "server/UdpThread.h"
#include <iostream>

constexpr std::size_t BUFFER_SIZE = 40000;

UdpConnectionManager::UdpConnectionManager(UdpThread &thread) :
    mThread(thread) {

    mUdpBuffer.resize(BUFFER_SIZE);
    mTempBuffer.resize(mUdpBuffer.size());

    setSaveFile("");

    mPublishSocket = std::make_unique<zmq::socket_t>(thread.getZmq(), zmq::socket_type::pub);
    mPublishSocket->bind("tcp://*:*");

}

UdpConnectionManager::~UdpConnectionManager() {

    mPublishSocket->close();

}

void UdpConnectionManager::poll() {

    mAsio.poll();
    mAsio.restart();

}

void UdpConnectionManager::attemptConnection(const std::string &host_ip, int host_port) {

    mUdpSocket = std::make_unique<asio::ip::udp::socket>(mAsio);

    try {

        mUdpSocket->open(asio::ip::udp::v4());
        mUdpSocket->set_option(asio::socket_base::reuse_address(true));
        mUdpSocket->bind(asio::ip::udp::endpoint(asio::ip::address::from_string(host_ip), host_port));

        mUdpSocket->async_receive_from(asio::buffer(mUdpBuffer), mUdpEndpoint, [this](const asio::error_code &err, std::size_t bytes_transferred){
            handleMessage(err, bytes_transferred);
        });

    } catch (asio::system_error &ex) {
        mThread.sendErr("An unknown error occurred in the UDP server thread.");

        std::string str = ex.what();

        std::size_t pos = 0;
        while((pos = str.find('\n')) != std::string::npos)
            str.replace(pos, 1, "<br>");

        mThread.sendErr(str);

        if(DEBUG_OUTPUT)
            std::cout << "An unknown error occurred in the UDP server thread; terminating server." << std::endl;

        mThread.cancel();
    }

}

void UdpConnectionManager::handleMessage(const asio::error_code &err, std::size_t bytes) {

    parseBytes(mUdpBuffer, bytes);

    // queue up the next message
    mUdpSocket->async_receive_from(asio::buffer(mUdpBuffer), mUdpEndpoint, [this](const asio::error_code &err, std::size_t bytes_transferred){
        handleMessage(err, bytes_transferred);
    });

}

void UdpConnectionManager::parseBytes(const std::vector<std::uint8_t> &buffer, std::size_t size) {

    if(size == 0)
        return;

    if(size % 8) {
        mThread.sendWarn("Received a UDP chunk with a byte size of " + std::to_string(size) + "; expected a multiple of 8");
        return;
    }

    auto bytes = reinterpret_cast<const std::uint8_t*>(buffer.data());

    unsigned num_clicks = 0;

    for(auto ix = 0; ix < (size/8); ++ix) {
        auto type_byte = (bytes[(8*ix)+7] & 0xF0) >> 4;
        if(type_byte == 0xb)
            ++num_clicks;
    }

    auto chunk_size = num_clicks*8;

    if(num_clicks && mFile) {

        mFile << "TPX3" << '\0' << '\0';
        std::uint8_t size_lsb = chunk_size & 0xFF,
                     size_msb = (chunk_size>>8) & 0xFF;

        mFile << size_lsb << size_msb;

        for(auto ix = 0; ix < (size/8); ++ix) {
            auto type_byte = (bytes[(8*ix)+7] & 0xF0) >> 4;
            if(type_byte == 0xb) {
                for(auto jx = 0; jx < 8; ++jx)
                    mFile << bytes[(8*ix)+jx];
            }
        }

    }

    publishRawData(buffer, size);

}

bool UdpConnectionManager::setSaveFile(const std::string &path) {

    if(mFile.is_open())
        mFile.close();

    if(path.empty()) {
        if(DEBUG_OUTPUT)
            std::cout << "Not saving to file" << std::endl;
        return true;
    }

    bool success = true;

    try {
        mFile.open(path, std::ios_base::out | std::ios_base::binary);
    } catch (...) {
        success = false;
    }

    success &= mFile.is_open();
    if(!success){
        if(DEBUG_OUTPUT)
            std::cout << "Error opening file at " << path << std::endl;
        return false;
    }

    if(DEBUG_OUTPUT)
        std::cout << "Saving to file " << path << std::endl;
    mThread.sendLog("Saving image to " + path);

    return true;

}

std::string UdpConnectionManager::getPublishServerAddress() {

    auto zmq_addr = mPublishSocket->get(zmq::sockopt::last_endpoint);
    auto port_str = zmq_addr.substr(zmq_addr.find_last_of(':')+1);

    return "tcp://localhost:" + port_str;

}

void UdpConnectionManager::publishRawData(const std::vector<std::uint8_t> &data, std::size_t size) {

    // we publish data with the following format:
    // for each packet:
    //      X address:  8 bits
    //      Y address:  8 bits
    //      ToT:       10 bits
    //      ToA:       38 bits  ( Timepix reports 34 bits; the other 4 bits leave room for rollover detection )

    mTempBuffer.clear();

    auto packet_ptr = reinterpret_cast<const std::uint64_t*>(data.data());
    auto num_packets = size/8;

    for(auto ix = 0; ix < num_packets; ++ix) {

        auto type = (packet_ptr[ix] & 0xF000000000000000);
        if(type != 0xB000000000000000)
            continue;

        auto addr = (packet_ptr[ix] & 0x0FFFF00000000000) >> 44;
        std::uint8_t x = ((addr >> 1) & 0x00FC) | (addr & 0x0003);
        std::uint8_t y = ((addr >> 8) & 0x00FE) | ((addr >> 2) & 0x0001);

        auto tot = (packet_ptr[ix] & 0x000000003FF00000) >> 20;

        auto toa = (packet_ptr[ix] & 0x00000FFFC0000000) >> 30;
        auto ftoa = ((packet_ptr[ix] & 0x00000000000F0000) >> 16) ^ 0x0F;
        auto stime = (packet_ptr[ix] & 0x000000000000FFFF);

        auto full_toa = (stime << 18) | (toa << 4) | ftoa;

        auto tot_toa = (tot << 38) | full_toa;

        mTempBuffer.push_back(x);
        mTempBuffer.push_back(y);
        mTempBuffer.push_back(tot_toa >> 40);
        mTempBuffer.push_back((tot_toa >> 32) & 0xFF);
        mTempBuffer.push_back((tot_toa >> 24) & 0xFF);
        mTempBuffer.push_back((tot_toa >> 16) & 0xFF);
        mTempBuffer.push_back((tot_toa >> 8) & 0xFF);
        mTempBuffer.push_back(tot_toa & 0xFF);

    }

    mPublishSocket->send(zmq::buffer(mTempBuffer));

}
