#include "server/HistogramManager.h"

HistogramManager::HistogramManager(HistogramThread &thread) :
    mThread(thread) {

    mPublishSocket = std::make_unique<zmq::socket_t>(thread.getZmq(), zmq::socket_type::pub);
    mPublishSocket->bind("tcp://*:*");

    std::fill(mHistogram.begin(), mHistogram.end(), 0);
    mLastOutputTime = std::chrono::high_resolution_clock::now();

}

HistogramManager::~HistogramManager() {

    if(mInputSocket)
        mInputSocket->close();

}

std::string HistogramManager::getPublishServerAddress() {

    auto zmq_addr = mPublishSocket->get(zmq::sockopt::last_endpoint);
    auto port_str = zmq_addr.substr(zmq_addr.find_last_of(':')+1);

    return "tcp://localhost:" + port_str;

}

void HistogramManager::setInputServerAddress(const std::string &path) {

    if(mInputSocket) {
        mInputSocket->close();
        mInputSocket.reset();
    }

    if(path == "")
        return;

    try {
        mInputSocket = std::make_unique<zmq::socket_t>(mThread.getZmq(), zmq::socket_type::sub);
        mInputSocket->set(zmq::sockopt::subscribe, "");
        mInputSocket->connect(path);
        mThread.sendLog("Connected histogram server to output at " + path);
    } catch (...) {
        mThread.sendWarn("Unable to connect histogram server to output at " + path);
    }

}

void HistogramManager::poll() {

    if(!mInputSocket)
        return;

    try {
        zmq::message_t msg;
        auto has_msg = mInputSocket->recv(msg, zmq::recv_flags::dontwait);

        if(has_msg && msg.size() != 0) {
            auto num_packets = msg.size() / 8;
            auto packet_ptr = reinterpret_cast<const std::uint64_t*>(msg.data());
            handlePackets(packet_ptr, num_packets);

            auto new_time = std::chrono::high_resolution_clock::now();
            if(std::chrono::duration_cast<std::chrono::milliseconds>(new_time - mLastOutputTime).count() >= mOutputPeriod) {
                mPublishSocket->send(zmq::buffer(mHistogram), zmq::send_flags::dontwait);
                std::fill(mHistogram.begin(), mHistogram.end(), 0); // reset histogram
                mLastOutputTime = new_time;
            }
        }

    } catch (...) {

        mThread.sendWarn("Error occured while handling data in histogram thread");
        DEBUG("Error occured while handling data in histogram thread");

    }

}

void HistogramManager::setOutputPeriod(int period_ms) {

    mThread.sendLog("Setting output period to " + std::to_string(period_ms));
    mOutputPeriod = period_ms;

}

void HistogramManager::handlePackets(const std::uint64_t *data, std::size_t num_packets) {

    for(std::size_t ix = 0; ix < num_packets; ++ix) {

        auto packet = data[ix];
        auto x = (packet >> 56) & 0xFF;
        auto y = (packet >> 48) & 0xFF;

        std::size_t hist_ix = x + y*256;
        ++mHistogram[hist_ix];

    }

}
