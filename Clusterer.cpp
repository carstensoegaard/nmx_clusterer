//
// Created by soegaard on 10/24/17.
//

#include <iostream>
#include <limits>
#include <iomanip>
#include <cassert>

#include "ClusterAssembler.h"

#include "Clusterer.h"

Clusterer::Clusterer()
    : m_i1(nmx::MINOR_BITMASK),
      m_nB(0),
      m_nC(0),
      m_nD(0)
{

    checkBitSum();
    printInitialization();

    m_boxes = new BoxAdministration;

    reset();

    pro = std::thread(&Clusterer::producer, this);
    con = std::thread(&Clusterer::consumer, this);
}

Clusterer::~Clusterer()
{
    pro.join();
    con.join();
}

bool Clusterer::addDataPoint(uint32_t strip, uint32_t time, uint32_t charge) {

    nmx::data_point point = {strip, charge, time};

    return addDataPoint(point);
}

bool Clusterer::addDataPoint(const nmx::data_point &point) {

    bool verbose = false;

    if (point.strip >= nmx::STRIPS_PER_PLANE) {
        std::cerr << "<addDataPoint> Strip # " << point.strip << " is larger than " << nmx::STRIPS_PER_PLANE - 1
                  << std::endl;
        std::cerr << "Omitting point!\n";

        return false;
    }

    if (verbose) {
       // printTimeOrderedBuffer();
        //printMask();
        printClusterBuffer();
    }

    m_databuffer.push_back(point);
}

void Clusterer::producer(void) {

    bool verbose = true;

    while(1) {

        while (m_databuffer.size() == 0)
            std::this_thread::yield();

        nmx::data_point point = m_databuffer.front();

        uint minorTime = getMinorTime(point.time);
        uint majorTime = getMajorTime(point.time);

        if (verbose) {
            printPoint(point);
            std::cout << "B1 = " << minorTime << ", B2 = " << majorTime << ", B2_buffer[" << minorTime << "] = "
                      << m_majortime_buffer[minorTime] << " B2_buffer[0] = " << m_majortime_buffer.at(0)
                      << " i1 = " << m_i1 << std::endl;
        }

        if (majorTime >= (m_majortime_buffer.at(0) + 1)) {

            if (majorTime == (m_majortime_buffer.at(0) + 1)) {

                if (verbose)
                    std::cout << "Case 1\n";

                while (m_nB != m_nC)
                    std::this_thread::yield();

                moveToClusterer(nmx::MINOR_BITMASK-m_i1+std::min(m_i1,minorTime));
                for (uint idx = m_i1+1; idx < nmx::MINOR_BITMASK; idx++)
                    m_majortime_buffer[idx] = majorTime -1;
                for (uint idx = 0; idx < majorTime; idx++)
                    m_majortime_buffer[idx] = majorTime;

                /*
                flushBuffer(m_i1 + 1, nmx::MINOR_BITMASK, majorTime - 1);
                flushBuffer(0, std::min(m_i1, minorTime), majorTime);
*/
                m_i1 = minorTime;
                addToBuffer(point, minorTime);

            } else { // majorTime > (m_majortime_buffer.at(0) + 1)

                if (verbose)
                    std::cout << "Case 2\n";

                while (m_nB != m_nC)
                    std::this_thread::yield();

                moveToClusterer(nmx::MINOR_BITMASK);
                for (uint idx = 0; idx < minorTime; idx++)
                    m_majortime_buffer[idx] = majorTime;
                for (uint idx = minorTime +1; idx < nmx::MINOR_BITMASK; idx++)
                    m_majortime_buffer[idx] = majorTime -1;


                /*
                flushBuffer(m_i1 + 1, nmx::MINOR_BITMASK, majorTime - 1);
                flushBuffer(0, m_i1, majorTime);
*/
                m_i1 = minorTime;
                addToBuffer(point, minorTime);

                while (m_nB != m_nC)
                    std::this_thread::yield();

                m_nD = 1;
                m_nB = majorTime;
                m_nC = majorTime;
                m_nD = 0;
            }

        } else { // majorTime <= m_buffer.at(0)

            switch (majorTime - m_majortime_buffer.at(minorTime)) {

                case 1:

                    if (verbose)
                        std::cout << "Case 3\n";

                    while (m_nB != m_nC)
                        std::this_thread::yield();

                    moveToClusterer(minorTime-m_i1);
                    for (uint idx = m_i1+1; idx < minorTime; idx++)
                        m_majortime_buffer[idx] = majorTime;
                    //flushBuffer(m_i1 + 1, minorTime, majorTime);

                    m_i1 = minorTime;
                    addToBuffer(point, minorTime);

                    break;

                case 0:

                    if (verbose)
                        std::cout << "Case 4\n";

                    addToBuffer(point, minorTime);

                    break;
                default:

                    //std::cout << "Case 5\n";
                    // Do nothing !
                    std::cout << "Old data-point - omitting!\n";
            }
        }

        m_databuffer.erase(m_databuffer.begin());
    }

    if (verbose) {
        //printTimeOrderedBuffer();
        //printMask();
        printClusterBuffer();
    }
}

