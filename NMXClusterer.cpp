//
// Created by soegaard on 2/13/18.
//

#include <iostream>
#include "NMXClusterer.h"

NMXClusterer::NMXClusterer()
        : m_XplaneClusterer(0, m_clusterManager, m_clusterPairing, m_mutex),
          m_YplaneClusterer(1, m_clusterManager, m_clusterPairing, m_mutex),
          m_clusterPairing(m_clusterManager)
{
}

void NMXClusterer::addDatapoint(unsigned int plane, nmx::data_point &point) {

    if (plane > 1) {
        std::cerr << "<NMXClusterer::addDatapoint> Plane value " << plane << " out of range!\n";
        return;
    }

    if (point.strip >= nmx::STRIPS_PER_PLANE) {
        std::cerr << "<NMXClusterer::addDatapoint> Stip value of data-point out of range ! Value = "
                  << point.strip << " Valid range = [0, " << nmx::STRIPS_PER_PLANE-1 << "]" << std::endl;
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