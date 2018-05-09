//
// Created by soegaard on 4/26/18.
//

#ifndef PROJECT_CLUSTERREADER_H
#define PROJECT_CLUSTERREADER_H

#include <fstream>
#include <vector>

#include "NMXClustererDefinitions.h"

class ClusterReader {

public:

    ClusterReader();
    ClusterReader(const char* filename);
    ~ClusterReader();

    nmx::fullCluster getNextEvent();
    std::vector<nmx::fullCluster> getAllEvents();

    nmx::fullCluster getNextCluster();
    std::vector<nmx::fullCluster> getAllClusters();

private:

    std::ifstream m_ifile;


    nmx::cluster convertToPlaneCluster(std::vector<int> time, std::vector<int> strip, std::vector<int> charge);

    std::vector<int> readLine();
    nmx::cluster readPlane();
    std::vector<std::string> lineToVector(std::string &line);
    std::vector<int> stringVectorToIntVector(std::vector<std::string> &vector);

    bool checkLine(const std::string &line, const std::string &comp);

};


#endif //PROJECT_CLUSTERREADER_H
