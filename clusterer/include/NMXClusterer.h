//
// Created by soegaard on 2/13/18.
//

#ifndef NMXCLUSTERER_H
#define NMXCLUSTERER_H

#include <thread>

#include "../include/NMXClustererDefinitions.h"
#include "../include/NMXPlaneClusterer.h"
#include "../include/NMXClusterPairing.h"

/*! Main interface.
  *
  *  This class is the entry point for the data to be processed.
  *  It instaciates the two plane-clusterers, the pairing algorithm and the location finder.
  *  Note: the location finder is a dummy class, which currently only writes cluster pairs to dics. A proper location
  *  finder should be implemented here.
  */
class NMXClusterer {

public:

    /*! Constructor.
     *
     *  Instanciates clusterer, pairing algorithm and location finder.
     */
    NMXClusterer();

    /*! Data input.
     *
     *  This fuction recieves data from the NMX detector. The data must be transformed into the plane number and a
     *  triplet before entering the function.
     *
     * @param plane Plane of origin 0 for X and 1 for Y
     * @param point The triplet (strip, charge, time)
     */
    void addDatapoint(unsigned int plane, nmx::DataPoint &point);

    /*! End the run.
     *
     *  This function ends the run by flushing first all data in the clustering buffer, then all the clusters in the
     *  pairing algorithm.
     */
    void endRun();
    /*! Reset all counters and buffers.*/
    void reset();
    /*! Terminate all threads.*/
    void terminate();

    /*! Returns the number of produced clusters from the X-plane.
     *
     * @return Number of produced clusters from the X-plane.
     */
    uint64_t getNumberOfProducedClustersX() { return m_XplaneClusterer.getNumberOfProducedClusters(); }

    /*! Returns the number of produced clusters from the Y-plane.
     *
     * @return Number of produced clusters from the Y-plane.
     */
    uint64_t getNumberOfProducedClustersY() { return m_YplaneClusterer.getNumberOfProducedClusters(); }

    /*! Returns the number of paired X and Y clusters.
     *
     * @return Number of paired X and Y clusters.
     */
    uint64_t getNumberOfPaired()            { return m_clusterPairing.getNumberOfPaired(); }

    /*! Returns the number of times a data-point from the X-plane arrived too late. Late data-points are discarded.
     * Therefore this value should be monitored at run-time. If it increases too fast, the size of the sorting
     * buffer should be increased.
     *
     * @return The number of times a data-point arrives too late.
     */
    uint64_t getNumberOfOldPointsX()    { return m_XplaneClusterer.getNumberOfOldPoints(); }

    /*! Returns the number of times a data-point from the Y-plane arrived too late. Late data-points are discarded.
     * Therefore this value should be monitored at run-time. If it increases too fast, the size of the sorting
     * buffer should be increased.
     *
     * @return The number of times a data-point arrives too late.
     */
    uint64_t getNumberOfOldPointsY()    { return m_YplaneClusterer.getNumberOfOldPoints(); }

    /*! Returns the number of failed cluster requests. There is a certain, but at compile-time configurable,
     * number of clusters in the cluster-manager. This number should not grow too quickly, since a failed request will
     * yield the thread and thereby increasing processing time. Monitoring this value during running is therefore
     * highly recommended.
     *
     * @return Number of failed cluster requests.
     */
    uint64_t getFailedClusterRequests() { return m_clusterManager.getFailedClusterRequests(); }

    /*! Returns the number of times a cluster arrives too late. Late clusters are discarded. Therefore this value
     * should be monitored at run-time. If it increases too fast, the size of the cluster pairing buffer should be
     * increased.
     *
     * @return Number of late clusters
     */
    uint64_t getNumberOfLateClusters()  { return m_clusterPairing.getNumberOfLateClusters(); }

private:

    NMXPlaneClusterer m_XplaneClusterer; /*!< Instance of NMXPlaneClusterer for the X-plane.*/
    NMXPlaneClusterer m_YplaneClusterer; /*!< Instance of NMXPlaneClusterer for the Y-plane.*/

    NMXClusterManager m_clusterManager; /*!< Instance of the NMXClusterManager.*/

    NMXClusterPairing m_clusterPairing; /*!< Instance of the NMXClusterPairer.*/

    std::mutex m_mutex; /*!< Common mutex for the two instances of the NMXPlaneClusterer.*/
};

#endif // NMXCLUSTERER_H
