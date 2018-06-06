//
// Created by soegaard on 4/30/18.
//

#include "../include/WriteVerificationToDisk.h"

WriteVerificationToDisk::WriteVerificationToDisk() {

    m_file.open("NMX_verified_clusters.txt");
}

WriteVerificationToDisk::~WriteVerificationToDisk() {

    m_file.close();
}

void WriteVerificationToDisk::write(const nmx::FullCluster &event,
                                    const std::vector<nmx::FullCluster> &clusters) {

    writeEventToFile(event.eventNo, event);
    writeClustersToFile(clusters);
}

void WriteVerificationToDisk::writeEventToFile(unsigned int eventNo, const nmx::FullCluster &event) {

    //std::cout << "Writing event # " << eventNo << " to file!\n";
    m_file << "Event # " << eventNo << std::endl;
    writeObjectToFile(event);
}

void WriteVerificationToDisk::writeClustersToFile(const std::vector<nmx::FullCluster> &clusters) {

    m_file << "# of clusters : " << clusters.size() << std::endl;

    auto iter = clusters.begin();

    while (iter != clusters.end()) {

        writeObjectToFile(*iter);

        iter++;
    }
}

inline void WriteVerificationToDisk::writeObjectToFile(const nmx::FullCluster &object) {

    //std::cout << "Writing object to file\n";

    m_file << "X:\n";
    writePlaneToFile(object.clusters.at(0));
    m_file << "Y:\n";
    writePlaneToFile(object.clusters.at(1));
}

inline void WriteVerificationToDisk::writePlaneToFile(const nmx::Cluster &plane) {

    for (unsigned int i = 0; i < plane.nPoints; i++)
        m_file << plane.data.at(i).strip << " ";
    m_file << "\n";
    for (unsigned int i = 0; i < plane.nPoints; i++)
        m_file << plane.data.at(i).time << " ";
    m_file << "\n";
    for (unsigned int i = 0; i < plane.nPoints; i++)
        m_file << plane.data.at(i).charge << " ";
    m_file << "\n";
}