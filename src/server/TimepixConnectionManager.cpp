#include "server/TimepixConnectionManager.h"

#include <cstring>
#include <iomanip>
#include <iostream>
#include <map>
#include <chrono>
#include <thread>

#include "server/CommsThread.h"
#include "server/PythonConnectionManager.h"

const std::map<TpxCommand, void (TimepixConnectionManager::*)(const DataVec &)> HANDLER_MAP {
    {TpxCommand::CMD_NOP, &TimepixConnectionManager::genericHandler<1>},
    {TpxCommand::CMD_GET_SOFTWVERSION, &TimepixConnectionManager::genericHandler<1>},
    {TpxCommand::CMD_GET_FIRMWVERSION, &TimepixConnectionManager::genericHandler<1>},
    {TpxCommand::CMD_GET_CHIPBOARDID, &TimepixConnectionManager::genericHandler<1>},
    {TpxCommand::CMD_GET_DEVICEIDS, &TimepixConnectionManager::genericHandler<4>},
    {TpxCommand::CMD_GET_LOCALTEMP, &TimepixConnectionManager::genericHandler<1>},
    {TpxCommand::CMD_GET_REMOTETEMP, &TimepixConnectionManager::genericHandler<1>},
    {TpxCommand::CMD_GET_FPGATEMP, &TimepixConnectionManager::genericHandler<1>},
    {TpxCommand::CMD_GET_FANSPEED, &TimepixConnectionManager::genericHandler<1>},
    {TpxCommand::CMD_GET_PRESSURE, &TimepixConnectionManager::genericHandler<1>},
    {TpxCommand::CMD_GET_HUMIDITY, &TimepixConnectionManager::genericHandler<1>},
    {TpxCommand::CMD_GET_SPIDR_ADC, &TimepixConnectionManager::genericHandler<1>},
    {TpxCommand::CMD_SET_BIAS_ADJUST, &TimepixConnectionManager::genericHandler<1>},
    {TpxCommand::CMD_BIAS_SUPPLY_ENA, &TimepixConnectionManager::genericHandler<1>},
    {TpxCommand::CMD_GET_DEVICEPORT, &TimepixConnectionManager::genericHandler<1>},
    {TpxCommand::CMD_GET_SERVERPORT, &TimepixConnectionManager::genericHandler<1>},
    {TpxCommand::CMD_SET_SERVERPORT, &TimepixConnectionManager::genericHandler<1>},
    {TpxCommand::CMD_DECODERS_ENA, &TimepixConnectionManager::genericHandler<1>},
    {TpxCommand::CMD_RESTART_TIMERS, &TimepixConnectionManager::genericHandler<1>},
    {TpxCommand::CMD_RESET_TIMER, &TimepixConnectionManager::genericHandler<1>},
    {TpxCommand::CMD_RESET_PIXELS, &TimepixConnectionManager::genericHandler<1>},
    {TpxCommand::CMD_GET_SPIDRREG, &TimepixConnectionManager::genericHandler<2>},
    {TpxCommand::CMD_SET_SPIDRREG, &TimepixConnectionManager::genericHandler<1>},
    {TpxCommand::CMD_GET_PLLCONFIG, &TimepixConnectionManager::genericHandler<1>},
    {TpxCommand::CMD_SET_PLLCONFIG, &TimepixConnectionManager::genericHandler<1>},
    {TpxCommand::CMD_GET_HEADERFILTER, &TimepixConnectionManager::genericHandler<1>},
    {TpxCommand::CMD_SET_HEADERFILTER, &TimepixConnectionManager::genericHandler<1>},
    {TpxCommand::CMD_GET_GENCONFIG, &TimepixConnectionManager::genericHandler<1>},
    {TpxCommand::CMD_SET_GENCONFIG, &TimepixConnectionManager::genericHandler<1>},
    {TpxCommand::CMD_GET_TPPERIODPHASE, &TimepixConnectionManager::genericHandler<1>},
    {TpxCommand::CMD_SET_TPPERIODPHASE, &TimepixConnectionManager::genericHandler<1>},
    {TpxCommand::CMD_GET_TRIGCONFIG, &TimepixConnectionManager::genericHandler<5>},
    {TpxCommand::CMD_SET_TRIGCONFIG, &TimepixConnectionManager::genericHandler<1>},
    {TpxCommand::CMD_GET_DAC, &TimepixConnectionManager::genericHandler<1>},
    {TpxCommand::CMD_SET_DAC, &TimepixConnectionManager::genericHandler<1>},
    {TpxCommand::CMD_DDRIVEN_READOUT, &TimepixConnectionManager::genericHandler<1>},
    {TpxCommand::CMD_PAUSE_READOUT, &TimepixConnectionManager::genericHandler<1>},
    {TpxCommand::CMD_AUTOTRIG_START, &TimepixConnectionManager::genericHandler<1>},
    {TpxCommand::CMD_AUTOTRIG_STOP, &TimepixConnectionManager::genericHandler<1>},
    {TpxCommand::CMD_GET_PIXCONF, &TimepixConnectionManager::genericHandler<65>},
    {TpxCommand::CMD_SET_PIXCONF, &TimepixConnectionManager::genericHandler<1>},
    {TpxCommand::CMD_RESET_MODULE, &TimepixConnectionManager::genericHandler<1>},
    {TpxCommand::CMD_GET_READOUTSPEED, &TimepixConnectionManager::genericHandler<1>},
    {TpxCommand::CMD_SET_SENSEDAC, &TimepixConnectionManager::genericHandler<1>}
};

