//
// Created by soegaard on 2/13/18.
//

#include <iostream>
#include "NMXClusterer.h"

NMXClusterer::NMXClusterer()
        : m_XplaneClusterer(m_clusterManager, m_clusterPairing, m_mutex),
          m_YplaneClusterer(m_clusterManager, m_clusterPairing, m_mutex),
          m_clusterPairing(m_clusterManager)
{
    m_XplaneClusterer.setPlane(0);
    m_XplaneClusterer.setVerboseLevel(0);
    m_YplaneClusterer.setPlane(1);
    m_YplaneClusterer.setVerboseLevel(0);
}

void NMXClusterer::addDatapoint(unsigned int plane, nmx::data_point &point) {

    if (plane > 1) {
        std::cerr << "<NMXClusterer::addDatapoint> Plane value " << plane << " out of range!\n";
        return;
    }

    if (plane == 0)
        m_XplaneClusterer.addDataPoint(point);
    else
        m_YplaneClusterer.addDataPoint(point);
}

void NMXClusterer::endRun() {

    m_XplaneClusterer.endRun();
    m_YplaneClusterer.endRun();

    m_clusterPairing.endRun();
}

void NMXClusterer::reset() {

    m_XplaneClusterer.reset();
    m_YplaneClusterer.reset();

    m_clusterPairing.reset();

    m_clusterManager.reset();
}

void NMXClusterer::terminate() {

    m_XplaneClusterer.terminate();
    m_YplaneClusterer.terminate();

    m_clusterPairing.terminate();
}