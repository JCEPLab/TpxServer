#include <iostream>
#include <unordered_map>

#include "toml.hpp"

#include "server/TimepixCodes.h"

#include "DummySpidr.h"

using handler_fn = bool (SpidrServer::*) (const std::vector<uint32_t> &);

const std::unordered_map<TpxCommand, handler_fn> HANDLER_MAP {
        {TpxCommand::CMD_GET_SOFTWVERSION, &SpidrServer::getSoftwareVersionHandler},
        {TpxCommand::CMD_GET_FIRMWVERSION, &SpidrServer::getFirmwareVersionHandler},
        {TpxCommand::CMD_GET_CHIPBOARDID, &SpidrServer::getChipboardIdHandler},
        {TpxCommand::CMD_GET_DEVICEIDS, &SpidrServer::getDeviceIdsHandler},
        {TpxCommand::CMD_GET_REMOTETEMP, &SpidrServer::getRemoteTempHandler},
        {TpxCommand::CMD_GET_LOCALTEMP, &SpidrServer::getLocalTempHandler},
        {TpxCommand::CMD_GET_FPGATEMP, &SpidrServer::getFpgaTempHandler},
        {TpxCommand::CMD_GET_FANSPEED, &SpidrServer::getFanSpeedHandler},
        {TpxCommand::CMD_GET_PRESSURE, &SpidrServer::getPressureHandler},
        {TpxCommand::CMD_GET_HUMIDITY, &SpidrServer::getHumidityHandler},
        {TpxCommand::CMD_GET_SPIDR_ADC, &SpidrServer::getBiasVoltageHandler},
        {TpxCommand::CMD_SET_BIAS_ADJUST, &SpidrServer::setBiasVoltageHandler},
};

SpidrServer::SpidrServer(asio::io_service &service) {

    mTcpReceiveBuffer.resize(1024);
    mTcpSendBuffer.resize(1024);

    try {
        auto toml_file = toml::parse_file("../res/dummy-config.toml");
        mIp = toml_file["ip"].value<std::string>().value();
        mTcpPort = toml_file["tcpPort"].value<int>().value();
        mUdpPort = toml_file["udpPort"].value<int>().value();
    } catch (toml::parse_error &ex) {
        std::cout << "TOML error:\n" << ex.what() << "\n" << ex.source() << "\n" << ex.description() << std::endl;
    }

    mTcpAcceptor = std::make_unique<asio::ip::tcp::acceptor>(service, asio::ip::tcp::endpoint(asio::ip::address::from_string(mIp), mTcpPort));
    mTcp = std::make_unique<asio::ip::tcp::socket>(service);
    mTcpAcceptor->async_accept(*mTcp, [this](const asio::error_code &err) {
        acceptTcp();
    });
    std::cout << "TCP server started at " << mIp << ":" << mTcpPort << std::endl;

    /*
    mUdp = std::make_unique<asio::ip::udp::socket>(service);
    mUdp->open(asio::ip::udp::v4());
    mUdp->bind(asio::ip::udp::endpoint(asio::ip::address::from_string(mIp), mUdpPort));
    std::cout << "UDP server started at " << mIp << ":" << mUdpPort << std::endl;
     */

}

void SpidrServer::acceptTcp() {

    std::cout << "Connected to TCP client" << std::endl;

    waitForTcpCommand();

}

void SpidrServer::waitForTcpCommand() {

    mTcp->async_read_some(asio::buffer(mTcpReceiveBuffer), [this](const asio::error_code &err, std::size_t bytes) {
        readTcp(bytes);
    });

}