inline void Clusterer::addToBuffer(const nmx::data_point &point, const uint minorTime) {

    if ((point.strip >= nmx::STRIPS_PER_PLANE) || (minorTime >= nmx::MAX_MINOR)) {
        std::cerr << "<addToBuffer> Invalid data:\n";
        std::cerr << "              Strip = " << point.strip << ", max strip = " << nmx::STRIPS_PER_PLANE-1 << "\n";
        std::cerr << "              Minor-time = " << minorTime << " max minor-time = " << nmx::MAX_MINOR-1 << "\n";
        std::cerr << " *** Point will not be added to the buffer! ***\n";
        return;
    }

    uint32_t i0 = m_SortQ[minorTime];

    nmx::tbuffer &tbuf = m_time_ordered_buffer.at(i0);
    nmx::buffer &buf = tbuf.at(minorTime);

    buf.data.at(buf.npoints) = point;
    buf.npoints++;
}

/*
inline void Clusterer::flushBuffer(uint lo_idx, uint hi_idx, uint32_t buffertime) {

    bool verbose = false;

    for (uint idx = lo_idx; idx <= hi_idx; idx++) {
        if (verbose)
            std::cout << "Transfering buffer " << idx << std::endl;
        transfer(m_time_ordered_buffer.at(idx));
        m_majortime_buffer.at(idx) = buffertime;
    }
}
*/

inline void Clusterer::moveToClusterer(uint d) {

    bool verbose = true;

    for (uint i = 0; i < d; ++i) {

        uint idx = (i+m_nB)%nmx::MAX_MINOR;
        m_SortQ[idx]    = m_ClusterQ[idx];
        m_ClusterQ[idx] = m_SortQ[idx];
    }

    m_nB += d;
}

//bool Clusterer::transfer(nmx::buffer &buf) {


void Clusterer::consumer(void) {

    bool verbose = false;

    ClusterAssembler assembler;

    m_produced_clusters = assembler.getClusterRef();

    if (verbose) {
        std::cout << "Transfering ...\n";
        //printTimeOrderedBuffer();
    }

    while (1) {

        while ((m_nB == m_nC) || m_nD)
            std::this_thread::yield();

        uint idx = m_nC % nmx::MAX_MINOR;

        nmx::buffer &buf = m_time_ordered_buffer.at(m_ClusterQ[idx]).at(idx);

        for (int ipoint = 0; ipoint < buf.npoints; ipoint++) {

            nmx::data_point point = buf.data.at(ipoint);
            assembler.addPointToCluster(point);
        }

        buf.npoints = 0;
        m_nC++;

    }
}


void Clusterer::endRun() {

    bool verbose = false;

    if (verbose)
        std::cout << "END of run - flushing time-ordered buffer ...";

    uint buffer_max = nmx::MINOR_BITMASK;

    moveToClusterer(nmx::MINOR_BITMASK);

    //flushBuffer(m_i1+1, buffer_max, buffer_max);
    //flushBuffer(     0,       m_i1, buffer_max);

    if (verbose)
        std::cout << " Done!\n";
/*
    for (uint i = 0; i < nmx::STRIPS_PER_PLANE; i++) {

        if (verbose)
            std::cout << "Mask.at(" << i << ")=" << m_mask.at(i) << std::endl;

        if (m_mask.at(i) > 0)
            flushCluster(m_mask.at(i));
    }*/
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


void Clusterer::reset() {

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

    // Reset major-time buffer
    for (uint i = 0; i < nmx::MAX_MINOR; ++i)
        m_majortime_buffer.at(i) = 0;


   // m_final_cluster.npoints = 0;
}

// This must go in final version

/*void Clusterer::printMask() {

    std::cout << "Mask:\n";
    for (int i = 0; i < m_mask.size(); ++i) {
        if (i%25 == 0) {
            std::cout << "\n strip " << std::setw(4) << i << " : ";
        }
        std::cout << std::setw(5) << m_mask.at(i) << " ";
    }
    std::cout << "\n";
}*/

void Clusterer::printPoint(const nmx::data_point &point) {

    std::cout << "Point : S = " << point.strip << " C = " << point.charge << " T = " << point.time << std::endl;


}

void Clusterer::printBox(const nmx::box &box) {

    std::cout << "Strips [" << box.min_strip << ", " << box.max_strip << "]\n";
    std::cout << "Time   [" << box.min_time << ", " << box.max_time << "]\n";
}

void Clusterer::printBox(int boxid) {

    std::cout << "Box-id " << std::endl;

    nmx::box box = m_boxes->getBox(boxid);

    printBox(box);
}

void Clusterer::printClusterBuffer() {

    std::cout << "Cluster buffer :\n";
/*
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
    */
}
/*
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
*/
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