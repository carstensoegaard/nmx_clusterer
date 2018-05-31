//
// Created by soegaard on 2/8/18.
//

#ifndef PROJECT_NMXTIMEORDEREDBUFFER_H
#define PROJECT_NMXTIMEORDEREDBUFFER_H

#include <vector>
#include <thread>
#include <mutex>

#include "clusterer/include/NMXClustererDefinitions.h"
#include "NMXClustererHelper.h"
#include "NMXClusterManager.h"
#include "NMXClusterPairing.h"

class NMXTimeOrderedBuffer {

public:

    NMXTimeOrderedBuffer(NMXClusterManager &clusterManager, NMXClusterPairing &clusterPairing);
    ~NMXTimeOrderedBuffer();

    void insert(unsigned int plane, unsigned int idx, uint32_t time);
    nmx::cluster_queue getNextSorted();

    void endRun();
    void terminate() { m_terminate = true; }

    void setVerboseLevel(uint level = 0) { m_verbose_level = level; }


private:

    uint m_verbose_level;

    uint m_i1;

    unsigned int m_currentPlane;
    unsigned int m_currentIdx;
    uint32_t m_currentTime;
    bool m_pointProcessed;
    bool m_terminate;

    std::thread pro;

    void producer();

    NMXClusterManager &m_clusterManager;
    NMXClusterPairing &m_clusterPairing;

    nmx::row_array m_majortime_buffer;

    std::array<nmx::cluster_queue, nmx::MAX_MINOR> m_time_ordered_buffer;

    uint32_t getMinorTime(uint32_t time);
    uint32_t getMajorTime(uint32_t time);

    void addToBuffer(int idx, uint minorTime);
    void slideTimeWindow(uint d, uint minorTime, uint majorTime);

    void reset();

    void checkBitSum();
    void printInitialization();
};








