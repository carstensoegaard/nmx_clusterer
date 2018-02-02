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

    for (int i = 0; i < cluster.size(); i++) {
        auto point = cluster.at(i);
        std::cout << std::setw(8) << point.strip;
    }
    std::cout << "\n";
    for (int i = 0; i < cluster.size(); i++) {
        auto point = cluster.at(i);
        std::cout << std::setw(8) << point.time;
    }
    std::cout << "\n";
    for (int i = 0; i < cluster.size(); i++) {
        auto point = cluster.at(i);
        std::cout << std::setw(8) << point.charge;
    }
    std::cout << "\n";
}

std::vector<nmx::data_point> convertToVector(nmx::cluster &cluster) {

    std::vector<nmx::data_point> cl(cluster.data.begin(), cluster.data.begin()+cluster.npoints);

    return cl;
}

int32_t findDescrepancy(std::vector<nmx::data_point> cluster, std::vector<nmx::data_point> plane){

    //std::cout << "plane size = " << plane.size() << ", cluster size = " << cluster.size() << std::endl;

    int c_size = (int)cluster.size();
    int p_size = (int)plane.size();

    int iter1 = 0;

    for (auto it1 = plane.begin(); it1 != plane.end(); ) {
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
            if ((cpoint.strip == epoint.strip)
                && (cpoint.time == epoint.time)
                && (cpoint.charge == epoint.charge)) {

              //  std::cout << "MATCH !!!!\n";

                plane.erase(it1);
                cluster.erase(it2);

              //  std::cout << "plane size = " << plane.size() << ", cluster size = " << cluster.size() << std::endl;

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

    if (c_size - cluster.size() != p_size - plane.size()) {
        std::cout << "Somethings wrong! Did not find the same number of points in cluster and plane\n";
        std::cout << "cluster : " << c_size << " - " << cluster.size() << " = " << c_size - cluster.size() << std::endl;
        std::cout << "plane   : " << p_size << " - " << plane.size() << " = " << p_size - plane.size() << std::endl;
    }

    return c_size - (c_size - (int)cluster.size());
}

void compareClusterToEvents(nmx::cluster cluster, std::vector<plane> &events) {

    int32_t min_lostpoints = INT32_MAX;
    int32_t min_gainedpoints = INT32_MAX;
    int32_t best_lost = INT32_MAX;
    int32_t best_gained = INT32_MAX;

    uint ievent = 0;

    auto cl = convertToVector(cluster);

    plane pbuf_orig = cl;
    plane pbuf_lost;
    plane pbuf_gained;
/*
    std::cout << "Cluster size = " << pbuf_orig.size() << std::endl;
*/
    for (auto it = events.begin(); it != events.end();) {

        plane plane = *it;
/*
        std::cout << "Comparing to event with " << plane.size() << " points\n";
*/
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

            std::cout << "Descrepancy is " << descrepancy << std::endl;

            if (descrepancy < 0) {
                if (std::abs(descrepancy) < min_lostpoints) {
                    min_lostpoints = std::abs(descrepancy);
                    best_lost = ievent;
                    pbuf_lost = plane;
                }
            } else {
                if (descrepancy < min_gainedpoints) {
                    min_gainedpoints = descrepancy;
                    best_gained = ievent;
                    pbuf_gained = plane;
                }
            }
        }

        it++;
        ievent++;
    }


//    std::cout << "Orig size " << pbuf_orig.size() << std::endl;

    std::cout << "Best match ";

    printCluster(pbuf_orig);

    if (min_lostpoints < min_gainedpoints) {
        std::cout << "lost " << min_lostpoints << " points from :\n";
        printCluster(pbuf_lost);
        events.erase(events.begin()+best_lost);

    } else {
        std::cout << "gained " << min_gainedpoints << " points!\n";
        printCluster(pbuf_gained);
        events.erase(events.begin()+best_gained);
    }
}

int main() {

    srand(1);

    uint32_t nspertimebin = 32;
    uint32_t maxbinsperevent = 30;
    uint32_t maxtimeperevent = nspertimebin*maxbinsperevent*2;

    Clusterer c;

    //printBuffer(c.getMajorTimeBuffer(), c.getTimeOrderedBuffer());

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
    //evman.setClusterBuffer(c.getProducedClusters());

    int nrepeats = 1;

    int multiplier = 0;

    std::cout << "\nWill repeat " << events.size() << " events " << nrepeats << " times.\n";

    while (repeat < nrepeats) {

        std::cout << "*** Repeat # " << repeat << " ***\n";

        for (int ievent = 0; ievent < /*1*/events.size(); ievent++) {

            event ev = events.at(ievent);

            for (int iplane = 0; iplane < 2; iplane++) {

                //std::cout << "Plane " << iplane << std::endl;

                plane xplane = (ev.at(iplane));

                for (int i = 0; i < xplane.size(); i++) {

                    //xplane.at(i).strip -= 150;
                    xplane.at(i).time = xplane.at(i).time * nspertimebin + multiplier * maxtimeperevent;
                    //xplane.at(i).strip = xplane.at(i).strip - 150;
                }

                //printCluster(xplane);

                evman.insertEvent(xplane);

                insertedEvents.push_back(xplane);

                while (xplane.size() > 0) {

                    uint ipoint = rand() % xplane.size();

                    nmx::data_point point = xplane.at(ipoint);
                    xplane.erase(xplane.begin() + ipoint);

                    uint32_t strip = point.strip;
                    uint32_t time = point.time;
                    uint32_t charge = point.charge;

                    //p.push_back(point);
                    //std::cout << "Added data-point (" << strip << ", " << time << ", " << charge << ")\n";
                    c.addDataPoint(strip, time, charge);
                    //std::cout << "Buffer:\n";
                    //printBuffer(c.getMajorTimeBuffer(), c.getTimeOrderedBuffer());
                    //printMask(c.getClusterMask());

                    std::vector<nmx::cluster> &produced_clusters = c.getProducedClusters();

                    if (produced_clusters.size() > 0)
                        std::cout << "Received " << produced_clusters.size() << " clusters.\n";

                    while (produced_clusters.size() > 0) {

                        evman.compareToStored(produced_clusters);
                        /*

                        auto it = produced_clusters.begin();

                        nmx::cluster produced_cluster = *it;
*/
                         /*
                        std::cout << "Produced cluster contains " << produced_cluster.npoints << " points\n";
*/
/*
                        std::cout << "Comparing cluster to " << insertedEvents.size() << " stored events\n";

                        compareClusterToEvents(produced_cluster, insertedEvents);
*/
                        /*
                        printCluster(convertToVector(finalcluster));

                        std::cout << "Stored clusters :\n";

                        for (int i = 0; i < insertedEvents.size(); i++) {
                            std::cout << "# " << i << "\n";
                            printCluster(insertedEvents.at(i));
                        }
                        */

                        //produced_clusters.erase(it);

                    }
                }

                multiplier++;
                //insertedEvents.push_back(p);
            }
        }

        c.endRun();

        std::vector<nmx::cluster> &produced_clusters = c.getProducedClusters();

        while (produced_clusters.size() > 0) {

            evman.compareToStored(produced_clusters);
        }

        repeat++;
    }

    evman.flushBuffer();

    evman.printStats();

    return 0;
}