TimepixConnectionManager::TimepixConnectionManager(CommsThread &parent_thread, const std::deque<TimepixCommandInfo> &queue) :
    mThread(parent_thread),
    mQueuedCommands(queue) {

    // do nothing

}

TimepixConnectionManager::~TimepixConnectionManager() {

    mIsCancelled = true;
    terminateConnection();

}

void TimepixConnectionManager::terminateConnection() {

    if(mTpxSocket && mTpxSocket->is_open()) {
        try {
            mTpxSocket->cancel();
            mTpxSocket->shutdown(asio::ip::tcp::socket::shutdown_both);
            mTpxSocket->close();
            mTpxSocket.reset();

            mAsioIO.reset();

            DEBUG("Closed Timepix connection");
        } catch (asio::system_error &err) {
            mThread.sendWarn("Exception thrown while closing TCP connection");
            DEBUG("Exception thrown while closing TCP connection");
            mThread.sendWarn(err.what());
        }
    }
    mIsConnected = false;
}

void TimepixConnectionManager::poll() {

    try {

        if(mIsConnected && !mIsExecutingCommand && !mQueuedCommands.empty()) {
            auto next_command = mQueuedCommands.front();

            sendCommand(next_command.command, next_command.data);
            mLastCommandSource = next_command.sender;

            if(next_command.command == TpxCommand::CMD_RESET_MODULE) {
                terminateConnection();
                mLastCommandSource->sendResponse({}); // this server is going to die, so send a response to keep ZMQ happy
                mQueuedCommands.pop_front();
                mThread.sendLog("Resetting Timepix connection");
                mLastCommandSource = nullptr;
                DEBUG("Resetting Timepix connection due to SPIDR reset");
                std::this_thread::sleep_for(std::chrono::milliseconds(1000));
                attemptConnection(mLastHostIp, mLastHostPort, mLastServerIp, mLastServerPort, true);
                return;
            }
        }

        mAsioIO->poll();
        mAsioIO->restart();

    } catch (const asio::system_error &ex) {
        mThread.sendErr("A network error occurred while communicating with the Timepix (errcode=" + std::to_string(ex.code().value()) + ")");
        mThread.sendErr(ex.what());

        DEBUG("Network error: [" + std::to_string(ex.code().value()) + "]: " + ex.what());

        terminateConnection();
        //attemptConnection(mLastHostIp, mLastHostPort, mLastServerIp, mLastServerPort);
        mThread.cancel();

        this->clearAnyClientRequest();
    }

}

void TimepixConnectionManager::attemptConnection(const std::string &host_ip, int host_port, const std::string &server_ip, int server_port, bool wait) {

    mAsioIO = std::make_unique<asio::io_context>();
    mTpxSocket = std::make_unique<asio::ip::tcp::socket>(*mAsioIO);

    try {
        mTpxSocket->open(asio::ip::tcp::v4());
        mTpxSocket->set_option(asio::socket_base::reuse_address(true));
        mTpxSocket->bind(asio::ip::tcp::endpoint(asio::ip::address::from_string(host_ip), host_port));

        mThread.sendLog("Connecting to Timepix...");
        mTcpEndpoint = asio::ip::tcp::endpoint(asio::ip::address::from_string(server_ip), server_port);
        if(!wait) {
            mTpxSocket->async_connect(mTcpEndpoint,
                               [this](const asio::error_code &code) {
                                   initializeConnection(code);
            });
        } else {
            mTpxSocket->connect(mTcpEndpoint);
            initializeConnection(asio::error_code{});
        }

        mLastHostIp = host_ip;
        mLastServerIp = server_ip;
        mLastHostPort = host_port;
        mLastServerPort = server_port;

    } catch (asio::system_error &ex) {
        mThread.sendErr("An unknown error occurred in the server thread.");

        std::string str = ex.what();

        std::size_t pos = 0;
        while((pos = str.find('\n')) != std::string::npos)
            str.replace(pos, 1, "<br>");

        mThread.sendErr(str);

        DEBUG("An unknown error occurred in the server thread; terminating server.");

        mThread.cancel();
    }

}

