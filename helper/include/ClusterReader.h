//
// Created by soegaard on 4/26/18.
//

#ifndef PROJECT_CLUSTERREADER_H
#define PROJECT_CLUSTERREADER_H

#include <fstream>
#include <vector>

#include "../../clusterer/include/NMXClustererDefinitions.h"

class ClusterReader {

public:

    ClusterReader();
    ClusterReader(const char* filename);
    ~ClusterReader();

    nmx::FullCluster getNextEvent();
    std::vector<nmx::FullCluster> getAllEvents();

    nmx::FullCluster getNextCluster();
    std::vector<nmx::FullCluster> getAllClusters();

private:

    std::ifstream m_ifile;


    nmx::Cluster convertToPlaneCluster(std::vector<int> time, std::vector<int> strip, std::vector<int> charge);

    std::vector<int> readLine();
    nmx::Cluster readPlane();
    std::vector<std::string> lineToVector(std::string &line);
    std::vector<int> stringVectorToIntVector(std::vector<std::string> &vector);

    bool checkLine(const std::string &line, const std::string &comp);

};


#endif //PROJECT_CLUSTERREADER_H
