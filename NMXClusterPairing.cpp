//
// Created by soegaard on 1/31/18.
//

#include <iostream>
#include <iomanip>

#include "NMXLocationFinder.h"
#include "NMXClustererHelper.h"

NMXClusterPairing::NMXClusterPairing(NMXClusterManager &clusterManager)
        : m_i1(nmx::MAX_MINOR),
          m_nXcurrent(0),
          m_nYcurrent(0),
          m_nXnext(0),
          m_nYnext(0),
          m_verbose_level(2),
          m_terminate(false),
          m_clusterManager(clusterManager)
{
    reset();

    t_process = std::thread(&NMXClusterPairing::process, this);
   // m_tprocess = std::thread(&NMXClusterPairing::process, this);
}

NMXClusterPairing::~NMXClusterPairing() {

    t_process.join();
    //m_tprocess.join();
}

void NMXClusterPairing::insertClusterInQueue(int plane, unsigned int cluster_idx) {

    if (m_clusterManager.getCluster(plane, cluster_idx).npoints == 0)
        std::cout << "<NMXClusterPairing::insertClusterInQueue> For some perculiar reason this cluster is empty!\n";

    nmx::cluster &cluster = m_clusterManager.getCluster(plane, cluster_idx);
    if ((cluster.box.link1 != -1) || (cluster.box.link2 != -1))
        std::cout << "<NMXClusterPairing::insertClusterInQueue> Cluster " << cluster_idx << " from plane " << plane
                  << " Link1 = " << cluster.box.link1 << ", link2 = " << cluster.box.link2 << std::endl;

    if (cluster_idx > nmx::NCLUSTERS) {
        std::cout << "<NMXClusterPairing::insertClusterInQueueX> Index " << cluster_idx << " out of range!\n";
        throw 1;
    }

    while (m_nIn.at(plane) - m_nOut.at(plane) > nmx::CLUSTER_BUFFER_SIZE-1)
        std::this_thread::yield();

    m_mutex.lock();
    unsigned int buffer_idx = m_nIn.at(plane) % nmx::CLUSTER_BUFFER_SIZE;
    m_queue.at(plane).at(buffer_idx) = cluster_idx;
    std::cout << "<NMXClusterPairing::insertClusterInQueue> Inserted index " << cluster_idx << " at " << buffer_idx
              << " in plane " << plane << std::endl;
    m_nIn.at(plane)++;
    m_mutex.unlock();
}

void NMXClusterPairing::process() {

    unsigned int plane = 0;

    while (1) {

        while ((m_nIn.at(0) == m_nOut.at(0)) && (m_nIn.at(1) == m_nOut.at(1)))
            std::this_thread::yield();

        if (m_nIn.at(plane) > m_nOut.at(plane)) {

            unsigned int buffer_idx = m_nOut.at(plane) % nmx::CLUSTER_BUFFER_SIZE;
            unsigned int cluster_idx = m_queue.at(plane).at(buffer_idx);
            uint32_t time = m_clusterManager.getCluster(plane, cluster_idx).box.max_time;
            m_nOut.at(plane)++;

            uint32_t minorTime = getMinorTime(time);
            uint32_t majorTime = getMajorTime(time);

            std::cout << "<NMXClusterPairing::insert> Got idx " << cluster_idx << " from " << buffer_idx << std::endl;
            /*std::cout << "Time = " << time << ", B1 = " << minorTime << ", B2 = " << majorTime << ", B2_buffer["
                          << minorTime << "] = " << m_majortime_buffer[minorTime] << " B2_buffer[0] = "
                          << m_majortime_buffer.at(0) << " i1 = " << m_i1 << std::endl;*/

            if (majorTime >= (m_majortime_buffer.at(0) + 1)) {

                if (majorTime == (m_majortime_buffer.at(0) + 1)) {

                    if (m_verbose_level > 0)
                        std::cout << "Case 1\n";

                    slideTimeWindow(nmx::MAX_MINOR - m_i1 + std::min(m_i1, minorTime), minorTime, majorTime);
                    addToBuffer(plane, cluster_idx, minorTime);

                } else { // majorTime > (m_majortime_buffer.at(0) + 1)

                    if (m_verbose_level > 0)
                        std::cout << "Case 2\n";

                    slideTimeWindow(nmx::MAX_MINOR, minorTime, majorTime);
                    addToBuffer(plane, cluster_idx, minorTime);
                }

            } else { // majorTime <= m_buffer.at(0)

                switch (majorTime - m_majortime_buffer.at(minorTime)) {

                    case 1:

                        if (m_verbose_level > 0)
                            std::cout << "Case 3\n";

                        slideTimeWindow(minorTime - m_i1, minorTime, majorTime);
                        addToBuffer(plane, cluster_idx, minorTime);

                        break;

                    case 0:

                        if (m_verbose_level > 0)
                            std::cout << "Case 4\n";

                        addToBuffer(plane, cluster_idx, minorTime);

                        break;

                    default:

                        std::cout << "Old data-point - omitting!\n";
                        m_clusterManager.returnClusterToStack(plane, cluster_idx);
                }
            }
        }

        plane = !plane;

        if (m_terminate)
            return;
    }
}

