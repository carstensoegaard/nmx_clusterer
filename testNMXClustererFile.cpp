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

void printBuffer(const nmx::row_array &mask, const nmx::time_ordered_buffer &buffer) {

    for (int i = 0; i < nmx::MAX_MINOR; ++i) {

        std::cout << "b2[" << std::setw(2) << i << "]=" << std::setw(2) << mask[i] << ", number of points = "
                  << buffer.at(i).npoints;
        std::cout << "\n" << std::setw(10) << " ";
        for (int j = 0; j < buffer.at(i).npoints; ++j)
            std::cout << std::setw(5) << buffer.at(i).data.at(j).strip << " ";
        std::cout << "\n" << std::setw(10) << " ";
        for (int j = 0; j < buffer.at(i).npoints; ++j)
            std::cout << std::setw(5) << buffer.at(i).data.at(j).time << " ";
        std::cout << "\n" << std::setw(10) << " ";
        for (int j = 0; j < buffer.at(i).npoints; ++j)
            std::cout  << std::setw(5) << buffer.at(i).data.at(j).charge << " ";
        std::cout << "\n";
        std::cout << "\n";
    }
}

void printMask(const nmx::col_array &mask) {

    std::cout << "Mask:\n" << std::setw(10) << " " ;
    for (int i = 0; i < mask.size(); ++i) {
        std::cout << std::setw(5) << mask.at(i) << " ";
    }
    std::cout << "\n";
}

void printCluster(const std::vector<nmx::data_point> &cluster) {

    std::cout << "Cluster :\n";

    uint ipoint  =  0;

    for (auto &point : cluster) {
        std::cout << "        Point " << ipoint << std::endl;
        std::cout << "          Strip = " << point.strip << ", charge = " << point.charge << ", time = " << point.time
                  << std::endl;

        ipoint++;
    }
}

std::vector<nmx::data_point> convertToVector(nmx::cluster &cluster) {

    std::vector<nmx::data_point> cl(cluster.data.begin(), cluster.data.begin()+cluster.npoints);

    return cl;
}

int32_t findDescrepancy(std::vector<nmx::data_point> cluster, std::vector<nmx::data_point> plane){

    //std::cout << "plane size = " << plane.size() << ", cluster size = " << cluster.size() << std::endl;

    int iter1 = 0;

    for (auto it1 = plane.begin(); it1 != plane.end(); ) {

        //std::cout << "plane[" << iter1 << "] : ";

        nmx::data_point epoint = *it1;
/*
        std::cout << "strip = " << epoint.strip << ", time = " << epoint.time << ", charge = " << epoint.charge
                  << std::endl;
*/
        int iter2 = 0;

        for (auto it2 = cluster.begin(); it2 != cluster.end(); ) {

           // std::cout << "cluster[" << iter2 << "] : ";

            nmx::data_point cpoint = *it2;
/*
            std::cout << "strip = " << cpoint.strip << ", time = " << cpoint.time << ", charge = " << cpoint.charge
                      << std::endl;
*/
            if ((cpoint.strip == epoint.strip)
                && (cpoint.time == epoint.time)
                && (cpoint.charge == epoint.charge)) {

                //std::cout << "MATCH !!!!\n";

                plane.erase(it1);
                cluster.erase(it2);

                //std::cout << "plane size = " << plane.size() << ", cluster size = " << cluster.size() << std::endl;

                it1--;
                iter1--;

                break;
            }

            it2++;
            iter2++;
        }

        it1++;
        iter1++;
    }

    return cluster.size() - plane.size();
}

void compareClusterToEvents(nmx::cluster cluster, std::vector<plane> &events) {

    int32_t min_lostpoints = INT32_MAX;
    int32_t min_gainedpoints = INT32_MAX;
    int32_t best_lost = INT32_MAX;
    int32_t best_gained = INT32_MAX;

    uint ievent = 0;

    auto cl = convertToVector(cluster);

    for (auto it = events.begin(); it != events.end();) {

        plane plane = *it;

/*
        std::cout << "Comparing : \n";
        printCluster(cl);
        std::cout << "To : \n";
        printCluster(plane);
*/
        int32_t descrepancy = findDescrepancy(cl, plane);

        if (descrepancy == 0) {
            std::cout << "Exact match found!" << std::endl;

            events.erase(it);
            return;
        } else {

            if (descrepancy < 0) {
                if (std::abs(descrepancy) < min_lostpoints) {
                    min_lostpoints = std::abs(descrepancy);
                    best_lost = ievent;
                }
            } else {
                if (descrepancy < min_gainedpoints) {
                    min_gainedpoints = descrepancy;
                    best_gained = ievent;
                }
            }
        }

        it++;
        ievent++;
    }

    std::cout << "Best match ";

    if (min_lostpoints < min_gainedpoints) {
        std::cout << "lost " << min_lostpoints << " points!\n";
        events.erase(events.begin()+best_lost);

    } else {
        std::cout << "gained " << min_gainedpoints << " points!\n";
        events.erase(events.begin()+best_gained);
    }
}

int main() {

    uint32_t nspertimebin = 32;
    uint32_t maxbinsperevent = 30;
    uint32_t maxtimeperevent = nspertimebin*maxbinsperevent*1.5;

    Clusterer c;

    //printBuffer(c.getMajorTimeBuffer(), c.getTimeOrderedBuffer());

    std::vector<plane> insertedEvents;

    uint repeat = 0;

    SpecialDataReader reader;

    event event = reader.ReadNextEvent();

    while (repeat < 10) {

        std::cout << "*** Repeat # " << repeat << " ***\n";

        for (int iplane = 0; iplane < 2; iplane ++) {

            std::cout << "Plane " << iplane << std::endl;

            plane xplane = (event.at(iplane));

            for (int i = 0; i < xplane.size(); i++) {

                //xplane.at(i).strip -= 150;
                xplane.at(i).time = xplane.at(i).time * nspertimebin + (2 * repeat + iplane) * maxtimeperevent;
                //xplane.at(i).strip = xplane.at(i).strip - 150;
            }

            insertedEvents.push_back(xplane);

            while (xplane.size() > 0) {

                uint ipoint = rand() % xplane.size();

                nmx::data_point point = xplane.at(ipoint);
                xplane.erase(xplane.begin() + ipoint);

                uint32_t strip = point.strip;
                uint32_t time = point.time;
                uint32_t charge = point.charge;

                //std::cout << "Added data-point (" << strip << ", " << time << ", " << charge << ")\n";
                c.addDataPoint(strip, time, charge);
                //std::cout << "Buffer:\n";
                //printBuffer(c.getMajorTimeBuffer(), c.getTimeOrderedBuffer());
                //printMask(c.getClusterMask());

                nmx::cluster &finalcluster = c.getFinalCluster();

                if (finalcluster.npoints > 0) {

                    std::cout << "Got a new cluster. Comparint to << " << insertedEvents.size() << " stored events\n";

                    compareClusterToEvents(finalcluster, insertedEvents);

                    /*
                    printCluster(convertToVector(finalcluster));

                    std::cout << "Stored clusters :\n";

                    for (int i = 0; i < insertedEvents.size(); i++) {
                        std::cout << "# " << i << "\n";
                        printCluster(insertedEvents.at(i));
                    }
*/

                    finalcluster.npoints = 0;
                }
            }

        }

        repeat++;
    }

    return 0;
}