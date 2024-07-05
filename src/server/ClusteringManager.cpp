#include "server/ClusteringManager.h"

#include <cmath>

#include "server/CommsThread.h" // for htonll

struct Packet {

    std::uint64_t raw_value;

    Packet(std::uint64_t val) :
        raw_value(val) {}

    int x() const { return (raw_value >> 56) & 0xFF; }
    int y() const { return (raw_value >> 48) & 0xFF; }
    std::int64_t t() const { return raw_value & 0x3FFFFFFFFF; }
    int tot() const { return (raw_value >> 38) & 0x3FF; }

};

struct ClusterSettings {
    int xy_sep;
    int t_sep;
    std::size_t max_t_separation = 20000;
};

struct Cluster {

    static ClusterSettings settings;

    double xmean, ymean, tmean, tot_total;
    int xmin, xmax, ymin, ymax;
    std::int64_t tmin, tmax;

    Cluster(const Packet &p) :
        xmean(p.x()),
        ymean(p.y()),
        tmean(p.t()),
        tot_total(p.tot()),
        xmin(p.x() - settings.xy_sep),
        xmax(p.x() + settings.xy_sep),
        ymin(p.y() - settings.xy_sep),
        ymax(p.y() + settings.xy_sep),
        tmin(p.t() - settings.t_sep),
        tmax(p.t() + settings.t_sep) {}

    void addClick(const Packet &p) {
        xmean = (xmean * tot_total + p.x() * p.tot()) / (tot_total + p.tot());
        ymean = (ymean * tot_total + p.y() * p.tot()) / (tot_total + p.tot());
        tmean = (tmean * tot_total + p.t() * p.tot()) / (tot_total + p.tot());
        tot_total += p.tot();
        xmin = std::min(xmin, p.x() - settings.xy_sep);
        xmax = std::max(xmax, p.x() + settings.xy_sep);
        ymin = std::min(ymin, p.y() - settings.xy_sep);
        ymax = std::max(ymax, p.y() + settings.xy_sep);
        tmin = std::min(tmin, p.t() - settings.t_sep);
        tmax = std::max(tmax, p.t() + settings.t_sep);
    }

    void addCluster(const Cluster &c) {
        xmean = (xmean * tot_total + c.xmean * c.tot_total)/(tot_total + c.tot_total);
        ymean = (ymean * tot_total + c.ymean * c.tot_total)/(tot_total + c.tot_total);
        tmean = (tmean * tot_total + c.tmean * c.tot_total)/(tot_total + c.tot_total);
        tot_total += c.tot_total;
        xmin = std::min(xmin, c.xmin);
        xmax = std::max(xmax, c.xmax);
        ymin = std::min(ymin, c.ymin);
        ymax = std::max(ymax, c.ymax);
        tmin = std::min(tmin, c.tmin);
        tmax = std::max(tmax, c.tmax);
    }

    std::uint64_t toRawValue() const {
        std::uint64_t val = 0;
        val |= ((static_cast<std::uint64_t>(xmean) & 0xFF) << 56);
        val |= ((static_cast<std::uint64_t>(ymean) & 0xFF) << 48);
        val |= (static_cast<std::uint64_t>(tmean) & 0x3FFFFFFFFF);
        return val;
    }

    bool containsClick(const Packet &p) {
        return (xmin <= p.x()) && (xmax >= p.x()) && (ymin <= p.y()) && (ymax >= p.y()) && (tmin <= p.t()) && (tmax >= p.t());
    }

};

struct ClusterList {

    std::vector<Cluster> list {};

    void add(const Cluster &c) {
        list.push_back(c);
    }

    Cluster& get(std::size_t ix) {
        return list[ix];
    }

    void remove(std::size_t ix) {
        if (list.size() > 1) {
            std::iter_swap(list.begin() + ix, list.end() - 1);
            list.pop_back();
        } else {
            list.clear();
        }
    }

    std::size_t size() const {
        return list.size();
    }

};

// default cluster settings
ClusterSettings Cluster::settings = {
    .xy_sep = 5,
    .t_sep = 20,
    .max_t_separation = 50000
};

constexpr std::size_t INITIAL_ARRAY_SIZE = 10000;

ClusteringManager::ClusteringManager(ClusterThread &thread) :
    mThread(thread) {

    mPublishSocket = std::make_unique<zmq::socket_t>(thread.getZmq(), zmq::socket_type::pub);
    mPublishSocket->bind("tcp://*:*");

    mOpenClusters = std::make_unique<ClusterList>();
    mOpenClusters->list.reserve(INITIAL_ARRAY_SIZE);

}

ClusteringManager::~ClusteringManager() {

    if(mRawPacketSocket)
        mRawPacketSocket->close();

}

std::string ClusteringManager::getPublishServerAddress() {

    auto zmq_addr = mPublishSocket->get(zmq::sockopt::last_endpoint);
    auto port_str = zmq_addr.substr(zmq_addr.find_last_of(':')+1);

    return "tcp://localhost:" + port_str;

}

