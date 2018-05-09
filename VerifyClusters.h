//
// Created by soegaard on 4/30/18.
//

//#ifndef PROJECT_VERIFYCLUSTERS_H
//#define PROJECT_VERIFYCLUSTERS_H


#include <vector>
#include "NMXClustererDefinitions.h"

class VerifyClusters {

public:

    VerifyClusters();

    std::vector<nmx::fullCluster> findMatchingClusters(const nmx::fullCluster &event,
                                                       std::vector<nmx::fullCluster> &clusters);

    void associateClusterWithEvent(nmx::fullCluster &cluster, const std::vector<nmx::fullCluster> &events);

private:

    int numberOfMatchingPoints(const nmx::fullCluster &event, const nmx::fullCluster &cluster);
    int numberOfMatchingPointsPlane(const nmx::cluster &event, const nmx::cluster &cluster);
    bool pointsMatch(const nmx::data_point &p1, const nmx::data_point &p2);
};


//#endif //PROJECT_VERIFYCLUSTERS_H
