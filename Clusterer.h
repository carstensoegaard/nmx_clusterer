//
// Created by soegaard on 10/24/17.
//

#ifndef NMX_CLUSTERER_CLUSTERER_H
#define NMX_CLUSTERER_CLUSTERER_H

#include <vector>
#include <thread>
#include <mutex>

#include "NMXClustererDefinitions.h"
#include "BoxAdministration.h"

class Clusterer {

public:

    Clusterer(std::mutex& m);
    ~Clusterer();

    bool addDataPoint(const nmx::data_point &point);
    bool addDataPoint(uint32_t strip, uint32_t time, uint32_t charge);

    void producer();
    void consumer();

    void endRun();
    void terminate() { m_terminate = true; }

    std::vector<nmx::cluster>& getProducedClusters() { return m_produced_clusters; }

    void setVerboseLevel(uint level = 0) { m_verbose_level = level; }


private:

    uint m_verbose_level;

    uint m_i1;

    uint32_t m_nB;
    uint32_t m_nC;
    uint32_t m_nD;

    std::thread pro;
    std::thread con;

    bool m_new_point;

    std::mutex& m_mutex;

    bool m_terminate;

    nmx::data_point m_point_buffer;

    nmx::buffer_data m_cluster;
    std::vector<nmx::cluster> m_produced_clusters;

    BoxAdministration m_boxes;

    nmx::col_array m_mask;

    nmx::row_array m_majortime_buffer;
    nmx::row_array m_SortQ;
    nmx::row_array m_ClusterQ;
    nmx::time_ordered_buffer m_time_ordered_buffer;

    uint32_t getMinorTime(uint32_t time);
    uint32_t getMajorTime(uint32_t time);

    void addToBuffer(const nmx::data_point &point, uint minorTime);
    void moveToClusterer(uint d, uint minorTime, uint majorTime);

    uint checkMask(uint strip, int &lo_idx, int &hi_idx);
    bool newCluster(nmx::data_point &point);
    bool insertInCluster(nmx::data_point &point);
    bool mergeAndInsert(uint32_t lo_idx, uint32_t hi_idx, nmx::data_point &point);
    bool flushCluster(int boxid);
    uint getLoBound(int strip);
    uint getHiBound(int strip);

    void guardB();

    void reset();

    void checkBitSum();
    void printInitialization();
};

#endif //NMX_CLUSTERER_CLUSTERER_H
