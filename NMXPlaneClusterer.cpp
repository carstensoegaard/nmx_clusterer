//
// Created by soegaard on 10/24/17.
//

#include <iostream>
#include <limits>
#include <iomanip>

#include "NMXPlaneClusterer.h"

NMXPlaneClusterer::NMXPlaneClusterer(NMXClusterManager &clusterManager, NMXClusterPairing &clusterPairing,
                                     std::mutex &mutex)
    : m_verbose_level(0),
      m_i1(nmx::MINOR_BITMASK),
      m_nB(0),
      m_nC(0),
      m_nD(0),
      m_latestClusterTime(0),
      //m_nInserted(0),
      m_plane(-1),
      m_new_point(false),
      m_clusterManager(clusterManager),
      m_clusterParing(clusterPairing),
      m_mutex(mutex),
      m_terminate(false)
{
    checkBitSum();
    printInitialization();

    reset();

    // Start threads
    pro = std::thread(&NMXPlaneClusterer::producer, this);
    con = std::thread(&NMXPlaneClusterer::consumer, this);
}

NMXPlaneClusterer::~NMXPlaneClusterer()
{
    // Join threads
    pro.join();
    con.join();
}

bool NMXPlaneClusterer::addDataPoint(uint32_t strip, uint32_t time, uint32_t charge) {

    nmx::data_point point = {strip, charge, time};

    return addDataPoint(point);
}

bool NMXPlaneClusterer::addDataPoint(const nmx::data_point &point) {

    if (!nmx::checkPoint(point, "NMXPlaneClusterer::addDataPoint"))
        return false;

    while (m_new_point)
        std::this_thread::yield();

    m_point_buffer = point;
    m_new_point = true;
}

void NMXPlaneClusterer::producer() {

    std::cout << "Started sorting thread!\n";

    while(1) {

        while (!m_new_point) {
            if (m_terminate) {
                std::cout << "Stopped sorting thread!\n";
                return;
            }
            std::this_thread::yield();
        }

        uint minorTime = getMinorTime(m_point_buffer.time);
        uint majorTime = getMajorTime(m_point_buffer.time);

        if (m_verbose_level > 1) {
            nmx::printPoint(m_point_buffer);
            std::cout << "B1 = " << minorTime << ", B2 = " << majorTime << ", B2_buffer[" << minorTime << "] = "
                      << m_majortime_buffer[minorTime] << " B2_buffer[0] = " << m_majortime_buffer.at(0)
                      << " i1 = " << m_i1 << std::endl;
        }

        if (majorTime >= (m_majortime_buffer.at(0) + 1)) {

            if (majorTime == (m_majortime_buffer.at(0) + 1) && minorTime <= m_i1) {

                if (m_verbose_level > 0) {
                    std::cout << "Case 1\n";
                    std::cout << "Moving " << nmx::MINOR_BITMASK - m_i1 + std::min(m_i1, minorTime) +1 << " points\n";
                }

                moveToClusterer(nmx::MINOR_BITMASK - m_i1 + std::min(m_i1, minorTime) + 1, minorTime, majorTime);
                addToBuffer(m_point_buffer, minorTime);

            } else { // majorTime > (m_majortime_buffer.at(0) + 1)

                if (m_verbose_level > 0) {
                    std::cout << "Case 2\n";
                    std::cout << "Moving " << nmx::MINOR_BITMASK << " points\n";
                }

                moveToClusterer(nmx::MINOR_BITMASK+1, minorTime, majorTime);
                addToBuffer(m_point_buffer, minorTime);

                guardB();
                m_nD = 1;
                m_nC = minorTime+1;
                m_nB = minorTime+1;
                m_nD = 0;
            }

        } else { // majorTime <= m_buffer.at(0)

            switch (majorTime - m_majortime_buffer.at(minorTime)) {

                case 1:

                    if (m_verbose_level > 0) {
                        std::cout << "Case 3\n";
                        std::cout << "nB = " << m_nB << ", nC = " << m_nC << std::endl;
                        std::cout << "Moving " << minorTime - m_i1 << " points\n";
                        std::cout << "Minor-time = " << minorTime << ", i1 = " << m_i1 << std::endl;
                    }

                    //guardB();
                    moveToClusterer(minorTime - m_i1, minorTime, majorTime);
                    addToBuffer(m_point_buffer, minorTime);

                    break;

                case 0:

                    if (m_verbose_level > 0)
                        std::cout << "Case 4\n";

                    addToBuffer(m_point_buffer, minorTime);

                    break;

                default:

                    std::cout << "Old data-point - omitting!\n";
            }
        }


        if (m_verbose_level > 1) {
            nmx::printTimeOrderedBuffer(m_time_ordered_buffer, m_SortQ);
            nmx::printMajorTimeBuffer(m_majortime_buffer);
        }

        m_new_point = false;
    }
}

