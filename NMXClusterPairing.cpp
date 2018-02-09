//
// Created by soegaard on 1/31/18.
//

#include "NMXClusterPairing.h"

NMXClusterPairing::NMXClusterPairing()
        : m_nIn(0),
          m_nOut(0)
{
    m_time_ordered_buffer.setArrayLength(nmx::STRIPS_PER_PLANE);
    m_time_ordered_buffer.setIgnoreBits(nmx::CLUSTER_IGNORE_BITS);
    m_time_ordered_buffer.setMinorBits(nmx::CLUSTER_MINOR_BITS);
    m_time_ordered_buffer.setMajorBits(nmx::CLUSTER_MAJOR_BITS);
}

void NMXClusterPairing::transferCluster(uint plane, const nmx::cluster &cl) {

    auto &cm = NMXClusterManager::getInstance();

    int idx = cm.getClusterFromStack();

    auto &cluster = cm.getCluster(idx);

    cluster.box = cl.box;
    cluster.npoints = cl.npoints;
    cluster.data = cl.data;

    m_time_ordered_buffer.addToBuffer(idx, plane, cluster.box.max_time);
}





