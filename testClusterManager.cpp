//
// Created by soegaard on 2/8/18.
//

#ifndef TEST_CLUSTER_MANAGER
#define TEST_CLUSTER_MANAGER

#include <vector>
#include <iostream>
#include "NMXClusterManager.h"

int main() {

    srand(0);

    std::vector<int> clusters;

    NMXClusterManager &cm = NMXClusterManager::getInstance();

    cm.printStack();

    for (uint i = 0; i < 15; i++) {


        std::cout << "i = " << i << std::endl;

        int idx = cm.getClusterFromStack();

        if (idx >= 0)
            clusters.push_back(idx);
        cm.printStack();
    }

    while (!clusters.empty()) {

        int idx = rand() % clusters.size();
        int clusteridx = clusters.at(idx);
        std::cout << "Returning cluster # " << clusteridx << std::endl;

        cm.returnClusterToStack(clusteridx);
        cm.printStack();

        clusters.erase(clusters.begin()+idx);
    }

    for (uint i = 0; i < 15; i++) {


        std::cout << "i = " << i << std::endl;

        int idx = cm.getClusterFromStack();

        if (idx >= 0)
            clusters.push_back(idx);
        cm.printStack();
    }

    while (!clusters.empty()) {

        int idx = rand() % clusters.size();
        int clusteridx = clusters.at(idx);
        std::cout << "Returning cluster # " << clusteridx << std::endl;

        cm.returnClusterToStack(clusteridx);
        cm.printStack();

        clusters.erase(clusters.begin()+idx);
    }
}

#endif