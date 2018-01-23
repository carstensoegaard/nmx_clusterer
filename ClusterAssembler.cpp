//
// Created by soegaard on 1/16/18.
//

#include <iostream>

#include "ClusterAssembler.h"

ClusterAssembler::ClusterAssembler() {

    reset();
}

void ClusterAssembler::addPointToCluster(nmx::data_point &point) {

    bool verbose = false;

    if (point.strip >= nmx::STRIPS_PER_PLANE) {
        std::cerr << "<ClusterAssembler::addPointToCluster> Strip # " << point.strip << " is larger than "
                  << nmx::STRIPS_PER_PLANE - 1 << std::endl;
        std::cerr << "Point will not be added to the buffer!\n";
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

uint ClusterAssembler::checkMask(uint strip, int &lo_idx, int &hi_idx) {

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

bool ClusterAssembler::newCluster(nmx::data_point &point) {

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

    m_boxes.updateBox(newbox, point);

    m_cluster.at(point.strip) = point;

    point = {0,0,0};

    return true;
};


bool ClusterAssembler::insertInCluster(nmx::data_point &point) {

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

    m_boxes.updateBox(boxid, point);

    m_cluster.at(point.strip) = point;

    point = {0, 0, 0};
}

bool ClusterAssembler::mergeAndInsert(uint32_t lo_idx, uint32_t hi_idx, nmx::data_point &point) {

    bool verbose = false;

    if (verbose) {
        std::cout << "Merging clusters ";
        //printMask();
    }

    if (point.strip >= nmx::STRIPS_PER_PLANE) {
        std::cerr << "<mergeAndInsert> Strip # " << point.strip << " is larger than " << nmx::STRIPS_PER_PLANE - 1
                  << std::endl;
        std::cerr << "Cannot merge nor insert!\n";

        return false;
    }

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

    /*if (verbose)
        printMask();*/
}

bool ClusterAssembler::flushCluster(const int boxid) {

    bool verbose = false;

    nmx::cluster produced_cluster;
    produced_cluster.npoints = 0;

    if (verbose) {
        std::cout << "Flushing cluster " << boxid << std::endl;
    //    printMask();
    }

    nmx::box box = m_boxes.getBox(boxid);

    /*
    if (verbose) {
        std::cout << "\nBox # " << boxid << ":\n";
        printBox(box);
    }*/

    uint lo = getLoBound(box.min_strip);
    uint hi = getHiBound(box.max_strip);

    /*if (verbose)
        std::cout << "Flushing from strip " << lo << " to strip " << hi << std::endl;*/

    for (uint istrip = lo; istrip <= hi; istrip++) {

        /*if (verbose)
            std::cout << "Resetting mask at strip " << istrip << std::endl;*/

        m_mask.at(istrip) = -1;

        if ((istrip >= box.min_strip) && (istrip <= box.max_strip)) {

            nmx::data_point &point = m_cluster.at(istrip);

            /*if (verbose) {
                std::cout << "Inserting strip " << istrip << std::endl;
                std::cout << "Point : strip = " << point.strip << ", time = " << point.time << ", charge = "
                          << point.charge
                          << std::endl;
            }*/

            if (point.charge != 0) {
                produced_cluster.data.at(produced_cluster.npoints) = point;
                produced_cluster.npoints++;

                /*if (verbose)
                    std::cout << "Inserted strip\n";*/
            }

            point = {0, 0, 0};
        }
    }

    m_produced_clusters.push_back(produced_cluster);
    if (verbose)
        std::cout << "Number of produced clusters = " << m_produced_clusters.size() << std::endl;

    /*if (verbose)
        std::cout << "Realeasing box # " << boxid;*/

    m_boxes.releaseBox(boxid);

    /*if (verbose)
        std::cout << " - box released!\n";

    if (verbose) {
        std::cout << "Done flushing\n";
        printMask();
    }*/
}

uint ClusterAssembler::getLoBound(int strip) {

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

uint ClusterAssembler::getHiBound(int strip) {

    strip += nmx::INCLUDE_N_NEIGHBOURS;

    while (true) {

        if (strip < nmx::STRIPS_PER_PLANE)

            return static_cast<uint>(strip);

        strip--;
    }
}

void ClusterAssembler::reset() {

    // Reset mask and cluster-buffer
    for (uint i = 0; i < nmx::STRIPS_PER_PLANE; i++) {
        m_mask.at(i) = -1;
        m_cluster.at(i) = {0,0,0};
    }
}
