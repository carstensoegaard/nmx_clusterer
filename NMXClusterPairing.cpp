//
// Created by soegaard on 1/31/18.
//

#include <iostream>
#include <iomanip>
#include <algorithm>

#include "NMXLocationFinder.h"
#include "NMXClustererHelper.h"
#include "NMXClusterPairing.h"

NMXClusterPairing::NMXClusterPairing(NMXClusterManager &clusterManager)
        : m_i1(nmx::MAX_MINOR),
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
    m_nIn.at(plane)++;
    m_mutex.unlock();
    if (m_verbose_level > 1)
        std::cout << "<NMXClusterPairing::insertClusterInQueue> Inserted index " << cluster_idx << " at " << buffer_idx
                  << " in plane " << plane << std::endl;
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

                    slideTimeWindow(nmx::MAX_MINOR - m_i1 + std::min(m_i1, minorTime), minorTime, majorTime);
                    addToBuffer(plane, cluster_idx, minorTime);

                } else { // majorTime > (m_majortime_buffer.at(0) + 1)

                    if (m_verbose_level > 1)
                        std::cout << "Case 2\n";

                    slideTimeWindow(nmx::MAX_MINOR, minorTime, majorTime);
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

    nmx::clusterParingEntry &entry = m_time_ordered_buffer.at(minorTime);

    if (m_verbose_level > 2) {
        std::cout << "Before:\n";
        nmx::printQueue(0, entry.queue.at(0), m_clusterManager);
        nmx::printQueue(1, entry.queue.at(1), m_clusterManager);
    }

    if (entry.queueLength.at(plane) == 0) {
        entry.queue.at(plane) = idx;
        if (m_verbose_level > 2)
            std::cout << "First cluster!\n";
    } else {

        int queueIdx = entry.queue.at(plane);

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

    entry.queueLength.at(plane)++;

    if (m_verbose_level > 1) {
        std::cout << "After:\n";
        nmx::printQueue(0, entry.queue.at(0), m_clusterManager);
        nmx::printQueue(1, entry.queue.at(1), m_clusterManager);
    }
}

void NMXClusterPairing::slideTimeWindow(uint d, uint minorTime, uint majorTime) {

    if (m_verbose_level > 1) {
        std::string s("\nWill move ");
        s.append(std::to_string(d));
        s.append(" entries to clusterer !\n");
        std::cout << s;
    }

    for (uint64_t i = 1; i <= d; ++i) {

        uint64_t this_idx = (i+m_i1)%nmx::MAX_MINOR;
        uint64_t next_idx = (i+m_i1+1)%nmx::MAX_MINOR;

        nmx::clusterParingEntry &this_queue = m_time_ordered_buffer.at(this_idx);
        nmx::clusterParingEntry &next_queue = m_time_ordered_buffer.at(next_idx);

        m_nXthis = this_queue.queueLength.at(0);
        m_nYthis = this_queue.queueLength.at(1);
        m_nXnext = next_queue.queueLength.at(0);
        m_nYnext = next_queue.queueLength.at(1);

        unsigned int thisCase = (m_nXthis << 1) + m_nYthis;
        unsigned int nextCase = (m_nXnext << 1) + m_nYnext;

        if (thisCase == 0)
            continue;

        if (thisCase < 4 && nextCase < 4) {

            unsigned int Case = (thisCase << 2) + nextCase;

            std::cout << "nXthis = " << m_nXthis << ", nYthis = " << m_nYthis << std::endl;
            std::cout << "nXnext = " << m_nXnext << ", nYnext = " << m_nYnext << std::endl;
            std::cout << "thisCase = " << thisCase << ", nextCase = " << nextCase << ", Case = " << Case << std::endl;
            std::cout << "m_nXthis << 1 = " << (m_nXthis << 1) << std::endl;

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
                default:
                    std::cout << "<NMXClusterParing::slideTimeWindow> Default case reached - this should not happen.\n";
            }

        } else
            pairQueues(this_queue, next_queue);

        this_queue.queue.at(0) = -1;
        this_queue.queue.at(1) = -1;

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
}

void NMXClusterPairing::pairQueues(nmx::clusterParingEntry &this_queue, nmx::clusterParingEntry &next_queue) {

    nmx::pairBuffer pairBuffer;
    pairBuffer.npairs = 0;

    m_Qmatrix.reset();
    calculateQmatrix(this_queue, next_queue);

    this_queue.queue.at(0) = -1;
    this_queue.queueLength.at(0) = 0;
    next_queue.queue.at(1) = -1;
    next_queue.queueLength.at(1) = 0;

    while (1) {

        nmx::clusterPair entry = findMinQ(m_Qmatrix);

        if (m_Qmatrix.at(entry.x_idx, entry.y_idx) > nmx::DELTA_Q)
            break;

        if (entry.x_idx >= m_nXthis && entry.y_idx >= m_nYthis) {

            appendIndexToQueue(0, next_queue, m_Qmatrix.getLink(entry.x_idx, 0));
            appendIndexToQueue(1, next_queue, m_Qmatrix.getLink(entry.y_idx, 1));
            m_Qmatrix.setQ(entry.x_idx, entry.y_idx, 2.);
            m_Qmatrix.setLink(entry.x_idx, 0, -1);
            m_Qmatrix.setLink(entry.y_idx, 1, -1);

        } else {

            if (m_Qmatrix.getLink(entry.x_idx, 0) == -1 || m_Qmatrix.getLink(entry.y_idx, 1) == -1)
                continue;

            nmx::clusterPair &nextPair = pairBuffer.pairs.at(pairBuffer.npairs);
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
    for (int i = 0; i < m_nXthis; i++) {
        int idx = m_Qmatrix.getLink(i, 0);
        if (idx >= 0)
            m_clusterManager.returnClusterToStack(0, idx);
    }
    for (int i = 0; i < m_nYthis; i++) {
        int idx = m_Qmatrix.getLink(i, 1);
        if (idx >= 0)
            m_clusterManager.returnClusterToStack(1, idx);
    }

    // Now return 'next-queue' to buffer
    for (int i = m_nXthis; i < m_nXthis+m_nXnext; i++) {
        int idx = m_Qmatrix.getLink(i, 0);
        if (idx >= 0)
            appendIndexToQueue(0, next_queue, idx);
    }
    for (int i = m_nYthis; i < m_nYthis+m_nYnext; i++) {
        int idx = m_Qmatrix.getLink(i, 1);
        if (idx >= 0)
            appendIndexToQueue(1, next_queue, idx);
    }

    m_locationFinder.find(pairBuffer);
}

nmx::Qmatrix NMXClusterPairing::calculateQmatrix(nmx::clusterParingEntry &this_queue,
                                                 nmx::clusterParingEntry &next_queue) {

    m_Qmatrix.setDIM(m_nXthis+m_nXnext, m_nYthis+m_nYnext);

    int Xidx = (this_queue.queue.at(0) >= 0 ? this_queue.queue.at(0) : next_queue.queue.at(0));
    int Yidx = (this_queue.queue.at(1) >= 0 ? this_queue.queue.at(1) : next_queue.queue.at(1));

    for (int ix = 0; ix < m_nXthis + m_nXnext; ix++) {

        nmx::cluster &Xcluster = m_clusterManager.getCluster(0, Xidx);
        double xCharge = static_cast<double>(Xcluster.box.chargesum);

        for (int iy = 0; iy < m_nYthis + m_nYnext; iy++) {

            nmx::cluster &Ycluster = m_clusterManager.getCluster(1, Yidx);
            double yCharge = static_cast<double>(Ycluster.box.chargesum);

            double Qval = 2 * std::abs(xCharge - yCharge) / (xCharge + yCharge);
            m_Qmatrix.setQ(ix, iy, Qval);
            m_Qmatrix.setLink(ix, 0, Xidx);
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
}

nmx::clusterPair NMXClusterPairing::findMinQ(const nmx::Qmatrix &qmatrix) {

    nmx::clusterPair pair = {-1, -1};
    double minQ = 2.;

    for (int i = 0; i < m_nXthis+m_nXnext; i++) {
        for (int j = 0; j < m_nYthis+m_nYnext; j++) {
            double qval = qmatrix.at(i, j);
            int iLink = qmatrix.getLink(i, 0);
            int jLink = qmatrix.getLink(j, 1);
            if ((qval < minQ) && (iLink >= 0) && (jLink >= 0)) {
                minQ = qval;
                pair.x_idx = iLink;
                pair.y_idx = jLink;
            }
        }
    }

    return pair;
}

void NMXClusterPairing::appendIndexToQueue(unsigned int plane, nmx::clusterParingEntry &queue, int clusterIdx) {

    if (clusterIdx < 0)
        return;

    if (m_verbose_level > 1)
        std::cout << "<NMXClusterPairing::appendIndexToQueue> Appending idx " << clusterIdx << " to queue\n";
    if (m_verbose_level > 2) {
        std::cout << "Queue now:\n";
        nmx::printQueue(plane, queue.queue.at(plane), m_clusterManager);
    }

    int currentIdx = queue.queue.at(plane);

    if (m_verbose_level > 2)
        std::cout << "Propagating queue : " << currentIdx;

    if (queue.queue.at(plane) == -1) {
        queue.queue.at(plane) = currentIdx;
        queue.queueLength.at(plane)++;
        return;
    }

    while (true) {

        nmx::cluster &nextCluster = m_clusterManager.getCluster(plane, currentIdx);

        currentIdx = nextCluster.box.link1;

        if (m_verbose_level > 2)
            std::cout << " -> " << currentIdx;

        if (currentIdx == -1) {
            nextCluster.box.link1 = clusterIdx;
            queue.queueLength.at(plane)++;
            break;
        }
    }
    if (m_verbose_level > 2) {
        std::cout << "\n";
        std::cout << "Queue now:\n";
        nmx::printQueue(plane, queue.queue.at(plane), m_clusterManager);
    }
}

inline uint32_t NMXClusterPairing::getMinorTime(uint32_t time) {

    time = time >> nmx::IGNORE_BITS;
    time = time & nmx::MINOR_BITMASK;

    return time;
}

inline uint32_t NMXClusterPairing::getMajorTime(uint32_t time) {

    return time >> nmx::IGNORE_BITS >> nmx::MINOR_BITS;
}

void NMXClusterPairing::reset() {

    for (unsigned int idx = 0; idx < nmx::MAX_MINOR; idx++) {

        m_time_ordered_buffer.at(idx).queue.at(0) = -1;
        m_time_ordered_buffer.at(idx).queueLength.at(0) = 0;
        m_time_ordered_buffer.at(idx).queue.at(1) = -1;
        m_time_ordered_buffer.at(idx).queueLength.at(1) = 0;

        m_majortime_buffer.at(idx) = 0;
    }
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

    std::cout << "Time ordered buffer:\n";

    for (int idx = 0; idx < nmx::MAX_MINOR; idx++) {

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
    for (int i = m_nOut.at(0)%nmx::NCLUSTERS; i < m_nIn.at(0)%nmx::NCLUSTERS; i++ )
        std::cout << m_queue.at(0).at(i) << " ";
    std::cout << "\n";

    std::cout << "Buffer-queue Y:\n";
    for (int i = m_nOut.at(1)%nmx::NCLUSTERS; i < m_nIn.at(1)%nmx::NCLUSTERS; i++ )
        std::cout << m_queue.at(1).at(i) << " ";
    std::cout << "\n";
}