void NMXPlaneClusterer::consumer() {

    std::cout << "Started clustering thread!\n";

    while (1) {

        while ((m_nB <= m_nC) || m_nD) {
            if (m_terminate && (m_nC == m_nB)) {
                std::cout << "Stopped clustering thread!\n";
                return;
            }
            std::this_thread::yield();
        }

        if (m_nC % nmx::NCLEANUP == 0)
            checkBoxes();

        uint idx = m_nC % nmx::MAX_MINOR;

        nmx::buffer &buf = m_time_ordered_buffer.at(m_ClusterQ[idx]).at(idx);

        m_latestClusterTime = ((m_majortime_buffer.at(idx) << nmx::CLUSTER_MINOR_BITS) + idx) << nmx::IGNORE_BITS;

        for (int ipoint = 0; ipoint < buf.npoints; ipoint++) {

            nmx::data_point& point = buf.data.at(ipoint);

            if (point.strip >= nmx::STRIPS_PER_PLANE) {
                std::cerr << "<ClusterAssembler::addPointToCluster> Strip # " << point.strip << " is larger than "
                          << nmx::STRIPS_PER_PLANE - 1 << std::endl;
                std::cerr << "Point will not be added to the buffer!\n";
            }

            int lo_idx;
            int hi_idx;

            uint what = checkMask(point.strip, lo_idx, hi_idx);

            if (m_verbose_level > 1) {
                std::cout << "lo-idx = " << (int) lo_idx << ", hi-idx = " << (int) hi_idx << std::endl;
                std::cout << "What = " << what << std::endl;
                m_boxes.printStack();
                m_boxes.printQueue();
            }

            switch (what) {

                case 0:
                    // Not in cluster
                    newCluster(point);
                    break;

                case 1:
                    // In exactly one  cluster
                    if (m_boxes.checkBox(std::max(lo_idx, hi_idx), point)) {
                        flushCluster(std::max(lo_idx, hi_idx));
                        newCluster(point);
                    } else
                        insertInCluster(point);
                    break;

                case 2:
                    // On boundary of two clusters
                    uint oldness = 0;

                    oldness += (m_boxes.checkBox(lo_idx, point) ? 1 : 0);
                    oldness += (m_boxes.checkBox(hi_idx, point) ? 2 : 0);

                    switch (oldness) {

                        case 0:
                            // Neither clusters are "old"
                            if (m_verbose_level > 1)
                                std::cout << "Neither are old" << std::endl;

                            mergeAndInsert(lo_idx, hi_idx, point);
                            break;

                        case 1:
                            // Only lo-cluster is "old"
                            if (m_verbose_level > 1)
                                std::cout << "Lo is old" << std::endl;

                            flushCluster(lo_idx);
                            insertInCluster(point);
                            break;

                        case 2:
                            // Only hi-cluster is "old"
                            if (m_verbose_level > 1)
                                std::cout << "Hi is old" << std::endl;

                            flushCluster(hi_idx);
                            insertInCluster(point);
                            break;

                        case 3:
                            // Both clusters are "old"
                            if (m_verbose_level > 1)
                                std::cout << "Both are old" << std::endl;

                            flushCluster(lo_idx);
                            flushCluster(hi_idx);
                            newCluster(point);
                            break;

                        default:
                            std::cerr << "Oldness is " << oldness << std::endl;
                    }
            }
        }

        buf.npoints = 0;
        m_nC++;
    }
}

void NMXPlaneClusterer::addToBuffer(const nmx::data_point &point, const uint minorTime) {

    if (!nmx::checkPoint(point, "NMXPlaneClusterer::addToBuffer") ||
        !nmx::checkMinorTime(minorTime, "NMXPlaneClusterer::AddToBuffer"))
        return;

    uint32_t i0 = m_SortQ[minorTime];

    nmx::tbuffer &tbuf = m_time_ordered_buffer.at(i0);
    nmx::buffer &buf = tbuf.at(minorTime);

    buf.data.at(buf.npoints) = point;
    buf.npoints++;
}

