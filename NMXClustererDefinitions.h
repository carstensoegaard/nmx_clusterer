//
// Created by soegaard on 10/24/17.
//
#ifndef NMX_CLUSTER_DEFINITIONS_H
#define NMX_CLUSTER_DEFINITIONS_H

#include <array>

#include "NMXClustererSettings.h"

// Create matrices of size [nrows, ncols]
//template <class T, size_t nrows, size_t ncols>
//using matrix = std::array<std::array<T, ncols>, nrows>;

namespace nmx {

    // A data-point for either X or Y plane
    struct data_point {
        uint32_t strip;
        uint32_t charge;
        uint32_t time;
    };

    // Define an array of length NROWS
    typedef std::array<uint32_t, MAX_MINOR> row_array;
    // Define an array of length CHANNELS_PER_PLANE
    typedef std::array<int32_t, STRIPS_PER_PLANE> col_array;

    typedef std::array<data_point, STRIPS_PER_PLANE> buffer_data;

    struct buffer {
        uint npoints;
        buffer_data data;
    };

    typedef std::array<buffer, MAX_MINOR> tbuffer;
    typedef std::array<tbuffer, 2> time_ordered_buffer;

    // Cluster formation element definitions

    struct box {
        uint32_t min_strip;
        uint32_t max_strip;
        uint32_t min_time;
        uint32_t max_time;
        uint64_t chargesum;
        uint64_t maxcharge;
        uint8_t  plane;
        int link1;
        int link2;
    };

    typedef std::array<data_point, STRIPS_PER_PLANE> cluster_points;

    struct cluster {
        nmx::box box;
        uint npoints;
        cluster_points data;
    };

    typedef std::array<cluster, NCLUSTERS> cluster_buffer;
}



#endif //NMX_CLUSTER_DEFINITIONS_H
