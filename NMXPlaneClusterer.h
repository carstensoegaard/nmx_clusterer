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
#include "NMXClusterManager.h"
#include "NMXClusterPairing.h"
#include "NMXClustererHelper.h"

class NMXPlaneClusterer {

public:

    NMXPlaneClusterer(int plane, NMXClusterManager &clusterManager, NMXClusterPairing &clusterPairing,
                      std::mutex &mutex);
    ~NMXPlaneClusterer();

    bool addDataPoint(const nmx::data_point &point);
    bool addDataPoint(uint32_t strip, uint32_t time, uint32_t charge);

    void producer();
    void consumer();

    void endRun();
    void terminate() { m_terminate = true; }
    void reset();

    void setVerboseLevel(uint level = 0) { m_verbose_level = level; }

    uint64_t getNumberOfProducedClusters() { return m_nClusters; }
    uint64_t getNumberOfOldPoints() { return m_nOldPoints; }

    void setTrackPoint(bool set = false, uint32_t time = 0, uint32_t strip = 0, uint32_t charge = 0)
    { m_trackPoint = set; m_pointToTrack.time = time; m_pointToTrack.strip = strip; m_pointToTrack.charge = charge; }


private:

    unsigned int m_verbose_level;

    uint32_t m_i1;

    uint32_t m_nB;
    uint32_t m_nC;
    uint32_t m_nD;

    int m_plane;

    uint64_t m_nOldPoints;

    std::thread pro;
    std::thread con;

    bool m_new_point;

    NMXClusterManager &m_clusterManager;
    NMXClusterPairing &m_clusterParing;
    std::mutex& m_mutex;

    bool m_terminate;

    nmx::data_point m_point_buffer;

    nmx::buffer_data m_cluster;

    BoxAdministration m_boxes;

    nmx::col_array m_mask;

    nmx::row_array m_majortime_buffer;
    nmx::row_array m_SortQ;
    nmx::row_array m_ClusterQ;
    nmx::time_ordered_buffer m_time_ordered_buffer;

    uint64_t m_nClusters = 0;

    uint32_t getMinorTime(uint32_t time);
    uint32_t getMajorTime(uint32_t time);

    void addToBuffer(const nmx::data_point &point, uint minorTime);
    void moveToClusterer(uint d, uint minorTime, uint majorTime);

    unsigned int checkMask(uint strip, int &lo_idx, int &hi_idx);
    bool newCluster(nmx::data_point &point);
    bool insertInCluster(nmx::data_point &point);
    bool mergeAndInsert(uint32_t lo_idx, uint32_t hi_idx, nmx::data_point &point);
    bool flushCluster(int boxid);
    uint32_t getLoBound(int strip);
    uint32_t getHiBound(uint32_t strip);

    void checkBoxes(uint32_t latestTime);

    void guardB();
    //void guardC();

    void checkBitSum();
    void printInitialization();

    bool m_trackPoint = false;
    nmx::data_point m_pointToTrack;
    bool checkTrackPoint(const nmx::data_point &point);

};

#endif //NMX_CLUSTERER_CLUSTERER_H
