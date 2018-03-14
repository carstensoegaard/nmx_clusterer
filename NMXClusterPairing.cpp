//
// Created by soegaard on 1/31/18.
//

#include <iomanip>
#include "NMXLocationFinder.h"

NMXClusterPairing::NMXClusterPairing(NMXClusterManager &clusterManager)
        : m_nInX(0),
          m_nOutX(0),
          m_nInY(0),
          m_nOutY(0),
          m_nXcurrent(0),
          m_nYcurrent(0),
          m_nXprevious(0),
          m_nYprevious(0),
          m_switch(true),
          m_terminate(false),
          m_cluster_manager(clusterManager),
          m_time_ordered_buffer(clusterManager, this)
{
    m_time_ordered_buffer.setVerboseLevel(0);

    m_tinsert = std::thread(&NMXClusterPairing::insert, this);
    m_tprocess = std::thread(&NMXClusterPairing::process, this);
}

NMXClusterPairing::~NMXClusterPairing() {

    m_tinsert.join();
    m_tprocess.join();
}

void NMXClusterPairing::insertClusterInQueue(int plane, unsigned int idx) {

    if (m_cluster_manager.getCluster(plane, idx).npoints == 0)
        std::cout << "<NMXClusterPairing::insertClusterInQueue> For some perculiar reason this cluster is empty!\n";

    nmx::cluster &cluster = m_cluster_manager.getCluster(plane, idx);
    if ((cluster.box.link1 != -1) || (cluster.box.link2 != -1))
        std::cout << "<NMXClusterPairing::insertClusterInQueue> Cluster " << idx << " from plane " << plane
                  << " Link1 = " << cluster.box.link1 << ", link2 = " << cluster.box.link2 << std::endl;

    switch (plane) {

        case 0:
            insertClusterInQueueX(idx);
            break;
        case 1:
            insertClusterInQueueY(idx);
            break;
        default:
        std::cerr << "<NMXClusterPairing::insertClusterInQueue> Plane " << plane << " has no meaning!\n";
    }
}

void NMXClusterPairing::insertClusterInQueueX(unsigned int cluster_idx) {

    if (cluster_idx > nmx::NCLUSTERS) {
        std::cout << "<NMXClusterPairing::insertClusterInQueueX> Index " << cluster_idx << " out of range!\n";
        throw 1;
    }

  /*  std::cout << "Will attempt to insert cluster " << cluster_idx << std::endl;
    std::cout << "Before: InX = " << m_nInX << ", OutX = " << m_nOutX << std::endl;
    std::cout << "Diff = " << m_nInX - m_nOutX << " to large ? "
              << ((m_nInX - m_nOutX > nmx::CLUSTER_BUFFER_SIZE) ? "Yes\n" : "No\n");*/
    while (m_nInX - m_nOutX > nmx::CLUSTER_BUFFER_SIZE) {
        std::this_thread::yield();
    }

    unsigned int buffer_idx = m_nInX % nmx::CLUSTER_BUFFER_SIZE;
    m_xQueue.at(buffer_idx) = cluster_idx;
    m_nInX++;

    /*
    std::cout << "<NMXClusterPairing::insertClusterInQueueX> Inserted cluster " << cluster_idx << " at " << buffer_idx
              << std::endl;
    std::cout << "After: InX = " << m_nInX << ", OutX = " << m_nOutX << std::endl;
*/
}

void NMXClusterPairing::insertClusterInQueueY(unsigned int cluster_idx) {

    if (cluster_idx > nmx::NCLUSTERS) {
        std::cout << "<NMXClusterPairing::insertClusterInQueueY> Index " << cluster_idx << " out of range!\n";
        throw 1;
    }

    while (m_nInY - m_nOutY > nmx::CLUSTER_BUFFER_SIZE)
        std::this_thread::yield();

    unsigned int buffer_idx = m_nInY % nmx::CLUSTER_BUFFER_SIZE;
    m_yQueue.at(buffer_idx) = cluster_idx;
    m_nInY++;
}

