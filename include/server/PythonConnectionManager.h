#ifndef TPXSERVER_PYTHONCONNECTIONMANAGER_H
#define TPXSERVER_PYTHONCONNECTIONMANAGER_H

#include <iomanip>
#include <sstream>
#include <utility>

#include "zmq.hpp"

#include "ServerCodes.h"
#include "TimepixCodes.h"

#include "common_defs.h"

class CommsThread;
class TimepixConnectionManager;

class PythonConnectionManager {

public:
    PythonConnectionManager(CommsThread &thread, TimepixConnectionManager &tpx_manager);
    ~PythonConnectionManager();
    PythonConnectionManager(const PythonConnectionManager &rhs) = delete;

    void open(unsigned int port);
    void close();
    void poll();

    void handleCommand(zmq::message_t &command);

    void sendError(ServerCommand errcode);
    void sendResponse(const DataVec &data);

    template<std::size_t sz, TpxCommand cmd>
    void genericForward(const DataVec &data);

    void forwardToSecondaryThread(zmq::socket_t *cmd_socket, zmq::message_t &msg);

    void sendPixelConfigData(const DataVec &data);

    void bindUdpPort(const DataVec &data);

    void setTcpServer(TimepixConnectionManager &tpx_manager);

private:

    CommsThread &mThread;
    TimepixConnectionManager *mTpxManager;
    std::unique_ptr<zmq::socket_t> mCommandSocket {nullptr};
    ServerCommand mLastCommand {ServerCommand::ERROR_OCCURED};

};

#include "server/CommsThread.h"
#include "server/TimepixConnectionManager.h"

template<std::size_t sz, TpxCommand cmd>
void PythonConnectionManager::genericForward(const DataVec &data) {
    if(data.size() != sz) {
        std::stringstream ss;
        ss << "An error occurred while executing command [0x";
        ss << std::hex << std::uppercase << static_cast<std::uint32_t>(cmd);
        ss << "]: expected command of size " << std::dec << sz << ", received " << data.size();
        emit mThread.warn(ss.str());
        sendError(ServerCommand::INVALID_COMMAND_DATA);
    } else {
        mTpxManager->queueCommand(this, cmd, data);
    }
}

#endif //TPXSERVER_PYTHONCONNECTIONMANAGER_H
