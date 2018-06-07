//
// Created by soegaard on 10/24/17.
//

#include <iostream>
#include <limits>
#include <iomanip>

#include "../include/NMXPlaneClusterer.h"

NMXPlaneClusterer::NMXPlaneClusterer(int plane, NMXClusterManager &clusterManager, NMXClusterPairing &clusterPairing,
                                     std::mutex &mutex)
    : m_verbose_level(0),
      m_i1(nmx::DATA_MINOR_BITMASK),
      m_nB(0),
      m_nC(0),
      m_nD(0),
      m_plane(plane),
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
    pro = std::thread(&NMXPlaneClusterer::timeSorting, this);
    con = std::thread(&NMXPlaneClusterer::clustering, this);
}

NMXPlaneClusterer::~NMXPlaneClusterer()
{
    // Join threads
    pro.join();
    con.join();
}

bool NMXPlaneClusterer::addDataPoint(uint32_t strip, uint32_t time, uint32_t charge) {

    nmx::DataPoint point = {strip, charge, time};

    return addDataPoint(point);
}

bool NMXPlaneClusterer::addDataPoint(const nmx::DataPoint &point) {

    if (!nmx::checkPoint(point, "NMXPlaneClusterer::addDataPoint"))
        return false;

    while (m_new_point)
        std::this_thread::yield();

    m_point_buffer = point;
    m_new_point = true;

    return true;
}

void NMXPlaneClusterer::timeSorting() {

    std::cout << "<THREAD> Started sorting thread for plane " << (m_plane ? "Y" : "X") << std::endl;

    while(1) {

        while (!m_new_point) {
            if (m_terminate) {
                std::cout << "<THREAD> Stopped sorting thread for plane " << (m_plane ? "Y" : "X") << std::endl;
                return;
            }
            std::this_thread::yield();
        }

        uint minorTime = getMinorTime(m_point_buffer.time);
        uint majorTime = getMajorTime(m_point_buffer.time);

        if (checkTrackPoint(m_point_buffer))
            std::cout << "<NMXPlaneClusterer::timeSorting> Track-point arrived for sorting!" << std::endl;

        if (m_verbose_level > 0) {
            nmx::printPoint(m_point_buffer);
            std::cout << "B1 = " << minorTime << ", B2 = " << majorTime << ", B2_buffer[" << minorTime << "] = "
                      << m_majortimeBuffer[minorTime] << " B2_buffer[0] = " << m_majortimeBuffer.at(0)
                      << " i1 = " << m_i1 << std::endl;
        }

        if (majorTime >= (m_majortimeBuffer.at(0) + 1)) {

            if (majorTime == (m_majortimeBuffer.at(0) + 1) && minorTime <= m_i1) {

                if (m_verbose_level > 0) {
                    std::cout << "Case 1\n";
                    std::cout << "Moving " << nmx::DATA_MINOR_BITMASK - m_i1 + std::min(m_i1, minorTime) +1 << " points\n";
                }

                moveToClusterer(nmx::DATA_MINOR_BITMASK - m_i1 + std::min(m_i1, minorTime) + 1, minorTime, majorTime);
                addToBuffer(m_point_buffer, minorTime);

            } else { // majorTime > (m_majortimeBuffer.at(0) + 1)

                if (m_verbose_level > 0) {
                    std::cout << "Case 2\n";
                    std::cout << "Moving " << nmx::DATA_MINOR_BITMASK << " points\n";
                }

                moveToClusterer(nmx::DATA_MINOR_BITMASK+1, minorTime, majorTime);
                addToBuffer(m_point_buffer, minorTime);

                guardB();
                m_nD = 1;
                m_nC = minorTime+1;
                m_nB = minorTime+1;
                m_nD = 0;
            }

        } else { // majorTime <= m_buffer.at(0)

            switch (majorTime - m_majortimeBuffer.at(minorTime)) {

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

                    if (m_verbose_level > 0)
                        std::cout << "Old data-point - omitting!\n";
                    m_nOldPoints++;
            }
        }


        if (m_verbose_level > 2) {
            //nmx::printTimeOrderedBuffer(m_time_ordered_buffer, m_SortQ);
            nmx::printMajorTimeBuffer(m_majortimeBuffer);
        }

        m_new_point = false;
    }
}

