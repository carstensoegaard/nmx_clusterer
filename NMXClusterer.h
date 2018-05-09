//
// Created by soegaard on 2/13/18.
//

#ifndef PROJECT_NMXCLUSTERER_H
#define PROJECT_NMXCLUSTERER_H

#include <thread>

#include "NMXClustererDefinitions.h"
#include "NMXPlaneClusterer.h"
#include "NMXClusterPairing.h"

class NMXClusterer {

public:

    NMXClusterer();

    void addDatapoint(unsigned int plane, nmx::data_point &point);

    void endRun();
    void reset();
    void terminate();

    uint64_t getNumberOfOldPointsX() { return m_XplaneClusterer.getNumberOfOldPoints(); }
    uint64_t getNumberOfOldPointsY() { return m_YplaneClusterer.getNumberOfOldPoints(); }
    uint64_t getFailedClusterRequests() { return m_clusterManager.getFailedClusterRequests(); }
    uint64_t getNumberOfLateClusters() { return m_clusterPairing.getNumberOfLateClusters(); }


private:

    NMXPlaneClusterer m_XplaneClusterer;
    NMXPlaneClusterer m_YplaneClusterer;

    NMXClusterManager m_clusterManager;

    NMXClusterPairing m_clusterPairing;

    std::mutex m_mutex;
};


#endif //PROJECT_NMXCLUSTERER_H
