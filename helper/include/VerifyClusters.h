//
// Created by soegaard on 4/30/18.
//

//#ifndef PROJECT_VERIFYCLUSTERS_H
//#define PROJECT_VERIFYCLUSTERS_H


#include <vector>
#include "../../clusterer/include/NMXClustererDefinitions.h"

class VerifyClusters {

public:

    VerifyClusters();

    std::vector<nmx::FullCluster> findMatchingClusters(const nmx::FullCluster &event,
                                                       std::vector<nmx::FullCluster> &clusters);

    void associateClusterWithEvent(nmx::FullCluster &cluster, const std::vector<nmx::FullCluster> &events);

private:

    int numberOfMatchingPoints(const nmx::FullCluster &event, const nmx::FullCluster &cluster);
    int numberOfMatchingPointsPlane(const nmx::Cluster &event, const nmx::Cluster &cluster);
    bool pointsMatch(const nmx::DataPoint &p1, const nmx::DataPoint &p2);
};


//#endif //PROJECT_VERIFYCLUSTERS_H