void SpidrServer::readTcp(std::size_t bytes) {

    if(bytes == 0)
        return; // ghost packets

    // parse TCP command
    if(bytes < 20) {

        std::cout << "TCP server received incomplete packet: size=" << bytes << std::endl;
        sendTcpError();
        return;

    }

    auto data = reinterpret_cast<uint32_t*>(mTcpReceiveBuffer.data());

    uint32_t com_int = io::ntohl(data[0]);
    uint32_t packet_size = io::ntohl(data[1]);
    uint32_t zero = io::ntohl(data[2]);
    uint32_t chip = io::ntohl(data[3]);

    if(zero != 0) {
        std::cout << "TCP server expected zero byte but saw " << zero << std::endl;
        sendTcpError();
        return;
    }

    if(chip != 0) {
        std::cout << "TCP server expected chip ID=0 but saw" << chip << std::endl;
        sendTcpError();
        return;
    }

    if(packet_size != bytes) {
        std::cout << "TCP server received " << bytes << " bytes but expected " << packet_size << " bytes" << std::endl;
        sendTcpError();
        return;
    }

    if(packet_size % 4) {
        std::cout << "TCP server received " << packet_size << " bytes, which is not a multiple of 4" << std::endl;
        sendTcpError();
        return;
    }

    std::size_t data_size = (packet_size - 16) / 4;
    std::vector<uint32_t> data_buffer;

    for(auto i = 0; i < data_size; ++i) {
        data_buffer.push_back(io::ntohl(data[i+4]));
    }

    handleTcpCommand(static_cast<TpxCommand>(com_int), data_buffer);

}

void SpidrServer::sendTcpResponse(TpxCommand command, const std::vector<uint32_t> &data) {

    uint32_t reply_cmd = static_cast<uint32_t>(command) | static_cast<uint32_t>(TpxCommand::CMD_REPLY);

    auto response_buffer = reinterpret_cast<uint32_t*>(mTcpSendBuffer.data());

    uint32_t buffer_size = std::max<uint32_t>(20, 16 + 4*data.size());

    response_buffer[0] = io::htonl(reply_cmd);
    response_buffer[1] = io::htonl(buffer_size);
    response_buffer[2] = io::htonl(0);
    response_buffer[3] = io::htonl(0);
    if(data.empty()) {
        response_buffer[4] = io::htonl(0);
    } else {
        for(auto i = 0; i < data.size(); ++i)
            response_buffer[4+i] = io::htonl(data[i]);
    }

    mTcp->async_send(asio::buffer(mTcpSendBuffer.data(), buffer_size), [this](const asio::error_code &err, std::size_t bytes) {
        waitForTcpCommand();
    });

}

void SpidrServer::sendTcpError() {

    for(int i = 0; i < 20; ++i)
        mTcpSendBuffer[i] = 0;

    mTcp->async_send(asio::buffer(mTcpSendBuffer.data(), 20), [this](const asio::error_code &err, std::size_t bytes) {
        waitForTcpCommand(); // reset TCP server
    });

}

void SpidrServer::handleTcpCommand(TpxCommand command, const std::vector<uint32_t> &data) {

    if(!HANDLER_MAP.contains(command)) {
        std::stringstream ss;
        ss << std::hex << std::setw(2) << std::setfill('0') << static_cast<uint32_t>(command);
        std::cout << "TCP server received unrecognised command code " << ss.str() << std::endl;
        sendTcpError();
        return;
    }

    auto handler = HANDLER_MAP.at(command);
    auto success = std::invoke(handler, this, data);

    if(success)
        waitForTcpCommand();
    else
        sendTcpError();

}

bool SpidrServer::getSoftwareVersionHandler(const std::vector<uint32_t> &data) {

    if(data.size() != 1) {
        std::cout << "TCP server: getSoftwareVersionHandler needs 1 data val, received " << data.size() << std::endl;
        return false;
    }

    sendTcpResponse(TpxCommand::CMD_GET_SOFTWVERSION, {1});
    return true;

}

bool SpidrServer::getFirmwareVersionHandler(const std::vector<uint32_t> &data) {

    if(data.size() != 1) {
        std::cout << "TCP server: getFirmwareVersionHandler needs 1 data val, received " << data.size() << std::endl;
        return false;
    }

    sendTcpResponse(TpxCommand::CMD_GET_FIRMWVERSION, {1});
    return true;

}