/*
#define N 333
#define IB 5
#define MINB 7
#define MAJB 20


#include <iomanip>
#include <stdint-gcc.h>
#include <zconf.h>
#include <thread>
#include <array>
#include <iostream>
#include "NMXClustererDefinitions.h"
*/
/*
template <typename T>
struct buffer {
    uint32_t npoints;
    std::array<T, N> data;
};

template <typename T>
using buffer_array = std::array<buffer<T>, 1 << MINB>;
*/
/*
template <typename T>
class NMXTimeOrderedBuffer {

public:

    NMXTimeOrderedBuffer();
    ~NMXTimeOrderedBuffer();

    void insert(const T &entry, uint32_t time);
    //void insert(const nmx::data_point &entry);

    buffer<T> getNextProcessed();

private:

    uint32_t m_i1;
    uint64_t m_nSorted;
    uint64_t m_nProcessed;

    T m_entry_buffer;
    bool m_pointInserted;
    bool m_haltProcessing;
    bool m_terminate;

    std::thread m_sortThread;

    static const uint32_t m_ignorebits = IB;
    static const uint32_t m_minorbits  = MINB;
    static const uint32_t m_majorbits  = MAJB;

    static const uint32_t m_minormax = 1 << m_minorbits;
    static const uint32_t m_minorbitmask = m_minormax-1;

    uint32_t m_minortime;
    uint32_t m_majortime;

    std::array<int32_t, m_minormax> m_majorTimeBuffer;
    std::array<int, m_minormax> m_sortBuffer;
    std::array<int, m_minormax> m_processedBuffer;

    std::array<buffer_array<T>, 2> m_timeOrderedBuffer;

    void sortProcessor();
    void addToBuffer(const T &entry);
    void slideTimeWindow(int d);

    uint32_t getMinorTime(uint32_t time);
    uint32_t getMajorTime(uint32_t time);

    void init();

    void printSortBuffer();
    void printSortProcessedIndexes();
};
*/
/*
template <typename T>
NMXTimeOrderedBuffer<T>::NMXTimeOrderedBuffer()
        : m_i1(m_minorbitmask),
          m_nSorted(0),
          m_nProcessed(0),
          m_pointInserted(false),
          m_terminate(false)
{
    init();

    m_sortThread = std::thread(&NMXTimeOrderedBuffer::sortProcessor, this);
}
*/
/*
template <typename T>
void NMXTimeOrderedBuffer<T>::insert(T const &entry, uint32_t time) {

    while (m_pointInserted)
        std::this_thread::yield();

    int minortime = getMinorTime(time);
    int majortime = getMajorTime(time);

    std::cout << "<NMXTimeOrderedBuffer<" << typeid(T).name() << ">> inserted " << entry << ", at b2 = " << majortime
              << ", idx = " << minortime << std::endl;

    m_pointInserted = true;
    m_minortime = minortime;
    m_majortime = majortime;
    m_entry_buffer = entry;
}
*/
/*
template <typename T>
NMXTimeOrderedBuffer<T>::~NMXTimeOrderedBuffer()
{
    m_sortThread.join();
}
*/
/*
template <typename T>
void NMXTimeOrderedBuffer<T>::sortProcessor() {

    std::cout << "Started time-ordered buffer\n";

    while (1) {

        while (!m_pointInserted) {
            if (m_terminate)
                return;
            std::this_thread::yield();
        }

        int d = 0;

        if (m_majortime >= (m_majorTimeBuffer.at(0) + 1)) {

            if (m_majortime == (m_majorTimeBuffer.at(0) + 1)) {

                d = m_minorbitmask - m_i1 + std::min(m_i1, m_minortime) + 1;

            } else { // majorTime > (m_majortime_buffer.at(0) + 1)

                d = m_minormax;

            }

        } else { // majorTime <= m_buffer.at(0)

            switch (m_majortime - m_majorTimeBuffer.at(m_minortime)) {

                case 1:

                    d = m_minortime - m_i1;
                    break;

                case 0:

                    d = 0;
                    break;

                default:

                    d = -1;
            }
        }

        if (d > 0)
            slideTimeWindow(d);
        if (d >= 0)
            addToBuffer(m_entry_buffer);
        if (d == m_minormax) {
            while (m_nSorted != m_nProcessed)
                std::this_thread::yield();

            m_haltProcessing = true;
            m_nProcessed = m_minortime + 1;
            m_nSorted = m_minortime + 1;
            m_haltProcessing = false;
        }

        m_pointInserted = false;
    }
}
*/
/*

template <typename T>
buffer<T> NMXTimeOrderedBuffer<T>::getNextProcessed() {

    std::cout << "nSorted = " << m_nSorted << ", nProcessed = " << m_nProcessed << std::endl;

    while ((m_nSorted <= m_nProcessed) || m_haltProcessing)
        std::this_thread::yield();


    uint idx = m_nProcessed % m_minormax;

    uint i0 = m_processedBuffer[idx];

    buffer_array<T> &buf_arr = m_timeOrderedBuffer.at(i0);
    buffer<T> buf_temp = buf_arr.at(idx);



    //if (buf_temp.npoints != 0) {
        std::cout << "Providing the following indexes from [" << i0 << ", " << idx << "]\n";
        for (int i = 0; i < buf_temp.npoints; i++)
            std::cout << buf_temp.data.at(i) << " ";
        std::cout << "\n";
    //}

    buf_arr.at(idx).npoints = 0;

    //buf.npoints = 0;

    m_nProcessed++;

    return buf_temp;
}

*/
/*
template <typename T>
inline uint32_t NMXTimeOrderedBuffer<T>::getMinorTime(uint32_t time) {

    time = time >> m_ignorebits;
    time = time & m_minorbitmask;

    return time;
}
*/
/*
template <typename T>
inline uint32_t NMXTimeOrderedBuffer<T>::getMajorTime(uint32_t time) {

    return time >> m_ignorebits >> m_minorbits;
}
*/
/*
template <typename T>
void NMXTimeOrderedBuffer<T>::init() {

    for (uint idx = 0; idx < 2; idx++) {

        buffer_array<T> &buf_arr = m_timeOrderedBuffer.at(idx);

        for (uint minoridx = 0; minoridx < m_minormax; minoridx++) {
            buffer<T> &buf = buf_arr.at(minoridx);

            buf.npoints = 0;
        }
    }

    for (uint i = 0; i < m_minormax; ++i) {
        m_majorTimeBuffer.at(i) = 0;
        m_sortBuffer.at(i) = 0;
        m_processedBuffer.at(i) = 1;
    }
}
*/
/*
template <typename T>
void NMXTimeOrderedBuffer<T>::addToBuffer(const T &entry) {

    uint32_t i0 = m_sortBuffer[m_minortime];

    buffer_array<T> &tobuf = m_timeOrderedBuffer.at(i0);
    buffer<T> &buf = tobuf.at(m_minortime);
    auto &data = buf.data;

    data.at(buf.npoints) = entry;
    buf.npoints++;
}
*/
/*
template <typename T>
void NMXTimeOrderedBuffer<T>::slideTimeWindow(int d) {

    std::cout << "Sliding timewindow " << d << " steps\n";
    std::cout << "nSorted = " << m_nSorted << ", nProcessed = " << m_nProcessed << ", minormax = "
              << m_minormax << ", d = " << d << std::endl;

    printSortBuffer();
    printSortProcessedIndexes();

    while (m_nSorted != m_nProcessed)
        std::this_thread::yield();


    for (uint i = 0; i < d; ++i) {

        uint64_t idx = (i+m_nSorted)%m_minormax;

        int i0 = m_processedBuffer.at(idx);

        buffer_array<T> &bufarr = m_timeOrderedBuffer.at(i0);
        buffer<T> &buf = bufarr.at(idx);

        if (buf.npoints > 0) {
            std::cout << "<NMXTimeOrderedBuffer> Processed buffer at idx = [" << i0 << ", " << idx
                      << "] not empty before new input!\n";
            throw 1;
        }

        m_sortBuffer.at(idx)      = m_processedBuffer.at(idx);
        m_processedBuffer.at(idx) = !m_sortBuffer.at(idx);

        if (idx <= m_minortime)
            m_majorTimeBuffer.at(idx) = m_majortime;
        else
            m_majorTimeBuffer.at(idx) = m_majortime-1;
    }

    m_nSorted += d;

    m_i1 = m_minortime;

    printSortProcessedIndexes();
}
*/
/*
template <typename T>
void NMXTimeOrderedBuffer<T>::printSortBuffer() {

    std::cout << "<NMXTimeOrderedBuffer<" << typeid(T).name() << ">::printSortBuffer> Time ordered buffer :\n";

    for (uint idx = 0; idx < nmx::MAX_MINOR; idx++) {

        auto tbuf = m_timeOrderedBuffer.at(m_sortBuffer.at(idx));
        auto buf = tbuf.at(idx);

        if (buf.npoints == 0)
            continue;

        std::cout << "Index " << idx << std::endl;

        std::cout << "Strip  ";
        for (uint ientry = 0; ientry < buf.npoints; ientry++) {

            auto point = buf.data.at(ientry);
            std::cout << std::setw(5) << point;
        }
        std::cout << "\n";
    }
}
*/
/*
template <typename T>
void NMXTimeOrderedBuffer<T>::printSortProcessedIndexes() {

    for (int i = 0; i < m_minormax; i++) {
        if (i % 64 == 0)
            std::cout << "\nIndex = " << std::setw(5) << i << " : ";
        std::cout << m_sortBuffer.at(i);
    }
    for (int i = 0; i < m_minormax; i++) {
        if (i % 64 == 0)
            std::cout << "\nIndex = " << std::setw(5) << i << " : ";
        std::cout << m_processedBuffer.at(i);
    }
    std::cout << "\n";

}
*/
#endif //PROJECT_NMXTIMEORDEREDBUFFER_H
