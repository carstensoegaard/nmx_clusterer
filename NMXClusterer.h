//
// Created by soegaard on 2/13/18.
//

#ifndef PROJECT_NMXCLUSTERER_H
#define PROJECT_NMXCLUSTERER_H

#include <thread>
#include "NMXClusterPairing.h"
#include "NMXClustererDefinitions.h"
#include "NMXPlaneClusterer.h"

class NMXClusterer {

public:

    NMXClusterer();

    void addDatapoint(unsigned int plane, nmx::data_point &point);

private:

    NMXPlaneClusterer m_XplaneClusterer;// = NMXPlaneClusterer(m_clusterManager, m_clusterPairing, m_mutex);
    NMXPlaneClusterer m_YplaneClusterer;// = NMXPlaneClusterer(m_clusterManager, m_clusterPairing, m_mutex);

    NMXClusterManager m_clusterManager;

    NMXClusterPairing m_clusterPairing;// = NMXClusterPairing(m_clusterManager);

    std::mutex m_mutex;
};


#endif //PROJECT_NMXCLUSTERER_H
