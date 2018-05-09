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

    for (unsigned int icluster = 0; icluster < clusters.size(); icluster++)
        verifier.associateClusterWithEvent(clusters.at(icluster), events);

    WriteVerificationToDisk writer;

    auto eventIter = events.begin();

    while (eventIter != events.end()) {

        std::vector<nmx::fullCluster> matching = verifier.findMatchingClusters(*eventIter, clusters);

        writer.write(*eventIter, matching);

        eventIter++;
    }

    return 1;
}