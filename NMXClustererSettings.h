//
// Created by soegaard on 11/20/17.
//

#ifndef PROJECT_NMXCLUSTERERSETTINGS_H
#define PROJECT_NMXCLUSTERERSETTINGS_H

#include <cmath>

namespace nmx {

    // Detector specific parameter: Number of readout channels per plane
    //const uint STRIPS_PER_PLANE = 2560;
    const uint STRIPS_PER_PLANE = 256;
    //const uint STRIPS_PER_PLANE = 20;

    // Clustering algorithm parameters
    // The time-stamp of the data-points are assumed to be 32 bits.
    // These bits are divided into 3 groups for time sorting:
    // 1) Lowest bits - are ignored
    // 2) "Middle" (minor) bits - used for sorting
    // 3) Higest (major) bits - used for flushing sorted data-points
    const uint IGNORE_BITS =  5; // Ignored bits
    const uint MINOR_BITS  =  7; // Sorting bits
    const uint MAJOR_BITS  = 20; // "Flushing bits

    // Number of neighboring bits to consider for merging two clusters
    const uint INCLUDE_N_NEIGHBOURS = 5;

    // Maximum length of a cluster.
    // Detector specific parameter.
    const uint MAX_CLUSTER_TIME = static_cast<uint>(30 * 32);
}

namespace nmx {

    // Derived quantities

    const uint32_t MAX_IGNORE = 1 << IGNORE_BITS;
    const uint32_t MAX_MINOR  = 1 << MINOR_BITS;
    const uint32_t MAX_MAJOR  = 1 << MAJOR_BITS;

    const uint32_t IGNORE_BITMASK = MAX_IGNORE - 1;
    const uint32_t MINOR_BITMASK  = MAX_MINOR  - 1;
    const uint32_t MAJOR_BITMASK  = MAX_MAJOR  - 1;

    const uint NBOXES = STRIPS_PER_PLANE/(2*INCLUDE_N_NEIGHBOURS+1)+1;

    const uint NCLUSTER_POINTS = 10;
    const uint NCLUSTERS = 100;
}

#endif //PROJECT_NMXCLUSTERERSETTINGS_H
