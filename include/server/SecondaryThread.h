#ifndef SECONDARYTHREAD_H
#define SECONDARYTHREAD_H

#include <string>
#include <memory>

#include "BgThread.h"

#include "zmq.hpp"

#include "CommsThread.h"

class SecondaryThread : public BgThread {

public:
    SecondaryThread(CommsThread &parent);
    ~SecondaryThread();
    SecondaryThread(const SecondaryThread &rhs) = delete;

    std::string getCommandServerAddress();
    std::unique_ptr<zmq::socket_t> getCommandClient();

    void pollCommands();
    virtual void handleCommand(ServerCommand cmd, const DataVec &data) = 0;

    void sendError(ServerCommand errcode);
    void sendResponse(const DataVec &data);

    CommsThread& getParentThread();
    zmq::context_t& getZmq();

private:
    std::unique_ptr<zmq::socket_t> mCommandSocket { nullptr };
    ServerCommand mLastCommand {ServerCommand::ERROR_OCCURED};
    CommsThread &mParent;

};

#endif // SECONDARYTHREAD_H
