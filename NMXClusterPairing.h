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



class NMXClusterPairing {

public:

    NMXClusterPairing(NMXClusterManager &clusterManager);
    ~NMXClusterPairing();

    void insertClusterInQueue(unsigned int idx);

   // pair_buffer pair(nmx::idx_buffer &buf, nmx::idx_buffer &prevbuf);

private:

    std::thread m_process;

    NMXTimeOrderedBuffer m_time_ordered_buffer;
    //NMXTimeOrderedBuffer m_time_ordered_buffer;

    NMXClusterManager &m_cluster_manager;

    std::array<bool, nmx::NCLUSTERS> m_used;

    unsigned int m_nXcurrent;
    unsigned int m_nYcurrent;
    unsigned int m_nXprevious;
    unsigned int m_nYprevious;

    nmx::Qmatrix m_Qmatrix;

    bool m_terminate;

    void calculateQmatrix(nmx::cluster_queue &current, nmx::cluster_queue &previous);

    unsigned int getQueueLength(unsigned int idx);

    nmx::idx_buffer addBuffers(nmx::idx_buffer &b1, nmx::idx_buffer &b2);
    void process();
    void reset(uint n);
};

#endif //PROJECT_PAIRCLUSTERS_H
