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

    m_mutex.lock();
    EVMAN::cluster c;
    c.eventnumber = m_nevents;
    c.data = ev;
    m_insertedEvents.push_back(c);

    m_eventBuffer.push_back(c);

    m_nevents++;
    m_mutex.unlock();
}

void EventManager::insertCluster(const nmx::cluster &X, const nmx::cluster &Y) {

    bool verbose = true;

    m_mutex.lock();
    auto XX = convertToVector(X);
    auto YY = convertToVector(Y);

    EVMAN::cluster cluster;
    cluster.data.at(0) = XX;
    cluster.data.at(1) = YY;

    m_nclusters++;

    if (verbose)
        std::cout << "Finding best match ...\n";

    std::array<int, 2> best_match = compare(cluster);

    if (verbose)
        std::cout << "Best match is " << best_match[1] << std::endl;

    auto matchingEvent = m_insertedEvents.at(best_match[1]);

    cluster.eventnumber = matchingEvent.eventnumber;

    m_produced_clusters.push_back(cluster);

    /*std::cout << "Event # " << best_match[1] << " contains " << matchingEvent.data.at(0).size() << " X-points and "
              << matchingEvent.data.at(1).size() << " Y-points.\n";*/

    unsigned long nPointsMatch = matchingEvent.data.at(0).size() + matchingEvent.data.at(1).size();

    if (nPointsMatch == best_match[0]) {
        if (verbose)
            std::cout << "Exact match found !!!\n";

        m_nexact++;
        flushEvent(best_match[1]);
        m_mutex.unlock();
        return;
    }

    if (best_match[0] == 0) {
        std::cout << "No points matched any inserted events\n";
        std::cout << "Cluster :\n";
        printEvent(cluster);
        for (int ievent = 0; ievent < m_insertedEvents.size(); ievent++)
            printEvent(m_insertedEvents.at(ievent));

        m_ofile.close();
        throw 1;
    }

    if (m_insertedEvents.size() > 30) {
        m_ndiscarted_events++;
        m_ndiscarded_points += m_insertedEvents.at(0).data.size();
        flushOldestEvent();
    }

    m_mutex.unlock();
}

std::array<int, 2> EventManager::compare(EVMAN::cluster &cluster) {

    bool verbose = true;

    std::array<int, 2> ret;
    ret[0] = 0; // Number of matching points,
    ret[1] = 0; // Index of best matching event

    // Set the number of points in the cluster

    if (verbose)
        std::cout << "Comparing to " << m_insertedEvents.size() << " inserted events!\n";

    // Loop over the inserted events
    for (int ievent = 0; ievent < m_insertedEvents.size(); ievent++) {

        // Get the inserted event
        const EVMAN::cluster &inserted = m_insertedEvents.at(ievent);

        // Compare cluster to event and get the discrepancy
        int nmatching = numberOfMatchingPoints(cluster, inserted);

        if (verbose)
            std::cout << "Stored event # " << ievent << " has " << nmatching << " matching points!\n";

        if (nmatching > ret[0]) {

            ret[0] = nmatching;
            ret[1] = ievent;
        }
    }

    return ret;
}

int EventManager::numberOfMatchingPoints(const EVMAN::cluster &produced, const EVMAN::cluster &stored) {

    bool verbose = false;

    int nmatches = 0;

    if (verbose)
        std::cout << "Processing X\n";
    nmatches += numberOfMatchingPointsPlane(produced.data.at(0), stored.data.at(0));
    if (verbose)
        std::cout << "Processing Y\n";
    nmatches += numberOfMatchingPointsPlane(produced.data.at(1), stored.data.at(1));

    if (verbose)
        std::cout << "Found " << nmatches << " matches!\n";

    return nmatches;
}