bool SpidrServer::getChipboardIdHandler(const std::vector<uint32_t> &data) {

    if(data.size() != 1) {
        std::cout << "TCP server: getChipboardIdHandler needs 1 data val, received " << data.size() << std::endl;
        return false;
    }

    sendTcpResponse(TpxCommand::CMD_GET_FIRMWVERSION, {0x1010002});
    return true;

}

bool SpidrServer::getDeviceIdsHandler(const std::vector<uint32_t> &data) {

    if(data.size() != 1) {
        std::cout << "TCP server: getDeviceIdsHandler needs 1 data val, received " << data.size() << std::endl;
        return false;
    }

    sendTcpResponse(TpxCommand::CMD_GET_DEVICEIDS, {1, 2147483647, 2147483647, 2147483647});
    return true;

}

bool SpidrServer::getRemoteTempHandler(const std::vector<uint32_t> &data) {

    if(data.size() != 1) {
        std::cout << "TCP server: getRemoteTempHandler needs 1 data val, received " << data.size() << std::endl;
        return false;
    }

    sendTcpResponse(TpxCommand::CMD_GET_REMOTETEMP, {0});
    return true;

}

bool SpidrServer::getLocalTempHandler(const std::vector<uint32_t> &data) {

    if(data.size() != 1) {
        std::cout << "TCP server: getLocalTempHandler needs 1 data val, received " << data.size() << std::endl;
        return false;
    }

    sendTcpResponse(TpxCommand::CMD_GET_LOCALTEMP, {0});
    return true;

}

bool SpidrServer::getFpgaTempHandler(const std::vector<uint32_t> &data) {

    if(data.size() != 1) {
        std::cout << "TCP server: getFpgaTempHandler needs 1 data val, received " << data.size() << std::endl;
        return false;
    }

    sendTcpResponse(TpxCommand::CMD_GET_FPGATEMP, {0});
    return true;

}

bool SpidrServer::getFanSpeedHandler(const std::vector<uint32_t> &data) {

    if(data.size() != 1) {
        std::cout << "TCP server: getFanSpeedHandler needs 1 data val, received " << data.size() << std::endl;
        return false;
    }

    sendTcpResponse(TpxCommand::CMD_GET_FANSPEED, {0});
    return true;

}

bool SpidrServer::getPressureHandler(const std::vector<uint32_t> &data) {

    if(data.size() != 1) {
        std::cout << "TCP server: getPressureHandler needs 1 data val, received " << data.size() << std::endl;
        return false;
    }

    sendTcpResponse(TpxCommand::CMD_GET_PRESSURE, {0});
    return true;

}

bool SpidrServer::getHumidityHandler(const std::vector<uint32_t> &data) {

    if(data.size() != 1) {
        std::cout << "TCP server: getHumidityHandler needs 1 data val, received " << data.size() << std::endl;
        return false;
    }

    sendTcpResponse(TpxCommand::CMD_GET_HUMIDITY, {0});
    return true;

}

bool SpidrServer::getBiasVoltageHandler(const std::vector<uint32_t> &data) {

    if(data.size() != 1) {
        std::cout << "TCP server: getBiasVoltageHandler needs 1 data val, received " << data.size() << std::endl;
        return false;
    }

    auto adc_code = static_cast<uint32_t>(((mBiasVoltage * 10 * 4096) - 4095) / 1500);
    adc_code &= 0xFFF;

    sendTcpResponse(TpxCommand::CMD_GET_SPIDR_ADC, {adc_code});
    return true;

}

bool SpidrServer::setBiasVoltageHandler(const std::vector<uint32_t> &data) {

    if(data.size() != 1) {
        std::cout << "TCP server: setBiasVoltageHandler needs 1 data val, received " << data.size() << std::endl;
        return false;
    }

    double voltage = data[0] * (104.0-12.0) / 4095 + 12;
    mBiasVoltage = voltage;

    sendTcpResponse(TpxCommand::CMD_SET_BIAS_ADJUST, {data[0]});
    return true;

}

int main() {

    std::cout << "Starting SPIDR..." << std::endl;

    asio::io_service ios;

    SpidrServer server(ios);

    while(true) {
        ios.poll();
    }

}