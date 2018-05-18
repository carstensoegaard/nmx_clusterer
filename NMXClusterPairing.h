//
// Created by soegaard on 1/31/18.
//

#ifndef PROJECT_CLUSTERPAIRING_H
#define PROJECT_CLUSTERPAIRING_H

#include <thread>
#include <mutex>

#include "NMXClustererDefinitions.h"
#include "NMXClusterManager.h"
#include "NMXLocationFinder.h"

class NMXClusterPairing {

public:

    NMXClusterPairing(NMXClusterManager &clusterManager);
    ~NMXClusterPairing();

    void insertClusterInQueue(int plane, unsigned int cluster_idx);

    void endRun();
    void reset();
    void terminate() { m_terminate = true; }

    uint64_t getNumberOfLateClusters() { return m_nLateClusters; }

   private:

    std::array<std::array<unsigned int, nmx::NCLUSTERS>, 2> m_queue;

    std::array<unsigned int, 2> m_nIn;
    std::array<unsigned int, 2> m_nOut;

    std::array<nmx::clusterParingEntry, nmx::CLUSTER_MAX_MINOR> m_time_ordered_buffer;
    nmx::row_array m_majortime_buffer;
    uint32_t m_i1;

    unsigned int m_nXthis;
    unsigned int m_nYthis;
    unsigned int m_nXnext;
    unsigned int m_nYnext;

    nmx::Qmatrix m_Qmatrix;

    unsigned int m_verbose_level;
    bool m_terminate;

    uint64_t m_nLateClusters = 0;

    NMXClusterManager &m_clusterManager;
    NMXLocationFinder  m_locationFinder;

    void process();
    std::thread t_process;
    std::mutex m_mutex;

    uint32_t getMinorTime(uint32_t time);
    uint32_t getMajorTime(uint32_t time);

    void addToBuffer(unsigned int plane, int idx, uint minorTime);
    void slideTimeWindow(uint d, uint minorTime, uint majorTime);

    void pairQueues(nmx::clusterParingEntry &this_queue, nmx::clusterParingEntry &next_queue);
    nmx::Qmatrix calculateQmatrix(nmx::clusterParingEntry &this_queue, nmx::clusterParingEntry &next_queue);
    nmx::clusterPair findMinQ(const nmx::Qmatrix &qmatrix);

    void appendIndexToQueue(unsigned int plane, nmx::clusterParingEntry &queue, int clusterIdx);

    unsigned int getQueueLength(unsigned int plane, int idx);

    // For debugging

    void returnQueueToStack(int plane, int idx);

    void printSortBuffer();
    void printQueue();
    void printQmatrix();

    void checkSortBuffer();
};

#endif //PROJECT_PAIRCLUSTERS_H
