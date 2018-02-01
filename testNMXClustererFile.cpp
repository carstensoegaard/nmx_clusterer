//
// Created by soegaard on 11/2/17.
//

#include <iomanip>
#include <iostream>
#include <vector>
#include <algorithm>

#include "NMXClustererDefinitions.h"
#include "Clusterer.h"
#include "SpecialDataReader.h"
#include "EventManager.h"

int main() {

    srand(1);

    uint32_t nspertimebin = 32;
    uint32_t maxbinsperevent = 30;
    uint32_t maxtimeperevent = nspertimebin*maxbinsperevent*2;

    Clusterer c;
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

    int nrepeats = 2;
    int multiplier = 5;

    std::cout << "\nWill repeat " << events.size() << " events " << nrepeats << " times.\n";

    uint nproducedclusters = 0;

    while (repeat < nrepeats) {

        std::cout << "*** Repeat # " << repeat << " ***\n";

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

                    //std::cout << "Inserting point # " << ipoint << " of " << xplane.size() << " points." << std::endl;

                    nmx::data_point point = xplane.at(ipoint);
                    xplane.erase(xplane.begin() + ipoint);

                    uint32_t strip = point.strip;
                    uint32_t time = point.time;
                    uint32_t charge = point.charge;

                    c.addDataPoint(strip, time, charge);
                    //std::this_thread::sleep_for(std::chrono::milliseconds(1000));

                    c.setReadLock();
                    std::vector<nmx::cluster> &produced_clusters = c.getProducedClusters();

                    //if (produced_clusters.size() > 0)
                      //  std::cout << "Received " << produced_clusters.size() << " clusters.\n";

/*  for (uint icluster = 0; icluster < produced_clusters.size(); icluster++) {

                        auto cluster = produced_clusters.at(icluster);

                        auto points = cluster.data;

                        for (uint ipoint = 0; ipoint < cluster.npoints; ipoint++) {

                            auto point = points.at(ipoint);

                            if (point.time == 19328)
                                std::cout << "I GOT THE POINT!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!\n";
                        }
                    }*/

                    /*while (produced_clusters.size() > 0) {
                        evman.compareToStored(produced_clusters);
                    }*/

                    nproducedclusters += produced_clusters.size();

                    c.getProducedClusters().clear();
                    c.releaseReadLock();
                }

                multiplier++;
            }
        }

        repeat++;
    }

    c.endRun();

    std::vector<nmx::cluster> &produced_clusters = c.getProducedClusters();
    //std::cout << "Received " << produced_clusters.size() << " clusters.\n";
    /*while (produced_clusters.size() > 0) {
        evman.compareToStored(produced_clusters);
    }*/


    nproducedclusters += produced_clusters.size();

    evman.flushBuffer();

    evman.printStats();

    std::cout << "Produced " << nproducedclusters << " clusters\n";

    c.terminate();

    return 0;
}