void NMXClusterPairing::insert() {

    while (1) {

        if ((m_nInX == m_nOutX) && (m_nInY == m_nOutY))
            std::this_thread::yield();

        if (m_switch) {
            if (m_nInX > m_nOutX) {
                unsigned int buffer_idx = m_nOutX % nmx::CLUSTER_BUFFER_SIZE;
                unsigned int cluster_idx = m_xQueue.at(buffer_idx);
                uint32_t time = m_cluster_manager.getCluster(0, cluster_idx).box.max_time;

                m_time_ordered_buffer.insert(0, cluster_idx, time);
                m_nOutX++;
                /*std::cout << "<NMXClusterPairing::insert> Inserted X cluster " << cluster_idx << " from " << buffer_idx
                          << std::endl;*/
            }
        } else {
            if (m_nInY > m_nOutY) {
                unsigned int buffer_idx = m_nOutY % nmx::CLUSTER_BUFFER_SIZE;
                unsigned int cluster_idx = m_yQueue.at(buffer_idx);
                uint32_t time = m_cluster_manager.getCluster(1, cluster_idx).box.max_time;

                m_time_ordered_buffer.insert(1, cluster_idx, time);
                m_nOutY++;
                /*std::cout << "<NMXClusterPairing::insert> Inserted Y cluster " << cluster_idx << " from " << buffer_idx
                          << std::endl;*/
            }
        }

        m_switch = !m_switch;

        if (m_terminate)
            return;
    }
}

void NMXClusterPairing::process() {

    NMXLocationFinder finder(m_cluster_manager);

    nmx::cluster_queue previous;
    previous.at(0) = -1;
    previous.at(1) = -1;

    nmx::pairBuffer pairbuf;
    pairbuf.npairs = 0;

    while (1) {

        //std::cout << "Getting next sorted ..." << std::flush;
        nmx::cluster_queue current = m_time_ordered_buffer.getNextSorted();
        if ((current.at(0) == -1) && (current.at(1) == -1))
            continue;
        //std::cout << " Got it!\n";

        std::cout << "Current x-";
        nmx::printQueue(0, current.at(0), m_cluster_manager);
        std::cout << "Current y-";
        nmx::printQueue(1, current.at(1), m_cluster_manager);

        calculateQmatrix(current, previous);

        std::cout << "Calculated the Qmatrix dim[ " << m_nXcurrent + m_nXprevious << " , " << m_nYcurrent + m_nYprevious
                  << " ]\n";

        int idx = previous.at(0);
        std::cout << "<NMXClusterPairing::process> Returning old clusters from X queue. Starting at " << idx << std::endl;
        while (idx >= 0) {
            //std::cout << "Getting next idx\n";
            int next_idx = m_cluster_manager.getCluster(0, idx).box.link1;
            //std::cout << "Next idx = " << next_idx << std::endl;
            //std::cout << "Returning cluster " << idx << std::endl;
            m_cluster_manager.returnClusterToStack(0, idx);
            idx = next_idx;
        }

        idx = previous.at(0);
        std::cout << "<NMXClusterPairing::process> Returning old clusters from Y queue. Starting at " << idx << std::endl;
        while (idx >= 0) {
            //std::cout << "Getting next idx\n";
            int next_idx = m_cluster_manager.getCluster(1, idx).box.link1;
            //std::cout << "Next idx = " << next_idx << std::endl;
            //std::cout << "Returning cluster " << idx << std::endl;
            m_cluster_manager.returnClusterToStack(1, idx);
            idx = next_idx;
        }

        idx = current.at(0);
        std::cout << "<NMXClusterPairing::process> Returning clusters from X queue. Starting at " << idx << std::endl;
        while (idx >= 0) {
            //std::cout << "Getting next idx\n";
            int next_idx = m_cluster_manager.getCluster(0, idx).box.link1;
            //std::cout << "Next idx = " << next_idx << std::endl;
            //std::cout << "Returning cluster " << idx << std::endl;
            m_cluster_manager.returnClusterToStack(0, idx);
            idx = next_idx;
        }

        idx = current.at(1);
        std::cout << "<NMXClusterPairing::process> Returning clusters from Y queue. Starting at " << idx << std::endl;
        while (idx >= 0) {
            //std::cout << "Getting next idx\n";
            int next_idx = m_cluster_manager.getCluster(1, idx).box.link1;
            //std::cout << "Next idx = " << next_idx << std::endl;
            //std::cout << "Returning cluster " << idx << std::endl;
            m_cluster_manager.returnClusterToStack(1, idx);
            idx = next_idx;
        }

        if (m_terminate)
            return;
    }
}
        /*
        bool cont = true;

        if ((m_nXcurrent+m_nXprevious == 0) || (m_nYcurrent+m_nYprevious == 0))
            cont = false;

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
                    std::cout << "Q = " << Q << ", minQ = " << minQ << std::endl;
                    if (Q < minQ) {
                        minQ = Q;
                        x = ix;
                        y = iy;
                        pair.x_idx = m_Qmatrix.at(ix).at(iy).queue.at(0);
                        pair.y_idx = m_Qmatrix.at(ix).at(iy).queue.at(1);
                    }
                }
            }

            std::cout << "Examining ( " << x << " , " << y << " )\n";
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
                        std::cout << "<NMXClusterParing::process> Attempting to get cluster " << currentXidx << std::endl;
                        m_cluster_manager.getCluster(currentXidx).box.link1 = Xidx;
                        currentXidx = Xidx;
                        std::cout << "<NMXClusterParing::process> Attempting to get cluster " << currentYidx << std::endl;
                        m_cluster_manager.getCluster(currentYidx).box.link1 = Yidx;
                        currentYidx = Yidx;
                    }
                } else {
                    m_cluster_manager.returnClusterToStack(Xidx);
                    m_cluster_manager.returnClusterToStack(Yidx);
                }
            }
        }
*/
  //      finder.find(pairbuf);
    //    pairbuf.npairs = 0;
