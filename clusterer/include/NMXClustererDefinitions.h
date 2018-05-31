//
// Created by soegaard on 10/24/17.
//
#ifndef NMX_CLUSTERER_DEFINITIONS_H
#define NMX_CLUSTERER_DEFINITIONS_H

#include <iostream>
#include <array>

#include "NMXClustererSettings.h"
#include "../../NMXQmatrix.h"

namespace nmx {

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

    ///@}

}


namespace nmx {

    // A data-point for either X or Y plane
    struct data_point {
        uint32_t strip;
        uint32_t charge;
        uint32_t time;
    };

    // Define an array of length CHANNELS_PER_PLANE
    typedef std::array<int32_t, STRIPS_PER_PLANE> col_array;

    typedef std::array<data_point, STRIPS_PER_PLANE> dataBuffer_t;

    struct buffer {
        unsigned int npoints;
        dataBuffer_t data;
    };

    /*struct idx_buffer {
        int nidx;
        std::array<int, 100> data;
    };*/

    struct clusterParingEntry {

        std::array<int, 2> queue;
        std::array<unsigned int, 2> queueLength;
    };

    typedef std::array<buffer, DATA_MAX_MINOR> tbuffer;
    typedef std::array<tbuffer, 2> time_ordered_buffer;

    // Cluster formation element definitions

    struct box {
        uint32_t min_strip = UINT32_MAX;
        uint32_t max_strip = 0;
        uint32_t min_time  = UINT32_MAX;
        uint32_t max_time  = 0;
        uint64_t chargesum = 0;
        uint64_t maxcharge = 0;
        int link1 = -1;
        int link2 = -1;
    };

    typedef std::array<data_point, STRIPS_PER_PLANE> cluster_points;

    struct cluster {
        nmx::box box;
        unsigned int npoints = 0;
        cluster_points data;
    };

    typedef std::array<cluster, NCLUSTERS> cluster_buffer;

    struct clusterPair {
        int x_idx;
        int y_idx;
    };

    struct pairBuffer {
        unsigned int npairs;
        std::array<clusterPair, 100> pairs;
    };

    // For verification and debugging

    struct fullCluster {
        std::array<nmx::cluster, 2> clusters;
        unsigned int eventNo;
    };
}

#endif //NMX_CLUSTERER_DEFINITIONS_H
