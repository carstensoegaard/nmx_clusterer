//
// Created by soegaard on 1/31/18.
//

#ifndef PROJECT_CLUSTERPAIRING_H
#define PROJECT_CLUSTERPAIRING_H

#include <fstream>
#include <thread>
#include <mutex>

#include "NMXClustererDefinitions.h"

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

    uint64_t m_ievent;

    typedef std::array<nmx::fullCluster, 100> queue;
    std::array<queue, 2> m_queue;

    unsigned int m_In[2];
    unsigned int m_Out[2];

    typedef std::array<std::vector<nmx::fullCluster>, 2> bufferEntry;

    std::array<bufferEntry, nmx::MAX_MINOR> m_time_ordered_buffer;
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

    void findMatches(bufferEntry &entry);
    void writeEventToFile(int eventNo, nmx::fullCluster &event);
    void writeClustersToFile(int eventNo, bufferEntry &entry);
    void writeObjectToFile(nmx::fullCluster &object);
    void writePlaneToFile(nmx::cluster &plane);

    // For debugging
    /*
    void printSortBuffer();
    void printQueue();

    void checkSortBuffer();
    */
};

NMXClustererVerification* NMXClustererVerification::instance = 0;

#endif //PROJECT_PAIRCLUSTERS_H