void ClusteringManager::setRawPacketServerAddress(const std::string &path) {

    if(mRawPacketSocket) {
        mRawPacketSocket->close();
        mRawPacketSocket.reset();
    }

    if(path == "")
        return;

    try {
        mRawPacketSocket = std::make_unique<zmq::socket_t>(mThread.getZmq(), zmq::socket_type::sub);
        mRawPacketSocket->set(zmq::sockopt::subscribe, "");
        mRawPacketSocket->connect(path);
        mThread.sendLog("Connected clustering server to raw packet output at " + path);
    } catch (...) {
        mThread.sendWarn("Unable to connect clustering server to raw packet output at " + path);
    }

}

void ClusteringManager::poll() {

    if(!mRawPacketSocket)
        return;

    try {

        zmq::message_t msg;
        auto has_msg = mRawPacketSocket->recv(msg, zmq::recv_flags::dontwait);

        if(has_msg && msg.size() != 0) {
            auto num_packets = msg.size() / 8;
            auto packet_ptr = reinterpret_cast<const std::uint64_t*>(msg.data());
            handlePackets(packet_ptr, num_packets);
        }

    } catch (...) {

        mThread.sendWarn("Error occured while handling packets in clustering thread");
        DEBUG("Error occured while handling packets in clustering thread");

    }

}

void ClusteringManager::flush() {

    std::vector<std::uint64_t> output;

    for(std::size_t cix = 0; cix < mOpenClusters->size(); ++cix) {
        output.push_back(mOpenClusters->get(cix).toRawValue());
    }
    mOpenClusters->list.clear();

    mPublishSocket->send(zmq::buffer(output), zmq::send_flags::dontwait);

}

void ClusteringManager::handlePackets(const std::uint64_t *data, std::size_t num_packets) {

    static auto last_update_time = std::chrono::high_resolution_clock::now();
    static int num_new_clusters = 0;

    std::vector<std::uint64_t> output;

    for(std::size_t ix = 0; ix < num_packets; ++ix) {

        Packet click = data[ix];

        bool in_cluster = false;
        Cluster *last_cluster = nullptr;

        for(std::size_t cix = 0; cix < mOpenClusters->size(); ++cix) {
            auto &cluster = mOpenClusters->get(cix);
            if(cluster.containsClick(click)) {
                cluster.addClick(click);
                if(in_cluster) {
                    last_cluster->addCluster(cluster);
                    mOpenClusters->remove(cix);
                } else {
                    in_cluster = true;
                    last_cluster = &cluster;
                }
            }
        }

        if(!in_cluster) {
            mOpenClusters->add(click); // create new cluster
        }

        for(std::int64_t cix = mOpenClusters->size() - 1; cix >= 0; --cix) { // reverse iteration, so that we don't accidentally remove prior objects while iterating
            auto &cluster = mOpenClusters->get(cix);
            if(click.t() > cluster.tmax + Cluster::settings.max_t_separation) {
                output.push_back(cluster.toRawValue());
                mOpenClusters->remove(cix);
            }
        }

    }

    num_new_clusters += output.size();

    auto new_time = std::chrono::high_resolution_clock::now();
    if(std::chrono::duration_cast<std::chrono::milliseconds>(new_time - last_update_time).count() > 1000) {
        mThread.sendLog("Clustered " + std::to_string(num_new_clusters) + " clusters/s; " + std::to_string(mOpenClusters->size()) + " clusters are still in progress");
        num_new_clusters = 0;
        last_update_time = new_time;
    }

    if(!output.empty() && mFile) {
        for(const auto &cluster : output) {
            auto ordered_cluster = io::htonll(cluster); // fix byte order if necessary
            mFile.write(reinterpret_cast<const char*>(&ordered_cluster), sizeof(ordered_cluster));
        }
        //DEBUG("Saved " + std::to_string(output.size()) + " clusters to file");
    }

    mPublishSocket->send(zmq::buffer(output), zmq::send_flags::dontwait);

}

void ClusteringManager::setClusterParameters(int max_sep_xy, int max_sep_t, int max_t_sep) {

    Cluster::settings.xy_sep = max_sep_xy;
    Cluster::settings.t_sep = max_sep_t;
    Cluster::settings.max_t_separation = max_t_sep;

    DEBUG("Changed cluster parameters to [XY=" + std::to_string(max_sep_xy) + ", T=" + std::to_string(max_sep_t) + ", max separation=" + std::to_string(max_t_sep) + "]");

}

bool ClusteringManager::setSaveFile(const std::string &path) {

    if(mFile.is_open()) {
        mFile.close();
    }

    if(path.empty()) {
        DEBUG("Not saving clusters to file");
        return true;
    }

    bool success = true;

    try {
        mFile.open(path, std::ios_base::out | std::ios_base::binary);
        mFile << "CLUSTERS"; // header, and 8 reserved bytes to store the size
    } catch (...) {
        success = false;
    }

    success &= mFile.is_open();
    if(!success){
        DEBUG("Error opening file at " + path);
        return false;
    }

    DEBUG("Saving clusters to file " + path);
    mThread.sendLog("Saving clusters to " + path);

    return true;

}

