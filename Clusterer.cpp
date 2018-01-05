//
// Created by soegaard on 10/24/17.
//

#include <iostream>
#include <limits>
#include <iomanip>
#include <cassert>

#include "Clusterer.h"

Clusterer::Clusterer()
    : m_i1(nmx::MINOR_BITMASK)
{

    checkBitSum();
    printInitialization();

    reset();
}

bool Clusterer::addDataPoint(uint32_t strip, uint32_t time, uint32_t charge) {

    nmx::data_point point = {strip, charge, time};

    return addDataPoint(point);
}

bool Clusterer::addDataPoint(const nmx::data_point &point) {

    bool verbose = false;

    if (point.strip >= nmx::STRIPS_PER_PLANE) {
        std::cerr << "<addDataPoint> Strip # " << point.strip << " is larger than " << nmx::STRIPS_PER_PLANE-1
                  << std::endl;
        std::cerr << "Omitting point!\n";

        return false;
    }

    if (verbose) {
        printTimeOrderedBuffer();
        printMask();
        printClusterBuffer();
    }

    uint minorTime = getMinorTime(point.time);
    uint majorTime = getMajorTime(point.time);

    if (verbose) {
        printPoint(point);
        std::cout << "B1 = " << minorTime << ", B2 = " << majorTime << ", B2_buffer[" << minorTime << "] = "
                  << m_majortime_buffer[minorTime] << " B2_buffer[0] = " << m_majortime_buffer.at(0) << " i1 = " << m_i1
                  << std::endl;
    }

    if ( majorTime >= (m_majortime_buffer.at(0) + 1) ) {

        if ( majorTime == (m_majortime_buffer.at(0) + 1) ) {

            if (verbose)
                std::cout << "Case 1\n";

            flushBuffer(m_i1+1, nmx::MINOR_BITMASK, majorTime-1);
            flushBuffer(0, std::min(m_i1, minorTime), majorTime);

            m_i1 = minorTime;
            addToBuffer(point, minorTime);

        } else { // majorTime > (m_majortime_buffer.at(0) + 1)

            if (verbose)
                std::cout << "Case 2\n";

            flushBuffer(m_i1+1, nmx::MINOR_BITMASK, majorTime-1);
            flushBuffer(0, m_i1, majorTime);

            m_i1 = minorTime;
            addToBuffer(point, minorTime);
        }

    } else { // majorTime <= m_buffer.at(0)

        switch (majorTime - m_majortime_buffer.at(minorTime)) {

            case 1:

                if (verbose)
                    std::cout << "Case 3\n";

                flushBuffer(m_i1+1, minorTime, majorTime);

                m_i1 = minorTime;
                addToBuffer(point, minorTime);

                break;

            case 0:

                if (verbose)
                    std::cout << "Case 4\n";

                addToBuffer(point, minorTime);

                break;
            default:
;
                //std::cout << "Case 5\n";
                std::cout << "Old data-point - omitting!\n";

                // Do nothing !

        }
    }

    if (verbose) {
        printTimeOrderedBuffer();
        printMask();
        printClusterBuffer();
    }
}

inline void Clusterer::addToBuffer(const nmx::data_point &point, const uint &minorTime) {

    if ((point.strip >= nmx::STRIPS_PER_PLANE) || (minorTime >= nmx::MAX_MINOR)) {
        std::cerr << "<addToBuffer> Invalid data:\n";
        std::cerr << "              Strip = " << point.strip << ", max strip = " << nmx::STRIPS_PER_PLANE-1 << "\n";
        std::cerr << "              Minor-time = " << minorTime << " max minor = " << nmx::MAX_MINOR-1 << "\n";
        std::cerr << " *** Point will not be added to the buffer! ***\n";
        return;
    }

    nmx::buffer &buf = m_time_ordered_buffer.at(minorTime);

    buf.data.at(buf.npoints) = point;
    buf.npoints++;
}

