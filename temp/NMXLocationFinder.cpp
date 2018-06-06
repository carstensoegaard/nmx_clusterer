//
// Created by soegaard on 2/19/18.
//

#include <iomanip>

//#include "NMXClusterPairing.h"
#include "../helper/include/NMXClustererVerification.h"
#include "../clusterer/include/NMXLocationFinder.h"

NMXLocationFinder::NMXLocationFinder(NMXClusterManager &clustermanager)
        : m_clusterManager(clustermanager)
{
    m_file.open("NMX_PairedClusters.txt");
}

NMXLocationFinder::~NMXLocationFinder() {

    //std::cout << "Total xPoints = " << totalxPoints << std::endl;
    //std::cout << "Total yPoints = " << totalyPoints << std::endl;
}

nmx_location NMXLocationFinder::find(nmx::PairBuffer &buf) {

    nmx_location loc;
    loc.time = -1;
    loc.x_strip = -1;
    loc.y_strip = -1;

       for (unsigned int i = 0; i < buf.npairs; i++) {

           nmx::Cluster xcluster = m_clusterManager.getCluster(0, buf.pairs.at(i).x_idx);
           nmx::Cluster ycluster = m_clusterManager.getCluster(1, buf.pairs.at(i).y_idx);

           nmx::FullCluster cluster;
           cluster.clusters.at(0) = xcluster;
           cluster.clusters.at(1) = ycluster;

           //totalxPoints += xcluster.nPoints;
           //totalyPoints += ycluster.nPoints;


           m_file << "******\n";
           m_file << "x-points:\n";
           for (unsigned int ix = 0; ix < xcluster.nPoints; ix++)
               m_file << std::setw(10) << xcluster.data.at(ix).time << " ";
           m_file << "\n";
           for (unsigned int ix = 0; ix < xcluster.nPoints; ix++)
               m_file << std::setw(10) << xcluster.data.at(ix).strip << " ";
           m_file << "\n";
           for (unsigned int ix = 0; ix < xcluster.nPoints; ix++)
               m_file << std::setw(10) << xcluster.data.at(ix).charge << " ";
           m_file << "\n";
           m_file << "y-points:\n";
           for (unsigned int iy = 0; iy < ycluster.nPoints; iy++)
               m_file << std::setw(10) << ycluster.data.at(iy).time << " ";
           m_file << "\n";
           for (unsigned int iy = 0; iy < ycluster.nPoints; iy++)
               m_file << std::setw(10) << ycluster.data.at(iy).strip << " ";
           m_file << "\n";
           for (unsigned int iy = 0; iy < ycluster.nPoints; iy++)
               m_file << std::setw(10) << ycluster.data.at(iy).charge << " ";
           m_file << "\n";

           m_clusterManager.returnClusterToStack(0, buf.pairs.at(i).x_idx);
           m_clusterManager.returnClusterToStack(1, buf.pairs.at(i).y_idx);
       }

    return loc;
}