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
          m_i1(m_minorBitmask),
          m_verbose_level(0),
          m_terminate(false)
{
    m_file.open("NMX_matched_clusters.txt");

    reset();

    std::cout << "<NMXClustererVerification::NMXClustererVerification> :\n";
    for (int i = 0; i < 2; i++)
        std::cout << "   nIn[" << i << "] = " << m_In[i] << ", nOut[" << i << "] = " << m_Out[i] << std::endl;

    t_process = std::thread(&NMXClustererVerification::process, this);
}

NMXClustererVerification::~NMXClustererVerification() {

    m_file.close();

    t_process.join();
}

void NMXClustererVerification::insertEventInQueue(const nmx::fullCluster &event) {

    //std::cout << "Inserting event:\n";
    //printFullCluster(event);

    if ((m_In[0] - m_Out[0]) > 99)
        std::this_thread::yield();

    unsigned int clusterQueueIdx = m_In[0] % 100;

    m_queue.at(0).at(clusterQueueIdx) = event;
    m_queue.at(0).at(clusterQueueIdx).eventNo = m_ievent;
    m_In[0]++;
    m_ievent++;

    /*
    std::cout << "Associated event # : " << m_queue.at(0).at(clusterQueueIdx).eventNo << std::endl;

    std::cout << "Event inserted - queue-length now " << m_In[0] << std::endl;
     */
}

void NMXClustererVerification::insertClusterInQueue(const nmx::fullCluster &cluster) {

    /*
    std::cout << "Inserting cluster :\n";
    printFullCluster(cluster);
*/

    if ((m_In[1] - m_Out[1]) > 99)
        std::this_thread::yield();

    unsigned int clusterQueueIdx = m_In[1] % 100;

    m_queue.at(1).at(clusterQueueIdx) = cluster;
    m_In[1]++;
    //std::cout << "Cluster inserted. Queue length now " << m_In[1] - m_Out[1] << std::endl;
}