void NMXClusterPairing::addToBuffer(unsigned int plane, int idx, uint minorTime) {

    if (m_verbose_level > 1)
        std::cout << "<NMXClusterPairing::addToBuffer> Adding cluster " << idx << " to " << (plane ? "Y" : "X")
                  << " at idx " << minorTime << std::endl;

    nmx::cluster_queue &queue = m_time_ordered_buffer.at(minorTime);

    if (m_verbose_level > 2) {
        std::cout << "Before:\n";
        nmx::printQueue(0, queue.at(0), m_clusterManager);
        nmx::printQueue(1, queue.at(1), m_clusterManager);
    }

    int queueIdx = queue.at(plane);

    if (queueIdx == -1) {
        queue.at(plane) = idx;
        if (m_verbose_level > 2)
            std::cout << "First cluster!\n";
    } else {

        if (m_verbose_level > 2)
            std::cout << "Propagating queue : " << queueIdx;

        while (true) {

            nmx::cluster &nextCluster = m_clusterManager.getCluster(plane, queueIdx);

            queueIdx = nextCluster.box.link1;

            if (m_verbose_level > 2)
                std::cout << " -> " << queueIdx;

            if (queueIdx == -1) {
                nextCluster.box.link1 = idx;
                break;
            }
        }
        if (m_verbose_level > 2)
            std::cout << "\n";
    }

    if (m_verbose_level > 1) {
        std::cout << "After:\n";
        nmx::printQueue(0, queue.at(0), m_clusterManager);
        nmx::printQueue(1, queue.at(1), m_clusterManager);
    }
}

void NMXClusterPairing::slideTimeWindow(uint d, uint minorTime, uint majorTime) {

    if (m_verbose_level > 1) {
        std::string s("\nWill move ");
        s.append(std::to_string(d));
        s.append(" entries to clusterer !\n");
        std::cout << s;
    }

    for (uint i = 0; i < d; ++i) {

        unsigned int this_idx = (i+m_i1+1)%nmx::MAX_MINOR;
        unsigned int next_idx = (i+1+m_i1+1)%nmx::MAX_MINOR;

        nmx::cluster_queue &this_queue = m_time_ordered_buffer.at(this_idx);
        nmx::cluster_queue &next_queue = m_time_ordered_buffer.at(next_idx);

        if ((this_queue.at(0) != -1) ||
                (this_queue.at(1) != -1) ||
                (next_queue.at(0) != -1) ||
                (next_queue.at(1) != -1)) {

            if (m_verbose_level > 2) {
                std::cout << "i = " << i << ", this_idx = " << this_idx << ", next_idx = " << next_idx << std::endl;
                std::cout << "This x-";
                nmx::printQueue(0, this_queue.at(0), m_clusterManager);
                std::cout << "This y-";
                nmx::printQueue(1, this_queue.at(1), m_clusterManager);
                std::cout << "Next x-";
                nmx::printQueue(0, next_queue.at(0), m_clusterManager);
                std::cout << "Next y-";
                nmx::printQueue(1, next_queue.at(1), m_clusterManager);
            }

            calculateQmatrix(this_queue, next_queue);
            if (m_verbose_level > 1)
                std::cout << "Calculated the Qmatrix dim[ " << m_nXcurrent + m_nXnext << " , " << m_nYcurrent + m_nYnext
                          << " ]\n";

            returnQueueToStack(0, this_queue.at(0));
            returnQueueToStack(1, this_queue.at(1));
        }

        this_queue.at(0) = -1;
        this_queue.at(1) = -1;

        if (this_idx <= minorTime)
            m_majortime_buffer.at(this_idx) = majorTime;
        else
            m_majortime_buffer.at(this_idx) = majorTime -1;
    }

    if (m_verbose_level > 2) {
        std::cout << "Setting i1 to " << minorTime << std::endl;
        nmx::printMajorTimeBuffer(m_majortime_buffer);
    }

    m_i1 = minorTime;

    /*
    m_mutex.lock();
    m_clusterManager.printStack(0);
    m_clusterManager.printStack(1);
    printSortBuffer();
    printQueue();
    m_mutex.unlock();*/
}