void NMXPlaneClusterer::moveToClusterer(uint d, uint minorTime, uint majorTime) {

    if (!nmx::checkD(d, "NMXPlaneClusterer::moveToClusterer"))
        throw 1;

    if (m_verbose_level > 1) {
        std::string s("\nWill move ");
        s.append(std::to_string(d));
        s.append(" entries to clusterer !\n");
        std::cout << s;
    }

    guardB();

    for (uint i = 0; i < d; ++i) {

        uint idx = (i+m_nB)%nmx::MAX_MINOR;

        if (m_verbose_level > 1)
            std::cout << "Moving cluster with idx " << idx << std::endl;

        if (m_time_ordered_buffer.at(m_ClusterQ[idx]).at(idx).npoints > 0) {
            std::cout << "Buffer not empty!!!!!!!!!!!!!!!!!!!!\n";
            std::cout << "nB = " << m_nB << std::endl;
            std::cout << "nC = " << m_nC << std::endl;
        }

        m_SortQ[idx]    = m_ClusterQ[idx];
        m_ClusterQ[idx] = !m_SortQ[idx];

        if (idx <= minorTime)
            m_majortime_buffer.at(idx) = majorTime;
        else
            m_majortime_buffer.at(idx) = majorTime -1;
    }

    m_nB += d;

    if (m_verbose_level > 1)
        std::cout << "Setting i1 to " << minorTime << std::endl;

    m_i1 = minorTime;
 }

uint NMXPlaneClusterer::checkMask(uint strip, int &lo_idx, int &hi_idx) {

    // Return values :
    //     0 : Not in cluster
    //     1 : In a cluster
    //     2 : At the boundary of two clusters, which are to be merged

    lo_idx = m_mask.at(getLoBound(strip));
    hi_idx = m_mask.at(getHiBound(strip));

    if (m_verbose_level > 2)
        nmx::printMask(m_mask);

    int diff = std::abs( (lo_idx+1) - (hi_idx+1) );

    if (diff > 0) {
        if (diff != std::max(lo_idx+1, hi_idx+1))
            return 2;
        else
            return 1;
    } else if (m_mask.at(strip) != -1)
        return 1;

    return 0;
}

bool NMXPlaneClusterer::newCluster(nmx::data_point &point) {

    if (m_verbose_level > 2)
        std::cout << "Making new cluster ";

    if (!nmx::checkPoint(point, "NMXPlaneClusterer::newCluster"))
        return false;

    unsigned int newbox = static_cast<unsigned int>(m_boxes.getBoxFromStack());
    if (newbox > nmx::NBOXES -1) {
        std::cerr << "<NMXPlaneClusterer::newCluster> Got new box with # " << newbox << " which is not in range [0,"
                  << nmx::NBOXES -1 << "]\n";
        return false;
    }

    if (m_verbose_level > 2)
        std::cout << newbox << " at strip " << point.strip << std::endl;

    m_boxes.insertBoxInQueue(newbox);

    uint lo = getLoBound(point.strip);
    uint hi = getHiBound(point.strip);

    for (uint istrip = lo; istrip <= hi; istrip++)
        m_mask.at(istrip) = newbox;

    m_boxes.updateBox(newbox, point);

    m_cluster.at(point.strip) = point;

    point = {0,0,0};

    return true;
};

bool NMXPlaneClusterer::insertInCluster(nmx::data_point &point) {

    if (m_verbose_level > 2)
        std::cout << "Inserting in cluster at strip " << point.strip << std::endl;

    if (point.strip >= nmx::STRIPS_PER_PLANE) {
        std::cerr << "<insertInCluster> Strip # " << point.strip << " is larger than " << nmx::STRIPS_PER_PLANE - 1
                  << std::endl;
        std::cerr << "Cannot insert into cluster!\n";

        return false;
    }

    uint lo = getLoBound(point.strip);
    uint hi = getHiBound(point.strip);

    int boxid = -1;

    if (m_verbose_level > 0)
        nmx::printMask(m_mask);

    for (uint istrip = lo; istrip <= hi; istrip++) {

        boxid = m_mask.at(istrip);

        if (boxid != -1)
            break;
    }

    if ((boxid < 0) || (boxid > nmx::NBOXES -1)) {
        std::cerr << "<NMXPlaneClusterer::insertInCluster> Box id " << boxid << " which is not in range [0,"
                  << nmx::NBOXES -1 << "]\n";
    }

    if (m_verbose_level > 2)
        std::cout << "Inserting in cluster " << boxid << std::endl;

    for (uint istrip = lo; istrip <= hi; istrip++)
        m_mask.at(istrip) = boxid;

    m_boxes.updateBox(boxid, point);

    m_cluster.at(point.strip) = point;

    point = {0, 0, 0};
}

