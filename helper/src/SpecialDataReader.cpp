//
// Created by soegaard on 11/6/17.
//

#include <sstream>
#include <iostream>

#include "../include/SpecialDataReader.h"

SpecialDataReader::SpecialDataReader() {

    m_ifile.open("NMX_events.txt");
}

nmx::FullCluster SpecialDataReader::ReadNextEvent() {

    uint ievent = -1;

    nmx::FullCluster event;

    if (m_verboseLevel > 0)
        std::cout << "nXpoints = " << event.clusters.at(0).nPoints << ", nYpoints = " << event.clusters.at(1).nPoints
                  << std::endl;

    std::string line;

    bool inevent = false;

    std::streampos oldpos = m_ifile.tellg();  // store current position

    while (std::getline(m_ifile, line)) {

        if (isEventHeader(line, ievent)) {

            if (!inevent) {

                if (m_verboseLevel > 0)
                    std::cout << "Processing event # " << ievent << std::endl;

                inevent = true;
                continue;

            } else {

                if (m_verboseLevel > 0)
                    std::cout << "Finished event\n";

                inevent = false;
                m_ifile.seekg(oldpos);

                return event;

            }
        } else {

            if (inevent) {

                if (m_verboseLevel > 0)
                    std::cout << "Reding data ...\n";

                line_data data = readDataPoint(line);

                if (m_verboseLevel > 0)
                    std::cout << "Getting " << (data.at(0) ? "Y" : "X") << " plane\n";

                auto &plane = event.clusters.at(data.at(0));

                if (m_verboseLevel > 0)
                    std::cout << "Forming point\n";

                nmx::DataPoint point;
                point.strip  = data.at(1);
                point.time   = data.at(2);
                point.charge = data.at(3);

                if (m_verboseLevel > 0)
                    std::cout << "Inserting point at " << plane.nPoints << "\n";

                plane.data.at(plane.nPoints) = point;
                plane.nPoints++;
                if (point.time > plane.box.max_time)
                    plane.box.max_time = point.time;

                oldpos = m_ifile.tellg();  // store current position

                if (m_verboseLevel > 0)
                    std::cout << "Done!\n";
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
