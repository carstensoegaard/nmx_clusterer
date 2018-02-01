//
// Created by soegaard on 1/31/18.
//

#ifndef PROJECT_PAIRCLUSTERS_H
#define PROJECT_PAIRCLUSTERS_H

#include "NMXClustererDefinitions.h"

class PairClusters {

public:

    PairClusters();



private:

    std::array<nmx::cluster, nmx::NCLUSTERS> m_buffer;


};

#endif //PROJECT_PAIRCLUSTERS_H
