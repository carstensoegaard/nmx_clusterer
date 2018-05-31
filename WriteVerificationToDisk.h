//
// Created by soegaard on 4/30/18.
//

#ifndef PROJECT_WRITEVERIFICATIONTODISK_H
#define PROJECT_WRITEVERIFICATIONTODISK_H


#include <vector>
#include <fstream>

#include "clusterer/include/NMXClustererDefinitions.h"

class WriteVerificationToDisk {

public:

    WriteVerificationToDisk();
    ~WriteVerificationToDisk();

    void write(const nmx::fullCluster &event, const std::vector<nmx::fullCluster> &clusters);

private:

    std::ofstream m_file;

    void writeEventToFile(unsigned int eventNo, const nmx::fullCluster &event);
    void writeClustersToFile(const std::vector<nmx::fullCluster> &clusters);
    void writeObjectToFile(const nmx::fullCluster &object);
    void writePlaneToFile(const nmx::cluster &plane);
};


#endif //PROJECT_WRITEVERIFICATIONTODISK_H
