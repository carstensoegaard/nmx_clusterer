//
// Created by soegaard on 2/8/18.
//

#include "NMXTimeOrderedBuffer.h"

#include "NMXPlaneClusterer.h"

NMXTimeOrderedBuffer::NMXTimeOrderedBuffer(NMXClusterManager &clusterManager, NMXClusterPairing &clusterPairing)
        : m_verbose_level(0),
          m_i1(nmx::MINOR_BITMASK),
//          m_nB(0),
//          m_nC(0),
//          m_nD(0),
          m_currentTime(0),
          m_pointProcessed(true),
          m_terminate(false),
          m_clusterManager(clusterManager),
          m_clusterPairing(clusterPairing)
{
    //checkBitSum();
    //printInitialization();

    reset();

    // Start threads
    pro = std::thread(&NMXTimeOrderedBuffer::producer, this);
}

NMXTimeOrderedBuffer::~NMXTimeOrderedBuffer()
{
    // Join threads
    pro.join();
}

void NMXTimeOrderedBuffer::insert(unsigned int plane, unsigned int idx, uint32_t time) {

    while (!m_pointProcessed)
        std::this_thread::yield();

    m_pointProcessed = false;
    m_currentPlane = plane;
    m_currentIdx = idx;
    m_currentTime = time;
}



void NMXTimeOrderedBuffer::producer() {

    std::cout << "Started sorting thread!\n";

    while(1) {

        while (m_pointProcessed) {
            if (m_terminate) {
                std::cout << "Stopped sorting thread!\n";
                return;
            }
            std::this_thread::yield();
        }

        uint minorTime = getMinorTime(m_currentTime);
        uint majorTime = getMajorTime(m_currentTime);

        if (m_verbose_level > 1) {
            std::cout << "Idx = " << m_currentIdx << ", B1 = " << minorTime << ", B2 = " << majorTime << ", B2_buffer["
                      << minorTime << "] = " << m_majortime_buffer[minorTime] << " B2_buffer[0] = "
                      << m_majortime_buffer.at(0) << " i1 = " << m_i1 << std::endl;
        }

        if (majorTime >= (m_majortime_buffer.at(0) + 1)) {

            if (majorTime == (m_majortime_buffer.at(0) + 1)) {

                if (m_verbose_level > 0)
                    std::cout << "Case 1\n";

                slideTimeWindow(nmx::MINOR_BITMASK - m_i1 + std::min(m_i1, minorTime) + 1, minorTime, majorTime);
                addToBuffer(m_currentIdx, minorTime);

            } else { // majorTime > (m_majortime_buffer.at(0) + 1)

                if (m_verbose_level > 0)
                    std::cout << "Case 2\n";

                slideTimeWindow(nmx::MINOR_BITMASK + 1, minorTime, majorTime);
                addToBuffer(m_currentIdx, minorTime);
            }

        } else { // majorTime <= m_buffer.at(0)

            switch (majorTime - m_majortime_buffer.at(minorTime)) {

                case 1:

                    if (m_verbose_level > 0)
                        std::cout << "Case 3\n";

                    slideTimeWindow(minorTime - m_i1, minorTime, majorTime);
                    addToBuffer(m_currentIdx, minorTime);

                    break;

                case 0:

                    if (m_verbose_level > 0)
                        std::cout << "Case 4\n";

                    addToBuffer(m_currentIdx, minorTime);

                    break;

                default:

                    std::cout << "Old data-point - omitting!\n";
                    m_clusterManager.returnClusterToStack(m_currentPlane, m_currentIdx);
            }
        }

        m_pointProcessed = true;
    }
}

nmx::cluster_queue NMXTimeOrderedBuffer::getNextSorted() {

    while ((m_nB <= m_nC) || m_nD) {
        std::this_thread::yield();
    }

    uint idx = m_nC % nmx::MAX_MINOR;

    nmx::cluster_queue &buf = m_time_ordered_buffer.at(m_ClusterQ[idx]).at(idx);

    nmx::cluster_queue ret_buf = buf;

    buf.at(0) = -1;
    buf.at(1) = -1;
    //m_nC++;

    return ret_buf;
}

void NMXTimeOrderedBuffer::addToBuffer(int idx, uint minorTime) {

   // uint32_t i0 = m_SortQ[minorTime];

    nmx::cluster_queue &queue = m_time_ordered_buffer.at(minorTime);

    std::cout << "<NMXTimeOrderedBuffer::addToBuffer> Cluster " << idx << " from plane "
              << m_currentPlane  << " will be inserted at " << minorTime
              << ", queue[0] = " << queue.at(0) << ", queue[1] = " << queue.at(1) << ", link1 = "
              << m_clusterManager.getCluster(m_currentPlane, idx).box.link1 << ", link2 = "
              << m_clusterManager.getCluster(m_currentPlane, idx).box.link2 << std::endl;

    if (queue.at(m_currentPlane) == -1) {
        queue.at(m_currentPlane) = idx;
        std::cout << "First cluster!\n";
    } else {

        bool cont = true;

        while (cont) {

            std::cout << "Inserting in queue\n";

            nmx::cluster &nextCluster = m_clusterManager.getCluster(m_currentPlane, idx);

            int nextIdx = nextCluster.box.link1;

            if (nextIdx == -1) {

                nextCluster.box.link1 = idx;
                cont = false;
            }
        }
    }
    nmx::printQueue(m_currentPlane, queue.at(m_currentPlane), m_clusterManager);
}