void TimepixConnectionManager::initializeConnection(const asio::error_code &err) {

    if(mIsCancelled)
        return;

    if(err)
        throw asio::system_error(err);

    mIsConnected = true;
    mIsExecutingCommand = false;

    while(mTpxSocket->available()) {
        mTpxSocket->read_some(asio::buffer(mResponseBuffer));
    }

    mThread.sendLog("Successfully connected.");
    DEBUG("New TCP connection established");

}

bool TimepixConnectionManager::isConnected() const {

    return mIsConnected;

}

bool TimepixConnectionManager::isExecutingCommand() const {

    return mIsExecutingCommand;

}

void TimepixConnectionManager::queueCommand(PythonConnectionManager *sender, TpxCommand command, DataVec data) {

    mQueuedCommands.push_back({
        .command = command,
        .data = std::move(data),
        .sender = sender
    });

}

void TimepixConnectionManager::sendCommand(TpxCommand cmd, const DataVec &data, bool noreply) {

    if(mIsCancelled)
        return;

    if(noreply)
        cmd = static_cast<TpxCommand>( static_cast<uint32_t>(cmd) & static_cast<uint32_t>(TpxCommand::CMD_NOREPLY) );

    std::size_t msg_size = std::max<std::size_t>(4 + data.size(), 5);
    std::size_t byte_size = msg_size * sizeof(std::uint32_t);
    DataVec message(msg_size);

    message[0] = io::htonl(static_cast<uint32_t>(cmd));
    message[1] = io::htonl(msg_size * sizeof(std::uint32_t));
    message[2] = 0;
    message[3] = 0; // device number; this only supports a single Tpx3 chip

    if(data.empty()) {
        message[4] = 0;
    } else {
        for(auto i = 0; i < data.size(); ++i)
            message[4+i] = io::htonl(data[i]);
    }

    std::memcpy(mCommandBuffer.data(), message.data(), byte_size);

    mIsExecutingCommand = true;

    if(DEBUG_OUTPUT) {
        std::stringstream ss;
        //ss << "Sending command; (" << byte_size << " bytes)";
        ss << "Sending command (" << byte_size << " bytes):\n\t";
        for(auto ix = 0; ix < byte_size; ++ix) {
            ss << std::hex << std::setfill('0') << std::setw(2) << static_cast<unsigned>(reinterpret_cast<std::uint8_t*>(message.data())[ix]);
            if(ix != byte_size - 1)
                ss << ", ";
        }
        DEBUG(ss.str());
    }

    mTpxSocket->async_send(asio::buffer(mCommandBuffer.data(), byte_size), [this](const asio::error_code &err, std::size_t bytes) {
        DEBUG("\tCommand sent");

        commandSent(err, bytes);
    });

}

void TimepixConnectionManager::commandSent(const asio::error_code &err, std::size_t bytes_sent) {

    if(mIsCancelled)
        return;

    // queue up a socket read for the response
    mTpxSocket->async_read_some(asio::buffer(mResponseBuffer), [this](const asio::error_code &err, std::size_t bytes_received) {
        commandRecv(err, bytes_received);
    });

}

