

/*
#include <fstream>
#include <iostream>
#include <vector>
#include <string>
#include <cstdint>
#include <iomanip>
#include <algorithm>
#include <chrono>

struct Packet {

    std::uint64_t raw_value;

    Packet(std::uint64_t val) :
        raw_value(val) {}

    int x() const { return (raw_value >> 56) & 0xFF; }
    int y() const { return (raw_value >> 48) & 0xFF; }
    std::int64_t t() const { return raw_value & 0x3FFFFFFFFF; }
    int tot() const { return (raw_value >> 38) & 0x3FF; }

};

//constexpr int XY_SEP = 5;
//constexpr std::int64_t T_SEP = 20;

constexpr int XY_SEP = 5;
constexpr std::int64_t T_SEP = 50;

struct Cluster {

    double xmean, ymean, tmean, tot_total;
    int xmin, xmax, ymin, ymax;
    std::int64_t tmin, tmax;

    Cluster(const Packet &p) :
        xmean(p.x()),
        ymean(p.y()),
        tmean(p.t()),
        tot_total(p.tot()),
        xmin(p.x() - XY_SEP),
        xmax(p.x() + XY_SEP),
        ymin(p.y() - XY_SEP),
        ymax(p.y() + XY_SEP),
        tmin(p.t() - T_SEP),
        tmax(p.t() + T_SEP) {}

    void addClick(const Packet &p) {
        xmean = (xmean * tot_total + p.x() * p.tot()) / (tot_total + p.tot());
        ymean = (ymean * tot_total + p.y() * p.tot()) / (tot_total + p.tot());
        tmean = (tmean * tot_total + p.t() * p.tot()) / (tot_total + p.tot());
        tot_total += p.tot();
        xmin = std::min(xmin, p.x() - XY_SEP);
        xmax = std::max(xmax, p.x() + XY_SEP);
        ymin = std::min(ymin, p.y() - XY_SEP);
        ymax = std::max(ymax, p.y() + XY_SEP);
        tmin = std::min(tmin, p.t() - T_SEP);
        tmax = std::max(tmax, p.t() + T_SEP);
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

std::chrono::high_resolution_clock::time_point startClock() {
    return std::chrono::high_resolution_clock::now();
}

long long stopClock(std::chrono::high_resolution_clock::time_point start_t) {
    return std::chrono::duration_cast<std::chrono::milliseconds>(startClock() - start_t).count();
}

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

std::vector<Packet> clusterPackets2(const std::vector<Packet> &packets) {

    auto start_t = startClock();
    std::vector<Packet> output;

    std::int64_t max_t_range = 20000;

    ClusterList open_clusters;
    for(std::size_t ix = 0; ix < packets.size(); ++ix) {

        auto &click = packets[ix];

        bool in_cluster = false;
        Cluster *last_cluster = nullptr;

        for(std::size_t cix = 0; cix < open_clusters.size(); ++cix) {
            auto &cluster = open_clusters.get(cix);
            if(cluster.containsClick(click)) {
                cluster.addClick(click);
                if(in_cluster) {
                    if(last_cluster) {
                        last_cluster->addCluster(cluster);
                    } else {
                        last_cluster = &cluster;
                    }
                } else {
                    in_cluster = true;
                    last_cluster = &cluster;
                }
            }
        }

        if(!in_cluster) {
            open_clusters.add(click); // create new cluster
        }

        for(std::int64_t cix = open_clusters.size() - 1; cix >= 0; --cix) {
            auto &cluster = open_clusters.get(cix);
            if(click.t() > cluster.tmax + max_t_range) {
                output.push_back(cluster.toRawValue());
                open_clusters.remove(cix);
            }
        }

    }

    std::cout << "End of clustering: " << output.size() << " clusters completed, " << open_clusters.size() << " clusters open" << std::endl;

    for(std::size_t cix = 0; cix < open_clusters.size(); ++cix) {
        output.push_back(open_clusters.get(cix).toRawValue());
    }

    std::cout << stopClock(start_t) << " ms to cluster" << std::endl;
    std::cout << output.size() << " clusters found" << std::endl;

    return output;

}

std::uint64_t reverseBytes(std::uint64_t x) {
    std::uint64_t y = 0;
    y |= (x >> 56) & 0x00000000000000FF;
    y |= (x >> 40) & 0x000000000000FF00;
    y |= (x >> 24) & 0x0000000000FF0000;
    y |= (x >>  8) & 0x00000000FF000000;
    y |= (x <<  8) & 0x000000FF00000000;
    y |= (x << 24) & 0x0000FF0000000000;
    y |= (x << 40) & 0x00FF000000000000;
    y |= (x << 56) & 0xFF00000000000000;
    return y;
}

std::vector<Packet> parsePackets(const std::vector<char> &data) {

    auto start_t = startClock();

    auto num64 = data.size() / 8;
    auto packet_ptr = reinterpret_cast<const std::uint64_t*>(data.data());

    std::size_t offset = 0;

    std::vector<Packet> packets;
    packets.reserve(num64);

    std::uint64_t oldest_time = std::numeric_limits<std::uint64_t>::max();
    std::uint64_t newest_time = 0;
    std::int64_t largest_jump = 0;

    while(offset < num64) {
        auto header = reverseBytes(packet_ptr[offset++]);
        auto name = header >> 32;
        if(name != 0x54505833) {
            std::cout << "Error: chunk header was not 'TPX3'" << std::endl;
        }
        //auto chip_ix = (header >> 24) & 0xFF;
        //auto zero = (header >> 16) & 0xFF;
        auto chunk_size = ((header >> 8) & 0xFF) | ((header << 8) & 0xFF00);

        for(std::size_t px = 0; px < chunk_size / 8; ++px) {
            auto packet = packet_ptr[offset + px];
            auto type = (packet & 0xF000000000000000) >> 60;
            if(type != 0xB) {
                std::cout << "Skipping packet of type " << std::hex << type << std::endl;
                continue;
            }
            auto addr = (packet & 0x0FFFF00000000000) >> 44;
            auto x = ((addr >> 1) & 0x00FC) | (addr & 0x0003);
            auto y = ((addr >> 8) & 0x00FE) | ((addr >> 2) & 0x0001);

            auto tot = (packet & 0x000000003FF00000) >> 20;

            auto toa = (packet & 0x00000FFFC0000000) >> 30;
            auto ftoa = ((packet & 0x00000000000F0000) >> 16) ^ 0x0F;
            auto stime = (packet & 0x000000000000FFFF);

            auto full_toa = (stime << 18) | (toa << 4) | ftoa;
            oldest_time = std::min(oldest_time, full_toa);
            newest_time = std::max(newest_time, full_toa);
            largest_jump = std::max(largest_jump, static_cast<std::int64_t>(newest_time) - static_cast<std::int64_t>(full_toa));

            auto tot_toa = (tot << 38) | full_toa;

            auto parsed_packet = (x << 56) | (y << 48) | tot_toa;
            packets.push_back(parsed_packet);
        }

        offset += chunk_size / 8;
    }

    std::cout << stopClock(start_t) << " ms to parse" << std::endl;
    std::cout << "Largest jump: " << largest_jump << std::endl;
    std::cout << "Total duration: " << (static_cast<double>(newest_time - oldest_time) * 1.5625e-9) << " s" << std::endl;

    return packets;

}

std::vector<Packet> clusterPackets(const std::vector<Packet> &packets) {

    std::vector<std::size_t> indices, uniqueIndices, invertedIndices;
    std::vector<Packet> centroids;
    std::vector<double> accumX, accumY, accumT, accumToT;

    auto start_t = startClock();

    auto arr_init_t = startClock();

    indices.reserve(packets.size());
    uniqueIndices.reserve(packets.size());
    invertedIndices.reserve(packets.size());
    accumX.resize(packets.size(), 0);
    accumY.resize(packets.size(), 0);
    accumT.resize(packets.size(), 0);
    accumToT.resize(packets.size(), 0);

    unsigned max_sep_xy = XY_SEP;
    unsigned max_sep_t = T_SEP;
    unsigned max_range = 10;

    for(auto ix = 0; ix < packets.size(); ++ix)
        indices.push_back(ix);

    std::cout << stopClock(arr_init_t) << " ms to initialize arrays" << std::endl;

    auto neighbour_t = startClock();

    for(std::size_t ix = 0; ix < packets.size(); ++ix) {
        Packet p1 = packets[ix];
        for(std::size_t r = 1; r < std::min(max_range, static_cast<decltype(max_range)>(packets.size() - ix - 1)); ++r) {
            Packet p2 = packets[ix+r];
            if((std::abs(p1.x()-p2.x()) <= max_sep_xy) && (std::abs(p1.y()-p2.y()) <= max_sep_xy) && (std::abs(p1.t()-p2.t()) <= max_sep_t)) {
                indices[ix+r] = indices[ix];
            }
        }
    }

    std::cout << stopClock(neighbour_t) << " ms to find neighbours" << std::endl;

    auto centroid_t = startClock();

    // get the unique packet indices
    uniqueIndices.resize(indices.size());
    invertedIndices.resize(indices.size());
    std::copy(indices.begin(), indices.end(), uniqueIndices.begin());
    std::sort(uniqueIndices.begin(), uniqueIndices.end());
    auto num_clusters = std::unique(uniqueIndices.begin(), uniqueIndices.end()) - uniqueIndices.begin();

    // invert mUniqueIndices to create an array mapping cluster indices to values in 0...num_clusters-1
    for(auto ix = 0; ix < num_clusters; ++ix) {
        invertedIndices[uniqueIndices[ix]] = ix;
    }

    for(auto ix = 0; ix < packets.size(); ++ix) {
        Packet p = packets[ix];
        auto arr_ix = invertedIndices[indices[ix]];
        accumX[arr_ix] += static_cast<double>(p.x()) * 1;
        accumY[arr_ix] += static_cast<double>(p.y()) * 1;
        accumT[arr_ix] += static_cast<double>(p.t()) * 1;
        accumToT[arr_ix] += 1;
    }

    std::cout << stopClock(centroid_t) << " ms to centroid" << std::endl;

    for(auto ix = 0; ix < num_clusters; ++ix) {
        auto x = static_cast<unsigned long long>(accumX[ix] / accumToT[ix]);
        auto y = static_cast<unsigned long long>(accumY[ix] / accumToT[ix]);
        auto t = static_cast<unsigned long long>(accumT[ix] / accumToT[ix]);
        unsigned long long val = 0;
        val |= ((x & 0xFF) << 56);
        val |= ((y & 0xFF) << 48);
        val |= (t & 0x3FFFFFFFFF);
        centroids.push_back(val);
    }

    std::cout << stopClock(start_t) << " ms to cluster" << std::endl;
    std::cout << "Found " << centroids.size() << " clusters" << std::endl;

    return centroids;

}

std::vector<std::uint32_t> histogram(const std::vector<Packet> &packets) {

    std::vector<std::uint32_t> image;
    image.resize(256*256, 0);

    for(auto &p : packets) {
        auto ix = p.y() * 256 + p.x();
        image[ix] += 1;
    }

    return image;

}

void saveImage(const std::vector<std::uint32_t> &image, std::string fname) {

    if(image.size() != 256*256) {
        std::cout << "Error: Tried to save image of size " << image.size() << std::endl;
        return;
    }

    std::ofstream out(fname);
    for(auto y = 0; y < 256; ++y) {
        for(auto x = 0; x < 256; ++x) {
            out << std::to_string(image[y * 256 + x]);
            if(x != 255)
                out << " ";
        }
        if(y != 255)
            out << "\n";
    }

}

int main(int argc, char* argv[]) {

    std::string fname = "C:/Users/JCEP Upsilon/Desktop/output.tpx3";
    std::string outname = "C:/Users/JCEP Upsilon/Desktop/image.txt";

    auto file_start_t = startClock();
    std::ifstream in(fname, std::ifstream::binary | std::ifstream::in);
    std::vector<char> raw_bytes;
    in.seekg(0, std::ios::end);
    auto num_bytes = in.tellg();
    raw_bytes.resize(num_bytes);
    in.seekg(0, std::ios::beg);
    in.read(reinterpret_cast<char*>(raw_bytes.data()), num_bytes);
    std::cout << stopClock(file_start_t) << " ms to load file" << std::endl;

    saveImage(histogram(parsePackets(raw_bytes)), outname);
    //saveImage(histogram(clusterPackets(parsePackets(raw_bytes))), outname);
    //saveImage(histogram(clusterPackets2(parsePackets(raw_bytes))), outname);

}
*/
