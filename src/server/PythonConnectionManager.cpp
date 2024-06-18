#include "server/PythonConnectionManager.h"

#include "server/CommsThread.h"

#include "server/ServerCodes.h"

#include "server/TimepixCodes.h"
#include "server/TimepixConnectionManager.h"

#include <map>
#include <iostream>

const std::map<ServerCommand, void (PythonConnectionManager::*)(const DataVec &)> COMMAND_MAP {
    {ServerCommand::NOP, &PythonConnectionManager::genericForward<0, TpxCommand::CMD_NOP>},
    {ServerCommand::GET_SOFTWARE_VERSION, &PythonConnectionManager::genericForward<0, TpxCommand::CMD_GET_SOFTWVERSION>},
    {ServerCommand::GET_FIRMWARE_VERSION, &PythonConnectionManager::genericForward<0, TpxCommand::CMD_GET_FIRMWVERSION>},
    {ServerCommand::GET_CAMERA_ID, &PythonConnectionManager::genericForward<0, TpxCommand::CMD_GET_CHIPBOARDID>},
    {ServerCommand::GET_DEVICE_IDS, &PythonConnectionManager::genericForward<0, TpxCommand::CMD_GET_DEVICEIDS>},
    {ServerCommand::GET_LOCAL_TEMP, &PythonConnectionManager::genericForward<0, TpxCommand::CMD_GET_LOCALTEMP>},
    {ServerCommand::GET_REMOTE_TEMP, &PythonConnectionManager::genericForward<0, TpxCommand::CMD_GET_REMOTETEMP>},
    {ServerCommand::GET_FPGA_TEMP, &PythonConnectionManager::genericForward<0, TpxCommand::CMD_GET_FPGATEMP>},
    {ServerCommand::GET_FAN_SPEED, &PythonConnectionManager::genericForward<0, TpxCommand::CMD_GET_FANSPEED>},
    {ServerCommand::GET_PRESSURE, &PythonConnectionManager::genericForward<0, TpxCommand::CMD_GET_PRESSURE>},
    {ServerCommand::GET_HUMIDITY, &PythonConnectionManager::genericForward<0, TpxCommand::CMD_GET_HUMIDITY>},
    {ServerCommand::GET_SPIDR_ADC, &PythonConnectionManager::genericForward<1, TpxCommand::CMD_GET_SPIDR_ADC>},
    {ServerCommand::SET_BIAS_VOLTAGE, &PythonConnectionManager::genericForward<1, TpxCommand::CMD_SET_BIAS_ADJUST>},
    {ServerCommand::SET_BIAS_SUPPLY_ENABLED, &PythonConnectionManager::genericForward<1, TpxCommand::CMD_BIAS_SUPPLY_ENA>},
    {ServerCommand::GET_DEVICE_PORT, &PythonConnectionManager::genericForward<0, TpxCommand::CMD_GET_DEVICEPORT>},
    {ServerCommand::GET_SERVER_PORT, &PythonConnectionManager::genericForward<0, TpxCommand::CMD_GET_SERVERPORT>},
    {ServerCommand::SET_SERVER_PORT, &PythonConnectionManager::genericForward<1, TpxCommand::CMD_SET_SERVERPORT>},
    {ServerCommand::SET_TOA_DECODERS_ENABLED, &PythonConnectionManager::genericForward<1, TpxCommand::CMD_DECODERS_ENA>},
    {ServerCommand::RESTART_TIMERS, &PythonConnectionManager::genericForward<0, TpxCommand::CMD_RESTART_TIMERS>},
    {ServerCommand::RESET_TIMERS, &PythonConnectionManager::genericForward<0, TpxCommand::CMD_RESET_TIMER>},
    {ServerCommand::RESET_PIXELS, &PythonConnectionManager::genericForward<0, TpxCommand::CMD_RESET_PIXELS>},
    {ServerCommand::GET_SPIDR_REGISTER, &PythonConnectionManager::genericForward<1, TpxCommand::CMD_GET_SPIDRREG>},
    {ServerCommand::SET_SPIDR_REGISTER, &PythonConnectionManager::genericForward<2, TpxCommand::CMD_SET_SPIDRREG>},
    {ServerCommand::GET_PLL_CONFIG, &PythonConnectionManager::genericForward<0, TpxCommand::CMD_GET_PLLCONFIG>},
    {ServerCommand::SET_PLL_CONFIG, &PythonConnectionManager::genericForward<1, TpxCommand::CMD_SET_PLLCONFIG>},
    {ServerCommand::GET_HEADER_FILTER, &PythonConnectionManager::genericForward<0, TpxCommand::CMD_GET_HEADERFILTER>},
    {ServerCommand::SET_HEADER_FILTER, &PythonConnectionManager::genericForward<1, TpxCommand::CMD_SET_HEADERFILTER>},
    {ServerCommand::GET_GEN_CONFIG, &PythonConnectionManager::genericForward<0, TpxCommand::CMD_GET_GENCONFIG>},
    {ServerCommand::SET_GEN_CONFIG, &PythonConnectionManager::genericForward<1, TpxCommand::CMD_SET_GENCONFIG>},
    {ServerCommand::GET_PERIOD_PHASE, &PythonConnectionManager::genericForward<0, TpxCommand::CMD_GET_TPPERIODPHASE>},
    {ServerCommand::SET_PERIOD_PHASE, &PythonConnectionManager::genericForward<1, TpxCommand::CMD_SET_TPPERIODPHASE>},
    {ServerCommand::GET_TRIGGER_CONF, &PythonConnectionManager::genericForward<0, TpxCommand::CMD_GET_TRIGCONFIG>},
    {ServerCommand::SET_TRIGGER_CONF, &PythonConnectionManager::genericForward<5, TpxCommand::CMD_SET_TRIGCONFIG>},
    {ServerCommand::GET_DAC, &PythonConnectionManager::genericForward<1, TpxCommand::CMD_GET_DAC>},
    {ServerCommand::SET_DAC, &PythonConnectionManager::genericForward<1, TpxCommand::CMD_SET_DAC>},
    {ServerCommand::START_READOUT, &PythonConnectionManager::genericForward<0, TpxCommand::CMD_DDRIVEN_READOUT>},
    {ServerCommand::STOP_READOUT, &PythonConnectionManager::genericForward<0, TpxCommand::CMD_PAUSE_READOUT>},
    {ServerCommand::AUTOTRIG_START, &PythonConnectionManager::genericForward<0, TpxCommand::CMD_AUTOTRIG_START>},
    {ServerCommand::AUTOTRIG_STOP, &PythonConnectionManager::genericForward<0, TpxCommand::CMD_AUTOTRIG_STOP>},
    {ServerCommand::GET_PIXEL_CONFIG, &PythonConnectionManager::genericForward<1, TpxCommand::CMD_GET_PIXCONF>},
    {ServerCommand::SET_PIXEL_CONFIG, &PythonConnectionManager::sendPixelConfigData},
    {ServerCommand::RESET_MODULE, &PythonConnectionManager::genericForward<1, TpxCommand::CMD_RESET_MODULE>},
    {ServerCommand::GET_READOUT_SPEED, &PythonConnectionManager::genericForward<0, TpxCommand::CMD_GET_READOUTSPEED>},
    {ServerCommand::SET_SENSEDAC, &PythonConnectionManager::genericForward<1, TpxCommand::CMD_SET_SENSEDAC>},

    {ServerCommand::SET_UDP_PORT, &PythonConnectionManager::bindUdpPort}
};

