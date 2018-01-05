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
    m_ofile.open("NMX_clusterer_output.txt");
}

EventManager::~EventManager() {

    m_ofile.close();
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

    if (verbose)
        std::cout << "Comparing " << cluster_buffer.size() << " clusters to " << m_insertedEvents.size()
                  << " stored events\n";

    // Loop over the produced clusters
    for (uint icluster = 0; icluster < cluster_buffer.size(); icluster++) {

        if (verbose)
            std::cout << "Comparing cluster " << icluster << std::endl;

        // Get the cluster and convert to event-type
        nmx::cluster cl = cluster_buffer.at(icluster);
        std::vector<nmx::data_point> cluster = convertToVector(cl);

        if (verbose)
            std::cout << "Finding best match ...\n";

        std::array<int, 2> best_match = compare(cluster);

        if (verbose)
            std::cout << "Best match is " << best_match[1] << std::endl;

        EVMAN::cluster c;
        c.eventnumber = m_insertedEvents.at(best_match[1]).eventnumber;
        c.data = cluster;

        m_produced_clusters.push_back(c);

        if ((cluster.size() == best_match[0])
            && (cluster.size() == m_insertedEvents.at(best_match[1]).data.size())) {

            if (verbose)
                std::cout << "Exact match found !!!\n";

            m_nexact++;
            flushEvent(best_match[1]);
            //m_insertedEvents.erase(m_insertedEvents.begin()+best_match[1]);

            continue;
        }

        if (best_match[0] == 0) {
            std::cout << "No points matched any inserted events\n";
            std::cout << "Cluster :\n";
            printEvent(cluster);
            for (int ievent = 0; ievent < m_insertedEvents.size(); ievent++)
                printEvent(m_insertedEvents.at(ievent));

            throw 1;
        }

        auto cluster_buf = cluster;
        auto inserted_buf = m_insertedEvents.at(best_match[1]);

        removeMatchingPoints(cluster, m_insertedEvents.at(best_match[1]).data);
/*
        if (cluster.size() != 0) {
            std::cout << "Cluster has " << cluster.size() << " remaining points\n";
            std::cout << "Cluster :\n";
            printEvent(cluster_buf);
            std::cout << "Event :\n";
            printEvent(inserted_buf);

            throw 2;
        }
*/
        if (verbose) {
            std::cout << best_match[0] << " points matched, " << m_insertedEvents.at(best_match[1]).data.size()
                      << " remain\n";
            std::cout << "Cluster :\n";
            printEvent(cluster_buf);
            std::cout << "Event :\n";
            printEvent(inserted_buf);
            std::cout << "Remaining : \n";
            printEvent(m_insertedEvents.at(best_match[1]));
        }

    }

    cluster_buffer.clear();

    if (m_insertedEvents.size() > 20) {
        m_ndiscarted_events++;
        m_ndiscarded_points += m_insertedEvents.at(0).data.size();
        flushOldestEvent();
    }
}

void EventManager::flushBuffer() {

    bool verbose = false;

    if (verbose)
        std::cout << "Flushing buffer with " << m_insertedEvents.size() << " events :\n";

    while (m_insertedEvents.size() > 0) {
        m_ndiscarted_events++;
        m_ndiscarded_points += m_insertedEvents.at(0).data.size();
        flushOldestEvent();

        if (verbose)
            std::cout << m_insertedEvents.size() << " events remain!\n";
    }
}

void EventManager::printStats() {

    std::cout << "\n";
    std::cout << "Number of inserted events                       : " << std::setw(10) << m_nevents << std::endl;
    std::cout << "Number of produced clusters                     : " << std::setw(10) << m_nclusters << std::endl;
    std::cout << "Number of exactly produced clusters             : " << std::setw(10) << m_nexact << std::endl;
    std::cout << "Number of discarded event fragments             : " << std::setw(10) << m_ndiscarted_events << std::endl;
    std::cout << "Numner og discarded points                      : " << std::setw(10) << m_ndiscarded_points << std::endl;
    std::cout << "Average number of discarded points per fragment : " << std::setw(10)
              << double(m_ndiscarded_points)/double(m_ndiscarted_events) << std::endl;

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
        if (nmatching > ret[0]) {

            ret[0] = nmatching;
            ret[1] = ievent;
        }
    }

    return ret;
}


