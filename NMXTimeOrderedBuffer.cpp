//
// Created by soegaard on 2/8/18.
//

#include "NMXTimeOrderedBuffer.h"

NMXTimeOrderedBuffer::NMXTimeOrderedBuffer()
        : m_i1(m_minorbitmask),
          m_nSorted(0),
          m_nProcessed(0),
          m_pointInserted(false),
          m_terminate(false)
{
    init();
}

template <typename T>
void NMXTimeOrderedBuffer::insert(T const &entry, uint32_t time) {

    while (m_pointInserted)
        std::this_thread::yield();

    m_pointInserted = true;
    m_minortime = getMinorTime(time);
    m_majortime = getMajorTime(time);
    m_entry_buffer = entry;
}

void NMXTimeOrderedBuffer::insert(const nmx::data_point &entry) {

    insert(entry, entry.time);
}

template <typename T>
void NMXTimeOrderedBuffer::sortProcessor() {

    while (1) {

        while (!m_pointInserted) {
            if (m_terminate)
                return;
            std::this_thread::yield();
        }

        uint d = 0;

        if (m_majortime >= (m_majorTimeBuffer.at(0) + 1)) {

            if (m_majortime == (m_majorTimeBuffer.at(0) + 1)) {

                d = m_minorbitmask - m_i1 + std::min(m_i1, m_minortime) + 1

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

template <typename T>
void NMXTimeOrderedBuffer::addToBuffer(const T &entry) {

    uint32_t i0 = m_sortBuffer[m_minortime];

    buffer_array &tobuf = m_timeOrderedBuffer.at(i0);
    buffer &buf = tobuf.at(m_minortime);
    auto &data = buf.data;

    data.at(buf.npoints) = entry;
    buf.npoints++;
}

void NMXTimeOrderedBuffer::slideTimeWindow(int d) {

    while (m_nSorted - m_nProcessed > m_minorbitmask - d)
        std::this_thread::yield();

    for (uint i = 0; i < d; ++i) {

        uint64_t idx = (i+m_nSorted)%m_minormax;

        if (m_timeOrderedBuffer.at(m_processedBuffer.at(idx)).at(idx).npoints > 0)
            throw "<NMXTimeOrderedBuffer> Processed buffer not empty before new input!\n";

        m_sortBuffer.at(idx)      = m_processedBuffer.at(idx);
        m_processedBuffer.at(idx) = !m_sortBuffer.at(idx);

        if (idx <= m_minortime)
            m_majorTimeBuffer.at(idx) = m_majortime;
        else
            m_majorTimeBuffer.at(idx) = m_majortime-1;
    }

    m_nSorted += d;

    m_i1 = m_minortime;
}

uint NMXTimeOrderedBuffer::nPointsAt(uint majoridx, uint32_t time) {

    buffer_array &tobuf = m_timeOrderedBuffer.at(majoridx);
    buffer &buf = tobuf.at(getMinorTime(time));

    return buf.npoints;
}

template <typename T>
T NMXTimeOrderedBuffer::getEntry(uint majoridx, uint32_t time, uint idx) {

    time_ordered_buffer &tobuf = m_majorbuffer.at(majoridx);
    buffer &buf = tobuf.at(getMinorTime(time));

    return buf.data.at(idx);
}

void NMXTimeOrderedBuffer::resetAt(uint majoridx, uint32_t time) {

    time_ordered_buffer &tobuf = m_majorbuffer.at(majoridx);
    buffer &buf = tobuf.at(getMinorTime(time));

    buf.npoints = 0;
}

inline uint32_t NMXTimeOrderedBuffer::getMinorTime(uint32_t time) {

    time = time >> m_ignorebits;
    time = time & m_minorbitmask;

    return time;
}

inline uint32_t NMXTimeOrderedBuffer::getMajorTime(uint32_t time) {

    return time >> m_ignorebits >> m_minorbits;
}

void NMXTimeOrderedBuffer::init() {

    for (uint idx = 0; idx < 2; idx++) {

        buffer_array &buf_arr = m_timeOrderedBuffer.at(idx);

        for (uint minoridx = 0; minoridx < m_minormax; minoridx++) {
            buffer &buf = buf_arr.at(minoridx);

            buf.npoints = 0;
        }
    }

    for (uint i = 0; i < m_minormax; ++i) {
        m_majorTimeBuffer.at(i) = 0;
        m_sortBuffer.at(i) = 0;
        m_processedBuffer.at(i) = 1;
    }
}

