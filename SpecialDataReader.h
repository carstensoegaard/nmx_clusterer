//
// Created by soegaard on 11/6/17.
//

#ifndef PROJECT_SPECIALDATAREADER_H
#define PROJECT_SPECIALDATAREADER_H

#include <fstream>
#include <string>
#include <vector>
#include <array>

#include "NMXClustererDefinitions.h"


typedef std::array<uint32_t, 4> line_data;
typedef std::vector<nmx::data_point> plane;
typedef std::array<plane, 2> event;

class SpecialDataReader {

public:

    SpecialDataReader();

    nmx::fullCluster ReadNextEvent();

private:

    std::ifstream m_ifile;

    bool isEventHeader(const std::string &line, uint &ievent);
    line_data readDataPoint(const std::string &line);

};

#endif //PROJECT_SPECIALDATAREADER_H
