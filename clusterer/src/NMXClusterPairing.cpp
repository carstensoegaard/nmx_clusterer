//
// Created by soegaard on 1/31/18.
//

#include <iostream>
#include <iomanip>
#include <algorithm>

#include "../include/NMXLocationFinder.h"
#include "../../helper/include/NMXClustererHelper.h"
#include "../include/NMXClusterPairing.h"

NMXClusterPairing::NMXClusterPairing(NMXClusterManager &clusterManager)
        : m_i1(nmx::CLUSTER_MINOR_BITMASK),
          m_nXthis(0),
          m_nYthis(0),
          m_nXnext(0),
          m_nYnext(0),
          m_verbose_level(0),
          m_terminate(false),
          m_clusterManager(clusterManager),
          m_locationFinder(clusterManager)
{
    reset();
    m_Qmatrix.reset();

    t_process = std::thread(&NMXClusterPairing::process, this);
}

NMXClusterPairing::~NMXClusterPairing() {

    t_process.join();
}

void NMXClusterPairing::insertClusterInQueue(int plane, unsigned int cluster_idx) {

    nmx::Cluster &cluster = m_clusterManager.getCluster(plane, cluster_idx);
    if ((cluster.box.link1 != -1) || (cluster.box.link2 != -1))
        std::cout << "<NMXClusterPairing::insertClusterInQueue> Cluster " << cluster_idx << " from plane " << plane
                  << " Link1 = " << cluster.box.link1 << ", link2 = " << cluster.box.link2 << std::endl;

    if (cluster_idx > nmx::NCLUSTERS) {
        std::cout << "<NMXClusterPairing::insertClusterInQueue> Index " << cluster_idx << " out of range!\n";
        throw 1;
    }

    while (m_nIn.at(plane) - m_nOut.at(plane) > nmx::NCLUSTERS-1)
        std::this_thread::yield();

    m_mutex.lock();

    if (m_verbose_level > 2) {
        std::cout << "<NMXClusterPairing::insertClusterInQueue> Queue before:\n";
        printQueue();
    }

    unsigned int buffer_idx = m_nIn.at(plane) % nmx::NCLUSTERS;
    m_queue.at(plane).at(buffer_idx) = cluster_idx;
    m_nIn.at(plane)++;

    if (m_verbose_level > 2) {
        std::cout << "<NMXClusterPairing::insertClusterInQueue> Queue after:\n";
        printQueue();
    }

    m_mutex.unlock();
    if (m_verbose_level > 1)
        std::cout << "<NMXClusterPairing::insertClusterInQueue> Inserted index " << cluster_idx << " at " << buffer_idx
                  << " in plane " << plane << std::endl;
}