void NMXPlaneClusterer::clustering() {

    std::cout << "<THREAD> Started clustering thread for plane " << (m_plane ? "Y" : "X") << std::endl;

    uint32_t lastFlush = 0;

    while (1) {

        // May be replaced by guardC()
        while ((m_nB == m_nC) || m_nD) {
            if (m_terminate && (m_nC == m_nB)) {
                std::cout << "<THREAD> Stopped clustering thread for plane " << (m_plane ? "Y" : "X") << std::endl;
                return;
            }
            std::this_thread::yield();
        }


        unsigned int idx = m_nC % nmx::DATA_MAX_MINOR;

        if (m_verbose_level > 1)
            std::cout << "Now extracting idx (" << m_ClusterQ[idx] << ", " << idx << ")" << std::endl;

        int queueIdx = m_time_ordered_buffer.at(m_ClusterQ[idx]).at(idx);
        m_time_ordered_buffer.at(m_ClusterQ[idx]).at(idx) = -1;
        //for (unsigned int ipoint = 0; ipoint < buf.nPoints; ipoint++) {

        while (queueIdx != -1) {

            BufferEntry& entry = m_bufferEntries.at(queueIdx);
            nmx::DataPoint& point = entry.point;

            if ((point.time-lastFlush)/nmx::MAX_CLUSTER_TIME > 2) {
                checkBoxes(point.time);
                lastFlush = point.time;
            }


           /* if (m_ClusterQ[idx] == 1 && idx == 43 && m_plane == 0) {
                std::cout << "Point # " << ipoint << " : time = " << point.time << ", strip = "
                          << point.strip << ", charge = " << point.charge << std::endl;
            }*/

            bool check = checkTrackPoint(point);

            /*if (m_ClusterQ[idx] == 1 && idx == 43 && m_plane == 0)
                std::cout << "Check = " << check << std::endl;*/

            if (check)
                std::cout << "<NMXPlaneClusterer::clustering> Track-point arrived." << std::endl;

            if (point.strip >= nmx::STRIPS_PER_PLANE) {
                std::cerr << "<ClusterAssembler::addPointToCluster> Strip # " << point.strip << " is larger than "
                          << nmx::STRIPS_PER_PLANE - 1 << std::endl;
                std::cerr << "Point will not be added to the DataBuffer!\n";
            }

            int lo_idx;
            int hi_idx;

            uint what = checkMask(point.strip, lo_idx, hi_idx);

            if (m_verbose_level > 2) {
                std::cout << "lo-idx = " << lo_idx << ", hi-idx = " << hi_idx << std::endl;
                std::cout << "What = " << what << std::endl;
                m_boxes.printStack();
                m_boxes.printQueue();
            }

            switch (what) {

                case 0:
                    // Not in Cluster
                    newCluster(point);
                    break;

                case 1:
                    // In exactly one  Cluster
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
                            if (m_verbose_level > 2)
                                std::cout << "Neither are old" << std::endl;

                            mergeAndInsert(lo_idx, hi_idx, point);
                            break;

                        case 1:
                            // Only lo-Cluster is "old"
                            if (m_verbose_level > 2)
                                std::cout << "Lo is old" << std::endl;

                            flushCluster(lo_idx);
                            insertInCluster(point);
                            break;

                        case 2:
                            // Only hi-Cluster is "old"
                            if (m_verbose_level > 2)
                                std::cout << "Hi is old" << std::endl;

                            flushCluster(hi_idx);
                            insertInCluster(point);
                            break;

                        case 3:
                            // Both clusters are "old"
                            if (m_verbose_level > 2)
                                std::cout << "Both are old" << std::endl;

                            flushCluster(lo_idx);
                            flushCluster(hi_idx);
                            newCluster(point);
                            break;

                        default:
                            std::cerr << "Oldness is " << oldness << std::endl;
                    }
            }

            int nextIdx = entry.link;
            insertEntryInQueue(queueIdx);
            queueIdx = nextIdx;
        }

        m_nC++;
    }
}

