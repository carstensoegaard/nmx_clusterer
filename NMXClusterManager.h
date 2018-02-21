//
// Created by soegaard on 2/5/18.
//

#ifndef PROJECT_NMXCLUSTERMANAGER_H
#define PROJECT_NMXCLUSTERMANAGER_H


#include "NMXClustererDefinitions.h"

class NMXClusterManager {

public:

    NMXClusterManager();

    //~NMXClusterManager() = default;

    /*static NMXClusterManager& getInstance() {
        static NMXClusterManager instance;

        return instance;
    }*/

    int getClusterFromStack();
    void returnClusterToStack(int idx);

    void insertClusterInQueue(uint idx);

    int getNextFormQueue();

    nmx::cluster& getCluster(uint idx) { return m_buffer.at(idx); }

    void printStack();

private:


    int m_stackHead;
    int m_stackTail;

    int m_queueHead;
    int m_queueTail;

    nmx::cluster_buffer m_buffer;

    /*
    NMXClusterManager();// = default;
    NMXClusterManager(const NMXClusterManager&) = delete;
    NMXClusterManager& operator=(const NMXClusterManager&) = delete;
*/
    void init();
};

#endif //PROJECT_NMXCLUSTERMANAGER_H