void NMXTimeOrderedBuffer::slideTimeWindow(uint d, uint minorTime, uint majorTime) {

    //if (!nmx::checkD(d, "NMXTimeOrderedBuffer::moveToClusterer"))
      //  throw 1;

    if (m_verbose_level > 1) {
        std::string s("\nWill move ");
        s.append(std::to_string(d));
        s.append(" entries to clusterer !\n");
        std::cout << s;
    }


    for (uint i = 0; i < d; ++i) {

        uint idx = (i+m_i1)%nmx::MAX_MINOR;

        if (m_verbose_level > 1)
            std::cout << "Moving cluster with idx " << idx << std::endl;

        m_clusterPairing.

        if (idx <= minorTime)
            m_majortime_buffer.at(idx) = majorTime;
        else
            m_majortime_buffer.at(idx) = majorTime -1;
    }

    if (m_verbose_level > 1)
        std::cout << "Setting i1 to " << minorTime << std::endl;

    m_i1 = minorTime;
}


void NMXTimeOrderedBuffer::endRun() {

    bool verbose = false;

    if (verbose)
        std::cout << "END of run - flushing time-ordered buffer ...\n";

    while (m_nB != m_nC) {
        if (verbose)
            std::cout << "nB = " << m_nB << " != " << " nC = " << m_nC << std::endl;
        std::this_thread::yield();
    }

    slideTimeWindow(nmx::MINOR_BITMASK, nmx::MINOR_BITMASK, 0);

    while (m_nB != m_nC) {
        if (verbose)
            std::cout << "nB = " << m_nB << " != " << " nC = " << m_nC << std::endl;
        std::this_thread::yield();
    }
}

inline uint32_t NMXTimeOrderedBuffer::getMinorTime(uint32_t time) {

    time = time >> nmx::IGNORE_BITS;
    time = time & nmx::MINOR_BITMASK;

    return time;
}

inline uint32_t NMXTimeOrderedBuffer::getMajorTime(uint32_t time) {

    return time >> nmx::IGNORE_BITS >> nmx::MINOR_BITS;
}

void NMXTimeOrderedBuffer::reset() {

    // Reset time-ordered buffer
    for (uint index0 = 0; index0 < 2; index0++) {

        std::array<nmx::cluster_queue, nmx::MAX_MINOR> &buf = m_time_ordered_buffer.at(index0);

        for (uint index1 = 0; index1 < nmx::MAX_MINOR; index1++) {

            auto &queue = buf.at(index1);

            queue.at(0) = -1;
            queue.at(1) = -1;
        }
    }

    // Reset major-time buffer
    for (uint i = 0; i < nmx::MAX_MINOR; ++i) {
        m_majortime_buffer.at(i) = 0;
        m_SortQ.at(i) = 0;
        m_ClusterQ.at(i) = 1;
    }
}

void NMXTimeOrderedBuffer::guardB() {

    while (m_nB != m_nC/*m_nB - m_nC > nmx::MINOR_BITMASK*/) {

        //std::this_thread::sleep_for(std::chrono::microseconds(1));
        std::this_thread::yield();
    }
}

void NMXTimeOrderedBuffer::checkBitSum() {

    if (nmx::IGNORE_BITS + nmx::MINOR_BITS + nmx::MAJOR_BITS != 32) {
        std::cout << std::setfill('*') << std::setw(40) << "*" << std::endl;
        std::cout << std::setfill('*') << std::setw(40) << "*" << std::endl;
        std::cout << "*" << std::setfill(' ') << std::setw(38) << " " << "*\n";
        std::cout << "*" << std::setw(4) << " " << "Sum of NBITSx does not equal 32" << std::setw(3) << " " << "*\n";
        std::cout << "*" << std::setw(2) << " " << "Cluster can not run - must be fixed" << std::setw(1) << " " << "*\n";
        std::cout << "*" << std::setfill(' ') << std::setw(38) << " " << "*\n";
        std::cout << std::setfill('*') << std::setw(40) << "*" << std::endl;
        std::cout << std::setfill('*') << std::setw(40) << "*" << std::endl;

        throw 1;
    }
}

void NMXTimeOrderedBuffer::printInitialization() {

    std::cout << "\n\nInitialising NMX-clusterer with the following parameters:\n\n";
    std::cout << "Number of strips :         " << std::setw(10) << nmx::STRIPS_PER_PLANE << std::endl;
    std::cout << "Bits to ignore :           " << std::setw(10) << nmx::IGNORE_BITS << std::endl;
    std::cout << "Minor bits :               " << std::setw(10) << nmx::MINOR_BITS << std::endl;
    std::cout << "Major bits :               " << std::setw(10) << nmx::MAJOR_BITS << std::endl;
    std::cout << "Neighbours to include :    " << std::setw(10) << nmx::INCLUDE_N_NEIGHBOURS << std::endl;
    std::cout << "Maximum time for cluster : " << std::setw(10) << nmx::MAX_CLUSTER_TIME << std::endl;
    std::cout << "Number of boxes :          " << std::setw(10) << nmx::NBOXES << std::endl;
    std::cout << "\n";
    std::cout << "Max IGNORE :               " << std::setw(10) << nmx::MAX_IGNORE << std::endl;
    std::cout << "Max MINOR :                " << std::setw(10) << nmx::MAX_MINOR << std::endl;
    std::cout << "Max MAJOR :                " << std::setw(10) << nmx::MAX_MAJOR << std::endl;
    std::cout << "IGNORE bit-mask :          " << std::setw(10) << nmx::IGNORE_BITMASK << std::endl;
    std::cout << "MINOR bit-mask :           " << std::setw(10) << nmx::MINOR_BITMASK << std::endl;
    std::cout << "MAJOR bit-mask :           " << std::setw(10) << nmx::MAJOR_BITMASK << std::endl;
    std::cout << "\n";
    std::cout << "Verbosity level :          " << std::setw(10) << m_verbose_level << std::endl;
    std::cout << "\n";
}





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

