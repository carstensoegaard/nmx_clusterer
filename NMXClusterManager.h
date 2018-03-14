//
// Created by soegaard on 2/5/18.
//

#ifndef PROJECT_NMXCLUSTERMANAGER_H
#define PROJECT_NMXCLUSTERMANAGER_H

#include <array>

#include "NMXClustererDefinitions.h"

class NMXClusterManager {

public:

    NMXClusterManager();

    int getClusterFromStackX();
    int getClusterFromStackY();
    int getClusterFromStack(unsigned int plane);

    void returnClusterToStackX(unsigned int idx);
    void returnClusterToStackY(unsigned int idx);
    void returnClusterToStack(unsigned int plane, unsigned int idx);

    int getLink1(unsigned int plane, unsigned int idx);
    int getLink2(unsigned int plane, unsigned int idx);

    bool setLink1(unsigned int plane, unsigned int idx, int link1);
    bool setLink2(unsigned int plane, unsigned int idx, int link1);

    nmx::cluster &getClusterX(unsigned int idx);
    nmx::cluster &getClusterY(unsigned int idx);
    nmx::cluster &getCluster(unsigned int plane, unsigned int idx);

    void printStack(unsigned int plane);

private:

    std::array<int, 2> m_stackHead;
    std::array<int, 2> m_stackTail;

    std::array<int, 2> m_queueHead;
    std::array<int, 2> m_queueTail;

    std::array<nmx::cluster_buffer, 2> m_buffer;

    void init();
};

#endif //PROJECT_NMXCLUSTERMANAGER_H
