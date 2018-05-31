//
// Created by soegaard on 2/13/18.
//

#include <iostream>
#include "../include/NMXClusterer.h"

NMXClusterer::NMXClusterer()
        : m_XplaneClusterer(0, m_clusterManager, m_clusterPairing, m_mutex),
          m_YplaneClusterer(1, m_clusterManager, m_clusterPairing, m_mutex),
          m_clusterPairing(m_clusterManager)
{
}

void NMXClusterer::addDatapoint(unsigned int plane, nmx::data_point &point) {

    // Check than the plane is either 0 or 1.
    if (plane > 1) {
        std::cerr << "<NMXClusterer::addDatapoint> Plane value " << plane << " out of range!\n";
        return;
    }

    // Check that the provided strip value is within range.
    if (point.strip >= nmx::STRIPS_PER_PLANE) {
        std::cerr << "<NMXClusterer::addDatapoint> Stip value of data-point out of range ! Value = "
                  << point.strip << " Valid range = [0, " << nmx::STRIPS_PER_PLANE-1 << "]" << std::endl;
        return;
    }

    // Add the data point to the correct plane.
    if (plane == 0)
        m_XplaneClusterer.addDataPoint(point);
    else
        m_YplaneClusterer.addDataPoint(point);
}

void NMXClusterer::endRun() {

    // End the run for the plan-clusterers
    m_XplaneClusterer.endRun();
    m_YplaneClusterer.endRun();

    // End the run for the paring algorithm.
    m_clusterPairing.endRun();
}

void NMXClusterer::reset() {

    // Reset the plane-clusterers.
    m_XplaneClusterer.reset();
    m_YplaneClusterer.reset();

    // Reset the pairing algorithm.
    m_clusterPairing.reset();

    // Reset the cluster-manager.
    m_clusterManager.reset();
}

void NMXClusterer::terminate() {

    // Termintate all threads in the plane-clusterers
    m_XplaneClusterer.terminate();
    m_YplaneClusterer.terminate();

    // Terminate the threads in the pairing algorithm
    m_clusterPairing.terminate();
}