//
// Created by soegaard on 4/26/18.
//

#include <iomanip>
#include "helper/include/ClusterReader.h"
#include "helper/include/SpecialDataReader.h"
#include "helper/include/VerifyClusters.h"
#include "helper/include/WriteVerificationToDisk.h"

int main() {

    ClusterReader eventReader("NMX_input_events.txt");
    ClusterReader clusterReader("NMX_PairedClusters.txt");

    std::cout << "Reading events ... " << std::endl;
    std::vector<nmx::FullCluster> events   = eventReader.getAllEvents();
    std::cout << "Recieved " << events.size() << " events." << std::endl;
    std::cout << "Reading clusters ... " << std::endl;
    std::vector<nmx::FullCluster> clusters = clusterReader.getAllClusters();
    std::cout << "Recieved " << clusters.size() << " clusters." << std::endl;

    int nclusters = clusters.size();

    VerifyClusters verifier;

    int old = -1;

    std::cout << "Associating " << clusters.size() << " clusters with " << events.size() << " events ..." << std::endl;
    for (unsigned int icluster = 0; icluster < clusters.size(); icluster++) {
        int percent = 100*icluster/clusters.size();
        if (((percent % 10) == 0) && (percent != old)) {
            std::cout << percent << " % " << std::flush;
            old = percent;
        }
        verifier.associateClusterWithEvent(clusters.at(icluster), events);
    }
    std::cout << "completed!" << std::endl;

    WriteVerificationToDisk writer;

    auto eventIter = events.begin();
    int matched = 0;
    int matching[7] = {0, 0, 0, 0, 0, 0, 0};
    old = -1;

    int cs = 0;

    std::cout << "Matching " << events.size() << " events with " << clusters.size() << " clusters ..." << std::endl;
    while (eventIter != events.end()) {

        int percent = 100*matched/events.size();
        if (((percent % 10) == 0) && (percent != old)) {
            std::cout << percent << " % " << std::flush;
            old = percent;
        }

        std::vector<nmx::FullCluster> matchingClusters = verifier.findMatchingClusters(*eventIter, clusters);

        unsigned int m = matchingClusters.size();

        cs += m;

        if (m < 6)
            matching[m]++;
        else
            matching[6]++;

        std::cout << m << " clusters matched!" << std::endl;
        for (unsigned int i = 0; i < 7; i++)
            std::cout << matching[i] << " ";
        std::cout << std::endl;

        writer.write(*eventIter, matchingClusters);

        eventIter++;
        matched++;
    }
    std::cout << "completed " << matched << " events and " << cs << " clusters!" << std::endl;

    unsigned int w1 = 45;
    unsigned int w2 = 5;

    std::cout << std::endl;

    std::cout.width(w1); std::cout << std::left << "Results of matching :" << std::endl;
    std::cout.width(w1); std::cout << std::left << "Number of events :" << std::right
                                   << std::setw(w2) << events.size() << std::endl;
    std::cout.width(w1); std::cout << std::left << "Number of clusters :" << std::right
                                   << std::setw(w2) << nclusters << std::endl;
    std::cout.width(w1); std::cout << std::left << "Events matched with 0 clusters :" << std::right
                                   << std::setw(w2) <<  matching[0] << std::endl;
    std::cout.width(w1); std::cout << std::left << "Events matched with 1 clusters :" << std::right
                                   << std::setw(w2) <<  matching[1] << std::endl;
    std::cout.width(w1); std::cout << std::left << "Events matched with 2 clusters :" << std::right
                                   << std::setw(w2) <<  matching[2] << std::endl;
    std::cout.width(w1); std::cout << std::left << "Events matched with 3 clusters :" << std::right
                                   << std::setw(w2) <<  matching[3] << std::endl;
    std::cout.width(w1); std::cout << std::left << "Events matched with 4 clusters :" << std::right
                                   << std::setw(w2) <<  matching[4] << std::endl;
    std::cout.width(w1); std::cout << std::left << "Events matched with 5 clusters :" << std::right
                                   << std::setw(w2) <<  matching[5] << std::endl;
    std::cout.width(w1); std::cout << std::left << "Events matched with more than 5 clusters :" << std::right
                                   << std::setw(w2) <<  matching[6] << std::endl;

    return 1;
}