bool NMXPlaneClusterer::mergeAndInsert(uint32_t lo_idx, uint32_t hi_idx, nmx::data_point &point) {

    if (m_verbose_level > 2)
        std::cout << "Merging clusters ";

    if (!nmx::checkPoint(point, "NMXPlaneClusterer::mergeAndInsert"))
        return false;

    int final_cluster = -1;
    int remove_cluster = -1;
    int incr = 0;

    int lo_boxsize = m_boxes.getBox(lo_idx).max_strip - m_boxes.getBox(lo_idx).min_strip;
    int hi_boxsize = m_boxes.getBox(hi_idx).max_strip - m_boxes.getBox(hi_idx).min_strip;

    m_cluster.at(point.strip) = point;

    uint pos;
    uint end;

    if (lo_boxsize > hi_boxsize) {
        final_cluster = lo_idx;
        remove_cluster = hi_idx;
        incr = 1;
        pos = m_boxes.getBox(lo_idx).max_strip + nmx::INCLUDE_N_NEIGHBOURS;
        end = m_boxes.getBox(hi_idx).max_strip + nmx::INCLUDE_N_NEIGHBOURS;
        m_boxes.getBox(final_cluster).max_strip = m_boxes.getBox(remove_cluster).max_strip;
        m_boxes.getBox(final_cluster).max_time = m_boxes.getBox(remove_cluster).max_time;
    }

    if (lo_boxsize < hi_boxsize) {
        final_cluster = hi_idx;
        remove_cluster = lo_idx;
        incr = -1;
        pos = m_boxes.getBox(hi_idx).min_strip - nmx::INCLUDE_N_NEIGHBOURS;
        end = m_boxes.getBox(lo_idx).min_strip - nmx::INCLUDE_N_NEIGHBOURS;
        m_boxes.getBox(final_cluster).min_strip = m_boxes.getBox(remove_cluster).min_strip;
        m_boxes.getBox(final_cluster).min_time = m_boxes.getBox(remove_cluster).min_time;
    }

    if (lo_boxsize == hi_boxsize) {
        if (lo_idx > hi_idx) {
            final_cluster = lo_idx;
            remove_cluster = hi_idx;
            incr = 1;
            pos = m_boxes.getBox(lo_idx).max_strip + nmx::INCLUDE_N_NEIGHBOURS;
            end = m_boxes.getBox(hi_idx).max_strip + nmx::INCLUDE_N_NEIGHBOURS;
            m_boxes.getBox(final_cluster).max_strip = m_boxes.getBox(remove_cluster).max_strip;
            m_boxes.getBox(final_cluster).max_time = m_boxes.getBox(remove_cluster).max_time;
        } else {
            final_cluster = hi_idx;
            remove_cluster = lo_idx;
            incr = -1;
            pos = m_boxes.getBox(hi_idx).max_strip - nmx::INCLUDE_N_NEIGHBOURS;
            end = m_boxes.getBox(lo_idx).min_strip - nmx::INCLUDE_N_NEIGHBOURS;
            m_boxes.getBox(final_cluster).min_strip = m_boxes.getBox(remove_cluster).min_strip;
            m_boxes.getBox(final_cluster).min_time = m_boxes.getBox(remove_cluster).min_time;
        }
    }

    if (m_verbose_level > 2) {
        std::cout << "Merging cluster " << (int) remove_cluster << " into cluster " << (int) final_cluster << std::endl;
        std::cout << "Resetting box " << (int) remove_cluster << std::endl;
    }

    m_boxes.releaseBox(static_cast<const uint>(remove_cluster));

    while (pos != end + incr) {

        m_mask.at(pos) = final_cluster;

        pos += incr;
    }
}

bool NMXPlaneClusterer::flushCluster(const int boxid) {

    if (m_verbose_level > 2) {
        std::cout << "Flushing cluster " << boxid << " from plane " << m_plane <<  std::endl;
        nmx::printMask(m_mask);
    }

    nmx::cluster produced_cluster;
    produced_cluster.npoints = 0;

    nmx::box box = m_boxes.getBox(boxid);
    produced_cluster.box = box;
    produced_cluster.box.link1 = -1;
    produced_cluster.box.link2 = -1;

    if (m_verbose_level > 2) {
        std::cout << "\nBox # " << boxid << ":\n";
        printBox(box);
    }

    uint lo = getLoBound(box.min_strip);
    uint hi = getHiBound(box.max_strip);

    for (uint istrip = lo; istrip <= hi; istrip++) {

        m_mask.at(istrip) = -1;

        if ((istrip >= box.min_strip) && (istrip <= box.max_strip)) {

            nmx::data_point &point = m_cluster.at(istrip);

            if (m_verbose_level > 2) {
                std::cout << "Inserting strip " << istrip << std::endl;
                std::cout << "Point : strip = " << point.strip << ", time = " << point.time << ", charge = "
                          << point.charge
                          << std::endl;
            }

            if (point.charge != 0) {
                produced_cluster.data.at(produced_cluster.npoints) = point;
                produced_cluster.npoints++;
            }

            point = {0, 0, 0};
        }
    }
    /*
    if (produced_cluster.npoints > 0) {
        int cluster_idx = m_clusterManager.getClusterFromStack(m_plane);
        m_clusterManager.getCluster(m_plane, cluster_idx) = produced_cluster;
        m_clusterParing.insertClusterInQueue(m_plane, cluster_idx);
    }
*/

    m_boxes.releaseBox(boxid);

    if (m_verbose_level > 2)
        nmx::printMask(m_mask);
    }