void TimepixConnectionManager::commandRecv(const asio::error_code &err, std::size_t bytes_received) {

    if(mIsCancelled)
        return;

    if(err && !mIsCancelled)
        throw asio::system_error(err);

    if(bytes_received < 20 || bytes_received % 4) {
        mThread.sendErr("Received an incomplete packet (" + std::to_string(bytes_received) + " bytes)");
        return;
    }

    auto cmd_int = io::ntohl(*reinterpret_cast<std::uint32_t*>(&mResponseBuffer[0]));
    if(!(cmd_int & static_cast<std::uint32_t>(TpxCommand::CMD_REPLY))) {
        mThread.sendErr("Received a packet without the REPLY bit set");
        return;
    }

    auto cmd_only_int = cmd_int ^ static_cast<std::uint32_t>(TpxCommand::CMD_REPLY);
    auto cmd = static_cast<TpxCommand>(cmd_only_int);

    auto num_bytes = io::ntohl(*reinterpret_cast<std::uint32_t*>(&mResponseBuffer[4]));
    auto error_int = io::ntohl(*reinterpret_cast<std::uint32_t*>(&mResponseBuffer[8]));
    auto chip_num = io::ntohl(*reinterpret_cast<std::uint32_t*>(&mResponseBuffer[12]));

    if(error_int) {
        std::stringstream ss;
        ss << std::hex << std::uppercase << cmd_only_int;
        auto command_str = ss.str();
        std::stringstream ss2;
        ss2 << std::hex << std::uppercase << error_int;
        auto error_str = ss2.str();
        mThread.sendErr("Timepix reported an error code (command=0x" + command_str + ", error=0x" + error_str + " [" + getErrorString(error_int) + "])");
    }

    if(DEBUG_OUTPUT) {
        std::stringstream ss;
        ss << "Received response:\n\t";
        for(auto ix = 0; ix < bytes_received; ++ix) {
            ss << std::hex << std::setfill('0') << std::setw(2) << static_cast<unsigned>(mResponseBuffer[ix]);
            if(ix != bytes_received - 1)
                ss << ", ";
        }
        std::cout << ss.str() << std::endl;
    }

    if(chip_num) {
        mThread.sendErr("Chip number in response was not zero (value=" + std::to_string(chip_num) + ")");
        return;
    }

    if(bytes_received < num_bytes) {
        mThread.sendErr("Timepix sent " + std::to_string(num_bytes) + " bytes, only received " + std::to_string(bytes_received) + " bytes");
        return;
    }

    auto data_bytes = num_bytes - 16;
    auto data_size = data_bytes / sizeof(std::uint32_t);

    DataVec data;
    data.reserve(data_size);

    auto data_ptr = reinterpret_cast<std::uint32_t*>(mResponseBuffer.data()) + 4;

    for(auto i = 0; i < data_size; ++i) {
        auto val = io::ntohl(data_ptr[i]);
        data.push_back(val);
    }

    processRecv(cmd, std::move(data));

}

void TimepixConnectionManager::processRecv(TpxCommand cmd, const DataVec data) {

    if(mIsCancelled)
        return;

    if(!mLastCommandSource)
        throw std::runtime_error("Received response, and there is no client!");

    if(!HANDLER_MAP.contains(cmd)) {
        mThread.sendWarn("Did not recognise response from Timepix: " + std::to_string(static_cast<std::uint32_t>(cmd)));
    } else {
        auto f = HANDLER_MAP.at(cmd);
        ((*this).*f)(data);
    }

    mQueuedCommands.pop_front(); // we no longer need to execute this command
    mIsExecutingCommand = false;
    mLastCommandSource = nullptr;

}

std::deque<TimepixCommandInfo> TimepixConnectionManager::getCommandQueue() {

    return mQueuedCommands;

}

void TimepixConnectionManager::clearAnyClientRequest() {

    if(mLastCommandSource) {
        mLastCommandSource->sendError(ServerCommand::ERROR_OCCURED);
        mLastCommandSource = nullptr;
    }

}

void TimepixConnectionManager::sendServerResetNotice() {

    if(mLastCommandSource) {
        mLastCommandSource->sendError(ServerCommand::SERVER_RESET);
        mLastCommandSource = nullptr;
    }

}

std::vector<std::string> SPIDR_ERROR_MESSAGES = {
    "NO_ERROR",
    "ERR_UNKNOWN_CMD",
    "ERR_MSG_LENGTH",
    "ERR_SEQUENCE",
    "ERR_ILLEGAL_PAR",
    "ERR_NOT_IMPLEMENTED",
    "ERR_TPX3_HARDW",
    "ERR_ADC_HARDW",
    "ERR_DAC_HARDW",
    "ERR_MON_HARDW",
    "ERR_FLASH_STORAGE"
};

std::vector<std::string> TIMEPIX_ERROR_MESSAGES = {
    "NO_ERROR",
    "TPX3_ERR_SC_ILLEGAL",
    "TPX3_ERR_SC_STATE",
    "TPX3_ERR_SC_ERRSTATE",
    "TPX3_ERR_SC_WORDS",
    "TPX3_ERR_TX_TIMEOUT",
    "TPX3_ERR_EMPTY",
    "TPX3_ERR_NOTEMPTY",
    "TPX3_ERR_FULL",
    "TPX3_ERR_UNEXP_REPLY",
    "TPX3_ERR_UNEXP_HEADER",
    "TPX3_ERR_LINKS_UNLOCKED"
};

std::string TimepixConnectionManager::getErrorString(std::uint32_t err_code) {

    auto err_byte = err_code & 0xFF;
    std::stringstream ss;
    ss << SPIDR_ERROR_MESSAGES[err_byte];

    if(err_byte == 6)
        ss << ": " << TIMEPIX_ERROR_MESSAGES[(err_code & 0xFF00) >> 8];

    return ss.str();

}
