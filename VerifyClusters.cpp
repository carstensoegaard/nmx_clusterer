//
// Created by soegaard on 4/30/18.
//

#include "VerifyClusters.h"

VerifyClusters::VerifyClusters() {

}

void VerifyClusters::associateClusterWithEvent(nmx::fullCluster &cluster,
                                               const std::vector<nmx::fullCluster> &events) {

    int maxMatching = 0;
    int bestMatch = -1;

    //std::cout << "Comparing cluster to " << events.size() << " events." << std::endl;

    for (unsigned int ievent = 0; ievent < events.size(); ievent++) {

        //std::cout << "Comparing to event # " << ievent << std::endl;

        int nMatches = numberOfMatchingPoints(events.at(ievent), cluster);

        //std::cout << "Cluster and event have " << nMatches << " matching points." << std::endl;

        if (nMatches > maxMatching) {
            maxMatching = nMatches;
            bestMatch = ievent;
        }
    }

    if (bestMatch >= 0)
        cluster.eventNo = events.at(bestMatch).eventNo;
    else
        std::cout << "No match found!" << std::endl;
}

std::vector<nmx::fullCluster> VerifyClusters::findMatchingClusters(const nmx::fullCluster &event,
                                                                   std::vector<nmx::fullCluster> &clusters) {

    std::vector<nmx::fullCluster> ret;

    auto iter = clusters.begin();

    while (iter != clusters.end()) {

        if (iter->eventNo == event.eventNo) {

            ret.push_back(*iter);
            clusters.erase(iter);
        } else
            iter++;
    }

    return ret;
}

int VerifyClusters::numberOfMatchingPoints(const nmx::fullCluster &event, const nmx::fullCluster &cluster) {

    int nMatches = 0;

    nMatches += numberOfMatchingPointsPlane(event.clusters.at(0), cluster.clusters.at(0));
    nMatches += numberOfMatchingPointsPlane(event.clusters.at(1), cluster.clusters.at(1));

    return nMatches;
}

int VerifyClusters::numberOfMatchingPointsPlane(const nmx::cluster &event, const nmx::cluster &cluster) {

    int nMatches = 0;

    for (unsigned int ievent = 0; ievent < event.npoints; ievent++) {

        const nmx::data_point &evPoint = event.data.at(ievent);

        for (unsigned int icluster = 0; icluster < cluster.npoints; icluster++) {

            const nmx::data_point &clPoint = cluster.data.at(icluster);

            if (pointsMatch(evPoint, clPoint))
                nMatches++;
        }
    }

    return nMatches;
}

bool VerifyClusters::pointsMatch(const nmx::data_point &p1, const nmx::data_point &p2) {

    if ((p1.strip == p2.strip) && (p1.time == p2.time) && (p1.charge == p2.charge))
        return true;

    return false;
}