uint NMXPlaneClusterer::getLoBound(int strip) {

    strip -= nmx::INCLUDE_N_NEIGHBOURS;

    while (true) {

        if (strip >= 0)
            return static_cast<uint>(strip);

        strip++;
    }
}

uint NMXPlaneClusterer::getHiBound(int strip) {

    strip += nmx::INCLUDE_N_NEIGHBOURS;

    while (true) {

        if (strip < nmx::STRIPS_PER_PLANE)

            return static_cast<uint>(strip);

        strip--;
    }
}

void NMXPlaneClusterer::endRun() {

    bool verbose = false;

    if (verbose)
        std::cout << "END of run - flushing time-ordered buffer ...\n";

    while (m_nB != m_nC) {
        if (verbose)
            std::cout << "nB = " << m_nB << " != " << " nC = " << m_nC << std::endl;
        std::this_thread::yield();
    }

    moveToClusterer(nmx::MINOR_BITMASK, nmx::MINOR_BITMASK, 0);

    while (m_nB != m_nC) {
        if (verbose)
            std::cout << "nB = " << m_nB << " != " << " nC = " << m_nC << std::endl;
        std::this_thread::yield();
    }

    for (uint i = 0; i < nmx::STRIPS_PER_PLANE; i++) {

        if (verbose)
            std::cout << "Mask.at(" << i << ")=" << m_mask.at(i) << std::endl;

        if (m_mask.at(i) > 0)
            flushCluster(m_mask.at(i));
    }
}

inline uint32_t NMXPlaneClusterer::getMinorTime(uint32_t time) {

    time = time >> nmx::IGNORE_BITS;
    time = time & nmx::MINOR_BITMASK;

    return time;
}

inline uint32_t NMXPlaneClusterer::getMajorTime(uint32_t time) {

    return time >> nmx::IGNORE_BITS >> nmx::MINOR_BITS;
}

void NMXPlaneClusterer::reset() {

    // Reset time-ordered buffer
    for (uint index0 = 0; index0 < 2; index0++) {

        nmx::tbuffer &buf = m_time_ordered_buffer.at(index0);

        for (uint index1 = 0; index1 < nmx::MAX_MINOR; index1++) {

            auto &buffer = buf.at(index1);

            buffer.npoints = 0;

            for (uint istrip = 0; istrip < nmx::STRIPS_PER_PLANE; istrip++)
                buffer.data.at(istrip) = {0, 0, 0};
        }
    }

    // Reset the mask

    for (uint idx = 0; idx < m_mask.size(); idx++)
        m_mask.at(idx) = -1;


    // Reset major-time buffer
    for (uint i = 0; i < nmx::MAX_MINOR; ++i) {
        m_majortime_buffer.at(i) = 0;
        m_SortQ.at(i) = 0;
        m_ClusterQ.at(i) = 1;
    }
}

void NMXPlaneClusterer::checkBoxes() {

    int idx = m_boxes.getQueueTail();

    if (m_verbose_level > 2)
        std::cout << "<NMXPlaneClusterer::checkBoxes> Queue starts at " << idx << std::endl;

    while (idx > 0) {

        nmx::box &box = m_boxes.getBox(idx);

        int diff = m_latestClusterTime - box.min_time;

        if (m_verbose_level > 2)
            std::cout << "Diff = " << m_latestClusterTime << " - " << box.min_time << " = " << diff;

        if (diff > nmx::MAX_CLUSTER_TIME) {
            if (m_verbose_level > 2)
                std::cout << " which is larger than " << nmx::MAX_CLUSTER_TIME << " so flushing\n";
            flushCluster(idx);
        }

        idx = box.link2;
        if (m_verbose_level > 2)
            std::cout << "Next idx is " << idx << std::endl;
    }
}

void NMXPlaneClusterer::guardB() {

    while (m_nB != m_nC)
        std::this_thread::yield();

}

void NMXPlaneClusterer::checkBitSum() {

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

void NMXPlaneClusterer::printInitialization() {

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
