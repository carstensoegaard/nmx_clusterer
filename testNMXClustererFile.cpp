//
// Created by soegaard on 11/2/17.
//

#ifndef TEST_CLUSTERER_FILE
#define TEST_CLUSTERER_FILE

#include <iomanip>
#include <iostream>
#include <vector>
#include <algorithm>

#include "NMXClustererDefinitions.h"
#include "Clusterer.h"
#include "SpecialDataReader.h"
#include "EventManager.h"

typedef std::chrono::high_resolution_clock Clock;

int main() {

    srand(1);

    uint32_t nspertimebin = 32;
    uint32_t maxbinsperevent = 30;
    uint32_t maxtimeperevent = nspertimebin*maxbinsperevent*2;

    std::mutex m;
    Clusterer c(m);
    c.setVerboseLevel(0);

    std::vector<plane> insertedEvents;


    uint repeat = 0;

    SpecialDataReader reader;

    std::vector<event> events;

    bool cont = true;

    while (cont) {

        event ievent = reader.ReadNextEvent();

        if ((ievent.at(0).size() == 0) && ievent.at(1).size() == 0)
            cont = false;
        else
            events.push_back(ievent);
    }

    EventManager evman;

    int nrepeats = 1000;
    int multiplier = 5;

    std::cout << "\nWill repeat " << events.size() << " events " << nrepeats << " times.\n";

    uint64_t npoints = 0;

    auto t1 = Clock::now();

    while (repeat < nrepeats) {

        //std::cout << "*** Repeat # " << repeat << " ***\n";

        for (int ievent = 0; ievent < /*1*/events.size(); ievent++) {

            event ev = events.at(ievent);

            for (int iplane = 0; iplane < 2; iplane++) {

                plane xplane = (ev.at(iplane));

                for (int i = 0; i < xplane.size(); i++) {

                    xplane.at(i).time = xplane.at(i).time * nspertimebin + multiplier * maxtimeperevent;
                }

                evman.insertEvent(xplane);

                insertedEvents.push_back(xplane);

                while (xplane.size() > 0) {

                    uint ipoint = rand() % xplane.size();

                    nmx::data_point point = xplane.at(ipoint);
                    xplane.erase(xplane.begin() + ipoint);

                    uint32_t strip = point.strip;
                    uint32_t time = point.time;
                    uint32_t charge = point.charge;

                    c.addDataPoint(strip, time, charge);
                    npoints++;

                    m.lock();
                    std::vector<nmx::cluster> &produced_clusters = c.getProducedClusters();
                    while (produced_clusters.size() > 0) {
                        evman.compareToStored(produced_clusters);
                    }
                    m.unlock();
                }

                multiplier++;
            }
        }

        repeat++;
    }

    c.endRun();

    std::vector<nmx::cluster> &produced_clusters = c.getProducedClusters();
    while (produced_clusters.size() > 0) {
        evman.compareToStored(produced_clusters);
    }

    evman.flushBuffer();

    auto t2 = Clock::now();

    std::cout << "Number of inserted data-points                  : " << std::setw(10) << npoints << std::endl;
    evman.printStats();

    auto time = std::chrono::duration_cast<std::chrono::microseconds>(t2 - t1).count();

    std::cout << "Performed in " << time << " micro-seconds\n";
    std::cout << "Time per data point is : " << 1.*time/npoints << " microseconds\n";
    std::cout << "Which is a rate of " << 1./(time/npoints/1000000.) << " Hz\n";

    c.terminate();

    return 0;
}

#endif