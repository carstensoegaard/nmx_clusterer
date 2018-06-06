//
// Created by soegaard on 12/12/17.
//

#ifndef PROJECT_EVENTMANAGER_H
#define PROJECT_EVENTMANAGER_H

#include <vector>
#include <array>
#include <fstream>
#include <thread>
#include <mutex>

#include "../clusterer/include/NMXClustererDefinitions.h"

namespace EVMAN {

    typedef std::vector<nmx::data_point> plane;
    typedef std::array<plane, 2> event;

    struct cluster {

        uint64_t eventnumber;
        event data;
    };

    typedef std::vector<cluster> buffer;
}

class EventManager {

private:
    EventManager();
public:

    static EventManager* getInstance();
    ~EventManager();

    void insertEvent(const EVMAN::event &ev);
    void insertCluster(const nmx::cluster &X, const nmx::cluster &Y);
    void flushBuffer();

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

    std::mutex m_mutex;

    std::ofstream m_ofile;

    std::array<int, 2> compare(EVMAN::cluster &cluster);

    int numberOfMatchingPoints(const EVMAN::cluster &produced, const EVMAN::cluster &stored);
    int numberOfMatchingPointsPlane(const EVMAN::plane &produced, const EVMAN::plane &stored);

    void removeMatchingPoints(EVMAN::cluster &produced, EVMAN::cluster &stored);
    void removeMatchingPointsPlane(EVMAN::plane &produced, EVMAN::plane &stored);

    bool pointsMatch(const nmx::data_point &p1, const nmx::data_point &p2);

    void flushOldestEvent();
    void flushEvent(uint eventIdx);
    void writeEventToFile(const EVMAN::cluster &cl);
    void writePlaneToFile(const EVMAN::plane &plane);

    EVMAN::plane convertToVector(const nmx::cluster &cluster);
    void printEvent(const EVMAN::cluster &cl);
    void printEvent(const EVMAN::plane &plane);

    static EventManager* instance;
};


#endif //PROJECT_EVENTMANAGER_H
