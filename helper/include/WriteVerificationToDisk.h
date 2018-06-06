//
// Created by soegaard on 4/30/18.
//

#ifndef PROJECT_WRITEVERIFICATIONTODISK_H
#define PROJECT_WRITEVERIFICATIONTODISK_H


#include <vector>
#include <fstream>

#include "../../clusterer/include/NMXClustererDefinitions.h"

class WriteVerificationToDisk {

public:

    WriteVerificationToDisk();
    ~WriteVerificationToDisk();

    void write(const nmx::FullCluster &event, const std::vector<nmx::FullCluster> &clusters);

private:

    std::ofstream m_file;

    void writeEventToFile(unsigned int eventNo, const nmx::FullCluster &event);
    void writeClustersToFile(const std::vector<nmx::FullCluster> &clusters);
    void writeObjectToFile(const nmx::FullCluster &object);
    void writePlaneToFile(const nmx::Cluster &plane);
};


#endif //PROJECT_WRITEVERIFICATIONTODISK_H