PythonConnectionManager::PythonConnectionManager(CommsThread &thread, TimepixConnectionManager &tpx) :
    mThread(thread),
    mTpxManager(&tpx) {

    // do nothing

}

PythonConnectionManager::~PythonConnectionManager() {

    close();

}

void PythonConnectionManager::open(unsigned int port) {

    mCommandSocket = std::make_unique<zmq::socket_t>(mThread.getZmq(), zmq::socket_type::rep);
    mCommandSocket->bind("tcp://*:" + std::to_string(port));
    if(DEBUG_OUTPUT)
        std::cout << "Opened client socket on port " << port << std::endl;

}

void PythonConnectionManager::close() {

    mCommandSocket->close();

    if(DEBUG_OUTPUT)
        std::cout << "Closing client connection" << std::endl;

}

void PythonConnectionManager::poll() {

    try {
        zmq::message_t request;
        auto incoming_msg = mCommandSocket->recv(request, zmq::recv_flags::dontwait);

        if (incoming_msg) {
            handleCommand(request);
        }

    } catch (zmq::error_t &ex) {
        mThread.sendErr("An exception occurred in managing client connections (errcode=" + std::to_string(ex.num()) + " - \"" + ex.what() + "\")");
        close();
        mThread.resetClientConnection();
    }

}

