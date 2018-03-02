//
// Created by soegaard on 1/31/18.
//

#include <iomanip>
#include "NMXLocationFinder.h"
//#include "NMXClusterPairing.h"

NMXClusterPairing::NMXClusterPairing(NMXClusterManager &clusterManager)
        : m_cluster_manager(clusterManager),
          m_time_ordered_buffer(clusterManager)
{

    m_time_ordered_buffer.setVerboseLevel(0);

    reset(nmx::NCLUSTERS);

    m_process = std::thread(&NMXClusterPairing::process, this);
}

NMXClusterPairing::~NMXClusterPairing() {

    m_process.join();
}

void NMXClusterPairing::insertClusterInQueue(unsigned int idx) {

    if (idx > nmx::NCLUSTERS) {
        std::cout << "<NMXClusterPairing> Index " << idx << " out of range!\n";
        throw 1;
    }

    nmx::cluster &cluster = m_cluster_manager.getCluster(idx);

    uint32_t time = cluster.box.max_time;

    m_time_ordered_buffer.insert(idx, time);
}

void NMXClusterPairing::process() {

    NMXLocationFinder finder(m_cluster_manager);

    nmx::cluster_queue previous;
    previous.at(0) = -1;
    previous.at(1) = -1;

    int xSize = 0;
    int ySize = 0;

    nmx::pairBuffer pairbuf;
    pairbuf.npairs = 0;

    while (1) {

        nmx::cluster_queue current = m_time_ordered_buffer.getNextSorted();

        calculateQmatrix(current, previous);

        bool cont = true;

        // This could be changed to a do-while loop
        while (cont) {

            nmx::clusterPair pair;
            pair.x_idx = UINT32_MAX;
            pair.y_idx = UINT32_MAX;

            double minQ = 2.;

            unsigned int x = UINT32_MAX;
            unsigned int y = UINT32_MAX;

            for (unsigned int ix = 0; ix < m_nXcurrent + m_nXprevious; ix++) {
                for (unsigned int iy = 0; iy < m_nYcurrent + m_nYprevious; iy++) {
                    double Q = m_Qmatrix.at(ix).at(iy).Qval;
                    if (Q < minQ) {
                        minQ = Q;
                        x = ix;
                        y = iy;
                        pair.x_idx = m_Qmatrix.at(ix).at(iy).queue.at(0);
                        pair.y_idx = m_Qmatrix.at(ix).at(iy).queue.at(1);
                    }
                }
            }

            nmx::QmatrixEntry &matrixEntry = m_Qmatrix.at(x).at(y);

            if (minQ < nmx::DELTA_Q) {
                if ((x < m_nXcurrent) && (y < m_nYcurrent))
                    matrixEntry.Qval = 2.;
                else {
                    pairbuf.pairs.at(pairbuf.npairs) = pair;
                    pairbuf.npairs++;
                    matrixEntry.queue.at(0) = UINT32_MAX;
                    matrixEntry.queue.at(1) = UINT32_MAX;
                }
            } else
                cont = false;
        }

        for (unsigned int ix = m_nXcurrent; ix < m_nXcurrent+m_nXprevious; ix++) {
            for (unsigned int iy = m_nYcurrent; iy < m_nYcurrent+m_nYprevious; iy++) {

                nmx::QmatrixEntry &matrixEntry = m_Qmatrix.at(ix).at(iy);
                unsigned int Xidx = matrixEntry.queue.at(0);
                unsigned int Yidx = matrixEntry.queue.at(1);

                if (Xidx != UINT32_MAX)
                    m_cluster_manager.returnClusterToStack(Xidx);
                if (Yidx != UINT32_MAX)
                    m_cluster_manager.returnClusterToStack(Yidx);
            }
        }

        previous.at(0) = UINT32_MAX;
        previous.at(1) = UINT32_MAX;

        unsigned int currentXidx = previous.at(0);
        unsigned int currentYidx = previous.at(1);

        for (unsigned int ix = 0; ix < m_nXcurrent; ix++) {
            for (unsigned int iy = 0; iy < m_nYcurrent; iy++) {

                nmx::QmatrixEntry &matrixEntry = m_Qmatrix.at(ix).at(iy);
                unsigned int Xidx = matrixEntry.queue.at(0);
                unsigned int Yidx = matrixEntry.queue.at(1);

                if (matrixEntry.Qval < nmx::DELTA_Q) {
                    if (previous.at(0) == UINT32_MAX) {
                        previous.at(0) = Xidx;
                        previous.at(1) = Yidx;
                        currentXidx = previous.at(0);
                        currentYidx = previous.at(1);
                    } else  {
                        m_cluster_manager.getCluster(currentXidx).box.link1 = Xidx;
                        currentXidx = Xidx;
                        m_cluster_manager.getCluster(currentYidx).box.link1 = Yidx;
                        currentYidx = Yidx;
                    }
                } else {
                    m_cluster_manager.returnClusterToStack(Xidx);
                    m_cluster_manager.returnClusterToStack(Yidx);
                }
            }
        }

        finder.find(pairbuf);
        pairbuf.npairs = 0;

        if (m_terminate)
            return;
    }
}

void NMXClusterPairing::calculateQmatrix(nmx::cluster_queue &current, nmx::cluster_queue &previous) {

    m_nXcurrent = getQueueLength(current.at(0));
    m_nYcurrent = getQueueLength(current.at(1));
    m_nXprevious = getQueueLength(previous.at(0));
    m_nYprevious = getQueueLength(previous.at(1));

    int Xidx = (current.at(0) >= 0 ? current.at(0) : previous.at(0));
    int Yidx = (current.at(1) >= 0 ? current.at(1) : previous.at(1));

    for (int ix = 0; ix < m_nXcurrent + m_nXprevious; ix++) {

        nmx::cluster &Xcluster = m_cluster_manager.getCluster(Xidx);
        double xCharge = static_cast<double>(Xcluster.box.chargesum);

        for (int iy = 0; iy < m_nYcurrent + m_nYprevious; iy++) {

            nmx::cluster &Ycluster = m_cluster_manager.getCluster(Yidx);
            double yCharge = static_cast<double>(Ycluster.box.chargesum);

            nmx::QmatrixEntry &Qentry = m_Qmatrix.at(ix).at(iy);

            Qentry.Qval = 2 * std::abs(xCharge - yCharge) / (xCharge + yCharge);
            Qentry.queue.at(0) = Xidx;
            Qentry.queue.at(1) = Yidx;

            if (iy == m_nYcurrent)
                Yidx = previous.at(1);
            else
                Yidx = Ycluster.box.link1;
        }

        if (ix == m_nXcurrent)
            Xidx = previous.at(0);
        else
            Xidx = Xcluster.box.link1;

        Yidx = (current.at(1) >= 0 ? current.at(1) : previous.at(1));
    }
}

unsigned int NMXClusterPairing::getQueueLength(unsigned int idx) {

    unsigned int length = 0;

    while (idx >= 0) {

        idx = m_cluster_manager.getCluster(idx).box.link1;

        length++;
    }

    return length;
}

void NMXClusterPairing::reset(uint n) {

    for (uint i = 0; i < n; i++)
        m_used.at(i) = false;
}