void NMXPlaneClusterer::addToBuffer(const nmx::DataPoint &point, const uint minorTime) {

    if (!nmx::checkPoint(point, "NMXPlaneClusterer::addToBuffer") ||
        !nmx::checkMinorTime(minorTime, "NMXPlaneClusterer::AddToBuffer"))
        return;

    uint32_t i0 = m_SortQ[minorTime];

    if (checkTrackPoint(point))
        std::cout << "<NMXPlaneClusterer::addToBuffer> Track-point added to DataBuffer at idx : (" << i0 << ", "
                  << minorTime
                  << ")" << std::endl;

    int entryIdx = getEntryFromQueue();
    BufferEntry* entry = getBufferEntry(entryIdx);
    entry->point.strip  = point.strip;
    entry->point.time   = point.time;
    entry->point.charge = point.charge;
    entry->link = -1;

    int &queue = m_time_ordered_buffer.at(i0).at(minorTime);
    if (queue == -1)
        queue = entryIdx;
    else
        getBufferEntry(getLastInQueue(queue))->link = entryIdx;
}

void NMXPlaneClusterer::moveToClusterer(uint d, uint minorTime, uint majorTime) {

    if (!nmx::checkD(d, "NMXPlaneClusterer::moveToClusterer"))
        throw 1;

    if (m_verbose_level > 0) {
        std::string s("\nWill move ");
        s.append(std::to_string(d));
        s.append(" entries to clusterer !\n");
        std::cout << s;
    }

    guardB();

    for (uint i = 0; i < d; ++i) {

        uint idx = (i+m_nB)%nmx::DATA_MAX_MINOR;

        /*if (m_verbose_level > 1)
            std::cout << "Moving Cluster with idx " << idx << std::endl;*/

        if (m_time_ordered_buffer.at(m_ClusterQ[idx]).at(idx) != -1) {
            std::cout << "Buffer not empty!!!!!!!!!!!!!!!!!!!!\n";
            std::cout << "nB = " << m_nB << std::endl;
            std::cout << "nC = " << m_nC << std::endl;
        }

        m_SortQ[idx]    = m_ClusterQ[idx];
        m_ClusterQ[idx] = !m_SortQ[idx];

        if (idx <= minorTime)
            m_majortimeBuffer.at(idx) = majorTime;
        else
            m_majortimeBuffer.at(idx) = majorTime -1;
    }

    m_nB += d;

    /*if (m_verbose_level > 1)
        std::cout << "Setting i1 to " << minorTime << std::endl;*/

    m_i1 = minorTime;
 }

