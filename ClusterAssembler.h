//
// Created by soegaard on 1/16/18.
//

#ifndef PROJECT_CLUSTERASSEMBLER_H
#define PROJECT_CLUSTERASSEMBLER_H

#include <vector>

#include "NMXClustererDefinitions.h"
#include "BoxAdministration.h"

#endif //PROJECT_CLUSTERASSEMBLER_H

class ClusterAssembler {

public:

    ClusterAssembler();

    void addPointToCluster(nmx::data_point &point);

    std::vector<nmx::cluster> *getClusterRef() { return &m_produced_clusters; }

private:

    BoxAdministration m_boxes;

    nmx::col_array m_mask;
    nmx::cluster_data m_cluster;
    std::vector<nmx::cluster> m_produced_clusters;

    uint checkMask(uint strip, int &lo_idx, int &hi_idx);
    bool newCluster(nmx::data_point &point);
    bool insertInCluster(nmx::data_point &point);
    bool mergeAndInsert(uint32_t lo_idx, uint32_t hi_idx, nmx::data_point &point);
    bool flushCluster(const int boxid);

    uint getLoBound(int strip);
    uint getHiBound(int strip);

    void reset();
};