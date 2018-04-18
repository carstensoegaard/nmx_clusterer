//
// Created by soegaard on 1/31/18.
//

#include <iostream>
#include <iomanip>
#include <algorithm>

#include "NMXClustererHelper.h"
#include "NMXClustererVerification.h"

NMXClustererVerification::NMXClustererVerification()
        : m_ievent(0),
          m_i1(nmx::MINOR_BITMASK),
          m_verbose_level(0),
          m_terminate(false)
{
    m_file.open("NMX_matched_clusters.txt");

    reset();

    t_process = std::thread(&NMXClustererVerification::process, this);
}

NMXClustererVerification::~NMXClustererVerification() {

    m_file.close();

    t_process.join();
}

void NMXClustererVerification::insertEventInQueue(const nmx::fullCluster &event) {

    if ((m_In[0] - m_Out[0]) > 99)
        std::this_thread::yield();

    unsigned int clusterQueueIdx = m_In[0] % 100;

    event.eventNo = m_ievent;
    m_queue.at(0).at(clusterQueueIdx) = event;
    m_In[0]++;
    m_ievent++;
}

void NMXClustererVerification::insertClusterInQueue(const nmx::fullCluster &cluster) {

    if ((m_In[1] - m_Out[1]) > 99)
        std::this_thread::yield();

    unsigned int clusterQueueIdx = m_In[1] % 100;

    m_queue.at(1).at(clusterQueueIdx) = event;
    m_In[1]++;
}

void NMXClustererVerification::process() {

    unsigned int shifter = 0;

    while (1) {

        while ((m_eventIn == m_eventOut) && (m_clusterIn == m_clusterOut)) {
            if (m_terminate)
                return;
            std::this_thread::yield();
        }

        if (m_In[shifter] > m_Out[shifter]) {

            auto &queue = m_queue.at(shifter);

            unsigned int queueIdx = m_Out[shifter]%100;

            nmx::cluster &object = queue.at(queueIdx);

            uint32_t minortime = getMinorTime(object.box.max_time);
            uint32_t majortime = getMajorTime(object.box.max_time);

            if (m_verbose_level > 1) {
                std::cout << "<NMXClustererVerification::insert> Got idx " << cluster_idx << " from " << buffer_idx
                          << std::endl;
                std::cout << "Time = " << time << ", B1 = " << minorTime << ", B2 = " << majorTime << std::endl;
                std::cout << "B2_buffer[" << minorTime << "] = " << m_majortime_buffer[minorTime] << " B2_buffer[0] = "
                          << m_majortime_buffer.at(0) << " i1 = " << m_i1 << std::endl;
            }

            if (majorTime >= (m_majortime_buffer.at(0) + 1)) {

                if (majorTime == (m_majortime_buffer.at(0) + 1)) {

                    if (m_verbose_level > 1)
                        std::cout << "Case 1\n";

                    slideTimeWindow(nmx::MAX_MINOR - m_i1 + std::min(m_i1, minorTime) + 1, minorTime, majorTime);
                    addToBuffer(shifter, object, minorTime);

                } else { // majorTime > (m_majortime_buffer.at(0) + 1)

                    if (m_verbose_level > 1)
                        std::cout << "Case 2\n";

                    slideTimeWindow(nmx::MAX_MINOR, minorTime, majorTime);
                    addToBuffer(shifter, object, minorTime);
                }

            } else { // majorTime <= m_buffer.at(0)

                switch (majorTime - m_majortime_buffer.at(minorTime)) {

                    case 1:

                        if (m_verbose_level > 1)
                            std::cout << "Case 3\n";

                        slideTimeWindow(minorTime - m_i1, minorTime, majorTime);
                        addToBuffer(shifter, object, minorTime);
                        break;

                    case 0:

                        if (m_verbose_level > 1)
                            std::cout << "Case 4\n";

                        addToBuffer(shifter, object, minorTime);
                        break;

                    default:

                        std::cout << "Old data-point - omitting!\n";
                }
            }

            m_Out[shifter]++;
        }

        shifter = !shifter;

        if (m_terminate)
            return;
    }
}

