#ifndef TPXSERVER_TIMEPIXCONNECTIONMANAGER_H
#define TPXSERVER_TIMEPIXCONNECTIONMANAGER_H

#include <string>
#include <deque>
#include <utility>

#include "asio.hpp"

#include "TimepixCodes.h"
#include "TimepixCommandInfo.h"

#include "common_defs.h"

class CommsThread;
class PythonConnectionManager;

class TimepixConnectionManager {

public:
    TimepixConnectionManager(CommsThread &parent_thread, const std::deque<TimepixCommandInfo> &queue = {});
    ~TimepixConnectionManager();
    TimepixConnectionManager(const TimepixConnectionManager &rhs) = delete;

    void poll();

    void queueCommand(PythonConnectionManager *sender, TpxCommand command, DataVec data = {});

    void attemptConnection(const std::string &host_ip, int host_port, const std::string &server_ip, int server_port, bool wait=false);
    void initializeConnection(const asio::error_code &code);
    void terminateConnection();

    bool isConnected() const;
    bool isExecutingCommand() const;

    template<std::size_t sz>
    void genericHandler(const DataVec &data);

    std::deque<TimepixCommandInfo> getCommandQueue();

    void clearAnyClientRequest();
    void sendServerResetNotice();

private:
    void sendCommand(TpxCommand cmd, const DataVec &data={}, bool noreply=false);
    void commandSent(const asio::error_code &err, std::size_t bytes_sent);
    void processRecv(TpxCommand cmd, const DataVec data);
    void commandRecv(const asio::error_code &err, std::size_t bytes_received);

    std::string getErrorString(std::uint32_t err_code);

    std::string mLastHostIp {};
    std::string mLastServerIp {};
    int mLastHostPort {};
    int mLastServerPort {};

    CommsThread &mThread;
    std::unique_ptr<asio::io_service> mAsioIO;
    std::unique_ptr<asio::ip::tcp::socket> mTpxSocket {nullptr};
    asio::ip::tcp::endpoint mTcpEndpoint {};

    bool mIsConnected {false};
    bool mIsCancelled {false};
    bool mIsExecutingCommand {false};

    PythonConnectionManager *mLastCommandSource {nullptr};
    TpxCommand mExpectedReply { TpxCommand::CMD_NOREPLY };
    std::vector<std::uint8_t> mCommandBuffer = std::vector<std::uint8_t>(1024);
    std::vector<std::uint8_t> mResponseBuffer = std::vector<std::uint8_t>(1024);

    std::deque<TimepixCommandInfo> mQueuedCommands;

};

#include "CommsThread.h"
#include "PythonConnectionManager.h"

template<std::size_t sz>
void TimepixConnectionManager::genericHandler(const DataVec &data) {
    if(data.size() != sz)
        mLastCommandSource->sendError(ServerCommand::ERROR_OCCURED);
    else
        mLastCommandSource->sendResponse(data);
}

#endif //TPXSERVER_TIMEPIXCONNECTIONMANAGER_H
