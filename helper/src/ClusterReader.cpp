//
// Created by soegaard on 4/26/18.
//

#include <sstream>
#include <iterator>
#include <vector>
#include "../include/ClusterReader.h"

ClusterReader::ClusterReader() {

    ClusterReader("NMX_PairedClusters.txt");
}

ClusterReader::ClusterReader(const char* filename) {

    m_ifile.open(filename);
}


ClusterReader::~ClusterReader() {

    m_ifile.close();
}

std::vector<nmx::FullCluster> ClusterReader::getAllClusters() {

    std::cout << "Getting all clusters ..." << std::endl;

    std::vector<nmx::FullCluster> ret;

    nmx::FullCluster cluster = getNextCluster();

    while (cluster.clusters.at(0).nPoints != 0) {

        ret.push_back(cluster);

        cluster = getNextCluster();
    }

    return ret;
}

std::vector<nmx::FullCluster> ClusterReader::getAllEvents() {

    std::cout << "Getting all events ... " << std::endl;

    std::vector<nmx::FullCluster> ret;

    nmx::FullCluster event = getNextEvent();

    /*std::cout << "Got an event with " << event.clusters.at(0).nPoints << " x-points and "
              << event.clusters.at(1).nPoints << " y-points." << std::endl;*/

    while (event.clusters.at(0).nPoints != 0) {

        ret.push_back(event);

        //std::cout << "Inserted the event. Now " << ret.size() << " events are inserted." << std::endl;

        event = getNextEvent();

        /*std::cout << "Got an event with " << event.clusters.at(0).nPoints << " x-points and "
                  << event.clusters.at(1).nPoints << " y-points." << std::endl;*/

    }

    return ret;
}

nmx::FullCluster ClusterReader::getNextEvent() {

    nmx::FullCluster ret;

    std::string line;

    std::getline(m_ifile, line);
    if (m_ifile.eof())
        return ret;
    if (line.compare(0, 7, "Event #")) {
        std::cout << "Not an event-header! '" << line << "'" << std::endl;
        return ret;
    }

    std::vector<std::string> vline = lineToVector(line);

    ret.eventNo = atoi(vline.at(2).c_str());

    std::getline(m_ifile, line);
    if (!checkLine(line, "X:"))
        return ret;

    ret.clusters.at(0) = readPlane();

    std::getline(m_ifile, line);
    if (!checkLine(line, "Y:"))
        return ret;

    ret.clusters.at(1) = readPlane();

    return ret;
}

nmx::FullCluster ClusterReader::getNextCluster() {

    nmx::FullCluster ret;

    std::string line;

    std::getline(m_ifile, line);
    if (m_ifile.eof())
        return ret;
    if (!checkLine(line, "******"))
        return ret;

    std::getline(m_ifile, line);
    if (!checkLine(line, std::string("x-points:")))
        return ret;

    ret.clusters.at(0) = readPlane();

    std::getline(m_ifile, line);
    if (!checkLine(line, std::string("y-points:")))
        return ret;

    ret.clusters.at(1) = readPlane();

    return ret;
}

std::vector<int> ClusterReader::readLine() {

    std::string line;

    std::getline(m_ifile, line);
    std::vector<std::string> vline = lineToVector(line);
    std::vector<int> ret = stringVectorToIntVector(vline);

    return ret;
}

nmx::Cluster ClusterReader::readPlane() {

    std::vector<int> time   = readLine();
    std::vector<int> strip  = readLine();
    std::vector<int> charge = readLine();

    return convertToPlaneCluster(time, strip, charge);
}

nmx::Cluster ClusterReader::convertToPlaneCluster(std::vector<int> time,
                                                  std::vector<int> strip,
                                                  std::vector<int> charge) {

    nmx::Cluster cluster;

    if (time.size() != strip.size() && strip.size() != charge.size()){
        std::cerr << "Array sizes do not match! Time-size = " << time.size() << ", strip-size = " << strip.size()
                  << ", charge-size = " << charge.size() << std::endl;
        return cluster;
    }

    for (unsigned int idx = 0; idx < time.size(); idx++) {

        nmx::DataPoint &point = cluster.data.at(cluster.nPoints);

        point.time   = static_cast<uint32_t >(time.at(idx));
        point.strip  = static_cast<uint32_t >(strip.at(idx));
        point.charge = static_cast<uint32_t >(charge.at(idx));
        cluster.nPoints++;
    }

    return cluster;
}

std::vector<std::string> ClusterReader::lineToVector(std::string &line) {

    std::stringstream ss(line);

    std::istream_iterator<std::string> begin(ss);
    std::istream_iterator<std::string> end;

    std::vector<std::string> vstrings(begin, end);

    return vstrings;
}


std::vector<int> ClusterReader::stringVectorToIntVector(std::vector<std::string> &vector) {

    std::vector<int> ret;

    auto iter = vector.begin();

    while (iter != vector.end()) {

        ret.push_back(atoi(iter->c_str()));
        iter++;
    }

    return ret;
}

bool ClusterReader::checkLine(const std::string &line, const std::string &comp) {

    if (line.compare(comp)) {
        std::cerr << "Unexpected input from file! Expected '" << comp << "' received '" << line << "'" << std::endl;
        return false;
    }

    return true;
}