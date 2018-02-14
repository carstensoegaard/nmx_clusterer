//
// Created by soegaard on 2/8/18.
//

#include "NMXTimeOrderedBuffer.h"





/*template <nmx::data_point>
void NMXTimeOrderedBuffer<nmx::data_point>::insert(const nmx::data_point &entry) {

    while (m_pointInserted)
        std::this_thread::yield();

    m_pointInserted = true;
    m_minortime = getMinorTime(time);
    m_majortime = getMajorTime(time);
    m_entry_buffer = entry;
}*/






/*
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
}*/

