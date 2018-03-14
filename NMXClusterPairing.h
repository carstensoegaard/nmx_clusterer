//
// Created by soegaard on 1/31/18.
//

#ifndef PROJECT_CLUSTERPAIRING_H
#define PROJECT_CLUSTERPAIRING_H

#include "NMXClustererDefinitions.h"
#include "NMXClusterManager.h"
#include "NMXTimeOrderedBuffer.h"

class NMXClusterPairing {

public:

    NMXClusterPairing(NMXClusterManager &clusterManager);
    ~NMXClusterPairing();

    void insertClusterInQueue(int plane, unsigned int idx);
    void insertClusterInQueueX(unsigned int idx);
    void insertClusterInQueueY(unsigned int idx);

    void terminate() { m_terminate = true; }

   private:

    std::array<unsigned int, nmx::CLUSTER_BUFFER_SIZE> m_xQueue;
    std::array<unsigned int, nmx::CLUSTER_BUFFER_SIZE> m_yQueue;

    unsigned int m_nInX;
    unsigned int m_nOutX;
    unsigned int m_nInY;
    unsigned int m_nOutY;

    unsigned int m_nXcurrent;
    unsigned int m_nYcurrent;
    unsigned int m_nXprevious;
    unsigned int m_nYprevious;

    nmx::Qmatrix m_Qmatrix;

    bool m_switch;
    bool m_terminate;

    NMXTimeOrderedBuffer m_time_ordered_buffer;
    NMXClusterManager &m_cluster_manager;
    std::thread m_tinsert;
    std::thread m_tprocess;

    void calculateQmatrix(nmx::cluster_queue &current, nmx::cluster_queue &previous);

    unsigned int getQueueLength(unsigned int plane, int idx);

    void insert();
    void process();
   // void reset(uint n);
};

#endif //PROJECT_PAIRCLUSTERS_H
