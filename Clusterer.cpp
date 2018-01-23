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
      m_nD(0),
      m_nAdded(0),
      m_nInserted(0),
      m_readlock(false),
      m_writelock(false),
      m_terminate(false)
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

    //std::cout << "Read lock is " << m_readlock << std::endl;

    while (m_readlock)
        std::this_thread::yield();

    while (m_nAdded - m_nInserted >= 100)
        std::this_thread::sleep_for(std::chrono::milliseconds(10));

    m_pointbuf = point;
    m_writelock = true;
    m_databuffer.at(m_nAdded%100)=point;
    m_nAdded++;
    m_writelock = false;
    //std::cout << m_nAdded << " points added\n";

    //std::cout << "<Clusterer::addDataPoint> data_buffer size = " << m_nAdded - m_nInserted << std::endl;
}

void Clusterer::producer(void) {

    std::cout << "Started sorting thread!\n";

    bool verbose = false;

    //std::cout << "Producer init: " << m_nAdded << ", " << m_nInserted << std::endl;

    uint nprocessed = 0;

    while(1) {

        if (m_terminate) {
            if (m_nAdded != m_nInserted) {
                std::cout << "<Clusterer::SortingThread> Terminate! Waiting for all points to be inserted\n";
                std::cout << m_nAdded << " points added\n";
                std::cout << m_nInserted << " points inserted\n";
            } else {
                std::cout << "<Clusterer::SortingThread> Terminated !!!\n";
                return;
            }
        }
        //  std::cout << "Added points = " << m_nAdded << ", inserted points = " << m_nInserted << std::endl;

        while (m_writelock || m_nAdded == m_nInserted) {
            //std::cout << "Write lock is " /*<< m_writelock*/ << std::endl;
            std::this_thread::yield();
        }

        m_readlock = true;
        nmx::data_point point = m_databuffer.at(m_nInserted%100);
        m_nInserted++;
        m_readlock = false;

        if (verbose)
            std::cout << m_nInserted << " points inserted\n";

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

                if (verbose) {
                    std::cout << "Case 1\n";
                    std::cout << "Moving " << nmx::MINOR_BITMASK - m_i1 + std::min(m_i1, minorTime) << " points\n";
                }

                while (m_nB != m_nC)
                    std::this_thread::yield();

                moveToClusterer(nmx::MINOR_BITMASK-m_i1+std::min(m_i1,minorTime)+1, minorTime, majorTime);
               /*
                for (uint idx = m_i1+1; idx < nmx::MINOR_BITMASK; idx++) {
                    if (verbose)
                        std::cout << "Setting majortime_buf[" << idx << "] = " << majorTime - 1 << std::endl;
                    m_majortime_buffer[idx] = majorTime - 1;
                }
                for (uint idx = 0; idx < minorTime; idx++) {
                    if (verbose)
                        std::cout << "Setting majortime_buf[" << idx << "] = " << majorTime << std::endl;
                    m_majortime_buffer[idx] = majorTime;
                }
*/
                if (verbose)
                    std::cout << "Setting i1 to " << minorTime << std::endl;
                //m_i1 = minorTime;
                addToBuffer(point, minorTime);
                //printTimeOrderedBuffer();

            } else { // majorTime > (m_majortime_buffer.at(0) + 1)

                if (verbose) {
                    std::cout << "Case 2\n";
                    std::cout << "Moving " << nmx::MINOR_BITMASK << " points\n";
                }

                while (m_nB != m_nC)
                    std::this_thread::yield();

                moveToClusterer(nmx::MINOR_BITMASK, minorTime, majorTime);
                /*-
                for (uint idx = 0; idx < minorTime; idx++)
                    m_majortime_buffer[idx] = majorTime;
                for (uint idx = minorTime +1; idx < nmx::MINOR_BITMASK; idx++)
                    m_majortime_buffer[idx] = majorTime -1;
*/
                if (verbose)
                    std::cout << "Setting i1 to " << minorTime << std::endl;
                //m_i1 = minorTime;
                addToBuffer(point, minorTime);
                //printTimeOrderedBuffer();

                while (m_nB != m_nC)
                    std::this_thread::yield();

                m_nD = 1;
                m_nB = majorTime;
                m_nC = majorTime;
                m_nD = 0;
            }

        } else { // majorTime <= m_buffer.at(0)

            if (verbose)
                std::cout << "What-what = " << majorTime - m_majortime_buffer.at(minorTime) << std::endl;

            switch (majorTime - m_majortime_buffer.at(minorTime)) {

                case 1:

                    if (verbose) {
                        std::cout << "Case 3\n";
                        std::cout << "nB = " << m_nB << ", nC = " << m_nC << std::endl;
                        std::cout << "Moving " << minorTime - m_i1 << " points\n";
                        std::cout << "Minor-time = " << minorTime << ", i1 = " << m_i1 << std::endl;
                    }

                    while (m_nB != m_nC) {
                        std::this_thread::yield();
                    }
                    /*
                    std::cout << "Changing majortime buffer from idx " << m_i1 +1 << " to " << minorTime << std::endl;
                    for (uint idx = m_i1 + 1; idx < minorTime; idx++) {
                        std::cout << "Setting b2[" << idx << "] to " << majorTime << std::endl;
                        m_majortime_buffer[idx] = majorTime;
                    }*/
                    moveToClusterer(minorTime - m_i1, minorTime, majorTime);

                    if (verbose)
                        std::cout << "Setting i1 to " << minorTime << std::endl;
                  //  m_i1 = minorTime;
                    addToBuffer(point, minorTime);
                    //printTimeOrderedBuffer();

                    break;

                case 0:

                    if (verbose)
                        std::cout << "Case 4\n";

                    addToBuffer(point, minorTime);
                    //printTimeOrderedBuffer();

                    break;
                default:

                    std::cout << "Old data-point - omitting!\n";
            }
        }

        nprocessed++;
        if (verbose)
            std::cout << "Processed " << nprocessed << " points\n";

        if (verbose)
            std::cout << "i1 = " << m_i1 << std::endl;

        if (verbose)
        printMajorTimeBuffer();
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

    bool verbose = false;

    uint32_t i0 = m_SortQ[minorTime];

    nmx::tbuffer &tbuf = m_time_ordered_buffer.at(i0);
    nmx::buffer &buf = tbuf.at(minorTime);

    buf.data.at(buf.npoints) = point;
    buf.npoints++;

    if (verbose)
        std::cout << "Buffer[" << i0 << "][" << minorTime << "] contains " << buf.npoints << " points\n";
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