inline void Clusterer::flushBuffer(uint lo_idx, uint hi_idx, uint32_t buffertime) {

    bool verbose = false;

    for (uint idx = lo_idx; idx <= hi_idx; idx++) {
        if (verbose)
            std::cout << "Transfering buffer " << idx << std::endl;
        transfer(m_time_ordered_buffer.at(idx));
        m_majortime_buffer.at(idx) = buffertime;
    }
}

bool Clusterer::transfer(nmx::buffer &buf) {

    bool verbose = false;

    if (verbose) {
        std::cout << "Transfering ...\n";
        printTimeOrderedBuffer();
    }

    for (int ipoint = 0; ipoint < buf.npoints; ipoint++) {

        nmx::data_point point = buf.data.at(ipoint);
/*
        if (point.strip == 162)
            verbose = true;
*/
        if (verbose)
            std::cout << "Transfering point " << ipoint << ", strip = " << point.strip << ", time = "
                      << point.time << ", charge = " << point.charge << std::endl;

        if (point.strip >= nmx::STRIPS_PER_PLANE) {
            std::cerr << "<transfer> Strip # " << point.strip << " of point " << ipoint << " is larger than "
                      << nmx::STRIPS_PER_PLANE - 1 << std::endl;
            std::cerr << "Point will not be added to the buffer!\n";

            return false;
        }

        int lo_idx;
        int hi_idx;

        uint what = checkMask(point.strip, lo_idx, hi_idx);

        if (verbose) {
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
                if (verbose)
                    printPoint(point);

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
                        if (verbose)
                            std::cout << "Neither are old" << std::endl;
                        mergeAndInsert(lo_idx, hi_idx, point);
                        break;
                    case 1:
                        // Only lo-cluster is "old"
                        if (verbose)
                            std::cout << "Lo is old" << std::endl;
                        flushCluster(lo_idx);
                        insertInCluster(point);
                        break;
                    case 2:
                        // Only hi-cluster is "old"
                        if (verbose)
                            std::cout << "Hi is old" << std::endl;
                        flushCluster(hi_idx);
                        insertInCluster(point);
                        break;
                    case 3:
                        // Both clusters are "old"
                        if (verbose)
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

    if (verbose) {
        std::cout << "Printing mask\n";
        printMask();
        printClusterBuffer();
        m_boxes.printStack();
        m_boxes.printQueue();
        m_boxes.printBoxesInQueue();
    }
   // m_boxes.printQueue();
   // m_boxes.printBoxesInQueue();

    buf.npoints = 0;

    return true;
}

bool Clusterer::newCluster(nmx::data_point &point) {

//    std::cout << "Making new cluster ";

    if (point.strip >= nmx::STRIPS_PER_PLANE) {
        std::cerr << "<newCluster> Strip # " << point.strip << " is larger than " << nmx::STRIPS_PER_PLANE - 1
                  << std::endl;
        std::cerr << "Cannot get new cluster\n";

        return false;
    }

    int newbox = m_boxes.getBoxFromStack();

//    std::cout << newbox << " at strip " << point.strip << std::endl;

    m_boxes.insertBoxInQueue(newbox);

    uint lo = getLoBound(point.strip);
    uint hi = getHiBound(point.strip);

    for (uint istrip = lo; istrip <= hi; istrip++)
        m_mask.at(istrip) = newbox;

    m_boxes.updateBox(newbox, point.strip, point.time);

    m_cluster.at(point.strip) = point;

    point = {0,0,0};

    return true;
};


bool Clusterer::insertInCluster(nmx::data_point &point) {

//    std::cout << "Inserting in cluster at strip " << point.strip << std::endl;

    if (point.strip >= nmx::STRIPS_PER_PLANE) {
        std::cerr << "<insertInCluster> Strip # " << point.strip << " is larger than " << nmx::STRIPS_PER_PLANE - 1
                  << std::endl;
        std::cerr << "Cannot insert into cluster!\n";

        return false;
    }

    uint lo = getLoBound(point.strip);
    uint hi = getHiBound(point.strip);

 //   std::cout << "Bounds : " << int(lo) << " -> " << int(hi) << std::endl;

    int boxid = -1;

    for (uint istrip = lo; istrip <= hi; istrip++) {

        boxid = m_mask.at(istrip);

   //     std::cout << "Mask[" << int(istrip) << "] = " << m_mask.at(istrip) << std::endl;

        if (boxid != -1)
            break;
    }

    //std::cout << "Inserting in cluster ";
    //std::cout << boxid << std::endl;

    for (int istrip = lo; istrip <= hi; istrip++)
        m_mask.at(istrip) = boxid;

    m_boxes.updateBox(boxid, point.strip, point.time);

    m_cluster.at(point.strip) = point;

    point = {0, 0, 0};
}

bool Clusterer::mergeAndInsert(uint32_t lo_idx, uint32_t hi_idx, nmx::data_point &point) {

    bool verbose = false;

    if (verbose) {
        std::cout << "Merging clusters ";
        printMask();
    }

    if (point.strip >= nmx::STRIPS_PER_PLANE) {
        std::cerr << "<mergeAndInsert> Strip # " << point.strip << " is larger than " << nmx::STRIPS_PER_PLANE - 1
                  << std::endl;
        std::cerr << "Cannot merge nor insert!\n";

        return false;
    }

    /*
    // Find cluster numbers
    uint lo = getLoBound(point.strip);
    uint hi = getHiBound(point.strip);

    int cluster_idx1 = -1;
    int cluster_idx2 = -1;

    for (uint istrip = lo; istrip <= hi; istrip++) {

        int cluster_idx = m_mask.at(istrip);

        if (cluster_idx != -1) {

            if (cluster_idx1 == -1)
                cluster_idx1 = cluster_idx;
            else if (cluster_idx2 == -1 && cluster_idx1 != cluster_idx) {
                cluster_idx2 = cluster_idx;
            }
        }
    }

    if (verbose)
        std::cout << "Lo-mask = " << cluster_idx1 << ", Hi-mask = " << cluster_idx2 << std::endl;

    if ((cluster_idx1 == -1) ||
            (cluster_idx2 == -1) ||
            (cluster_idx1 == cluster_idx2)) {
        std::cerr << "Something is very wrong !!! cluster_idx1 = " << cluster_idx1 << ", cluster_idx2 = "
                  << cluster_idx2 << std::endl;
        return false;
    }
*/

    int final_cluster = -1;
    int remove_cluster = -1;
    int incr = 0;

    int lo_boxsize = m_boxes.getBox(lo_idx).max_strip - m_boxes.getBox(lo_idx).min_strip;
    int hi_boxsize = m_boxes.getBox(hi_idx).max_strip - m_boxes.getBox(hi_idx).min_strip;

    if (verbose) {
        std::cout << "Size of box " << lo_idx << " = " << lo_boxsize << std::endl;
        std::cout << "Size of box " << hi_idx << " = " << hi_boxsize << std::endl;
    }

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

    if (verbose) {
        std::cout << "Merging cluster " << (int) remove_cluster << " into cluster " << (int) final_cluster << std::endl;
        std::cout << "Resetting box " << (int) remove_cluster << std::endl;
    }

    m_boxes.releaseBox(remove_cluster);

    uint mask_at_pos = m_mask.at(pos);

    while (pos != end + incr) {

        if (verbose)
            std::cout << "Pos = " << pos << std::endl;

        m_mask.at(pos) = final_cluster;

        pos += incr;

        mask_at_pos = m_mask.at(pos);
    }

    if (verbose)
        printMask();
}

uint Clusterer::checkMask(uint strip, int &lo_idx, int &hi_idx) {

    // Return values :
    //     0 : Not in cluster
    //     1 : In a cluster
    //     2 : At the boundary of two clusters, which are to be merged

    lo_idx = m_mask.at(getLoBound(strip));
    hi_idx = m_mask.at(getHiBound(strip));

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

bool Clusterer::flushCluster(const int boxid) {

    nmx::cluster produced_cluster;
    produced_cluster.npoints = 0;

    bool verbose = false;

    if (verbose) {
        std::cout << "Flushing cluster " << boxid << std::endl;
        printMask();
    }

    nmx::box box = m_boxes.getBox(boxid);

    if (verbose) {
        std::cout << "\nBox # " << boxid << ":\n";
        printBox(box);
    }

    uint lo = getLoBound(box.min_strip);
    uint hi = getHiBound(box.max_strip);

    if (verbose)
        std::cout << "Flushing from strip " << lo << " to strip " << hi << std::endl;

    for (uint istrip = lo; istrip <= hi; istrip++) {

        if (verbose)
            std::cout << "Resetting mask at strip " << istrip << std::endl;

        m_mask.at(istrip) = -1;

        if ((istrip >= box.min_strip) && (istrip <= box.max_strip)) {

            nmx::data_point &point = m_cluster.at(istrip);

            if (verbose) {
                std::cout << "Inserting strip " << istrip << std::endl;
                std::cout << "Point : strip = " << point.strip << ", time = " << point.time << ", charge = "
                          << point.charge
                          << std::endl;
            }

            if (point.charge != 0) {
                produced_cluster.data.at(produced_cluster.npoints) = point;
                produced_cluster.npoints++;

                if (verbose)
                    std::cout << "Inserted strip\n";
            }

            point = {0, 0, 0};
        }
    }

    m_produced_clusters.push_back(produced_cluster);

    if (verbose)
        std::cout << "Realeasing box # " << boxid;

    m_boxes.releaseBox(boxid);

    if (verbose)
        std::cout << " - box released!\n";

    if (verbose) {
        std::cout << "Done flushing\n";
        printMask();
    }
}

void Clusterer::endRun() {

    bool verbose = false;

    if (verbose)
        std::cout << "END of run - flushing time-ordered buffer ...";

    uint buffer_max = nmx::MINOR_BITMASK;

    flushBuffer(m_i1+1, buffer_max, buffer_max);
    flushBuffer(     0,       m_i1, buffer_max);

    if (verbose)
        std::cout << " Done!\n";

    for (uint i = 0; i < nmx::STRIPS_PER_PLANE; i++) {

        if (verbose)
            std::cout << "Mask.at(" << i << ")=" << m_mask.at(i) << std::endl;

        if (m_mask.at(i) > 0)
            flushCluster(m_mask.at(i));
    }
}

inline uint32_t Clusterer::getMinorTime(uint32_t time) {

//    std::cout << "getB1(" << time << ") step 1 time = ";

    time = time >> nmx::IGNORE_BITS;

//    std::cout << time << " step 2 time = ";

    time = time & nmx::MINOR_BITMASK;

//    std::cout << time << std::endl;

    return time;
}

inline uint32_t Clusterer::getMajorTime(uint32_t time) {

    return time >> nmx::IGNORE_BITS >> nmx::MINOR_BITS;
}

uint Clusterer::getLoBound(int strip) {

    //std::cout << "Getting low bound at " << strip << std::endl;

    strip -= nmx::INCLUDE_N_NEIGHBOURS;

    while (true) {

        //std::cout << "Bound = " << strip << std::endl;

        if (strip >= 0) {

            //std::cout << "Returning " << strip << std::endl;
            return static_cast<uint>(strip);
        }

        strip++;
    }
}

uint Clusterer::getHiBound(int strip) {

    strip += nmx::INCLUDE_N_NEIGHBOURS;

    while (true) {

        if (strip < nmx::STRIPS_PER_PLANE)

            return static_cast<uint>(strip);

        strip--;
    }
}

void Clusterer::reset() {

    // Reset time-ordered buffer
    for (uint index = 0; index < nmx::MAX_MINOR; index ++) {

        auto &buffer = m_time_ordered_buffer.at(index);

        buffer.npoints = 0;

        for (uint istrip = 0; istrip < nmx::STRIPS_PER_PLANE; istrip++)
            buffer.data.at(istrip) = {0, 0, 0};
    }

    // Reset major-time buffer
    for (uint i = 0; i < nmx::MAX_MINOR; ++i)
        m_majortime_buffer.at(i) = 0;

    // Reset mask and cluster-buffer
    for (uint i = 0; i < nmx::STRIPS_PER_PLANE; i++) {
        m_mask.at(i) = -1;
        m_cluster.at(i) = {0,0,0};
    }

   // m_final_cluster.npoints = 0;
}

// This must go in final version

void Clusterer::printMask() {

    std::cout << "Mask:\n";
    for (int i = 0; i < m_mask.size(); ++i) {
        if (i%25 == 0) {
            std::cout << "\n strip " << std::setw(4) << i << " : ";
        }
        std::cout << std::setw(5) << m_mask.at(i) << " ";
    }
    std::cout << "\n";
}

void Clusterer::printPoint(const nmx::data_point &point) {

    std::cout << "Point : S = " << point.strip << " C = " << point.charge << " T = " << point.time << std::endl;


}

void Clusterer::printBox(const nmx::box &box) {

    std::cout << "Strips [" << box.min_strip << ", " << box.max_strip << "]\n";
    std::cout << "Time   [" << box.min_time << ", " << box.max_time << "]\n";
}

void Clusterer::printBox(int boxid) {

    std::cout << "Box-id " << std::endl;

    nmx::box box = m_boxes.getBox(boxid);

    printBox(box);
}

void Clusterer::printClusterBuffer() {

    std::cout << "Cluster buffer :\n";

    std::cout << "Strip    ";
    for (uint i = 0; i < nmx::STRIPS_PER_PLANE; i++) {
        if (m_cluster.at(i).charge != 0)
            std::cout << std::setw(6) << i;
    }
    std::cout << "\n";
    std::cout << "Time     ";
    for (uint i = 0; i < nmx::STRIPS_PER_PLANE; i++)
        if (m_cluster.at(i).charge != 0)
            std::cout << std::setw(6) << m_cluster.at(i).time;
    std::cout << "\n";
    std::cout << "Charge   ";
    for (uint i = 0; i < nmx::STRIPS_PER_PLANE; i++)
        if (m_cluster.at(i).charge != 0)
            std::cout << std::setw(6) << m_cluster.at(i).charge;
    std::cout << "\n";
}

void Clusterer::printTimeOrderedBuffer() {

    std::cout << "Time ordered buffer :\n";

    for (uint idx = 0; idx < nmx::MAX_MINOR; idx++) {

        auto buf = m_time_ordered_buffer.at(idx);

        if (buf.npoints == 0)
            continue;

        std::cout << "Index " << idx << std::endl;

        std::cout << "Strip  ";
        for (uint ientry = 0; ientry < buf.npoints; ientry++) {

            auto point = buf.data.at(ientry);
            std::cout << std::setw(5) << point.strip;
        }

        std::cout << "\nTime   ";
        for (uint ientry = 0; ientry < buf.npoints; ientry++) {

            auto point = buf.data.at(ientry);
            std::cout << std::setw(5) << point.time;
        }

        std::cout << "\nCharge ";
        for (uint ientry = 0; ientry < buf.npoints; ientry++) {

            auto point = buf.data.at(ientry);
            std::cout << std::setw(5) << point.charge;
        }
        std::cout << "\n";
    }
}

void Clusterer::checkBitSum() {

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

void Clusterer::printInitialization() {

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
}