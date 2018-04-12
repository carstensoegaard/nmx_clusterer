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

    bool setLink1(unsigned int plane, unsigned int idx, int link1);

    nmx::cluster &getCluster(unsigned int plane, unsigned int idx);

    void printStack(unsigned int plane);

private:

    std::array<int, 2> m_stackHead;
    std::array<int, 2> m_stackTail;

    std::array<nmx::cluster_buffer, 2> m_buffer;

    std::mutex m_mutex[2];

    unsigned int m_verboseLevel;

    void init();
};

#endif //PROJECT_NMXCLUSTERMANAGER_H
