//
// Created by soegaard on 1/31/18.
//

#ifndef NMX_CLUSTERER_VERIFICATION_H
#define NMX_CLUSTERER_VERIFICATION_H

#include <vector>
#include <fstream>
#include <thread>
#include <mutex>

#include "clusterer/include/NMXClustererDefinitions.h"

class NMXClustererVerification {

private:
    NMXClustererVerification();

public:
    static NMXClustererVerification* getInstance();
    ~NMXClustererVerification();

    void insertEventInQueue(const nmx::fullCluster &event);
    void insertClusterInQueue(const nmx::fullCluster &cluster);

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

    typedef std::array<nmx::fullCluster, 100> queue;
    std::array<queue, 2> m_queue;

    unsigned int m_In[2];
    unsigned int m_Out[2];

    typedef std::array<std::vector<nmx::fullCluster>, 2> bufferEntry;

    std::array<bufferEntry, m_maxMinor> m_time_ordered_buffer;
    nmx::row_array m_majortime_buffer;
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

    void addToBuffer(unsigned int shifter, nmx::fullCluster &object, uint minorTime);
    void slideTimeWindow(uint d, uint minorTime, uint majorTime);

    void findMatches(bufferEntry &thisEntry, bufferEntry &nextEntry);
    void compareToClusters(const nmx::fullCluster &event,
                           std::vector<nmx::fullCluster> &thisClusterQueue,
                           std::vector<nmx::fullCluster> &nextClusterQueue);
    std::vector<nmx::fullCluster>::iterator compareToQueue(const nmx::fullCluster &event,
                                                                  std::vector<nmx::fullCluster> &clusterQueue,
                                                                  unsigned int &nMatches);
    int numberOfMatchingPoints(const nmx::fullCluster &event, const nmx::fullCluster &cluster);
    int numberOfMatchingPointsPlane(const nmx::cluster &event, const nmx::cluster &cluster);
    bool pointsMatch(const nmx::data_point &p1, const nmx::data_point &p2);

    int getTotalPoints(const nmx::fullCluster &object);

    void writeEventToFile(unsigned int eventNo, nmx::fullCluster &event);
    void writeClustersToFile(unsigned int eventNo, bufferEntry &entry);
    void writeObjectToFile(nmx::fullCluster &object);
    void writePlaneToFile(nmx::cluster &plane);

    // For debugging

    void printFullCluster(const nmx::fullCluster &cluster);

    /*
    void printSortBuffer();
    void printQueue();

    void checkSortBuffer();
    */
};

#endif //NMX_CLUSTERER_VERIFICATION_H
