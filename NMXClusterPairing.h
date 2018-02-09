//
// Created by soegaard on 1/31/18.
//

#ifndef PROJECT_CLUSTERPAIRING_H
#define PROJECT_CLUSTERPAIRING_H

#include "NMXClustererDefinitions.h"
#include "NMXTimeOrderedBuffer.h"
#include "NMXClusterManager.h"

class NMXClusterPairing {

public:

    NMXClusterPairing();

    void transferCluster(uint plane, const nmx::cluster& cl);

    void process();


private:

    uint64_t m_nIn;
    uint64_t m_nOut;

    NMXTimeOrderedBuffer m_time_ordered_buffer;

    void insertClusterInBuffer(uint plane, uint32_t time_idx, int cluster_idx);

    void reset();
};

#endif //PROJECT_PAIRCLUSTERS_H
