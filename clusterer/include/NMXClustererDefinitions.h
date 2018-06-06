//
// Created by soegaard on 10/24/17.
//
#ifndef NMX_CLUSTERER_DEFINITIONS_H
#define NMX_CLUSTERER_DEFINITIONS_H

#include <iostream>
#include <array>

#include "NMXClustererSettings.h"
#include "NMXQmatrix.h"

namespace nmx {

    // Some forward declarations.
    struct DataPoint;
    struct DataBuffer;
    struct Cluster;

    /*! @name Various types used in the code
     *
     * Types which are used in multiple classes are defined here.
     */
    ///@{
    /*! Array of length nmx::DATA_MAX_MINOR
     *
     * The columns length used in forming the matrix which contains the sorting buffer. The array has a length og
     * nmx::DATA_MAX_MINOR
     */
    typedef std::array<uint32_t, DATA_MAX_MINOR> dataColumn_t;

    /*! Array of length nmx::STRIPS_PER_PLANE
     *
     * The row length used in forming the matrix which contains the sorting buffer. The array has a length og
     * nmx::STRIPS_PER_PLANE
     */
    typedef std::array<int32_t, STRIPS_PER_PLANE> dataRow_t;

    /*! A buffer for data-points
     *
     * A buffer for data-points of length nmx::STRIPS_PER_PLANE.
     */
    typedef std::array<DataPoint, STRIPS_PER_PLANE> dataBufferRow_t;

    /*! An array of nmx::DataBuffer of legnth nmx::DATA_MAX_MINOR for the sorting buffer
     *
     */
    typedef std::array<DataBuffer, DATA_MAX_MINOR> dataBufferColumn_t;

    /*! The time-ordering-buffer
     *
     * This buffer contains the data after it has been sorted. It is two dimensional of nmx::dataBufferColumn_t.
     * The sorting algorithm alternates between these two dimensions, such that one is for sorting while the other for
     * clustering.
     */
    typedef std::array<dataBufferColumn_t, 2> time_ordered_buffer;
    ///@}
}


namespace nmx {

    /*! DataPoint definition
     *
     * This defines the triplet of data stemming from one track-point of the NMX. It consists of the strip number, the
     * charge (ADC-value), and the time-stamp.
     */
    struct DataPoint {
        uint32_t strip;
        uint32_t charge;
        uint32_t time;
    };

    /*! A buffer for nmx::DataPoint
     *
     * This buffer holds a number of nmx::DataPoint the buffer is reset by setting nPoints = 0.
     */
    struct DataBuffer {
        unsigned int nPoints;
        dataBufferRow_t data;
    };

    /*! Element for the pairing buffer
     *
     * The element contains 2 indexes, which are the index of the fist cluster index in the queue, Following
     * clusters are linked via the nmx::Box links. If the queue is empty the link is -1. The length of the queue
     * is incremented with each addition to the queue. The length must be reset to 0 when the queue is released and the
     * queue links to -1.
     */
    struct ClusterParingEntry {
        std::array<int, 2> queue;
        std::array<unsigned int, 2> queueLength;
    };

    /*! The box for clustering.
     *
     * As the clustering proceeds a nmx::Box is formed. It contains the bounds of the strips and of the time.
     * The charge sum and the maximum charge is also stored. The box contains two links for queueing.
     */
    struct Box {
        uint32_t min_strip = UINT32_MAX;
        uint32_t max_strip = 0;
        uint32_t min_time  = UINT32_MAX;
        uint32_t max_time  = 0;
        uint64_t chargesum = 0;
        uint64_t maxcharge = 0;
        int link1 = -1;
        int link2 = -1;
    };

    /*! The finalized cluster
     *
     * This struct contains the final cluster. It contains the nmx::Box, and the data. The data is filled into the array
     * while incrementing the counter. The data is reset by setting the counter to 0.
     */
    struct Cluster {
        nmx::Box box;
        unsigned int nPoints = 0;
        dataBufferRow_t data;
    };

    /*! The indexes of a cluster pair.
     *
     */
    struct ClusterIndexPair {
        int x_idx;
        int y_idx;
    };

    /*! A buffer to hold cluster pair indexes.
     *
     * This buffer hold cluster index pairs while transfered to the location finder.
     */
    struct PairBuffer {
        unsigned int npairs;
        std::array<ClusterIndexPair, 100> pairs;
    };

    // For verification and debugging

    struct FullCluster {
        std::array<nmx::Cluster, 2> clusters;
        unsigned int eventNo;
    };
}

#endif //NMX_CLUSTERER_DEFINITIONS_H
