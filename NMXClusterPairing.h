//
// Created by soegaard on 1/31/18.
//

#ifndef PROJECT_CLUSTERPAIRING_H
#define PROJECT_CLUSTERPAIRING_H

#include "NMXClustererDefinitions.h"
#include "NMXClusterManager.h"

struct buffer {
    uint32_t npoints;
    std::array<int, nmx::NCLUSTERS> data;
};

struct pair {
    uint64_t x_idx;
    uint64_t y_idx;
};

struct pair_buffer {
    uint64_t npairs;
    std::array<pair, 100>;
};

class NMXClusterPairing {

public:

    NMXClusterPairing(NMXClusterManager &clusterManager);

    pair_buffer pair(buffer &buf);

private:

    NMXClusterManager &m_cluster_manager;

    std::array<bool, nmx::NCLUSTERS> m_used;

    void reset(uint n);
};

#endif //PROJECT_PAIRCLUSTERS_H
