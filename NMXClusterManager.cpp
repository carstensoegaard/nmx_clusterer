//
// Created by soegaard on 2/5/18.
//

#include <iostream>
#include "NMXClusterManager.h"

NMXClusterManager::NMXClusterManager()
        : m_verboseLevel(0)
{
    init();
}

int NMXClusterManager::getClusterFromStack(unsigned int plane) {

    if (m_stackHead.at(plane) == -1) {
        std::cout << "<NMXClusterManager::getClusterFromStack> Stack " << (plane ? "Y" : "X") << " is empty!"
                  << std::endl;
        throw 1;
        //return m_stackHead.at(plane);
    }

    m_mutex.lock();
    int idx = m_stackHead.at(plane);

    nmx::cluster_buffer &buffer = m_buffer.at(plane);

    int newstackheadidx = getLink1(plane, idx);

    if (newstackheadidx < 0)
        m_stackTail.at(plane) = -1;

    m_stackHead.at(plane) = newstackheadidx;

    setLink1(plane, idx, -1);
    m_mutex.unlock();

    return idx;
}

void NMXClusterManager::returnClusterToStack(unsigned int plane, unsigned int idx) {

    if (m_verboseLevel > 0)
        std::cout << "<NMXClusterManager::returnClusterToStack> Returning cluster # " << idx
                  << " to plane " << (plane ? "Y" : "X")<< std::endl;

    m_mutex.lock();
    int &stackHead = m_stackHead.at(plane);
    int &stackTail = m_stackTail.at(plane);

    if (stackTail >= 0) // Stack is not empty
        setLink1(plane, stackTail, idx);
    else // Stack is empty
        stackHead = idx;

    setLink1(plane, idx, -1); // Set the link of the tail to -1

    stackTail = idx; // Set the tail to the new idx
    m_mutex.unlock();
}

nmx::cluster & NMXClusterManager::getCluster(unsigned int plane, unsigned int idx) {

    if (idx > nmx::NCLUSTERS) {
        std::cout << "<NMXClusterManager::getCluster> Index " << idx << " out of range!\n";
        throw 1;
    }

    nmx::cluster_buffer &buffer = m_buffer.at(plane);

    nmx::cluster& cluster = buffer.at(idx);

    return cluster;
}

int NMXClusterManager::getLink1(unsigned int plane, unsigned int idx) {

    nmx::cluster &cluster = getCluster(plane, idx);

    nmx::box &box = cluster.box;
    return box.link1;
}

bool NMXClusterManager::setLink1(unsigned int plane, unsigned int idx, int link1) {

    if (m_verboseLevel > 2)
    std::cout << "<NMXClusterManager::setLink1> Setting link1 of cluster " << idx << " plane "
              << (plane ? "Y" : "X") << " to " << link1 << std::endl;

    if (idx == link1) {
        std::cout << "<NMXClusterManager::setLink1> Cannot set link to itself! Plane = " << (plane ? "Y" : "X")
                  << ", idx = " << idx << ", link1 = " << link1 << std::endl;
        return false;
    }

    nmx::cluster &cluster = getCluster(plane, idx);

    nmx::box &box = cluster.box;
    box.link1 = link1;

    return true;
}

void NMXClusterManager::init() {

    for (int plane = 0; plane < 2; plane++) {
        m_stackHead.at(plane) = nmx::NCLUSTERS - 1;
        m_stackTail.at(plane) = 0;

        nmx::cluster_buffer &buffer = m_buffer.at(plane);

        for (int i = 0; i < nmx::NCLUSTERS; i++) {

            nmx::cluster &cluster = buffer.at(i);

            cluster.box.link1 = i - 1;
            cluster.box.link2 = -1;
        }
    }
}

void NMXClusterManager::printStack(unsigned int plane) {

    std::cout << "Stack " << (plane ? "Y" : "X") << " : ";

    int boxid = m_stackHead.at(plane);

    nmx::cluster_buffer &buffer = m_buffer.at(plane);

    while (boxid != -1) {

        std::cout << boxid << " ";
        boxid = buffer.at(boxid).box.link1;
    }

    std::cout << "\n";
}