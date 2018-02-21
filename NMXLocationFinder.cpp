//
// Created by soegaard on 2/19/18.
//

#include <iomanip>

#include "NMXLocationFinder.h"
#include "NMXClusterPairing.h"

NMXLocationFinder::NMXLocationFinder(NMXClusterManager &clustermanager)
        : m_clusterManager(clustermanager)
{
    m_file.open("NMX_PairedClusters.txt");
}

nmx_location NMXLocationFinder::find(pair_buffer &buf) {

    nmx_location loc;
    loc.time = -1;
    loc.x_strip = -1;
    loc.y_strip = -1;

    for (int i = 0; i < buf.npairs; i++) {

        nmx::cluster xcluster = m_clusterManager.getCluster(buf.pairs.at(i).x_idx);
        nmx::cluster ycluster = m_clusterManager.getCluster(buf.pairs.at(i).y_idx);

        m_file << "******\n";
        m_file << "x-points:\n";
        for (int ix = 0; ix < xcluster.npoints; ix++)
            m_file << std::setw(10) << xcluster.data.at(ix).time << " ";
        m_file << "\n";
        for (int ix = 0; ix < xcluster.npoints; ix++)
            m_file << std::setw(10) << xcluster.data.at(ix).strip << " ";
        m_file << "\n";
        for (int ix = 0; ix < xcluster.npoints; ix++)
            m_file << std::setw(10) << xcluster.data.at(ix).charge << " ";
        m_file << "\n";
        m_file << "y-points:\n";
        for (int iy = 0; iy < ycluster.npoints; iy++)
            m_file << std::setw(10) << ycluster.data.at(iy).time << " ";
        m_file << "\n";
        for (int iy = 0; iy < ycluster.npoints; iy++)
            m_file << std::setw(10) << ycluster.data.at(iy).strip << " ";
        m_file << "\n";
        for (int iy = 0; iy < ycluster.npoints; iy++)
            m_file << std::setw(10) << ycluster.data.at(iy).charge << " ";
        m_file << "\n";

        m_clusterManager.returnClusterToStack(buf.pairs.at(i).x_idx);
        m_clusterManager.returnClusterToStack(buf.pairs.at(i).y_idx);
    }



    return loc;
}