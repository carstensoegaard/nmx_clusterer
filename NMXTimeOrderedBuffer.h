//
// Created by soegaard on 2/8/18.
//

#ifndef PROJECT_NMXTIMEORDEREDBUFFER_H
#define PROJECT_NMXTIMEORDEREDBUFFER_H

#include <stdint-gcc.h>
#include <zconf.h>
#include <thread>
#include <array>
#include "NMXClustererDefinitions.h"

template <typename T, uint32_t N>
struct buffer {
    uint32_t npoints;
    std::array<T, N>;
};

template <typename T, uint32_t N, uint32_t MINB>
using buffer_array = std::array<buffer<T, N>, 1 << MINB>;

template <typename T, std::size_t N, std::size_t IB, std::size_t MINB, std::size_t MAJB>
class NMXTimeOrderedBuffer {

public:

    NMXTimeOrderedBuffer();

    void insert(const T &entry, uint32_t time);
    void insert(const nmx::data_point &entry);

    buffer_array getNextProcessed();

private:

    uint32_t m_i1;
    uint64_t m_nSorted;
    uint64_t m_nProcessed;

    T m_entry_buffer;
    bool m_pointInserted;
    bool m_haltProcessing;
    bool m_terminate;

    const uint32_t m_ignorebits = IB;
    const uint32_t m_minorbits  = MINB;
    const uint32_t m_majorbits  = MAJB;

    const uint32_t m_minormax = 1 << m_minorbits;
    const uint32_t m_minorbitmask = m_minormax-1;

    uint32_t m_minortime;
    uint32_t m_majortime;

    std::array<int32_t, m_minormax> m_majorTimeBuffer;
    std::array<int, m_minormax> m_sortBuffer;
    std::array<int, m_minormax> m_processedBuffer;

    std::array<buffer_array<T,N,MINB>, 2> m_timeOrderedBuffer;

    void sortProcessor();
    void addToBuffer(const T &entry);
    void slideTimeWindow(int d);

    uint32_t getMinorTime(uint32_t time);
    uint32_t getMajorTime(uint32_t time);

    void init();
};


#endif //PROJECT_NMXTIMEORDEREDBUFFER_H
