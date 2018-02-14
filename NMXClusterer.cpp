//
// Created by soegaard on 2/13/18.
//

#include <iostream>
#include "NMXClusterer.h"

NMXClusterer::NMXClusterer() {}


void NMXClusterer::addDatapoint(unsigned int plane, nmx::data_point &point) {

    if (plane > 1) {
        std::cerr << "<NMXClusterer::addDatapoint> Plane value " << plane << " out of range!\n";
        return;
    }

    if (plane == 0)
        m_XplaneClusterer.addDataPoint(point);
    else
        m_YplaneClusterer.addDataPoint(point);
}