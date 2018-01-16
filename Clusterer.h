//
// Created by soegaard on 10/24/17.
//

#ifndef NMX_CLUSTERER_CLUSTERER_H
#define NMX_CLUSTERER_CLUSTERER_H

#include <vector>
#include <thread>

#include "NMXClustererDefinitions.h"
#include "BoxAdministration.h"

class Clusterer {

public:

    Clusterer();
    ~Clusterer();

    bool addDataPoint(const nmx::data_point &point);
    bool addDataPoint(uint32_t strip, uint32_t time, uint32_t charge);

    void producer(void);
    void consumer(void);

    std::vector<nmx::cluster> *getProducedClusters() { return m_produced_clusters; }

    void endRun();


private:

    uint m_i1;

    uint32_t m_nB;
    uint32_t m_nC;
    uint32_t m_nD;

    std::thread pro;
    std::thread con;

    std::vector<nmx::data_point> m_databuffer;

    std::vector<nmx::cluster> *m_produced_clusters;

    BoxAdministration *m_boxes;

    nmx::row_array m_majortime_buffer;
    nmx::row_array m_SortQ;
    nmx::row_array m_ClusterQ;
    nmx::time_ordered_buffer m_time_ordered_buffer;


    uint32_t getMinorTime(uint32_t time);
    uint32_t getMajorTime(uint32_t time);

    void addToBuffer(const nmx::data_point &point, const uint minorTime);
    //void flushBuffer(uint lo_idx, uint hi_idx, uint32_t buffertime);
    void moveToClusterer(uint d);
    //bool transfer(nmx::buffer &buf);

    void reset();

    void checkBitSum();
    void printInitialization();

public:


    // Public helper functions
    // - intended for debugging only
    // - not intended for release version

    uint getI1() { return m_i1; }

    nmx::time_ordered_buffer getTimeOrderedBuffer() { return m_time_ordered_buffer; }
    //nmx::col_array getClusterMask() { return m_mask; }
    nmx::row_array getMajorTimeBuffer() { return m_majortime_buffer; }
    //nmx::cluster_data getCluster() { return m_cluster; }

    // This must go in final version
    //void printMask();
    void printBox(int boxid);
    void printBox(const nmx::box &box);
    void printPoint(const nmx::data_point &point);
    void printClusterBuffer();
    //void printTimeOrderedBuffer();
};

#endif //NMX_CLUSTERER_CLUSTERER_H
