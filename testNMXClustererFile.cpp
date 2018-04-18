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

int main() {

    srand(1);

    unsigned int nspertimebin = 32;
    unsigned int maxbinsperevent = 30;
    unsigned int maxtimeperevent = nspertimebin*maxbinsperevent*2;

    NMXClusterer c;
    //c.setVerboseLevel(0);

    NMXClustererVerification* verification = NMXClustererVerification::getInstance();

    SpecialDataReader reader;

    std::vector<event> events;

    bool cont = true;
    uint repeat = 0;

    while (cont) {

        event ievent = reader.ReadNextEvent();

        if ((ievent.at(0).size() == 0) && ievent.at(1).size() == 0)
            cont = false;
        else
            events.push_back(ievent);
    }

    int nrepeats = 10;
    int multiplier = 5;

    std::cout << "\nWill repeat " << events.size() << " events " << nrepeats << " times.\n";

    uint64_t npoints = 0;

    auto t1 = Clock::now();

    while (repeat < nrepeats) {

        std::cout << "*** Repeat # " << repeat << " ***\n";

        for (int ievent = 0; ievent < /*1*/events.size(); ievent++) {

            event ev = events.at(ievent);

            EVMAN::event mod_event;

            for (int iplane = 0; iplane < 2; iplane++) {

                plane p = (ev.at(iplane));

                for (int i = 0; i < p.size(); i++)
                    p.at(i).time = p.at(i).time * nspertimebin + multiplier * maxtimeperevent;

                mod_event.at(iplane) = p;

                while (p.size() > 0) {

                    uint ipoint = rand() % p.size();

                    nmx::data_point point = p.at(ipoint);
                    c.addDatapoint(iplane, point);
                    p.erase(p.begin() + ipoint);

                    npoints++;
                }
            }

            evman->insertEvent(mod_event);

            multiplier++;
        }

        repeat++;
    }

    c.endRun();

    evman->flushBuffer();

    auto t2 = Clock::now();

    std::cout << "Number of inserted data-points                  : " << std::setw(10) << npoints << std::endl;
    evman->printStats();

    auto time = std::chrono::duration_cast<std::chrono::microseconds>(t2 - t1).count();

    std::cout << "Performed in " << time << " micro-seconds\n";
    std::cout << "Time per data point is : " << 1.*time/npoints << " microseconds\n";
    std::cout << "Which is a rate of " << 1./(time/npoints/1000000.) << " Hz\n";

    c.terminate();

    return 0;
}

#endif