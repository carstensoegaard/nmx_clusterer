//
// Created by soegaard on 12/12/17.
//

#include <iostream>
#include <iomanip>

#include "EventManager.h"

EventManager::EventManager()
: m_nevents(0),
  m_nclusters(0),
  m_nexact(0),
  m_ndiscarded_points(0),
  m_ndiscarted_events(0)
{

}

void EventManager::insertEvent(const EVMAN::event &ev) {

    EVMAN::cluster c;
    c.eventnumber = m_nevents;
    c.data = ev;
    m_insertedEvents.push_back(c);

    m_eventBuffer.push_back(c);

    m_nevents++;
}

void EventManager::compareToStored(std::vector<nmx::cluster> &cluster_buffer) {

    bool verbose = false;

    m_nclusters += cluster_buffer.size();

    if (1/*verbose*/)
        std::cout << "Comparing " << cluster_buffer.size() << " clusters to " << m_insertedEvents.size()
                  << " stored events\n";

    // Loop over the produced clusters
    for (uint icluster = 0; icluster < cluster_buffer.size(); icluster++) {

        if (verbose)
            std::cout << "Comparing cluster " << icluster << std::endl;

        // Get the cluster and convert to event-type
        nmx::cluster cl = cluster_buffer.at(icluster);
        std::vector<nmx::data_point> cluster = convertToVector(cl);

        std::array<int, 2> best_match = compare(cluster);

        EVMAN::cluster c;
        c.eventnumber = m_insertedEvents.at(best_match[1]).eventnumber;
        c.data = cluster;

        m_produced_clusters.push_back(c);

        if ((cluster.size() == best_match[0])
            && (cluster.size() == m_insertedEvents.at(best_match[1]).data.size())) {

            std::cout << "Exact match found !!!\n";

            continue;
        }

        if (best_match[1] == 0) {
            std::cout << "No points matched any inserted events\n";
            std::cout << "Cluster :\n";
            printEvent(cluster);
            for (int ievent = 0; ievent < m_insertedEvents.size(); ievent++)
                printEvent(m_insertedEvents.at(ievent));

            throw 1;
        }



        auto cluster_buf = cluster;
        auto inserted_buf = m_insertedEvents.at(best_match[2]);

        removeMatchingPoints(cluster, m_insertedEvents.at(best_match[2]).data);

        if (cluster.size() != 0) {
            std::cout << "Cluster has " << cluster.size() << " remaining points\n";
            std::cout << "Cluster :\n";
            printEvent(cluster_buf);
            std::cout << "Event :\n";
            printEvent(inserted_buf);

            throw 2;
        }

        if (verbose) {
            std::cout << best_match[1] << " points matched, " << m_insertedEvents.at(best_match[2]).data.size()
                      << " remain\n";
            std::cout << "Cluster :\n";
            printEvent(cluster_buf);
            std::cout << "Event :\n";
            printEvent(inserted_buf);
            std::cout << "Remaining : \n";
            printEvent(m_insertedEvents.at(best_match[2]));
        }

        std::cout << "Removing repeated strips\n";
        removeRepeatedStrips(m_insertedEvents.at(best_match[2]).data, inserted_buf.data);
        if (m_insertedEvents.at(best_match[2]).data.size() == 0) {
            std::cout << "All points accounted for\n";
            m_insertedEvents.erase(m_insertedEvents.begin()+best_match[2]);
        } else {
            std::cout << m_insertedEvents.at(best_match[2]).data.size() << " points still remain\n";
            std::cout << "Remaining : \n";
            printEvent(m_insertedEvents.at(best_match[2]));
        }
    }

    cluster_buffer.clear();
/*
    if (m_insertedEvents.size() > 20) {
        m_ndiscarted_events++;
        m_ndiscarded_points += m_insertedEvents.at(0).size();
        m_insertedEvents.erase(m_insertedEvents.begin());
    }
*/
}

std::array<int, 2> EventManager::compare(std::vector<nmx::data_point> &cluster) {

    std::array<int, 2> ret;
    ret[0] = 0; // Number of matching points
    ret[1] = 0; // Index of best matching event

    // Set the number of points in the cluster
    //uint npoints = cluster.size();

    // Loop over the inserted events
    for (int ievent = 0; ievent < m_insertedEvents.size(); ievent++) {

        // Get the inserted event
        const EVMAN::cluster &inserted = m_insertedEvents.at(ievent);

        // Compare cluster to event and get the discrepancy
        int nmatching = numberOfMatchingPoints(cluster, inserted.data);

        //std::cout << nmatching << " matching points found\n";
        if (nmatching > ret[1]) {

            ret[1] = nmatching;
            ret[2] = ievent;
        }
    }

    return ret;
}