unsigned int NMXPlaneClusterer::checkMask(uint strip, int &lo_idx, int &hi_idx) {

    // Return values :
    //     0 : Not in Cluster
    //     1 : In a Cluster
    //     2 : At the boundary of two clusters, which are to be merged

    if (strip >= nmx::STRIPS_PER_PLANE) {
        std::cerr << "NMXPlaneClusterer::checkMask> Out of bounds ! Provided strip number = " << strip
                  << " Range [0 - " << nmx::STRIPS_PER_PLANE-1 << "]" << std::endl;
    }

    uint32_t loBound = getLoBound(strip);
    uint32_t hiBound = getHiBound(strip);

    if (loBound >= nmx::STRIPS_PER_PLANE || hiBound >= nmx::STRIPS_PER_PLANE) {
        std::cerr << "NMXPlaneClusterer::checkMask> Out of bounds ! Lo = " << loBound << ", hi = " << hiBound
                  << " Range [0 - " << nmx::STRIPS_PER_PLANE-1 << "]" << std::endl;
    }

    lo_idx = m_mask.at(loBound);
    hi_idx = m_mask.at(hiBound);

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

bool NMXPlaneClusterer::newCluster(nmx::DataPoint &point) {

    if (m_verbose_level > 2)
        std::cout << "Making new Cluster ";

    if (!nmx::checkPoint(point, "NMXPlaneClusterer::newCluster"))
        return false;

    unsigned int newbox = static_cast<unsigned int>(m_boxes.getBoxFromStack());
    if (newbox > nmx::NBOXES -1) {
        std::cerr << "<NMXPlaneClusterer::newCluster> Got new Box with # " << newbox << " which is not in range [0,"
                  << nmx::NBOXES -1 << "]\n";
        return false;
    }

    if (m_verbose_level > 2)
        std::cout << newbox << " at strip " << point.strip << std::endl;

    m_boxes.insertBoxInQueue(newbox);

    uint lo = getLoBound(point.strip);
    uint hi = getHiBound(point.strip);

    if (lo >= nmx::STRIPS_PER_PLANE || hi >= nmx::STRIPS_PER_PLANE) {
        std::cerr << "NMXPlaneClusterer::newCluster> Out of bounds ! Lo = " << lo << ", hi = " << hi
                  << " Range [0 - " << nmx::STRIPS_PER_PLANE - 1 << "]" << std::endl;
    }

    for (uint istrip = lo; istrip <= hi; istrip++)
        m_mask.at(istrip) = newbox;

    m_boxes.updateBox(newbox, point);

    m_cluster.at(point.strip) = point;

    point = {0,0,0};

    return true;
};

bool NMXPlaneClusterer::insertInCluster(nmx::DataPoint &point) {

    if (m_verbose_level > 2)
        std::cout << "Inserting in Cluster at strip " << point.strip << std::endl;

    if (point.strip >= nmx::STRIPS_PER_PLANE) {
        std::cerr << "<insertInCluster> Strip # " << point.strip << " is larger than " << nmx::STRIPS_PER_PLANE - 1
                  << std::endl;
        std::cerr << "Cannot insert into Cluster!\n";

        return false;
    }

    unsigned int lo = getLoBound(point.strip);
    unsigned int hi = getHiBound(point.strip);

    if (lo >= nmx::STRIPS_PER_PLANE || hi >= nmx::STRIPS_PER_PLANE) {
        std::cerr << "NMXPlaneClusterer::insertInCluster> Out of bounds ! Lo = " << lo << ", hi = " << hi
                  << " Range [0 - " << nmx::STRIPS_PER_PLANE - 1 << "]" << std::endl;
    }

    int boxid = -1;

    if (m_verbose_level > 2)
        nmx::printMask(m_mask);

    for (unsigned int istrip = lo; istrip <= hi; istrip++) {

        boxid = m_mask.at(istrip);

        if (boxid != -1)
            break;
    }

    if ((boxid < 0) || (static_cast<unsigned int>(boxid) > nmx::NBOXES-1) ) {
        std::cerr << "<NMXPlaneClusterer::insertInCluster> Box id " << boxid << " which is not in range [0,"
                  << nmx::NBOXES -1 << "]\n";
    }

    if (m_verbose_level > 2) {
        nmx::printBox(boxid, &m_boxes);
        nmx::printPoint(point);
        std::cout << "inserted in Cluster " << boxid << std::endl;
    }

    for (unsigned int istrip = lo; istrip <= hi; istrip++)
        m_mask.at(istrip) = boxid;

    m_boxes.updateBox(boxid, point);

    if (m_verbose_level > 2) {
        std::cout << "After update:" << std::endl;
        nmx::printBox(boxid, &m_boxes);
    }

    m_cluster.at(point.strip) = point;

    if (checkTrackPoint(point)) {
        std::cout << "Track-point inserted in Cluster " << boxid << std::endl;
        nmx::printBox(boxid, &m_boxes);
    }

    point = {0, 0, 0};

    return true;
}

bool NMXPlaneClusterer::mergeAndInsert(uint32_t lo_idx, uint32_t hi_idx, nmx::DataPoint &point) {

    bool local_verbose = false;

    if (lo_idx >= nmx::NBOXES || hi_idx >= nmx::NBOXES) {
        std::cerr << "<NMX::PlaneClusterer::mergeAndInsert> Indexes out of range! lo-idx = " << lo_idx << ", hi-idx = "
                  << hi_idx << " Range = [0," << nmx::NBOXES-1 << "]" << std::endl;
        return false;
    }

    if (m_verbose_level > 2 || local_verbose) {
        std::cout << "Merging clusters :" << std::endl;
        nmx::printBox(lo_idx, &m_boxes);
        nmx::printBox(hi_idx, &m_boxes);
    }

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

    if (lo_boxsize >= hi_boxsize) {
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
        pos = m_boxes.getBox(hi_idx).min_strip - nmx::INCLUDE_N_NEIGHBOURS;
        end = m_boxes.getBox(lo_idx).min_strip - nmx::INCLUDE_N_NEIGHBOURS;
        m_boxes.getBox(final_cluster).min_strip = m_boxes.getBox(remove_cluster).min_strip;
        m_boxes.getBox(final_cluster).min_time = m_boxes.getBox(remove_cluster).min_time;
    }

    if (m_verbose_level > 2 || local_verbose) {
        std::cout << "Merging Cluster " << remove_cluster << ": " << std::endl;
        nmx::printBox(remove_cluster, &m_boxes);
        std::cout << "Into Cluster " << final_cluster << ": " << std::endl;
        nmx::printBox(final_cluster, &m_boxes);
        std::cout << "Resetting Box " << remove_cluster << std::endl;
    }

    m_boxes.releaseBox(static_cast<const uint>(remove_cluster));

    if (m_verbose_level > 2 || local_verbose)
        std::cout << "Changing mask from " << pos << " to " << end << std::endl;

    while (pos != end + incr) {

        if (pos >= nmx::STRIPS_PER_PLANE) {
            std::cerr << "NMXPlaneClusterer::mergeAndInsert> Out of bounds ! Pos = " << pos
                      << " Range [0 - " << nmx::STRIPS_PER_PLANE - 1 << "]" << std::endl;
        }

        m_mask.at(pos) = final_cluster;

        pos += incr;
        if (pos >= nmx::STRIPS_PER_PLANE || pos < 0)
            break;
    }

    m_verbose_level = 0;

    return true;
}

bool NMXPlaneClusterer::flushCluster(const int boxid) {

    if (m_verbose_level > 2) {
        std::cout << "Flushing Cluster " << boxid << " from plane " << m_plane <<  std::endl;
        nmx::printMask(m_mask);
    }

    nmx::Cluster produced_cluster;
    produced_cluster.nPoints = 0;

    nmx::Box box = m_boxes.getBox(boxid);
    produced_cluster.box = box;
    produced_cluster.box.link1 = -1;
    produced_cluster.box.link2 = -1;

    if (m_verbose_level > 2) {
        std::cout << "\nBox # " << boxid << ":\n";
        printBox(box);
    }

    unsigned int lo = getLoBound(box.min_strip);
    unsigned int hi = getHiBound(box.max_strip);

    if (lo >= nmx::STRIPS_PER_PLANE || hi >= nmx::STRIPS_PER_PLANE) {
        std::cerr << "NMXPlaneClusterer::flushCluster> Out of bounds ! Lo = " << lo << ", hi = " << hi
                  << " Range [0 - " << nmx::STRIPS_PER_PLANE - 1 << "]" << std::endl;
    }

    for (unsigned int istrip = lo; istrip <= hi; istrip++) {

        m_mask.at(istrip) = -1;

        if ((istrip >= box.min_strip) && (istrip <= box.max_strip)) {

            nmx::DataPoint &point = m_cluster.at(istrip);

            if (m_verbose_level > 2) {
                std::cout << "Inserting strip " << istrip << std::endl;
                std::cout << "Point : strip = " << point.strip << ", time = " << point.time << ", charge = "
                          << point.charge
                          << std::endl;
            }

            if (point.charge != 0) {
                produced_cluster.data.at(produced_cluster.nPoints) = point;
                produced_cluster.nPoints++;
            }

            point = {0, 0, 0};
        }
    }

    if (produced_cluster.nPoints > 0) {
        int cluster_idx = m_clusterManager.getClusterFromStack(m_plane);
        m_clusterManager.getCluster(m_plane, cluster_idx) = produced_cluster;
        m_clusterParing.insertClusterInQueue(m_plane, cluster_idx);
        m_nClusters++;
    }

    m_boxes.releaseBox(boxid);

    if (m_verbose_level > 2)
        nmx::printMask(m_mask);

    return true;
}

int NMXPlaneClusterer::getEntryFromQueue() {

    while (m_queueHead == -1) {
        m_nFailedEntryRequests++;
        std::this_thread::yield();
    }

    int idx = m_queueHead;
    m_queueHead = m_bufferEntries.at(idx).link;

    return idx;
}

void NMXPlaneClusterer::insertEntryInQueue(unsigned int idx) {

    if (idx >= nmx::STRIPS_PER_PLANE) {
        std::cerr << "<NMXPlaneClusterer::insertEntryInQueue> Index = " << idx << " is too large!" << std::endl;
        return;
    }

    if (m_queueHead == -1)
        m_queueHead = idx;
    else
        m_bufferEntries.at(m_queueTail).link = idx;

    m_queueTail = idx;
}

BufferEntry* NMXPlaneClusterer::getBufferEntry(unsigned int idx) {

    BufferEntry* ret = nullptr;

    if (idx >= nmx::STRIPS_PER_PLANE)
        std::cerr << "<NMXPlaneClusterer::insertEntryInQueue> Index = " << idx << " is too large!" << std::endl;
    else
        ret = &m_bufferEntries.at(idx);

    return ret;
}

int NMXPlaneClusterer::getLastInQueue(int idx) {

    int ret = idx;

    if (idx < -1 || idx >= static_cast<int>(nmx::STRIPS_PER_PLANE)) {
        std::cerr << "<NMXPLaneClusterer::getLastInQueue> Index cannot be " << idx << " must be in [-1, "
                  << nmx::STRIPS_PER_PLANE-1 << "]" << std::endl;
        ret = INT32_MAX;
    }

    while (idx != -1) {

        idx = m_bufferEntries.at(idx).link;

        if (idx != -1)
            ret = idx;
    }

    return ret;
}

uint32_t NMXPlaneClusterer::getLoBound(int strip) {

    strip -= nmx::INCLUDE_N_NEIGHBOURS;

    while (true) {

        if (strip >= 0)
            return static_cast<uint32_t>(strip);

        strip++;
    }
}

uint32_t NMXPlaneClusterer::getHiBound(uint32_t strip) {

    strip += nmx::INCLUDE_N_NEIGHBOURS;

    while (true) {

        if (strip < nmx::STRIPS_PER_PLANE)

            return strip;

        strip--;
    }
}

void NMXPlaneClusterer::endRun() {

    bool verbose = false;

    if (verbose)
        std::cout << "END of run - flushing time-ordered DataBuffer ...\n";

    while (m_nB != m_nC) {
        /*if (verbose)
            std::cout << "nB = " << m_nB << " != " << " nC = " << m_nC << std::endl;*/
        std::this_thread::yield();
    }

    moveToClusterer(nmx::DATA_MAX_MINOR, nmx::DATA_MINOR_BITMASK, 0);

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

    time = time >> nmx::DATA_IGNORE_BITS;
    time = time & nmx::DATA_MINOR_BITMASK;

    return time;
}

inline uint32_t NMXPlaneClusterer::getMajorTime(uint32_t time) {

    return time >> nmx::DATA_IGNORE_BITS >> nmx::DATA_MINOR_BITS;
}

void NMXPlaneClusterer::reset() {

    // Reset time-ordered DataBuffer
    for (uint index0 = 0; index0 < 2; index0++) {

        for (uint index1 = 0; index1 < nmx::DATA_MAX_MINOR; index1++)
            m_time_ordered_buffer.at(index0).at(index1) = -1;
    }

    // Reset i1
    m_i1 = nmx::DATA_MINOR_BITMASK;

    // Reset the mask
    for (uint idx = 0; idx < m_mask.size(); idx++)
        m_mask.at(idx) = -1;

    // Reset major-time DataBuffer
    for (uint i = 0; i < nmx::DATA_MAX_MINOR; ++i) {
        m_majortimeBuffer.at(i) = 0;
        m_SortQ.at(i) = 0;
        m_ClusterQ.at(i) = 1;
    }

    // Reset the buffer-entry queue
    unsigned int idxMax = nmx::STRIPS_PER_PLANE-1;
    m_queueHead = 0;
    for (unsigned int i = 0; i < idxMax; i++)
        m_bufferEntries.at(i).link = i+1;
    m_bufferEntries.at(idxMax).link = -1;
    m_queueTail = idxMax;

    // Start threads again
    m_terminate = false;

    m_nClusters = 0;
}

void NMXPlaneClusterer::checkBoxes(uint32_t latestTime) {

    int idx = m_boxes.getQueueTail();

    if (m_verbose_level > 2)
        std::cout << "<NMXPlaneClusterer::checkBoxes> Queue starts at " << idx << std::endl;

    while (idx > 0) {

        nmx::Box &box = m_boxes.getBox(idx);

        int diff = latestTime - box.min_time;

        if (m_verbose_level > 1)
            std::cout << "<NMXPlaneClusterer::checkBoxes> Diff = " << latestTime << " - "
                      << box.min_time << " = " << diff;

        if (static_cast<unsigned int>(diff) > nmx::MAX_CLUSTER_TIME) {
            if (m_verbose_level > 1)
                std::cout << " which is larger than " << nmx::MAX_CLUSTER_TIME << " so flushing\n";
            flushCluster(idx);
        }

        if (diff < 0) {
            std::cerr << "Diff = " << diff << " !!!" << std::endl;
            std::cerr << "Algorithm does not expect negative values. Unpredicted results are likely to occur!"
                      << std::endl;
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

/*void NMXPlaneClusterer::guardC() {

    while (m_nB == m_nC || m_nD)
        std::this_thread::yield();
}*/

void NMXPlaneClusterer::checkBitSum() {

    if (nmx::DATA_IGNORE_BITS + nmx::DATA_MINOR_BITS > 31) {
        std::cout << std::setfill('*') << std::setw(40) << "*" << std::endl;
        std::cout << std::setfill('*') << std::setw(40) << "*" << std::endl;
        std::cout << "*" << std::setfill(' ') << std::setw(38) << " " << "*\n";
        std::cout << "*" << std::setw(4) << " " << "Sum of NBITSx exceeds 31" << std::setw(3) << " " << "*\n";
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
    std::cout << "Bits to ignore :           " << std::setw(10) << nmx::DATA_IGNORE_BITS << std::endl;
    std::cout << "Minor bits :               " << std::setw(10) << nmx::DATA_MINOR_BITS << std::endl;
    std::cout << "Neighbours to include :    " << std::setw(10) << nmx::INCLUDE_N_NEIGHBOURS << std::endl;
    std::cout << "Maximum time for Cluster : " << std::setw(10) << nmx::MAX_CLUSTER_TIME << std::endl;
    std::cout << "Number of boxes :          " << std::setw(10) << nmx::NBOXES << std::endl;
    std::cout << "\n";
    std::cout << "Max MINOR :                " << std::setw(10) << nmx::DATA_MAX_MINOR << std::endl;
    std::cout << "MINOR bit-mask :           " << std::setw(10) << nmx::DATA_MINOR_BITMASK << std::endl;
    std::cout << "\n";
    std::cout << "Verbosity level :          " << std::setw(10) << m_verbose_level << std::endl;
    std::cout << "\n";
}

bool NMXPlaneClusterer::checkTrackPoint(const nmx::DataPoint &point) {

    if (!m_trackPoint)
        return false;

    if ((m_pointToTrack.time == point.time) &&
        (m_pointToTrack.strip == point.strip) &&
        (m_pointToTrack.charge == point.charge))
        return true;

     return false;
}