inline uint32_t NMXClusterPairing::getMinorTime(uint32_t time) {

    time = time >> nmx::IGNORE_BITS;
    time = time & nmx::MINOR_BITMASK;

    return time;
}

inline uint32_t NMXClusterPairing::getMajorTime(uint32_t time) {

    return time >> nmx::IGNORE_BITS >> nmx::MINOR_BITS;
}
/*
void NMXClusterPairing::process() {

    NMXLocationFinder finder(m_clusterManager);

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
        nmx::printQueue(0, current.at(0), m_clusterManager);
        std::cout << "Current y-";
        nmx::printQueue(1, current.at(1), m_clusterManager);

        calculateQmatrix(current, previous);

        std::cout << "Calculated the Qmatrix dim[ " << m_nXcurrent + m_nXprevious << " , " << m_nYcurrent + m_nYprevious
                  << " ]\n";

        int idx = previous.at(0);
        std::cout << "<NMXClusterPairing::process> Returning old clusters from X queue. Starting at " << idx << std::endl;
        while (idx >= 0) {
            //std::cout << "Getting next idx\n";
            int next_idx = m_clusterManager.getCluster(0, idx).box.link1;
            //std::cout << "Next idx = " << next_idx << std::endl;
            //std::cout << "Returning cluster " << idx << std::endl;
            m_clusterManager.returnClusterToStack(0, idx);
            idx = next_idx;
        }

        idx = previous.at(0);
        std::cout << "<NMXClusterPairing::process> Returning old clusters from Y queue. Starting at " << idx << std::endl;
        while (idx >= 0) {
            //std::cout << "Getting next idx\n";
            int next_idx = m_clusterManager.getCluster(1, idx).box.link1;
            //std::cout << "Next idx = " << next_idx << std::endl;
            //std::cout << "Returning cluster " << idx << std::endl;
            m_clusterManager.returnClusterToStack(1, idx);
            idx = next_idx;
        }

        idx = current.at(0);
        std::cout << "<NMXClusterPairing::process> Returning clusters from X queue. Starting at " << idx << std::endl;
        while (idx >= 0) {
            //std::cout << "Getting next idx\n";
            int next_idx = m_clusterManager.getCluster(0, idx).box.link1;
            //std::cout << "Next idx = " << next_idx << std::endl;
            //std::cout << "Returning cluster " << idx << std::endl;
            m_clusterManager.returnClusterToStack(0, idx);
            idx = next_idx;
        }

        idx = current.at(1);
        std::cout << "<NMXClusterPairing::process> Returning clusters from Y queue. Starting at " << idx << std::endl;
        while (idx >= 0) {
            //std::cout << "Getting next idx\n";
            int next_idx = m_clusterManager.getCluster(1, idx).box.link1;
            //std::cout << "Next idx = " << next_idx << std::endl;
            //std::cout << "Returning cluster " << idx << std::endl;
            m_clusterManager.returnClusterToStack(1, idx);
            idx = next_idx;
        }

        if (m_terminate)
            return;
    }
}*/
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