void PythonConnectionManager::handleCommand(zmq::message_t &command) {

    if (!mTpxManager->isConnected()) {
        sendError(ServerCommand::ERROR_OCCURED);
        return;
    }

    auto *command_int_ptr = reinterpret_cast<const std::uint32_t*>(command.data());

    std::uint32_t com_code_int = command_int_ptr[0];
    auto command_code = static_cast<ServerCommand>(com_code_int);

    std::uint32_t com_size = command.size();
    if(com_size % 4) {
        sendError(ServerCommand::ERROR_OCCURED);
        return;
    }

    std::size_t data_size = (com_size / 4) - 1;

    if(DEBUG_OUTPUT)
        std::cout << "Client: received command " << com_code_int << std::endl;

    if (!COMMAND_MAP.contains(command_code)) {

        switch(command_code) {
        case ServerCommand::SET_RAW_TPX3_PATH:
        case ServerCommand::GET_RAW_DATA_SERVER_PATH:
            forwardToSecondaryThread(mThread.getUdpThreadSocket(), command);
            break;

        default:
            sendError(ServerCommand::UNKNOWN_COMMAND);
            return;
        }

    } else {
        DataVec data;
        data.reserve(data_size);
        for(auto ix = 0; ix < data_size; ++ix)
            data.push_back(*(command_int_ptr + 1 + ix));

        auto f = COMMAND_MAP.at(command_code);
        mLastCommand = command_code;
        ((*this).*f)(data);
    }
}

void PythonConnectionManager::sendError(ServerCommand errcode) {

    if(DEBUG_OUTPUT)
        std::cout << "Sending error code to client: " << static_cast<std::uint32_t>(errcode) << std::endl;

    mLastCommand = errcode;
    sendResponse({});

}

void PythonConnectionManager::sendResponse(const DataVec &data) {

    if(DEBUG_OUTPUT)
        std::cout << "Client: sending response" << std::endl;

    DataVec send_data;
    send_data.reserve(data.size() + 1);
    send_data.push_back(static_cast<std::uint32_t>(mLastCommand));
    for(auto x : data)
        send_data.push_back(x);

    mCommandSocket->send(zmq::buffer(send_data), zmq::send_flags::dontwait);

}

void PythonConnectionManager::sendPixelConfigData(const DataVec &data) {

    if((data.size() - 1) % 48)
        sendError(ServerCommand::ERROR_OCCURED);
    else
        mTpxManager->queueCommand(this, TpxCommand::CMD_SET_PIXCONF, data);

}

void PythonConnectionManager::bindUdpPort(const DataVec &data) {

    if(data.size() != 1) {
        sendError(ServerCommand::ERROR_OCCURED);
        return;
    }

    mThread.bindUdpPort(data[0]);

    sendResponse({});

}

void PythonConnectionManager::setTcpServer(TimepixConnectionManager &tpx) {

    mTpxManager = &tpx;

}

void PythonConnectionManager::forwardToSecondaryThread(zmq::socket_t *cmd_socket, zmq::message_t &msg) {

    try {

        if(cmd_socket == nullptr) {
            sendError(ServerCommand::THREAD_NOT_STARTED);
            return;
        } else if (!cmd_socket->connected()) {
            sendError(ServerCommand::THREAD_NOT_CONNECTED);
            return;
        }

        if(DEBUG_OUTPUT)
            std::cout << "Forwarding message to secondary thread" << std::endl;

        cmd_socket->send(msg, zmq::send_flags::none);

        if(DEBUG_OUTPUT)
            std::cout << "Forwarded message to secondary thread" << std::endl;

        zmq::message_t response;
        cmd_socket->recv(response);

        if(DEBUG_OUTPUT)
            std::cout << "Received response from secondary thread" << std::endl;

        mCommandSocket->send(response, zmq::send_flags::dontwait);

        if(DEBUG_OUTPUT)
            std::cout << "Forwarded response to client" << std::endl;

    } catch (...) {

        std::cout << "Error occurred while forwarding client command to secondary thread." << std::endl;

    }

}