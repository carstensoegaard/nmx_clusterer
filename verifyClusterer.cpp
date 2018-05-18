//
// Created by soegaard on 4/26/18.
//

#include "ClusterReader.h"
#include "SpecialDataReader.h"
#include "VerifyClusters.h"
#include "WriteVerificationToDisk.h"

int main() {

    ClusterReader eventReader("NMX_input_events.txt");
    ClusterReader clusterReader("NMX_PairedClusters.txt");

    std::cout << "Reading events ... " << std::endl;
    std::vector<nmx::fullCluster> events   = eventReader.getAllEvents();
    std::cout << "Recieved " << events.size() << " events." << std::endl;
    std::cout << "Reading clusters ... " << std::endl;
    std::vector<nmx::fullCluster> clusters = clusterReader.getAllClusters();
    std::cout << "Recieved " << clusters.size() << " clusters." << std::endl;

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
    old = -1;

    std::cout << "Matching " << events.size() << " events with " << clusters.size() << " clusters ..." << std::endl;
    while (eventIter != events.end()) {

        int percent = 100*matched/events.size();
        if (((percent % 10) == 0) && (percent != old)) {
            std::cout << percent << " % " << std::flush;
            old = percent;
        }

        std::vector<nmx::fullCluster> matching = verifier.findMatchingClusters(*eventIter, clusters);

        writer.write(*eventIter, matching);

        eventIter++;
        matched++;
    }
    std::cout << "completed!" << std::endl;

    return 1;
}