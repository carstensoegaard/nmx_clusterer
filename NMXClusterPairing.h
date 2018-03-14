//
// Created by soegaard on 1/31/18.
//

#ifndef PROJECT_CLUSTERPAIRING_H
#define PROJECT_CLUSTERPAIRING_H

#include <thread>

#include "NMXClustererDefinitions.h"
#include "NMXClusterManager.h"
//#include "NMXTimeOrderedBuffer.h"

class NMXClusterPairing {

public:

    NMXClusterPairing(NMXClusterManager &clusterManager);
    ~NMXClusterPairing();

    void insertClusterInQueue(int plane, unsigned int cluster_idx);

    void terminate() { m_terminate = true; }

   private:

    std::array<std::array<unsigned int, nmx::CLUSTER_BUFFER_SIZE>, 2> m_queue;

    std::array<unsigned int, 2> m_nIn;
    std::array<unsigned int, 2> m_nOut;

    std::array<nmx::cluster_queue, nmx::MAX_MINOR> m_time_ordered_buffer;
    nmx::row_array m_majortime_buffer;
    unsigned int m_i1;

    unsigned int m_nXcurrent;
    unsigned int m_nYcurrent;
    unsigned int m_nXnext;
    unsigned int m_nYnext;

    nmx::Qmatrix m_Qmatrix;

    unsigned int m_verbose_level;
    bool m_terminate;

//    NMXTimeOrderedBuffer m_time_ordered_buffer;
    NMXClusterManager &m_clusterManager;
    std::thread m_tinsert;
    std::thread m_tprocess;

    uint32_t getMinorTime(uint32_t time);
    uint32_t getMajorTime(uint32_t time);

    void addToBuffer(unsigned int plane, int idx, uint minorTime);
    void slideTimeWindow(uint d, uint minorTime, uint majorTime);

    void calculateQmatrix(nmx::cluster_queue &this_queue, nmx::cluster_queue &next_queue);

    unsigned int getQueueLength(unsigned int plane, int idx);

    void insert();
    void process();
    void reset();

    // For debugging

    void returnQueueToStack(int plane, int idx);

};

#endif //PROJECT_PAIRCLUSTERS_H
