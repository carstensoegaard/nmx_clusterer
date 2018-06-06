//
// Created by soegaard on 2/19/18.
//

#ifndef PROJECT_NMXLOCATIONFINDER_H
#define PROJECT_NMXLOCATIONFINDER_H

#include <cstdint>
#include <fstream>
#include "NMXClusterManager.h"
//#include "NMXClusterPairing.h"

struct nmx_location {

    uint32_t x_strip;
    uint32_t y_strip;
    uint64_t time;
};

class NMXLocationFinder {

public:

    NMXLocationFinder(NMXClusterManager &clustermanager);
    ~NMXLocationFinder();

    nmx_location find(nmx::PairBuffer &buf);

private:

    NMXClusterManager &m_clusterManager;

    std::ofstream m_file;

    //uint64_t totalxPoints = 0;
    //uint64_t totalyPoints = 0;
};


#endif //PROJECT_NMXLOCATIONFINDER_H
