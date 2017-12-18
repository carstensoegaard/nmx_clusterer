//
// Created by soegaard on 10/24/17.
//

#ifndef NMX_CLUSTERER_CLUSTERER_H
#define NMX_CLUSTERER_CLUSTERER_H

#include <vector>
#include "NMXClustererDefinitions.h"
#include "BoxAdministration.h"

class Clusterer {

public:

    Clusterer();

    bool addDataPoint(const nmx::data_point &point);
    bool addDataPoint(uint32_t strip, uint32_t time, uint32_t charge);

    //nmx::cluster& getFinalCluster() { return m_final_cluster; }
    std::vector<nmx::cluster> &getProducedClusters() { return m_produced_clusters; }


private:

    uint m_i1;
    bool m_firstevent;

    BoxAdministration m_boxes;

    nmx::row_array m_majortime_buffer;
    nmx::col_array m_mask;
    nmx::time_ordered_buffer m_time_ordered_buffer;


    nmx::cluster_data m_cluster;

    //nmx::cluster m_final_cluster;
    std::vector<nmx::cluster> m_produced_clusters;


    uint32_t getMinorTime(uint32_t time);
    uint32_t getMajorTime(uint32_t time);


    void addToBuffer(const nmx::data_point &point, const uint &minorTime);
    bool transfer(nmx::buffer &buf);
    uint checkMask(uint strip, int &lo_idx, int &hi_idx);
    bool newCluster(nmx::data_point &point);
    bool insertInCluster(nmx::data_point &point);
    bool mergeAndInsert(uint32_t lo_idx, uint32_t hi_idx, nmx::data_point &point);
    bool flushCluster(const int &boxid);

    uint getLoBound(int strip);
    uint getHiBound(int strip);

    void reset();


    // Helper functions - must be removed before release

public:

    uint getI1() { return m_i1; }

    nmx::time_ordered_buffer getTimeOrderedBuffer() { return m_time_ordered_buffer; }
    nmx::col_array getClusterMask() { return m_mask; }
    nmx::row_array getMajorTimeBuffer() { return m_majortime_buffer; }
    nmx::cluster_data getCluster() { return m_cluster; }


    // This must go in final version
    void printMask();
    void printBox(int boxid);
    void printBox(const nmx::box &box);
    void printPoint(const nmx::data_point &point);
    void printClusterBuffer();
    void printTimeOrderedBuffer();
};

#endif //NMX_CLUSTERER_CLUSTERER_H
