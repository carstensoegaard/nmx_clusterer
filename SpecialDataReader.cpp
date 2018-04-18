//
// Created by soegaard on 11/6/17.
//

#include <sstream>
#include <iostream>

#include "SpecialDataReader.h"

SpecialDataReader::SpecialDataReader() {

    m_ifile.open("NMX_events.txt");
}

nmx::fullCluster SpecialDataReader::ReadNextEvent() {

    uint ievent = -1;

    nmx::fullCluster event;

    std::string line;

    bool inevent = false;

    std::streampos oldpos = m_ifile.tellg();  // store current position

    while (std::getline(m_ifile, line)) {

        if (isEventHeader(line, ievent)) {

            if (!inevent) {

                //std::cout << "Processing event # " << ievent << std::endl;

                inevent = true;
                continue;

            } else {

                //std::cout << "Finished event\n";

                inevent = false;
                m_ifile.seekg(oldpos);

                return event;

            }
        } else {

            if (inevent) {

                line_data data = readDataPoint(line);

                auto &plane = event.at(data.at(0));

                nmx::data_point point;
                point.strip  = data.at(1);
                point.time   = data.at(2);
                point.charge = data.at(3);

                plane.data.at(plane.npoints) = point;
                plane.npoints++;
                if (point.time > plane.box.max_time)
                    plane.box.max_time = point.time;

                oldpos = m_ifile.tellg();  // store current position
            }
        }
    }

    return event;
}

bool SpecialDataReader::isEventHeader(const std::string &line, uint &ievent) {

    std::istringstream iss(line);

    std::string value;

    bool isEventHeader = false;

    while (iss >> value) {

        if (value == "Event" || value == "#") {
            isEventHeader = true;
            continue;
        }

        if (isEventHeader) {
            ievent = std::stoi(value);
            return true;
        }
    };

    return false;
}

line_data SpecialDataReader::readDataPoint(const std::string &line) {

    line_data data;

    std::istringstream iss(line);

    uint32_t value;

    uint iparameter = 0;

    while (iss >> value) {

        switch (iparameter) {
            case 0:
                data.at(0) = value; // Plane
                iparameter++;
                break;
            case 1:
                data.at(1) = value; // Strip
                iparameter++;
                break;
            case 2:
                data.at(2) = value; // Time-bin
                iparameter++;
                break;
            case 3:
                data.at(3) = value; // Charge
                iparameter++;
                break;
            default:
                std::cout << "THIS SHOULD NEVER HAPPEN!!!" << std::endl;
        }
    }

    return data;
}