/*

        if (m_terminate)
            return;
    }
}*/

void NMXClusterPairing::calculateQmatrix(nmx::cluster_queue &current, nmx::cluster_queue &previous) {

    m_nXcurrent = getQueueLength(0, current.at(0));
    m_nYcurrent = getQueueLength(1, current.at(1));
    m_nXprevious = getQueueLength(0, previous.at(0));
    m_nYprevious = getQueueLength(1, previous.at(1));

    int Xidx = (current.at(0) >= 0 ? current.at(0) : previous.at(0));
    int Yidx = (current.at(1) >= 0 ? current.at(1) : previous.at(1));

    for (int ix = 0; ix < m_nXcurrent + m_nXprevious; ix++) {

        std::cout << "<NMXClusterParing::calculateQmatrix> Attempting to get cluster " << Xidx << std::endl;

        nmx::cluster &Xcluster = m_cluster_manager.getCluster(0, Xidx);
        double xCharge = static_cast<double>(Xcluster.box.chargesum);

        for (int iy = 0; iy < m_nYcurrent + m_nYprevious; iy++) {

            std::cout << "<NMXClusterParing::calculateQmatrix> Attempting to get cluster " << Yidx << std::endl;

            nmx::cluster &Ycluster = m_cluster_manager.getCluster(1, Yidx);
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

unsigned int NMXClusterPairing::getQueueLength(unsigned int plane, int idx) {

    unsigned int length = 0;

    while (idx >= 0) {

        //std::cout << "<NMXClusterPairing::getQueueLength> Progressing to cluster " << idx << std::endl;
        idx = m_cluster_manager.getCluster(plane, idx).box.link1;

        length++;
    }

    return length;
}
/*
void NMXClusterPairing::reset(uint n) {

    for (uint i = 0; i < n; i++)
        m_used.at(i) = false;
}*/