void NMXClustererVerification::process() {

    unsigned int shifter = 0;

    while (1) {

        while ((m_In[0] == m_Out[0]) && (m_In[1] == m_Out[1])) {
            if (m_terminate)
                return;
            std::this_thread::yield();
        }

        std::cout << "Processing " << (shifter ? "Cluster" : "Event") << " queue ...\n";
        /*std::cout << (shifter ? "Y" : "X") << "-queue contains " << m_In[shifter] << " - " <<  m_Out[shifter]
                  << " = " << m_In[shifter] - m_Out[shifter] << " elements\n";*/

        if (m_In[shifter] > m_Out[shifter]) {

            auto &queue = m_queue.at(shifter);

            unsigned int queueIdx = m_Out[shifter]%100;

            //std::cout << "Accessing entry " << queueIdx << std::endl;

            nmx::fullCluster &object = queue.at(queueIdx);

            uint32_t maxTimeX = object.clusters.at(0).box.max_time;
            uint32_t maxTimeY = object.clusters.at(1).box.max_time;

            uint32_t maxTime = std::max(maxTimeX, maxTimeY);

            uint32_t minorTime = getMinorTime(maxTime);
            uint32_t majorTime = getMajorTime(maxTime);


            std::cout << "Time = " << maxTime << ", B1 = " << minorTime << ", B2 = " << majorTime << std::endl;
            std::cout << "B2_buffer[" << minorTime << "] = " << m_majortime_buffer[minorTime] << " B2_buffer[0] = "
                      << m_majortime_buffer.at(0) << " i1 = " << m_i1 << std::endl;


            if (majorTime >= (m_majortime_buffer.at(0) + 1)) {

                if (majorTime == (m_majortime_buffer.at(0) + 1)) {

                    if (m_verbose_level > 1)
                        std::cout << "Case 1\n";

                    slideTimeWindow(m_maxMinor - m_i1 + std::min(m_i1, minorTime), minorTime, majorTime);
                    addToBuffer(shifter, object, minorTime);

                } else { // majorTime > (m_majortime_buffer.at(0) + 1)

                    if (m_verbose_level > 1)
                        std::cout << "Case 2\n";

                    slideTimeWindow(m_maxMinor, minorTime, majorTime);
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

        uint64_t thisIdx = (i + m_i1 + 1) % m_maxMinor;
        uint64_t nextIdx = (thisIdx + 1) % m_maxMinor;

        //std::cout << "Moving # " << i << " with idx's " << thisIdx << " & " << nextIdx << std::endl;

        bufferEntry &thisEntry = m_time_ordered_buffer.at(thisIdx);
        bufferEntry &nextEntry = m_time_ordered_buffer.at(nextIdx);
/*
        std::cout << "# of 'this' events = " << thisEntry.at(0).size() << ", # of 'next' events = "
                  << nextEntry.at(0).size() << std::endl;
        std::cout << "# of 'this' clusters = " << thisEntry.at(1).size() << ", # of 'next' clusters = "
                  << nextEntry.at(1).size() << std::endl;
*/

        findMatches(thisEntry, nextEntry);

  /*      std::cout << "# of events = " << thisEntry.at(0).size() << ", # of clusters = " << thisEntry.at(1).size()
                  << std::endl;*/
        thisEntry.at(0).clear();
        thisEntry.at(1).clear();
        /*std::cout << "# of events = " << thisEntry.at(0).size() << ", # of clusters = " << thisEntry.at(1).size()
                  << std::endl;*/

        if (thisIdx <= minorTime)
            m_majortime_buffer.at(thisIdx) = majorTime;
        else
            m_majortime_buffer.at(thisIdx) = majorTime -1;
    }

    if (m_verbose_level > 2) {
        std::cout << "Setting i1 to " << minorTime << std::endl;
        nmx::printMajorTimeBuffer(m_majortime_buffer);
    }

    m_i1 = minorTime;
}


void NMXClustererVerification::findMatches(bufferEntry &thisEntry, bufferEntry &nextEntry) {

    unsigned int nEvents = thisEntry.at(0).size() + nextEntry.at(0).size();
    unsigned int nClusters = thisEntry.at(1).size() + nextEntry.at(1).size();

    std::cout << "Finding matches :\n";
    std::cout << "       Events   : " << thisEntry.at(0).size() << " + " << nextEntry.at(0).size() << " = " << nEvents
              << std::endl;
    std::cout << "       Clusters : " << thisEntry.at(1).size() << " + " << nextEntry.at(1).size() << " = " << nClusters
              << std::endl;

    auto eventIter1 = thisEntry.at(0).begin();

    while (eventIter1 != thisEntry.at(0).end()) {

        nmx::fullCluster &event = *eventIter1;

        compareToClusters(event, thisEntry.at(1), nextEntry.at(1));
    }

    auto eventIter2 = nextEntry.at(0).begin();

    while (eventIter2 != nextEntry.at(0).end()) {

        nmx::fullCluster &event = *eventIter2;

        compareToClusters(event, thisEntry.at(1), nextEntry.at(1));
    }


}

void NMXClustererVerification::compareToClusters(const nmx::fullCluster &event,
                                                 std::vector<nmx::fullCluster> &thisClusterQueue,
                                                 std::vector<nmx::fullCluster> &nextClusterQueue) {

    unsigned int nMatching1 = 0;
    unsigned int nMatching2 = 0;

    std::vector<nmx::fullCluster>::iterator cluster1 = compareToQueue(event, thisClusterQueue, nMatching1);
    std::vector<nmx::fullCluster>::iterator cluster2 = compareToQueue(event, nextClusterQueue, nMatching2);

    if (nMatching1 > nMatching2)
        cluster1->eventNo = event.eventNo;
    if (nMatching2 > nMatching1)
        cluster2->eventNo = event.eventNo;
}

std::vector<nmx::fullCluster>::iterator NMXClustererVerification::compareToQueue(const nmx::fullCluster &event,
                                                                                 std::vector<nmx::fullCluster> &clusterQueue,
                                                                                 unsigned int &nMatches) {

    std::vector<nmx::fullCluster>::iterator bestCluster;
    unsigned int maxMatching = 0;

    std::vector<nmx::fullCluster>::iterator clusterIt = clusterQueue.begin();

    while (clusterIt != clusterQueue.end()) {

        nmx::fullCluster cluster = *clusterIt;

        unsigned int nMatching = numberOfMatchingPoints(event, cluster);

        std::cout << "Number of maching points found = " << nMatching << std::endl;

        if (nMatching > maxMatching) {
            maxMatching = nMatching;
            bestCluster = clusterIt;
        }
    }

    nMatches = maxMatching;
    return bestCluster;
}

int NMXClustererVerification::numberOfMatchingPoints(const nmx::fullCluster &event, const nmx::fullCluster &cluster) {

    int nMatches = 0;

    nMatches += numberOfMatchingPointsPlane(event.clusters.at(0), cluster.clusters.at(0));
    nMatches += numberOfMatchingPointsPlane(event.clusters.at(1), cluster.clusters.at(1));

    return nMatches;
}

int NMXClustererVerification::numberOfMatchingPointsPlane(const nmx::cluster &event, const nmx::cluster &cluster) {

    int nMatches = 0;

    for (unsigned int ievent = 0; ievent < event.npoints; ievent++) {

        const nmx::data_point &evPoint = event.data.at(ievent);

        for (unsigned int icluster = 0; icluster < cluster.npoints; icluster++) {

            const nmx::data_point &clPoint = cluster.data.at(icluster);

            if (pointsMatch(evPoint, clPoint))
                nMatches++;
        }
    }

    return nMatches;
}

bool NMXClustererVerification::pointsMatch(const nmx::data_point &p1, const nmx::data_point &p2) {

    if ((p1.strip == p2.strip) && (p1.time == p2.time) && (p1.charge == p2.charge))
        return true;

    return false;
}

inline int NMXClustererVerification::getTotalPoints(const nmx::fullCluster &object) {

    int totalPoints = object.clusters.at(0).npoints;
    totalPoints += object.clusters.at(1).npoints;

    return totalPoints;
}

void NMXClustererVerification::writeEventToFile(unsigned int eventNo, nmx::fullCluster &event) {

    std::cout << "Writing event # " << eventNo << " to file!\n";
    m_file << "Event # " << eventNo << std::endl;
    writeObjectToFile(event);
}

void NMXClustererVerification::writeClustersToFile(unsigned int eventNo, bufferEntry &entry) {

    std::cout << "Searching for clusters which match event #" << eventNo << std::endl;

    auto it = entry.at(1).begin();

    while (it != entry.at(1).end()) {

        nmx::fullCluster &cluster = *it;

        std::cout << "Current cluster matches event # " << cluster.eventNo << std::endl;

        if (cluster.eventNo == eventNo) {
            writeObjectToFile(cluster);
            entry.at(1).erase(it);
        } else {

            std::cout << "Incrementing iterator\n";

            it++;
        }
    }
}

inline void NMXClustererVerification::writeObjectToFile(nmx::fullCluster &object) {

    std::cout << "Writing object to file\n";

    m_file << "X:\n";
    writePlaneToFile(object.clusters.at(0));
    m_file << "Y:\n";
    writePlaneToFile(object.clusters.at(1));
}

inline void NMXClustererVerification::writePlaneToFile(nmx::cluster &plane) {

    for (unsigned int i = 0; i < plane.npoints; i++)
        m_file << plane.data.at(i).strip << " ";
    m_file << "\n";
    for (unsigned int i = 0; i < plane.npoints; i++)
        m_file << plane.data.at(i).time << " ";
    m_file << "\n";
    for (unsigned int i = 0; i < plane.npoints; i++)
        m_file << plane.data.at(i).charge << " ";
    m_file << "\n";
}

inline uint32_t NMXClustererVerification::getMinorTime(uint32_t time) {

    time = time >> m_ignoreBits;
    time = time & m_minorBitmask;

    return time;
}

inline uint32_t NMXClustererVerification::getMajorTime(uint32_t time) {

    return time >> m_ignoreBits >> m_minorBits;
}

void NMXClustererVerification::endRun() {

    while ((m_In[0] != m_Out[0]) && (m_In[1] != m_Out[1]))
        std::this_thread::yield();

    slideTimeWindow(m_minorBitmask, m_minorBitmask, 0);
}

void NMXClustererVerification::reset() {

    for (unsigned int idx = 0; idx < m_maxMinor; idx++) {

        m_majortime_buffer.at(idx) = 0;
    }

    for (int i = 0; i < 2; i++) {
        m_In[i] = 0;
        m_Out[i] = 0;
    }
}

NMXClustererVerification* NMXClustererVerification::getInstance() {

    if (!instance) {
        instance = new NMXClustererVerification();
    }

    return instance;
}

NMXClustererVerification* NMXClustererVerification::instance = 0;

void NMXClustererVerification::printFullCluster(const nmx::fullCluster &cluster) {

    std::cout << "NMX-Full cluster:\n";
    std::cout << "    Associated event number : " << cluster.eventNo << std::endl;
    for (int i = 0; i < 2; i++) {
        auto &plane = cluster.clusters.at(i);
        std::cout << "    " << (i ? "Y" : "X") << " - plane :\n";
        std::cout << "        Number of points : " << plane.npoints << std::endl;
        std::cout << "        Max-time         : " << plane.box.max_time << std::endl;
    }
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