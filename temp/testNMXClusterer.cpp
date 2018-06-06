//
// Created by soegaard on 10/27/17.
//

#include <iostream>
#include <iomanip>

#include "../clusterer/include/NMXClustererDefinitions.h"
#include "../clusterer/include/NMXPlaneClusterer.h"

void printBuffer(const nmx::buffer &buffer) {

    for (int i = 0; i < nmx::MAX_MINOR; ++i) {
        for (int j = 0; j < nmx::STRIPS_PER_PLANE; ++j)
            std::cout << std::setw(5) << buffer.at(i).at(j).time << " ";
        std::cout << "\n";
        for (int j = 0; j < nmx::STRIPS_PER_PLANE; ++j)
            std::cout << std::setw(5) << buffer.at(i).at(j).charge << " ";
        std::cout << "\n";
        std::cout << "\n";
    }
}

int main() {

    Clusterer c;

    c.addDataPoint(3,  1230, 813);
    printBuffer(c.getDataContainer());
    c.addDataPoint(8,  1230, 417);
    printBuffer(c.getDataContainer());
    c.addDataPoint(10, 1230, 312);
    printBuffer(c.getDataContainer());
    c.addDataPoint(2,  1240, 112);
    printBuffer(c.getDataContainer());
    c.addDataPoint(7,  1240, 556);
    printBuffer(c.getDataContainer());
    c.addDataPoint(11, 1240, 648);
    printBuffer(c.getDataContainer());
    c.addDataPoint(4,  1250, 394);
    printBuffer(c.getDataContainer());
    c.addDataPoint(8,  1250, 119);
    printBuffer(c.getDataContainer());
    c.addDataPoint(12, 1250, 963);
    printBuffer(c.getDataContainer());

    nmx::col_array mask = c.getClusterMask();

    for (int i = 0; i < mask.size(); ++i) {
        std::cout << std::setw(5) << mask.at(i) << " ";
    }
    std::cout << "\n";

    return 0;
}