void NMXClusterPairing::process() {

    unsigned int plane = 0;

    while (1) {

        while ((m_nIn.at(0) == m_nOut.at(0)) && (m_nIn.at(1) == m_nOut.at(1))) {
            if (m_terminate)
                return;
            std::this_thread::yield();
        }

        if (m_nIn.at(plane) > m_nOut.at(plane)) {

            checkSortBuffer();

            unsigned int buffer_idx = m_nOut.at(plane) % nmx::NCLUSTERS;
            unsigned int cluster_idx = m_queue.at(plane).at(buffer_idx);
            uint32_t time = m_clusterManager.getCluster(plane, cluster_idx).box.max_time;
            m_nOut.at(plane)++;

            uint32_t minorTime = getMinorTime(time);
            uint32_t majorTime = getMajorTime(time);

            if (m_verbose_level > 1) {
                std::cout << "<NMXClusterPairing::insert> Got idx " << cluster_idx << " from " << buffer_idx
                          << std::endl;
                std::cout << "Time = " << time << ", B1 = " << minorTime << ", B2 = " << majorTime << std::endl;
                std::cout << "B2_buffer[" << minorTime << "] = " << m_majortime_buffer[minorTime] << " B2_buffer[0] = "
                          << m_majortime_buffer.at(0) << " i1 = " << m_i1 << std::endl;
            }

            if (majorTime >= (m_majortime_buffer.at(0) + 1)) {

                if (majorTime == (m_majortime_buffer.at(0) + 1)) {

                    if (m_verbose_level > 1)
                        std::cout << "Case 1\n";

                    slideTimeWindow(nmx::CLUSTER_MAX_MINOR - m_i1 + std::min(m_i1, minorTime), minorTime, majorTime);
                    addToBuffer(plane, cluster_idx, minorTime);

                } else { // majorTime > (m_majortimeBuffer.at(0) + 1)

                    if (m_verbose_level > 1)
                        std::cout << "Case 2\n";

                    slideTimeWindow(nmx::CLUSTER_MAX_MINOR, minorTime, majorTime);
                    addToBuffer(plane, cluster_idx, minorTime);
                }

            } else { // majorTime <= m_buffer.at(0)

                switch (majorTime - m_majortime_buffer.at(minorTime)) {

                    case 1:

                        if (m_verbose_level > 1)
                            std::cout << "Case 3\n";

                        slideTimeWindow(minorTime - m_i1, minorTime, majorTime);
                        addToBuffer(plane, cluster_idx, minorTime);

                        break;

                    case 0:

                        if (m_verbose_level > 1)
                            std::cout << "Case 4\n";

                        addToBuffer(plane, cluster_idx, minorTime);

                        break;

                    default:

                        if (m_verbose_level > 0)
                            std::cout << "Cluster arrived to late - omitting!\n";
                        m_clusterManager.returnClusterToStack(plane, cluster_idx);
                        m_nLateClusters++;
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
        std::cout << "<NMXClusterPairing::addToBuffer> Adding Cluster " << idx << " to " << (plane ? "Y" : "X")
                  << " at idx " << minorTime << std::endl;

    nmx::ClusterParingEntry &entry = m_time_ordered_buffer.at(minorTime);

    if (m_verbose_level > 2) {
        std::cout << "Before:\n";
        nmx::printQueue(0, entry.queue.at(0), m_clusterManager);
        nmx::printQueue(1, entry.queue.at(1), m_clusterManager);
    }

    if (entry.queueLength.at(plane) == 0) {
        if (entry.queue.at(plane) != -1) {
            std::cerr << "<NMXClusterPairing::addToBuffer> Plane = " << plane << ", idx = " << idx << ", minortime = "
                      << minorTime << std::endl;
            std::cerr << "                                 Entry length = 0, but queue start = "
                      << entry.queue.at(plane) << std::endl;
            throw 1;
        }
        entry.queue.at(plane) = idx;
        if (m_verbose_level > 2)
            std::cout << "First Cluster!\n";
    } else {

        int queueIdx = entry.queue.at(plane);

        if (m_verbose_level > 2)
            std::cout << "Propagating queue : " << queueIdx;

        while (true) {

            nmx::Cluster &nextCluster = m_clusterManager.getCluster(plane, queueIdx);

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

    entry.queueLength.at(plane)++;

    if (m_verbose_level > 1) {
        std::cout << "After:\n";
        nmx::printQueue(0, entry.queue.at(0), m_clusterManager);
        nmx::printQueue(1, entry.queue.at(1), m_clusterManager);
    }
}

void NMXClusterPairing::slideTimeWindow(uint d, uint minorTime, uint majorTime) {

    if (d > nmx::CLUSTER_MAX_MINOR) {
        std::cout << "d = " << d << " is too large!" << std::endl;
        nmx::printMajorTimeBuffer(m_majortime_buffer);
        std::cout << "i1 = " << m_i1 << std::endl;
        throw 1;
    }

    if (m_verbose_level > 1) {
        std::string s("\nWill move ");
        s.append(std::to_string(d));
        s.append(" entries to clusterer !\n");
        std::cout << s;
    }

    if (m_verbose_level > 2)
        std::cout << "Moving from idx " << (m_i1 + 1) % nmx::CLUSTER_MAX_MINOR << " to "
                  << (d + m_i1 + 1) % nmx::CLUSTER_MAX_MINOR
                  << std::endl;

    for (uint64_t i = 0; i < d; ++i) {

        uint64_t this_idx = (i + m_i1 + 1) % nmx::CLUSTER_MAX_MINOR;
        uint64_t next_idx = (this_idx + 1) % nmx::CLUSTER_MAX_MINOR;

        nmx::ClusterParingEntry &this_queue = m_time_ordered_buffer.at(this_idx);
        nmx::ClusterParingEntry &next_queue = m_time_ordered_buffer.at(next_idx);

        if (m_verbose_level > 2) {
            std::cout << "[" << this_idx << "]:\n";
            nmx::printQueue(0, this_queue.queue.at(0), m_clusterManager);
            nmx::printQueue(1, this_queue.queue.at(1), m_clusterManager);
            std::cout << "[" << next_idx << "]:\n";
            nmx::printQueue(0, next_queue.queue.at(0), m_clusterManager);
            nmx::printQueue(1, next_queue.queue.at(1), m_clusterManager);
        }

        m_nXthis = this_queue.queueLength.at(0);
        m_nYthis = this_queue.queueLength.at(1);
        m_nXnext = next_queue.queueLength.at(0);
        m_nYnext = next_queue.queueLength.at(1);

        unsigned int thisCase = (m_nXthis << 1) + m_nYthis;
        unsigned int nextCase = (m_nXnext << 1) + m_nYnext;

        if (m_verbose_level > 2) {
            std::cout << "nXthis = " << m_nXthis << ", nYthis = " << m_nYthis << std::endl;
            std::cout << "nXnext = " << m_nXnext << ", nYnext = " << m_nYnext << std::endl;
            std::cout << "thisCase = " << thisCase << ", nextCase = " << nextCase << std::endl;
        }

        if (m_nXthis + m_nXnext == 0 || m_nYthis + m_nYnext == 0) {

            returnQueueToStack(0, this_queue.queue.at(0));
            returnQueueToStack(1, this_queue.queue.at(1));

        } else if (m_nXthis < 2 && m_nXnext < 2 && m_nYthis < 2 && m_nYnext < 2) {

            unsigned int Case = (thisCase << 2) + nextCase;

            if (m_verbose_level > 2) {
                std::cout << "Case = " << Case << std::endl;
                nmx::printQueue(0, this_queue.queue.at(0), m_clusterManager);
                nmx::printQueue(1, this_queue.queue.at(1), m_clusterManager);
            }

            switch (Case) {

                case 4:
                    m_clusterManager.returnClusterToStack(1, this_queue.queue.at(1));
                    break;
                case 5:
                    m_clusterManager.returnClusterToStack(1, this_queue.queue.at(1));
                    break;
                case 6:
                    pairQueues(this_queue, next_queue);
                    break;
                case 7:
                    pairQueues(this_queue, next_queue);
                    break;
                case 8:
                    m_clusterManager.returnClusterToStack(0, this_queue.queue.at(0));
                    break;
                case 9:
                    pairQueues(this_queue, next_queue);
                    break;
                case 10:
                    m_clusterManager.returnClusterToStack(0, this_queue.queue.at(0));
                    break;
                case 11:
                    pairQueues(this_queue, next_queue);
                    break;
                case 12:
                    pairQueues(this_queue, next_queue);
                    break;
                case 13:
                    pairQueues(this_queue, next_queue);
                    break;
                case 14:
                    pairQueues(this_queue, next_queue);
                    break;
                case 15:
                    pairQueues(this_queue, next_queue);
                    break;
                default:;
            }

        } else
            pairQueues(this_queue, next_queue);

        this_queue.queue.at(0) = -1;
        this_queue.queueLength.at(0) = 0;
        this_queue.queue.at(1) = -1;
        this_queue.queueLength.at(1) = 0;

        if (this_idx <= minorTime)
            m_majortime_buffer.at(this_idx) = majorTime;
        else
            m_majortime_buffer.at(this_idx) = majorTime - 1;
    }

    if (m_verbose_level > 2) {
        std::cout << "Setting i1 to " << minorTime << std::endl;
        nmx::printMajorTimeBuffer(m_majortime_buffer);
    }

    m_i1 = minorTime;

    if (m_verbose_level > 2) {
        m_mutex.lock();
        m_clusterManager.printStack(0);
        m_clusterManager.printStack(1);
        printSortBuffer();
        printQueue();
        m_mutex.unlock();
    }
}

void NMXClusterPairing::pairQueues(nmx::ClusterParingEntry &this_queue, nmx::ClusterParingEntry &next_queue) {

    nmx::PairBuffer pairBuffer;
    pairBuffer.npairs = 0;

    m_Qmatrix.reset();
    calculateQmatrix(this_queue, next_queue);

    this_queue.queue.at(0) = -1;
    this_queue.queueLength.at(0) = 0;
    this_queue.queue.at(1) = -1;
    this_queue.queueLength.at(1) = 0;
    next_queue.queue.at(0) = -1;
    next_queue.queueLength.at(0) = 0;
    next_queue.queue.at(1) = -1;
    next_queue.queueLength.at(1) = 0;

    if (m_verbose_level > 2)
        std::cout << "<NMXClusterPairing::pairQueues> Processing Qmatrix dim[" << m_nXthis + m_nXnext << ", "
                  << m_nYthis + m_nYnext << "]\n";

    while (1) {

        nmx::ClusterIndexPair entry = findMinQ(m_Qmatrix);

        if (m_verbose_level > 2)
            std::cout << "Entry with min-Q is (" << entry.x_idx << ", " << entry.y_idx << ")" << std::endl;

        if (entry.x_idx < 0 || entry.y_idx < 0)
            break;

        if (m_verbose_level > 2) {
            std::cout << "Min_Q located at (" << entry.x_idx << ", " << entry.y_idx << ")\n";
            std::cout << "Qmatrix[" << entry.x_idx << ", " << entry.y_idx << "] > " << nmx::DELTA_Q << std::endl;
        }

        if (m_Qmatrix.at(entry.x_idx, entry.y_idx) > nmx::DELTA_Q)
            break;

        if ((static_cast<unsigned int>(entry.x_idx) >= m_nXthis) &&
            (static_cast<unsigned int>(entry.y_idx) >= m_nYthis)) {

            appendIndexToQueue(0, next_queue, m_Qmatrix.getLink(entry.x_idx, 0));
            appendIndexToQueue(1, next_queue, m_Qmatrix.getLink(entry.y_idx, 1));
            m_Qmatrix.setQ(entry.x_idx, entry.y_idx, 2.);
            m_Qmatrix.setLink(entry.x_idx, 0, -1);
            m_Qmatrix.setLink(entry.y_idx, 1, -1);

        } else {

            if (m_Qmatrix.getLink(entry.x_idx, 0) == -1 || m_Qmatrix.getLink(entry.y_idx, 1) == -1)
                continue;

            nmx::ClusterIndexPair &nextPair = pairBuffer.pairs.at(pairBuffer.npairs);
            nextPair.x_idx = m_Qmatrix.getLink(entry.x_idx, 0);
            nextPair.y_idx = m_Qmatrix.getLink(entry.y_idx, 1);
            pairBuffer.npairs++;
            m_Qmatrix.setQ(entry.x_idx, entry.y_idx, 2.);
            m_Qmatrix.setLink(entry.x_idx, 0, -1);
            m_Qmatrix.setLink(entry.y_idx, 1, -1);
        }
    }

    // Empty links from matrix

    // First clear 'this-queue' entries
    if (m_verbose_level > 2)
        std::cout << "Clearing 'this-queue' X ..." << std::endl;
    for (unsigned int i = 0; i < m_nXthis; i++) {
        int idx = m_Qmatrix.getLink(i, 0);
        if (m_verbose_level > 2)
            std::cout << "Index of item " << i << " is " << idx << std::endl;
        if (idx >= 0)
            m_clusterManager.returnClusterToStack(0, idx);
    }

    if (m_verbose_level > 2)
        std::cout << "Clearing 'this-queue' Y ..." << std::endl;
    for (unsigned int i = 0; i < m_nYthis; i++) {
        int idx = m_Qmatrix.getLink(i, 1);
        if (m_verbose_level > 2)
            std::cout << "Index of item " << i << " is " << idx << std::endl;
        if (idx >= 0)
            m_clusterManager.returnClusterToStack(1, idx);
    }

    // Now return 'next-queue' to DataBuffer
    for (unsigned int i = m_nXthis; i < m_nXthis+m_nXnext; i++) {
        int idx = m_Qmatrix.getLink(i, 0);
        if (idx >= 0)
            appendIndexToQueue(0, next_queue, idx);
    }
    for (unsigned int i = m_nYthis; i < m_nYthis+m_nYnext; i++) {
        int idx = m_Qmatrix.getLink(i, 1);
        if (idx >= 0)
            appendIndexToQueue(1, next_queue, idx);
    }

    m_nPairs += pairBuffer.npairs;
    m_locationFinder.find(pairBuffer);
}

nmx::Qmatrix NMXClusterPairing::calculateQmatrix(nmx::ClusterParingEntry &this_queue,
                                                 nmx::ClusterParingEntry &next_queue) {

    m_Qmatrix.setDIM(m_nXthis+m_nXnext, m_nYthis+m_nYnext);

    int Xidx = (this_queue.queue.at(0) >= 0 ? this_queue.queue.at(0) : next_queue.queue.at(0));
    int Yidx = (this_queue.queue.at(1) >= 0 ? this_queue.queue.at(1) : next_queue.queue.at(1));

    for (unsigned int ix = 0; ix < m_nXthis + m_nXnext; ix++) {

        nmx::Cluster &Xcluster = m_clusterManager.getCluster(0, Xidx);
        double xCharge = static_cast<double>(Xcluster.box.chargesum);

        m_Qmatrix.setLink(ix, 0, Xidx);

        for (unsigned int iy = 0; iy < m_nYthis + m_nYnext; iy++) {

            nmx::Cluster &Ycluster = m_clusterManager.getCluster(1, Yidx);
            double yCharge = static_cast<double>(Ycluster.box.chargesum);

            double Qval = 2 * std::abs(xCharge - yCharge) / (xCharge + yCharge);
            m_Qmatrix.setQ(ix, iy, Qval);

            if (ix == 0)
                m_Qmatrix.setLink(iy, 1, Yidx);

            if (iy == m_nYthis-1)
                Yidx = next_queue.queue.at(1);
            else
                Yidx = Ycluster.box.link1;
        }

        if (ix == m_nXthis-1)
            Xidx = next_queue.queue.at(0);
        else
            Xidx = Xcluster.box.link1;

        Yidx = (this_queue.queue.at(1) >= 0 ? this_queue.queue.at(1) : next_queue.queue.at(1));
    }

    return m_Qmatrix;
}

nmx::ClusterIndexPair NMXClusterPairing::findMinQ(const nmx::Qmatrix &qmatrix) {

    nmx::ClusterIndexPair pair = {-1, -1};
    double minQ = 2.;

    for (unsigned int i = 0; i < m_nXthis+m_nXnext; i++) {
        for (unsigned int j = 0; j < m_nYthis+m_nYnext; j++) {
            double qval = qmatrix.at(i, j);
            int iLink = qmatrix.getLink(i, 0);
            int jLink = qmatrix.getLink(j, 1);
            if ((qval < minQ) && (iLink >= 0) && (jLink >= 0)) {
                minQ = qval;
                pair.x_idx = i;
                pair.y_idx = j;
                //pair.x_idx = iLink;
                //pair.y_idx = jLink;
            }
        }
    }

    return pair;
}

void NMXClusterPairing::appendIndexToQueue(unsigned int plane, nmx::ClusterParingEntry &queue, int clusterIdx) {

    if (clusterIdx < 0)
        return;

    if (m_verbose_level > 1)
        std::cout << "<NMXClusterPairing::appendIndexToQueue> Appending idx " << clusterIdx << " to "
                  << (plane ? "Y" : "X") << " queue." << std::endl;
    if (m_verbose_level > 2) {
        std::cout << "Queue before:\n";
        nmx::printQueue(plane, queue.queue.at(plane), m_clusterManager);
    }

    int currentIdx = queue.queue.at(plane);

    if (currentIdx == -1) {
        queue.queue.at(plane) = clusterIdx;
        m_clusterManager.getCluster(plane, clusterIdx).box.link1 = -1;
        queue.queueLength.at(plane) = 1;
    } else {

        if (m_verbose_level > 2)
            std::cout << "Propagating queue : " << currentIdx;

        while (true) {

            nmx::Cluster &currentCluster = m_clusterManager.getCluster(plane, currentIdx);

            currentIdx = currentCluster.box.link1;

            if (m_verbose_level > 2)
                std::cout << " -> " << currentIdx;

            if (currentIdx == -1) {
                currentCluster.box.link1 = clusterIdx;
                m_clusterManager.getCluster(plane, clusterIdx).box.link1 = -1;
                queue.queueLength.at(plane)++;
                break;
            }
        }
    }

    if (m_verbose_level > 2) {
        std::cout << std::endl;
        std::cout << "Queue after:" << std::endl;
        nmx::printQueue(plane, queue.queue.at(plane), m_clusterManager);
    }
}

inline uint32_t NMXClusterPairing::getMinorTime(uint32_t time) {

    time = time >> nmx::CLUSTER_IGNORE_BITS;
    time = time & nmx::CLUSTER_MINOR_BITMASK;

    return time;
}

inline uint32_t NMXClusterPairing::getMajorTime(uint32_t time) {

    return time >> nmx::CLUSTER_IGNORE_BITS >> nmx::CLUSTER_MINOR_BITS;
}

void NMXClusterPairing::endRun() {

    while (m_nIn != m_nOut)
        std::this_thread::yield();

    slideTimeWindow(nmx::CLUSTER_MAX_MINOR, nmx::CLUSTER_MINOR_BITMASK, 0);
}

void NMXClusterPairing::reset() {

    for (unsigned int idx = 0; idx < nmx::CLUSTER_MAX_MINOR; idx++) {

        m_time_ordered_buffer.at(idx).queue.at(0) = -1;
        m_time_ordered_buffer.at(idx).queueLength.at(0) = 0;
        m_time_ordered_buffer.at(idx).queue.at(1) = -1;
        m_time_ordered_buffer.at(idx).queueLength.at(1) = 0;

        m_majortime_buffer.at(idx) = 0;
    }

    m_i1 = nmx::CLUSTER_MINOR_BITMASK;

    m_terminate = false;

    m_nPairs = 0;
}

void NMXClusterPairing::returnQueueToStack(int plane, int idx) {

    if (idx == -1)
        return;

    while (idx >= 0) {
        int nextIdx = m_clusterManager.getCluster(plane, idx).box.link1;
        m_clusterManager.returnClusterToStack(plane, idx);
        idx = nextIdx;
    }
}

void NMXClusterPairing::printSortBuffer() {

    std::cout << "Time ordered DataBuffer:\n";

    for (unsigned int idx = 0; idx < nmx::CLUSTER_MAX_MINOR; idx++) {

        int xQueue = m_time_ordered_buffer.at(idx).queue.at(0);
        int yQueue = m_time_ordered_buffer.at(idx).queue.at(1);

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
    for (unsigned int i = m_nOut.at(0); i < m_nIn.at(0); i++) {
        int idx = i % nmx::NCLUSTERS;
        std::cout << m_queue.at(0).at(idx) << " ";
    }
    std::cout << "\n";

    std::cout << "Buffer-queue Y:\n";
    for (unsigned int i = m_nOut.at(1); i < m_nIn.at(1); i++) {
        int idx = i % nmx::NCLUSTERS;
        std::cout << m_queue.at(1).at(idx) << " ";
    }
    std::cout << "\n";
}

void NMXClusterPairing::checkSortBuffer() {

    for (unsigned int i = 0; i < nmx::CLUSTER_MAX_MINOR; i++) {

        auto entry = m_time_ordered_buffer.at(i);

        for (unsigned int plane = 0; plane < 2; plane++) {

            if ((entry.queueLength.at(plane) == 0 && entry.queue.at(plane) != -1) ||
                    (entry.queueLength.at(plane) != 0 && entry.queue.at(plane) == -1)) {
                std::cerr << "<NMXClusterPairing::checkSortBuffer> Entry " << i << " on plane " << (plane ? "Y" : "X")
                          << std::endl;
                std::cerr << "Queue lenght is " << entry.queueLength.at(plane) << " but queue start is "
                          << entry.queue.at(plane) << std::endl;
                throw 1;
            }
        }
    }
}

void NMXClusterPairing::printQmatrix() {

    unsigned int dimX = m_Qmatrix.getDIM().at(0);
    unsigned int dimY = m_Qmatrix.getDIM().at(1);

    std::cout << "Matrix DIM = [" << dimX << "," << dimY << "]" << std::endl;

    for (unsigned int y = 0; y < dimY; y++) {
        for (unsigned int x = 0; x < dimX; x++)
            std::cout << std::setw(4) << m_Qmatrix.at(x,y);
    }
    std::cout << std::endl;

    std::cout << "\nLinks:" << std::endl;
    std::cout << "X: ";
    for (unsigned int x = 0; x < dimX; ++x)
        std::cout << m_Qmatrix.getLink(x, 0) << " ";
    std::cout << std::endl;
    std::cout << "Y: ";
    for (unsigned int y = 0; y < dimY; ++y)
        std::cout << m_Qmatrix.getLink(y, 1) << " ";
    std::cout << std::endl;
}