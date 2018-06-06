//
// Created by soegaard on 2/8/18.
//

#ifndef TEST_CLUSTER_MANAGER
#define TEST_CLUSTER_MANAGER

#include <vector>
#include <iostream>
#include "../clusterer/include/NMXClusterManager.h"

int main() {

    srand(0);

    std::vector<int> clusters;

    NMXClusterManager cm;// = NMXClusterManager::getInstance();

    cm.printStack(0);

    int plane = 0;

    for (uint i = 0; i < 10; i++) {


        std::cout << "i = " << i << std::endl;

        int idx = cm.getClusterFromStack(plane);

        std::cout << "Got cluster with idx = " << idx << std::endl;

        if (idx >= 0)
            clusters.push_back(idx);
        cm.printStack(plane);
    }

    while (!clusters.empty()) {

        int idx = rand() % clusters.size();
        int clusteridx = clusters.at(idx);
        std::cout << "Returning cluster # " << clusteridx << std::endl;

        cm.returnClusterToStack(plane, clusteridx);
        cm.printStack(plane);

        clusters.erase(clusters.begin()+idx);
    }

    for (uint i = 0; i < 10; i++) {


        std::cout << "i = " << i << std::endl;

        int idx = cm.getClusterFromStack(plane);

        if (idx >= 0)
            clusters.push_back(idx);
        cm.printStack(plane);
    }

    while (!clusters.empty()) {

        int idx = rand() % clusters.size();
        int clusteridx = clusters.at(idx);
        std::cout << "Returning cluster # " << clusteridx << std::endl;

        cm.returnClusterToStack(plane, clusteridx);
        cm.printStack(plane);

        clusters.erase(clusters.begin()+idx);
    }
}

#endif