void NMXClusterPairing::calculateQmatrix(nmx::cluster_queue &this_queue, nmx::cluster_queue &next_queue) {

    /*
    std::cout << "<NMXClusterPairing::calculateQmatix> \n";
    std::cout << "This-queue[" << this_queue.at(0) << ", " << this_queue.at(1) << "]\n";
    std::cout << "Next-queue[" << next_queue.at(0) << ", " << next_queue.at(1) << "]\n";
*/
    m_nXcurrent = getQueueLength(0, this_queue.at(0));
    m_nYcurrent = getQueueLength(1, this_queue.at(1));
    m_nXnext = getQueueLength(0, next_queue.at(0));
    m_nYnext = getQueueLength(1, next_queue.at(1));
/*
    std::cout << "This queue length X = " << m_nXcurrent << std::endl;
    std::cout << "This queue length Y = " << m_nYcurrent << std::endl;
    std::cout << "Next queue length X = " << m_nXnext << std::endl;
    std::cout << "Next queue length Y = " << m_nYnext << std::endl;
*/
    int Xidx = (this_queue.at(0) >= 0 ? this_queue.at(0) : next_queue.at(0));
    int Yidx = (this_queue.at(1) >= 0 ? this_queue.at(1) : next_queue.at(1));

    for (int ix = 0; ix < m_nXcurrent + m_nXnext; ix++) {

  //      std::cout << "<NMXClusterParing::calculateQmatrix> Attempting to get cluster " << Xidx << std::endl;

        nmx::cluster &Xcluster = m_clusterManager.getCluster(0, Xidx);
        double xCharge = static_cast<double>(Xcluster.box.chargesum);

        for (int iy = 0; iy < m_nYcurrent + m_nYnext; iy++) {

    //        std::cout << "<NMXClusterParing::calculateQmatrix> Attempting to get cluster " << Yidx << std::endl;

            nmx::cluster &Ycluster = m_clusterManager.getCluster(1, Yidx);
            double yCharge = static_cast<double>(Ycluster.box.chargesum);

            nmx::QmatrixEntry &Qentry = m_Qmatrix.at(ix).at(iy);

            Qentry.Qval = 2 * std::abs(xCharge - yCharge) / (xCharge + yCharge);
            Qentry.queue.at(0) = Xidx;
            Qentry.queue.at(1) = Yidx;

            if (iy == m_nYcurrent-1)
                Yidx = next_queue.at(1);
            else
                Yidx = Ycluster.box.link1;
        }

        if (ix == m_nXcurrent-1)
            Xidx = next_queue.at(0);
        else
            Xidx = Xcluster.box.link1;

        Yidx = (this_queue.at(1) >= 0 ? this_queue.at(1) : next_queue.at(1));
    }
}

unsigned int NMXClusterPairing::getQueueLength(unsigned int plane, int idx) {

    unsigned int length = 0;

    while (idx >= 0) {

        //std::cout << "<NMXClusterPairing::getQueueLength> Progressing to cluster " << idx << std::endl;
        idx = m_clusterManager.getCluster(plane, idx).box.link1;

        length++;
    }

    return length;
}

void NMXClusterPairing::reset() {

    for (unsigned int idx = 0; idx < nmx::MAX_MINOR; idx++) {

        m_time_ordered_buffer.at(idx).at(0) = -1;
        m_time_ordered_buffer.at(idx).at(1) = -1;

        m_majortime_buffer.at(idx) = 0;
    }

}

void NMXClusterPairing::returnQueueToStack(int plane, int idx) {

    if (idx == -1)
        return;

    /*std::cout << "<NMXClusterPairing::returnQueueToStack> Returning old clusters from "
              << (plane ? "Y" : "X") << " queue. Starting at " << idx << std::endl;*/
    while (idx >= 0) {
      //  std::cout << "Getting next idx\n";
        int nextIdx = m_clusterManager.getCluster(plane, idx).box.link1;
       // std::cout << "Next idx = " << nextIdx << std::endl;
       // std::cout << "Returning cluster " << idx << std::endl;
        m_clusterManager.returnClusterToStack(plane, idx);
        idx = nextIdx;
    }
}

void NMXClusterPairing::printSortBuffer() {

    std::cout << "Time ordered buffer:\n";

    for (int idx = 0; idx < nmx::MAX_MINOR; idx++) {

        int xQueue = m_time_ordered_buffer.at(idx).at(0);
        int yQueue = m_time_ordered_buffer.at(idx).at(1);

        if (xQueue >= 0 || yQueue >= 0)
            std::cout << "Index = " << idx << std::endl;

        if (xQueue >= 0) {
            std::cout << "X ";
            nmx::printQueue(0, xQueue, m_clusterManager);
        }
        if (yQueue >= 0) {
            std::cout << "Y ";
            nmx::printQueue(1, yQueue, m_clusterManager);
        }
    }
}

void NMXClusterPairing::printQueue() {

    std::cout << "Buffer-queue X:\n";
    for (int i = m_nOut.at(0)%nmx::NCLUSTERS; i < m_nIn.at(0)%nmx::NCLUSTERS; i++ )
        std::cout << m_queue.at(0).at(i) << " ";
    std::cout << "\n";

    std::cout << "Buffer-queue Y:\n";
    for (int i = m_nOut.at(1)%nmx::NCLUSTERS; i < m_nIn.at(1)%nmx::NCLUSTERS; i++ )
        std::cout << m_queue.at(1).at(i) << " ";
    std::cout << "\n";
}