inline void Clusterer::moveToClusterer(uint d, uint minorTime, uint majorTime) {

    if (d > nmx::MAX_MINOR) {
        std::string s("\n<Clusterer::moveToClusterer> Cannot move ");
        s.append(std::to_string(d));
        s.append(" entries to clusterer !\nMax is ");
        s.append(std::to_string(nmx::MAX_MINOR));
        s.append(" FATAL ERROR");
        std::cout << s;
        //return;
        throw 1;
    }

    bool verbose = false;

    if (verbose) {
        std::string s("\nWill move ");
        s.append(std::to_string(d));
        s.append(" entries to clusterer !\n");
        std::cout << s;
    }

    for (uint i = 0; i < d; ++i) {

        uint idx = (i+m_nB)%nmx::MAX_MINOR;

        if (verbose)
            std::cout << "Moving cluster with idx " << idx << std::endl;
        m_SortQ[idx]    = m_ClusterQ[idx];
        m_ClusterQ[idx] = !m_SortQ[idx];

        if (idx <= minorTime)
            m_majortime_buffer.at(idx) = majorTime;
        else
            m_majortime_buffer.at(idx) = majorTime -1;
    }


    m_nB += d;
    uint i1 = m_nB%nmx::MAX_MINOR - 1;
    if (verbose)
        std::cout << "Setting i1 to " << i1 << std::endl;
    m_i1 = i1;
}

//bool Clusterer::transfer(nmx::buffer &buf) {


void Clusterer::consumer(void) {

    std::cout << "Started clustering thread!\n";

    bool verbose = false;

    ClusterAssembler assembler;

    m_produced_clusters = assembler.getClusterRef();

    if (verbose) {
        std::cout << "Transfering ...\n";
        //printTimeOrderedBuffer();
    }

    while (1) {

        if ((m_nB == m_nC) && m_terminate)
            return;

        if (verbose)
            std::cout << "nD = " << m_nD << std::endl;

        while ((m_nB == m_nC) || m_nD)
            std::this_thread::yield();

        uint idx = m_nC % nmx::MAX_MINOR;

        nmx::buffer &buf = m_time_ordered_buffer.at(m_ClusterQ[idx]).at(idx);

        if (verbose)
            std::cout << "Number of points at [" << m_ClusterQ[idx] << "][" << idx << "] = " << buf.npoints << std::endl;

        for (int ipoint = 0; ipoint < buf.npoints; ipoint++) {

            if (verbose)
                std::cout << "Point # " << ipoint << std::endl;

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

    moveToClusterer(nmx::MINOR_BITMASK, nmx::MINOR_BITMASK, 0);

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
    for (uint i = 0; i < nmx::MAX_MINOR; ++i) {
        m_majortime_buffer.at(i) = 0;
        m_SortQ.at(i) = 0;
        m_ClusterQ.at(i) = 1;
    }


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

void Clusterer::printTimeOrderedBuffer() {

    std::cout << "Time ordered buffer :\n";

    for (uint idx = 0; idx < nmx::MAX_MINOR; idx++) {

        auto tbuf = m_time_ordered_buffer.at(m_SortQ.at(idx));
        auto buf = tbuf.at(idx);

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

void Clusterer::printMajorTimeBuffer() {

    std::cout <<"\nMajorTimeBuffer:\n";

    for (uint idx = 0; idx < nmx::MAX_MINOR; idx++) {
        if (idx%32 == 0)
            std::cout << "\nIdx : ";
        std::cout << std::setw(4) << idx << " ";
    }
    for (uint idx = 0; idx < nmx::MAX_MINOR; idx++) {
        if (idx%32 == 0)
            std::cout << "\nVal : ";
        std::cout << std::setw(4) << m_majortime_buffer.at(idx) << " ";
    }
    std::cout << "\n";
}