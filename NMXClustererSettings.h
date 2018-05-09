//
// Created by soegaard on 11/20/17.
//

#ifndef PROJECT_NMXCLUSTERERSETTINGS_H
#define PROJECT_NMXCLUSTERERSETTINGS_H

#include <cmath>

namespace nmx {

    // Detector specific parameter: Number of readout channels per plane
    const unsigned int STRIPS_PER_PLANE = 256;

    // Clustering algorithm parameters
    // The time-stamp of the data-points are assumed to be 32 bits.
    // These bits are divided into 3 groups for time sorting:
    // 1) Lowest bits - are ignored
    // 2) "Middle" (minor) bits - used for sorting
    // 3) Highest (major) bits - used for flushing sorted data-points
    const unsigned int IGNORE_BITS =  5; // Ignored bits
    const unsigned int MINOR_BITS  =  7; // Sorting bits
    const unsigned int MAJOR_BITS  = 20; // "Flushing bits

    // Number of neighboring bits to consider for merging two clusters
    const unsigned int INCLUDE_N_NEIGHBOURS = 5;

    // Maximum length of a cluster.
    // Detector specific parameter.
    const unsigned int MAX_CLUSTER_TIME = static_cast<uint>(30 * 32);

    // Number of clusters in the cluster-manger and in the cluster-pairing-queue
    const unsigned int NCLUSTERS = 100;

    const unsigned int DIM_Q_MATRIX = 15;
    const double DELTA_Q = 2.0;
}

namespace nmx {

    // Derived quantities

    const uint32_t MAX_IGNORE = 1 << IGNORE_BITS;
    const uint32_t MAX_MINOR  = 1 << MINOR_BITS;
    const uint32_t MAX_MAJOR  = 1 << MAJOR_BITS;

    const uint32_t IGNORE_BITMASK = MAX_IGNORE - 1;
    const uint32_t MINOR_BITMASK  = MAX_MINOR  - 1;
    const uint32_t MAJOR_BITMASK  = MAX_MAJOR  - 1;

    const unsigned int NBOXES = STRIPS_PER_PLANE/(2*INCLUDE_N_NEIGHBOURS+1)+1;

/*    const uint32_t CLUSTER_MAX_IGNORE = 1 << CLUSTER_IGNORE_BITS;
    const uint32_t CLUSTER_MAX_MINOR  = 1 << CLUSTER_MINOR_BITS;
    const uint32_t CLUSTER_MAX_MAJOR  = 1 << CLUSTER_MAJOR_BITS;

    const uint32_t CLUSTER_IGNORE_BITMASK = CLUSTER_MAX_IGNORE - 1;
    const uint32_t CLUSTER_MINOR_BITMASK  = CLUSTER_MAX_MINOR  - 1;
    const uint32_t CLUSTER_MAJOR_BITMASK  = CLUSTER_MAX_MAJOR  - 1;
*/
}

#endif //PROJECT_NMXCLUSTERERSETTINGS_H