void EventManager::removeMatchingPoints(std::vector<nmx::data_point> &cluster, std::vector<nmx::data_point> &stored) {

    bool verbose = false;

    if (verbose)
        std::cout << "Removing matching points ...\n";

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

    if (verbose)
        std::cout << "Matching points removed!\n";
}

int EventManager::numberOfMatchingPoints(const std::vector<nmx::data_point> &cluster,
                                         const std::vector<nmx::data_point> &event) {

    bool verbose = false;

    if (verbose)
        std::cout << "plane size = " << event.size() << ", cluster size = " << cluster.size() << std::endl;

    int iter1 = 0;

    int nmatchingpoints = 0;

        for (auto it1 = event.begin(); it1 != event.end(); ) {

            if (verbose)
                std::cout << "event[" << iter1 << "] : ";

            nmx::data_point epoint = *it1;

            if (verbose)
                std::cout << "strip = " << epoint.strip << ", time = " << epoint.time << ", charge = "
                          << epoint.charge << std::endl;

            int iter2 = 0;

            for (auto it2 = cluster.begin(); it2 != cluster.end(); ) {

                if (verbose)
                    std::cout << "cluster[" << iter2 << "] : ";

                nmx::data_point cpoint = *it2;

                if (verbose)
                    std::cout << "strip = " << cpoint.strip << ", time = " << cpoint.time << ", charge = " << cpoint.charge
                              << std::endl;

                if (pointsMatch(epoint, cpoint)) {

                    nmatchingpoints++;

                    if (verbose)
                        std::cout << "MATCH !!!!\n";

                    break;
                }

                it2++;
                iter2++;
            }

            it1++;
            iter1++;
        }

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

void EventManager::flushEvent(uint idx) {

    auto event = m_eventBuffer.at(idx);

    uint eventno = event.eventnumber;

    //std::cout << "Event # " << eventno << "\n";

    m_ofile << "Event # " << eventno << "\n";

    writeEventToFile(event);

    for (auto &cluster : m_produced_clusters) {

        if (cluster.eventnumber == eventno)
            writeEventToFile(cluster);
    }

    m_insertedEvents.erase(m_insertedEvents.begin()+idx);
    m_eventBuffer.erase(m_eventBuffer.begin()+idx);
}

void EventManager::flushOldestEvent() {

    flushEvent(0);

    /*
    uint eventno = m_eventBuffer.at(0).eventnumber;

    std::cout << "Event # " << eventno << "\n";

    m_ofile << "Event # " << eventno << "\n";

    writeEventToFile(m_eventBuffer.at(0));

    uint cl_cnt = 0;

    for (auto &val : m_produced_clusters) {

        if (val.eventnumber == eventno) {

//            m_ofile << "Cluster # " << cl_cnt << "\n";
            writeEventToFile(val);
        }
    }

    m_insertedEvents.erase(m_insertedEvents.begin());
    m_eventBuffer.erase(m_eventBuffer.begin());
     */
}


inline void EventManager::writeEventToFile(const EVMAN::cluster &cl) {

    const EVMAN::event &cl_data = cl.data;

    for(auto const& val: cl_data)
        m_ofile << val.strip << " ";
    m_ofile << "\n";
    for(auto const& val: cl_data)
        m_ofile << val.time << " ";
    m_ofile << "\n";
    for(auto const& val: cl_data)
        m_ofile << val.charge << " ";
    m_ofile << "\n";
}

void EventManager::printEvent(const EVMAN::cluster &cl) {

    std::cout << "Event # " << cl.eventnumber << std::endl;
    const EVMAN::event &ev = cl.data;

    printEvent(ev);
}

void EventManager::printEvent(const std::vector<nmx::data_point> &ev) {

    for (auto it = ev.begin(); it != ev.end(); it++) {
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