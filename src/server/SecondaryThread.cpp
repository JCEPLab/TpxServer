#include "server/SecondaryThread.h"

#include <iostream>

SecondaryThread::SecondaryThread(CommsThread &parent) :
    BgThread(),
    mParent(parent) {

    mParent.connectThread(this);

    mCommandSocket = std::make_unique<zmq::socket_t>(mParent.getZmq(), zmq::socket_type::rep);
    mCommandSocket->bind("tcp://*:*");

    if(DEBUG_OUTPUT)
        std::cout << "Opened secondary thread at " << mCommandSocket->get(zmq::sockopt::last_endpoint) << std::endl;

}

SecondaryThread::~SecondaryThread() {

    if(DEBUG_OUTPUT)
        std::cout << "Closing client connection" << std::endl;
    mCommandSocket->close();

}

std::string SecondaryThread::getCommandServerAddress() {

    auto zmq_addr = mCommandSocket->get(zmq::sockopt::last_endpoint);
    auto port_str = zmq_addr.substr(zmq_addr.find_last_of(':')+1);

    return "tcp://localhost:" + port_str;

}

std::unique_ptr<zmq::socket_t> SecondaryThread::getCommandClient(){

    auto socket = std::make_unique<zmq::socket_t>(mParent.getZmq(), zmq::socket_type::req);
    socket->connect(getCommandServerAddress());

    /*
    // make sure the socket works
    socket->send(zmq::buffer(std::vector<char>{0, 0, 0, 0}), zmq::send_flags::dontwait);
    zmq::message_t response;
    mCommandSocket->recv(response);
    */

    return std::move(socket);

}

void SecondaryThread::pollCommands() {

    try {
        zmq::message_t request;
        auto incoming_msg = mCommandSocket->recv(request, zmq::recv_flags::dontwait);

        if (incoming_msg) {
            auto *command_int_ptr = reinterpret_cast<const std::uint32_t*>(request.data());

            std::uint32_t com_code_int = command_int_ptr[0];
            auto command_code = static_cast<ServerCommand>(com_code_int);

            std::uint32_t com_size = request.size();
            if(com_size % 4) {
                return;
            }

            std::size_t data_size = (com_size / 4) - 1;

            if(DEBUG_OUTPUT)
                std::cout << "Secondary thread: received command " << com_code_int << std::endl;

            mLastCommand = command_code;
            DataVec data;
            data.reserve(data_size);
            for(auto ix = 0; ix < data_size; ++ix)
                data.push_back(*(command_int_ptr + 1 + ix));

            handleCommand(command_code, data);
        }

    } catch (zmq::error_t &ex) {
        emit err("An exception occurred while polling commands in a secondary thread:" + std::to_string(ex.num()) + " - \"" + ex.what() + "\")");
        cancel();
    }

}

void SecondaryThread::sendError(ServerCommand errcode) {

    mLastCommand = errcode;
    sendResponse({});

}

void SecondaryThread::sendResponse(const DataVec &data) {

    DataVec send_data;
    send_data.reserve(data.size() + 1);
    send_data.push_back(static_cast<std::uint32_t>(mLastCommand));
    for(auto x : data)
        send_data.push_back(x);

    mCommandSocket->send(zmq::buffer(send_data), zmq::send_flags::dontwait);

}

CommsThread& SecondaryThread::getParentThread() {

    return mParent;

}

zmq::context_t& SecondaryThread::getZmq() {

    return getParentThread().getZmq();

}