void NMXClustererVerification::addToBuffer(unsigned int shifter, nmx::fullCluster &object, uint minorTime) {

    bufferEntry &entry = m_time_ordered_buffer.at(minorTime);

    entry.at(shifter).push_back(object);
}

void NMXClustererVerification::slideTimeWindow(uint d, uint minorTime, uint majorTime) {

    if (m_verbose_level > 1) {
        std::string s("\nWill move ");
        s.append(std::to_string(d));
        s.append(" entries to clusterer !\n");
        std::cout << s;
    }

    for (uint64_t i = 0; i < d; ++i) {

        uint64_t thisIdx = (i + m_i1 + 1) % nmx::MAX_MINOR;
        uint64_t nextIdx = (thisIdx + 1) % nmx::MAX_MINOR;

        auto &thisEntry = m_time_ordered_buffer.at(thisIdx);
        auto &nextEntry = m_time_ordered_buffer.at(nextIdx);

        findMatches(thisEntry, nextEntry);

        thisEntry.at(0).clear();
        nextEntry.at(1).clear();
    }
}

void NMXClustererVerification::findMatches(bufferEntry &thisEntry, bufferEntry &nextEntry) {

    int nEvents = thisEntry.at(0).size() + nextEntry.at(0).size();
    int nClusters = thisEntry.at(1).size() + nextEntry.at(1).size();

    for (int ievent = 0; ievent < nEvents; ievent++) {

        auto &eventQueue = thisEntry.at(0);
        if (ievent >= thisEntry.at(0).size())
            eventQueue = nextEntry.at(0);

        nmx::fullCluster &event = eventQueue.at(ievent);
        unsigned int eventNo = event.eventNo;

        int maxMatching = 0;
        int bestCluster = -1;

        for (int icluster = 0; icluster < nClusters; icluster++) {

            if ((ievent >= thisEntry.at(0).size()) && (icluster >= thisEntry.at(1).size()))
                continue;

            auto &clusterQueue = thisEntry.at(1);
            if (icluster >= thisEntry.at(1).size())
                clusterQueue = nextEntry.at(1);

            nmx::fullCluster &cluster = clusterQueue.at(icluster);

            int nMatching = numberOfMatchingPoints(event, cluster)

            if (nMatching > maxMatching) {
                maxMatching = nMatching;
                bestCluster = icluster;
            }
        }

        auto &clusterQueue = thisEntry.at(1);

        if (bestCluster >= thisEntry.at(1).size()) {
            clusterQueue = nextEntry.at(1);
            bestCluster -= thisEntry.at(1).size();
        }

        nmx::fullCluster &cluster = clusterQueue.at(bestCluster);
        cluster.eventNo = eventNo;
    }

    for (int ievent = 0; ievent < nEvents; ievent++) {

        auto &eventQueue = thisEntry.at(0);
        if (ievent >= thisEntry.at(0).size())
            eventQueue = nextEntry.at(0);

        nmx::fullCluster &event = eventQueue.at(ievent);
        unsigned int eventNo = event.eventNo;

        writeEventToFile(eventNo, event);
        writeClustersToFile(eventNo, thisEntry.at(1));
        writeClustersToFile(eventNo, nextEntry.at(1));
    }
}

int NMXClustererVerification::numberOfMatchingPoints(const nmx::fullCluster &event, const nmx::fullCluster &cluster) {

    int nMatches = 0;

    nMatches += numberOfMatchingPointsPlane(event.at(0), cluster.at(0));
    nMatches += numberOfMatchingPointsPlane(event.at(1), cluster.at(1));

    return nMatches;
}

int NMXClustererVerification::numberOfMatchingPointsPlane(const nmx::cluster &event, const nmx::cluster &cluster) {

    int nMatches = 0;

    for (int ievent = 0; ievent < event.npoints; ievent++) {

        nmx::data_point &evPoint = event.data.at(ievent);

        for (int icluster = 0; icluster < cluster.npoints; icluster++) {

            nmx::data_point &clPoint = cluster.data.at(icluster);

            if (pointsMatch(produced_point, stored_point))
                nmatches++;
        }
    }

    return nmatches;
}

bool NMXClustererVerification::pointsMatch(const nmx::data_point &p1, const nmx::data_point &p2) {

    if ((p1.strip == p2.strip) && (p1.time == p2.time) && (p1.charge == p2.charge))
        return true;

    return false;
}

