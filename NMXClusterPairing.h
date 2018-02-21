//
// Created by soegaard on 1/31/18.
//

#ifndef PROJECT_CLUSTERPAIRING_H
#define PROJECT_CLUSTERPAIRING_H

#include "NMXClustererDefinitions.h"
#include "NMXClusterManager.h"
#include "NMXTimeOrderedBuffer.h"
/*
struct buffer {
    uint32_t npoints;
    std::array<int, nmx::NCLUSTERS> data;
};
*/
struct clusterPair {
    uint64_t time;
    uint64_t x_idx;
    uint64_t y_idx;
};

struct pair_buffer {
    uint64_t npairs;
    std::array<clusterPair, 100> pairs;
};

class NMXClusterPairing {

public:

    NMXClusterPairing(NMXClusterManager &clusterManager);

    void insertClusterInQueue(unsigned int idx);

    pair_buffer pair(buffer<int> &buf);

private:

    std::thread m_process;

    NMXTimeOrderedBuffer<int> m_time_ordered_buffer;

    NMXClusterManager &m_cluster_manager;

    std::array<bool, nmx::NCLUSTERS> m_used;

    void process();
    void reset(uint n);
};

#endif //PROJECT_PAIRCLUSTERS_H
