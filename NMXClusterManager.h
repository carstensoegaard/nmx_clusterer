//
// Created by soegaard on 2/5/18.
//

#ifndef PROJECT_NMXCLUSTERMANAGER_H
#define PROJECT_NMXCLUSTERMANAGER_H

#include <array>
#include <mutex>

#include "NMXClustererDefinitions.h"

class NMXClusterManager {

public:

    NMXClusterManager();

    int getClusterFromStack(unsigned int plane);

    void returnClusterToStack(unsigned int plane, unsigned int idx);

    int getLink1(unsigned int plane, unsigned int idx);

    nmx::cluster &getCluster(unsigned int plane, unsigned int idx);

    void reset();

    uint64_t getFailedClusterRequests() { return m_nFailedClusterRequests; }

    void printStack(unsigned int plane);

private:

    std::array<int, 2> m_stackHead;
    std::array<int, 2> m_stackTail;

    std::array<nmx::cluster_buffer, 2> m_buffer;

    std::mutex m_mutex[2];

    uint64_t m_nFailedClusterRequests = 0;

    unsigned int m_verboseLevel;
};

#endif //PROJECT_NMXCLUSTERMANAGER_H