inline int NMXClustererVerification::getTotalPoints(const nmx::fullCluster &object) {

    int totalPoints = object.at(0).npoints;
    totalPoints += object.at(1).npoints;

    return totalPoints;
}

void NMXClustererVerification::writeEventToFile(int eventNo, nmx::fullCluster &event) {

    m_file << "Event # " << eventNo << std::endl;
    writeObjectToFile(event);
}

void NMXClustererVerification::writeClustersToFile(int eventNo, bufferEntry &entry) {

    auto it = entry.begin();

    while (it != entry.at(1).end()) {

        nmx::fullCluster &cluster = *it;

        if (cluster.eventNo == eventNo) {
            writeObjectToFile(cluster);
            entry.at(1).erase(it);
        } else
            it++;
    }
}

inline void NMXClustererVerification::writeObjectToFile(nmx::fullCluster &object) {

    m_file << "X:\n";
    writePlaneToFile(event.at(0));
    m_file << "Y:\n";
    writePlaneToFile(event.at(1));
}

inline void NMXClustererVerification::writePlaneToFile(nmx::cluster &plane) {

    for (int i = 0; i < plane.npoints; i++)
        m_file << plane.data.at(i) << " ";

    m_file << "\n";
}

inline uint32_t NMXClustererVerification::getMinorTime(uint32_t time) {

    time = time >> nmx::IGNORE_BITS;
    time = time & nmx::MINOR_BITMASK;

    return time;
}

inline uint32_t NMXClustererVerification::getMajorTime(uint32_t time) {

    return time >> nmx::IGNORE_BITS >> nmx::MINOR_BITS;
}

void NMXClustererVerification::endRun() {

    while (m_nIn != m_nOut)
        std::this_thread::yield();

    slideTimeWindow(nmx::MINOR_BITMASK, nmx::MINOR_BITMASK, 0);
}

void NMXClustererVerification::reset() {

    for (unsigned int idx = 0; idx < nmx::MAX_MINOR; idx++) {

        m_time_ordered_buffer.at(idx).queue.at(0) = -1;
        m_time_ordered_buffer.at(idx).queueLength.at(0) = 0;
        m_time_ordered_buffer.at(idx).queue.at(1) = -1;
        m_time_ordered_buffer.at(idx).queueLength.at(1) = 0;

        m_majortime_buffer.at(idx) = 0;
    }
}

NMXClustererVerification* NMXClustererVerification::getInstance() {

    if (!instance) {
        instance = new NMXClustererVerification();
    }

    return instance;
}

/*
void NMXClustererVerification::printSortBuffer() {

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

void NMXClustererVerification::printQueue() {

    std::cout << "Buffer-queue X:\n";
    for (int i = m_nOut.at(0); i < m_nIn.at(0); i++) {
        int idx = i % nmx::CLUSTER_BUFFER_SIZE;
        std::cout << m_queue.at(0).at(idx) << " ";
    }
    std::cout << "\n";

    std::cout << "Buffer-queue Y:\n";
    for (int i = m_nOut.at(1); i < m_nIn.at(1); i++) {
        int idx = i % nmx::CLUSTER_BUFFER_SIZE;
        std::cout << m_queue.at(1).at(idx) << " ";
    }
    std::cout << "\n";
}

void NMXClustererVerification::checkSortBuffer() {

    for (unsigned int i = 0; i < nmx::MAX_MINOR; i++) {

        auto entry = m_time_ordered_buffer.at(i);

        for (unsigned int plane = 0; plane < 2; plane++) {

            if ((entry.queueLength.at(plane) == 0 && entry.queue.at(plane) != -1) ||
                    (entry.queueLength.at(plane) != 0 && entry.queue.at(plane) == -1)) {
                std::cerr << "<NMXClustererVerification::checkSortBuffer> Entry " << i << " on plane " << (plane ? "Y" : "X")
                          << std::endl;
                std::cerr << "Queue lenght is " << entry.queueLength.at(plane) << " but queue start is "
                          << entry.queue.at(plane) << std::endl;
                throw 1;
            }
        }
    }
}
 */