//
// Created by soegaard on 12/12/17.
//

#ifndef PROJECT_EVENTMANAGER_H
#define PROJECT_EVENTMANAGER_H

#include <vector>
#include <array>
#include <fstream>

#include "NMXClustererDefinitions.h"

namespace EVMAN {

    typedef std::vector<nmx::data_point> event;

    struct cluster {

        uint64_t eventnumber;
        event data;
    };

    typedef std::vector<cluster> buffer;
}

class EventManager {

public:

    EventManager();
    ~EventManager();

    void compareToStored(std::vector<nmx::cluster> &cluster_buffer);

    void insertEvent(const EVMAN::event &ev);

    void printStats();

private:

    uint64_t m_nevents;
    uint64_t m_nclusters;
    uint64_t m_nexact;
    uint64_t m_ndiscarded_points;
    uint64_t m_ndiscarted_events;

    EVMAN::buffer m_insertedEvents;
    EVMAN::buffer m_eventBuffer;
    EVMAN::buffer m_produced_clusters;

    std::ofstream m_ofile;

    std::array<int, 2> compare(EVMAN::event &cluster);

    int numberOfMatchingPoints(const EVMAN::event &cluster, const EVMAN::event &event);
    void removeMatchingPoints(EVMAN::event &cluster, EVMAN::event &stored);
    void removeRepeatedStrips(EVMAN::event &remain,  EVMAN::event &stored);
    bool pointsMatch(const nmx::data_point &p1, const nmx::data_point &p2);

    void flushOldestEvent();
    void writeEventToFile(const EVMAN::cluster &cl);

    EVMAN::event convertToVector(nmx::cluster &cluster);
    void printEvent(const EVMAN::cluster &cl);
    void printEvent(const EVMAN::event &ev);



};


#endif //PROJECT_EVENTMANAGER_H
