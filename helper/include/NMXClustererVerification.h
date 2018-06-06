//
// Created by soegaard on 1/31/18.
//

#ifndef NMX_CLUSTERER_VERIFICATION_H
#define NMX_CLUSTERER_VERIFICATION_H

#include <vector>
#include <fstream>
#include <thread>
#include <mutex>

#include "../../clusterer/include/NMXClustererDefinitions.h"

class NMXClustererVerification {

private:
    NMXClustererVerification();

public:
    static NMXClustererVerification* getInstance();
    ~NMXClustererVerification();

    void insertEventInQueue(const nmx::FullCluster &event);
    void insertClusterInQueue(const nmx::FullCluster &cluster);

    void endRun();
    void reset();
    void terminate() { m_terminate = true; }

private:

    static const uint32_t m_ignoreBits = 10;
    static const uint32_t m_minorBits  = 6;
    static const uint32_t m_majorBits  = 32 - m_ignoreBits - m_minorBits;

    static const uint32_t m_maxIgnore = 1 << m_ignoreBits;
    static const uint32_t m_maxMinor  = 1 << m_minorBits;
    static const uint32_t m_maxMajor  = 1 << m_majorBits;

    static const uint32_t m_ignoreBitmask = m_maxIgnore - 1;
    static const uint32_t m_minorBitmask  = m_maxMinor  - 1;
    static const uint32_t m_majorBitmask  = m_maxMajor  - 1;

    uint64_t m_ievent;

    typedef std::array<nmx::FullCluster, 100> queue;
    std::array<queue, 2> m_queue;

    unsigned int m_In[2];
    unsigned int m_Out[2];

    typedef std::array<std::vector<nmx::FullCluster>, 2> bufferEntry;

    std::array<bufferEntry, m_maxMinor> m_time_ordered_buffer;
    nmx::dataColumn_t m_majortime_buffer;
    uint32_t m_i1;

    std::ofstream m_file;

    unsigned int m_verbose_level;
    bool m_terminate;

    void process();
    std::thread t_process;
    std::mutex m_mutex;

    static NMXClustererVerification *instance;

    uint32_t getMinorTime(uint32_t time);
    uint32_t getMajorTime(uint32_t time);

    void addToBuffer(unsigned int shifter, nmx::FullCluster &object, uint minorTime);
    void slideTimeWindow(uint d, uint minorTime, uint majorTime);

    void findMatches(bufferEntry &thisEntry, bufferEntry &nextEntry);
    void compareToClusters(const nmx::FullCluster &event,
                           std::vector<nmx::FullCluster> &thisClusterQueue,
                           std::vector<nmx::FullCluster> &nextClusterQueue);
    std::vector<nmx::FullCluster>::iterator compareToQueue(const nmx::FullCluster &event,
                                                                  std::vector<nmx::FullCluster> &clusterQueue,
                                                                  unsigned int &nMatches);
    int numberOfMatchingPoints(const nmx::FullCluster &event, const nmx::FullCluster &cluster);
    int numberOfMatchingPointsPlane(const nmx::Cluster &event, const nmx::Cluster &cluster);
    bool pointsMatch(const nmx::DataPoint &p1, const nmx::DataPoint &p2);

    int getTotalPoints(const nmx::FullCluster &object);

    void writeEventToFile(unsigned int eventNo, nmx::FullCluster &event);
    void writeClustersToFile(unsigned int eventNo, bufferEntry &entry);
    void writeObjectToFile(nmx::FullCluster &object);
    void writePlaneToFile(nmx::Cluster &plane);

    // For debugging

    void printFullCluster(const nmx::FullCluster &cluster);

    /*
    void printSortBuffer();
    void printQueue();

    void checkSortBuffer();
    */
};

#endif //NMX_CLUSTERER_VERIFICATION_H