void EventManager::removeRepeatedStrips(std::vector<nmx::data_point> &remain, std::vector<nmx::data_point> &stored) {

    bool verbose = false;

    for (auto ipoint = remain.begin(); ipoint != remain.end();) {

        int cnt = 0;
        nmx::data_point point1 = *ipoint;

        if (verbose)
            std::cout << "Checking for strip " << point1.strip << std::endl;

        for (int iipoint = 0; iipoint < stored.size(); iipoint++) {

            nmx::data_point point2 = stored.at(iipoint);

            if (verbose)
                std::cout << std::setw(5) << point2.strip;

            if (point1.strip == point2.strip) {

                cnt++;
                if (verbose)
                    std::cout << " Found strip " << cnt << " times ";
            }
        }

        if (cnt > 1) {
            if (verbose)
                std::cout << " Strip " << point1.strip << " is repeated - removing\n";
            remain.erase(ipoint);
        } else
            ipoint++;
    }
}

void EventManager::removeMatchingPoints(std::vector<nmx::data_point> &cluster, std::vector<nmx::data_point> &stored) {

    for (auto it1 = cluster.begin(); it1 != cluster.end();) {

        bool foundmatch = false;

        for (auto it2 = stored.begin(); it2 != stored.end(); it2++) {

            if (pointsMatch(*it1, *it2)) {

                //  std::cout << "MATCH !!!!\n";

                cluster.erase(it1);
                stored.erase(it2);
                foundmatch = true;
                break;
            }
        }

        if (!foundmatch)
            it1++;
    }
}

int EventManager::numberOfMatchingPoints(const std::vector<nmx::data_point> &cluster,
                                         const std::vector<nmx::data_point> &event) {

    //std::cout << "plane size = " << plane.size() << ", cluster size = " << cluster.size() << std::endl;

    int iter1 = 0;

    int nmatchingpoints = 0;

        for (auto it1 = event.begin(); it1 != event.end(); ) {
/*
        std::cout << "plane[" << iter1 << "] : ";
*/
            nmx::data_point epoint = *it1;
/*
        std::cout << "strip = " << epoint.strip << ", time = " << epoint.time << ", charge = " << epoint.charge
                  << std::endl;
*/
            int iter2 = 0;

            for (auto it2 = cluster.begin(); it2 != cluster.end(); ) {
/*
           std::cout << "cluster[" << iter2 << "] : ";
*/
                nmx::data_point cpoint = *it2;
/*
            std::cout << "strip = " << cpoint.strip << ", time = " << cpoint.time << ", charge = " << cpoint.charge
                      << std::endl;
*/
                if (pointsMatch(epoint, cpoint)) {

                    nmatchingpoints++;
                    //  std::cout << "MATCH !!!!\n";

                    //plane.erase(it1);
                    //cluster.erase(it2);

                    //  std::cout << "plane size = " << plane.size() << ", cluster size = " << cluster.size() << std::endl;

                    //it1--;
                    //iter1--;

                    break;
                }

                it2++;
                iter2++;
            }

            it1++;
            iter1++;
        }
/*
    if (c_size - cluster.size() != p_size - plane.size()) {
        std::cout << "Somethings wrong! Did not find the same number of points in cluster and plane\n";
        std::cout << "cluster : " << c_size << " - " << cluster.size() << " = " << c_size - cluster.size() << std::endl;
            std::cout << "plane   : " << p_size << " - " << plane.size() << " = " << p_size - plane.size() << std::endl;
    }
  */
    return nmatchingpoints;
}

bool EventManager::pointsMatch(const nmx::data_point &p1, const nmx::data_point &p2) {

    if ((p1.strip == p2.strip) && (p1.time == p2.time) && (p1.charge == p2.charge))
        return true;

    return false;
}


std::vector<nmx::data_point> EventManager::convertToVector(nmx::cluster &cluster) {

    std::vector<nmx::data_point> cl(cluster.data.begin(), cluster.data.begin()+cluster.npoints);

    return cl;
}

void EventManager::printStats() {

    std::cout << "\n";
    std::cout << "Number of inserted events                       : " << std::setw(10) << m_nevents << std::endl;
    std::cout << "Number of produced clusters                     : " << std::setw(10) << m_nclusters << std::endl;
    std::cout << "Number of exactly produced clusters             : " << std::setw(10) << m_nexact << std::endl;
    std::cout << "Number of discarded events fragments            : " << std::setw(10) << m_ndiscarted_events << std::endl;
    std::cout << "Numner og discarded points                      : " << std::setw(10) << m_ndiscarded_points << std::endl;
    std::cout << "Average number of discarded points per fragment : " << std::setw(10)
              << double(m_ndiscarded_points)/double(m_ndiscarted_events) << std::endl;

}

void EventManager::flushClusters() {

    cluster

}


void EventManager::printEvent(const cluster &cl) {

    std::cout << "Event # " << cl.eventnumber << std::endl;

    ins_event &ev = cl.data;

    printEvent(ev);
}

void EventManager::printEvent(const std::vector<nmx::data_point> &ev) {

    for (auto it = ev.begin(); it != ev.end(); it++) {
        //nmx::data_point p = *it;
        std::cout << std::setw(8) << (*it).strip;
    }
    std::cout << "\n";
    for (auto it = ev.begin(); it != ev.end(); it++) {
        std::cout << std::setw(8) << (*it).time;
    }
    std::cout << "\n";
    for (auto it = ev.begin(); it != ev.end(); it++) {
        std::cout << std::setw(8) << (*it).charge;
    }
    std::cout << "\n";
}