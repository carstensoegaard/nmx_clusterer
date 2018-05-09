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
#include "NMXPlaneClusterer.h"
#include "SpecialDataReader.h"
#include "NMXClustererVerification.h"
#include "NMXClusterer.h"

typedef std::chrono::high_resolution_clock Clock;

void writeEventToFile(std::ofstream &file, nmx::fullCluster &event) {

    file << "Event # " << event.eventNo << std::endl;

    for (int plane = 0; plane < 2; plane++) {

        file << (plane ? "Y:" : "X:") << std::endl;

        int nPoints = event.clusters.at(plane).npoints;

        for (int ipoint = 0; ipoint < nPoints; ipoint++)
            file << event.clusters.at(plane).data.at(ipoint).time << " ";
        file << std::endl;

        for (int ipoint = 0; ipoint < nPoints; ipoint++)
            file << event.clusters.at(plane).data.at(ipoint).strip << " ";
        file << std::endl;

        for (int ipoint = 0; ipoint < nPoints; ipoint++)
            file << event.clusters.at(plane).data.at(ipoint).charge << " ";
        file << std::endl;
    }
}

int main() {

    srand(1);

    unsigned int nspertimebin = 32;
    unsigned int maxbinsperevent = 30;
    unsigned int maxtimeperevent = nspertimebin*maxbinsperevent*2;

    NMXClusterer c;

    SpecialDataReader reader;
    std::vector<nmx::fullCluster> events;

    std::ofstream file;
    file.open("NMX_input_events.txt");

    unsigned int nrepeats = 1;
    unsigned int multiplier = 5;

    bool cont = true;
    unsigned int repeat = 0;

    while (cont) {

        nmx::fullCluster ievent = reader.ReadNextEvent();

        if ((ievent.clusters.at(0).npoints == 0) && (ievent.clusters.at(1).npoints == 0))
            cont = false;
        else
            events.push_back(ievent);
    }

    std::cout << "\nWill repeat " << events.size() << " events " << nrepeats << " times.\n";

    uint64_t npoints = 0;

    auto t1 = Clock::now();

    while (repeat < nrepeats) {

        std::cout << "*** Repeat # " << repeat << " ***\n";

        for (unsigned int ievent = 0; ievent < /*1*/events.size(); ievent++) {

            // Create a copy of the event
            nmx::fullCluster ev = events.at(ievent);
            ev.eventNo = ievent + events.size()*repeat;

            for (unsigned int iplane = 0; iplane < 2; iplane++) {

                // Get the reference to the specific plane
                nmx::cluster_points &plane = ev.clusters.at(iplane).data;

                // Modify time for the copy
                for (unsigned int i = 0; i < ev.clusters.at(iplane).npoints; i++) {
                    uint32_t time = plane.at(i).time * nspertimebin + multiplier * maxtimeperevent;
                    plane.at(i).time = time;
                }


                // Convert the copy to a vector - for simple jumbling of point order
                std::vector<nmx::data_point> planeV(plane.begin(), plane.begin()+ev.clusters.at(iplane).npoints);

                // Jumble points and erase them once inserted
                while (planeV.size() > 0) {

                    uint ipoint = rand() % planeV.size();

                    nmx::data_point point = planeV.at(ipoint);
                    c.addDatapoint(iplane, point);
                    planeV.erase(planeV.begin() + ipoint);

                    npoints++;
                }
            }

            writeEventToFile(file, ev);

            multiplier++;
        }

        repeat++;
    }

    c.endRun();

    auto t2 = Clock::now();

    std::cout << "Number of inserted data-points                  : " << std::setw(10) << npoints << std::endl;

    auto time = std::chrono::duration_cast<std::chrono::microseconds>(t2 - t1).count();

    std::cout << "Performed in " << time << " micro-seconds\n";
    std::cout << "Time per data point is : " << 1.*time/npoints << " microseconds\n";
    std::cout << "Which is a rate of " << 1./(time/npoints/1000000.) << " Hz\n";
    std::cout << "\nNumber of old points (X,Y) : ( " << c.getNumberOfOldPointsX() << " , " << c.getNumberOfOldPointsY()
              << " )" << std::endl;
    std::cout << "Number of failed cluster-requests : " << c.getFailedClusterRequests() << std::endl;
    std::cout << "Number of late clusters : " << c.getNumberOfLateClusters() << std::endl;

    c.terminate();

    file.close();

    return 0;
}

#endif