int EventManager::numberOfMatchingPointsPlane(const EVMAN::plane &produced, const EVMAN::plane &stored) {

    bool verbose = false;

    int nmatches = 0;

    if (verbose)
        std::cout << "Produced 'size' = " << produced.size() << ", stored 'size' = " << stored.size() << std::endl;

    for (int ievent = 0; ievent < produced.size(); ievent++) {
        for (int icluster = 0; icluster < stored.size(); icluster++) {

            auto produced_point = produced.at(ievent);
            auto stored_point   = stored.at(icluster);

            if (pointsMatch(produced_point, stored_point))
                nmatches++;
        }
    }

    return nmatches;
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

void EventManager::removeMatchingPoints(EVMAN::cluster &produced, EVMAN::cluster &stored) {

    bool verbose = false;

    if (verbose)
        std::cout << "Removing matching points ...\n";

    if (verbose)
        std::cout << "X plane :\n";
    removeMatchingPointsPlane(produced.data.at(0), stored.data.at(0));

    if (verbose)
        std::cout << "X plane :\n";
    removeMatchingPointsPlane(produced.data.at(1), stored.data.at(1));

    if (verbose)
        std::cout << "Matching points removed!\n";
}


void EventManager::removeMatchingPointsPlane(EVMAN::plane &produced, EVMAN::plane &stored) {

    bool verbose = false;

    for (auto it1 = produced.begin(); it1 != produced.end();) {

        bool foundmatch = false;

        for (auto it2 = stored.begin(); it2 != stored.end(); it2++) {

            if (pointsMatch(*it1, *it2)) {

                produced.erase(it1);
                stored.erase(it2);
                foundmatch = true;
                break;
            }
        }

        if (!foundmatch)
            it1++;
    }
}

bool EventManager::pointsMatch(const nmx::data_point &p1, const nmx::data_point &p2) {

    if ((p1.strip == p2.strip) && (p1.time == p2.time) && (p1.charge == p2.charge))
        return true;

    return false;
}

EVMAN::plane EventManager::convertToVector(const nmx::cluster &cluster) {

    std::vector<nmx::data_point> cl(cluster.data.begin(), cluster.data.begin()+cluster.npoints);

    return cl;
}

void EventManager::flushEvent(uint eventIdx) {

    bool verbose = true;

    auto event = m_eventBuffer.at(eventIdx);

    uint eventno = event.eventnumber;

    if (verbose)
        std::cout << "Flushing event # " << eventno << " at eventIdx " << eventIdx << std::endl;

    m_ofile << "Event # " << eventno << "\n";

    writeEventToFile(event);

    std::cout << "Will now check " << m_produced_clusters.size() << " stored clusters\n";

    for (int clusterIdx = 0; clusterIdx < m_produced_clusters.size(); clusterIdx++) {

        std::cout << "Cluster at idx " << clusterIdx << std::endl;

        auto &cluster = m_produced_clusters.at(clusterIdx);

        if (verbose)
            std::cout << "Cluster matches event # " << cluster.eventnumber << std::endl;

        if (cluster.eventnumber == eventno) {
            if (verbose)
                std::cout << "Writing to file!\n";

            writeEventToFile(cluster);
            m_produced_clusters.erase(m_produced_clusters.begin()+clusterIdx);
        }
    }

    m_insertedEvents.erase(m_insertedEvents.begin()+eventIdx);

    std::cout << "Done flushing event\n";
}

void EventManager::flushOldestEvent() {

    flushEvent(0);
}

void EventManager::printStats() {

    std::cout << "\n";
    std::cout << "Number of inserted events                       : " << std::setw(10) << m_nevents << std::endl;
    std::cout << "Number of produced clusters                     : " << std::setw(10) << m_nclusters << std::endl;
    std::cout << "Number of exactly produced clusters             : " << std::setw(10) << m_nexact << std::endl;
    std::cout << "Number of discarded event fragments             : " << std::setw(10) << m_ndiscarted_events << std::endl;
    std::cout << "Number og discarded points                      : " << std::setw(10) << m_ndiscarded_points << std::endl;
    std::cout << "Average number of discarded points per fragment : " << std::setw(10)
              << double(m_ndiscarded_points)/double(m_ndiscarted_events) << std::endl;

}

inline void EventManager::writeEventToFile(const EVMAN::cluster &cl) {

    m_ofile << "X\n";
    writePlaneToFile(cl.data.at(0));
    m_ofile << "Y\n";
    writePlaneToFile(cl.data.at(1));
}

void EventManager::writePlaneToFile(const EVMAN::plane &plane) {

    for(auto const& val: plane)
        m_ofile << val.strip << " ";
    m_ofile << "\n";
    for(auto const& val: plane)
        m_ofile << val.time << " ";
    m_ofile << "\n";
    for(auto const& val: plane)
        m_ofile << val.charge << " ";
    m_ofile << "\n";
}

void EventManager::printEvent(const EVMAN::cluster &cl) {

    std::cout << "Event # " << cl.eventnumber << std::endl;
    
    std::cout << "X :\n";
    printEvent(cl.data.at(0));
    std::cout << "Y :\n";
    printEvent(cl.data.at(1));
}


void EventManager::printEvent(const EVMAN::plane &plane) {

    for (auto it = plane.begin(); it != plane.end(); it++) {
        std::cout << std::setw(8) << (*it).strip;
    }
    std::cout << "\n";
    for (auto it = plane.begin(); it != plane.end(); it++) {
        std::cout << std::setw(8) << (*it).time;
    }
    std::cout << "\n";
    for (auto it = plane.begin(); it != plane.end(); it++) {
        std::cout << std::setw(8) << (*it).charge;
    }
    std::cout << "\n";
}

EventManager* EventManager::getInstance() {

    if (!instance) {
        instance = new EventManager();
    }

    return instance;
}

EventManager* EventManager::instance = 0;

