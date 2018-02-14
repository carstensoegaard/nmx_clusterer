//
// Created by soegaard on 1/31/18.
//

#include "NMXClusterPairing.h"

NMXClusterPairing::NMXClusterPairing(NMXClusterManager &clusterManager)
        : m_cluster_manager(clusterManager)
{

    reset(nmx::NCLUSTERS);
}

pair_buffer NMXClusterPairing::pair(buffer &buf) {

    pair_buffer pbuffer;
    pbuffer.npairs = 0;

    if (buf.npoints < 2)
        return pbuffer;

    for (uint i = 0; i < buf.npoints; i++) {

        if (m_used.at(i))
            continue;

        nmx::cluster& current_cluster = m_cluster_manager.getCluster(buf.data.at(i));

        uint n_matches = 0;
        uint32_t min_time_diff = UINT32_MAX;
        int64_t best_match = -1;

        for (uint ii = i; ii < buf.npoints; ii ++) {

            if (m_used.at(ii))
                continue;

            nmx::cluster& this_cluster = m_cluster_manager.getCluster(buf.data.at(ii));

            if (current_cluster.box.plane == this_cluster.box.plane)
                continue;

            uint32_t cluster_time_diff = std::abs(current_cluster.box.max_time - this_cluster.box.max_time);

            if (cluster_time_diff < nmx::MAX_CLUSTER_TIME_DIFF) {
                if (cluster_time_diff < min_time_diff) {
                    min_time_diff = cluster_time_diff;
                    best_match = ii;
                }
            }
        }

        if (best_match > -1) {

            pair p;

            if (current_cluster.box.plane == 0) {

                p.x_idx = buf.data.at(i);
                p.y_idx = buf.data.at(best_match);
            } else {

                p.x_idx = buf.data.at(best_match);
                p.y_idx = buf.data.at(i);
            }

            pbuffer.at(pbuffer.npairs) = p;

            m_used.at(i) = true;
            m_used.at(best_match) = true;
        }
    }

    // Reset used array
    reset(buf.npoints);

    return pbuffer;
}

void NMXClusterPairing::reset(uint n) {

    for (uint i = 0; i < n; i++)
        m_used.at(i) = false;
}




