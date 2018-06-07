//
// Created by soegaard on 10/24/17.
//

#ifndef NMX_CLUSTERER_CLUSTERER_H
#define NMX_CLUSTERER_CLUSTERER_H

#include <vector>
#include <thread>
#include <mutex>

#include "NMXClustererDefinitions.h"
#include "NMXBoxAdministration.h"
#include "NMXClusterManager.h"
#include "NMXClusterPairing.h"
#include "../../helper/include/NMXClustererHelper.h"

/*! Sorting and clustering class.
 *
 * This class receives data-triplets (nmx::dataPoint) from the detector. It then first orders the data accoring
 * to their time-stamps. Then the data are grouped into clusters.
 *
 * The sorting part works by a "infinite" buffer. The buffer has a time window. When data arrives with a time-stamp
 * later than the window boundary, the window slides thereby providing room for the new data. The data, in the buffer,
 * but earlier than the window boudary at handed over to the clustering algorithm. Data arriving earlier than the
 * window boundary are discarded.
 *
 * The clustering algorithm considers one point at a time. If a point arrives within the accepted range, the point is
 * added to the current cluster. If not a new cluster is formed. If two clusters come within the specified range
 * they are merged together. A cluster is finalized if a new point will cause the cluster to exceed the maximum
 * time-span allowed. The maximum allowed time-span is related to the drift time of the liberated electrons in the
 * NMX gas medium.
 */

struct BufferEntry {
    nmx::DataPoint point;
    int link;
};

class NMXPlaneClusterer {


public:

    /*! Constructor
     *
     * @param plane Identify the associated plane 0 for X and 1 for Y
     * @param clusterManager Reference to the cluster-manager
     * @param clusterPairing Reference to the cluster pairing algorithm
     * @param mutex Common mutex for the two instances of this class
     */
    NMXPlaneClusterer(int plane, NMXClusterManager &clusterManager, NMXClusterPairing &clusterPairing,
                      std::mutex &mutex);
    /*! Destructor.
     *
     * Joins the threads.
     */
    ~NMXPlaneClusterer();

    /*! Add a data point (triplet)
     *
     * @param point a data triplet
     * @return true
     */
    bool addDataPoint(const nmx::DataPoint &point);
    /*! Not implented. */
    bool addDataPoint(uint32_t strip, uint32_t time, uint32_t charge);

    /*! Time sorting
     *
     * This function handles the time sorting/ordering of the inserted triplets (nmx::dataPoint)
     */
    void timeSorting();
    /*! Clustering
     *
     * This function clusters the already time-ordered data.
     */
    void clustering();

    void endRun();
    void terminate() { m_terminate = true; }
    void reset();

    void setVerboseLevel(uint level = 0) { m_verbose_level = level; }

    uint64_t getNumberOfProducedClusters() { return m_nClusters; }
    uint64_t getNumberOfOldPoints() { return m_nOldPoints; }

private:

    unsigned int m_verbose_level;

    uint32_t m_i1;

    uint32_t m_nB;
    uint32_t m_nC;
    uint32_t m_nD;

    int m_plane;

    std::array<BufferEntry, nmx::STRIPS_PER_PLANE> m_bufferEntries;

    uint64_t m_nOldPoints;

    std::thread pro;
    std::thread con;

    bool m_new_point;

    NMXClusterManager &m_clusterManager;
    NMXClusterPairing &m_clusterParing;
    std::mutex& m_mutex;

    bool m_terminate;

    nmx::DataPoint m_point_buffer;

    nmx::dataBufferRow_t m_cluster;

    NMXBoxAdministration m_boxes;

    nmx::dataRow_t m_mask;

    nmx::dataColumn_t m_majortimeBuffer;
    nmx::dataColumn_t m_SortQ;
    nmx::dataColumn_t m_ClusterQ;
    typedef std::array<int, nmx::CLUSTER_MAX_MINOR> dataBuffer_t;
    std::array<dataBuffer_t, 2> m_time_ordered_buffer;
    //nmx::time_ordered_buffer m_time_ordered_buffer;

    uint64_t m_nClusters = 0;

    uint32_t getMinorTime(uint32_t time);
    uint32_t getMajorTime(uint32_t time);

    uint64_t m_nFailedEntryRequests = 0;
    int m_queueHead = -1;
    int m_queueTail = -1;
    int getEntryFromQueue();
    void insertEntryInQueue(unsigned int idx);
    BufferEntry* getBufferEntry(unsigned int idx);
    int getLastInQueue(int idx);

    void addToBuffer(const nmx::DataPoint &point, uint minorTime);
    void moveToClusterer(uint d, uint minorTime, uint majorTime);

    unsigned int checkMask(uint strip, int &lo_idx, int &hi_idx);
    bool newCluster(nmx::DataPoint &point);
    bool insertInCluster(nmx::DataPoint &point);
    bool mergeAndInsert(uint32_t lo_idx, uint32_t hi_idx, nmx::DataPoint &point);
    bool flushCluster(int boxid);
    uint32_t getLoBound(int strip);
    uint32_t getHiBound(uint32_t strip);

    void checkBoxes(uint32_t latestTime);

    void guardB();
    //void guardC();

    void checkBitSum();
    void printInitialization();

    bool m_trackPoint = false;
    nmx::DataPoint m_pointToTrack;
    bool checkTrackPoint(const nmx::DataPoint &point);

};

#endif //NMX_CLUSTERER_CLUSTERER_H
