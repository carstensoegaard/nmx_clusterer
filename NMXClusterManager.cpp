//
// Created by soegaard on 2/5/18.
//

#include <iostream>
#include "NMXClusterManager.h"

NMXClusterManager::NMXClusterManager()
{
    init();
}

int NMXClusterManager::getClusterFromStackX() {

    return getClusterFromStack(0);
}

int NMXClusterManager::getClusterFromStackY() {

    return getClusterFromStack(1);
}

int NMXClusterManager::getClusterFromStack(unsigned int plane) {

    if (m_stackHead.at(plane) == -1) {
        std::cout << "<NMXClusterManager::getClusterFromStack> Stack " << (plane ? "Y" : "X") << " is empty!"
                  << std::endl;
        throw 1;
        //return m_stackHead.at(plane);
    }

    int idx = m_stackHead.at(plane);

    nmx::cluster_buffer &buffer = m_buffer.at(plane);

    //nmx::cluster &currentstackheadcluster = buffer.at(idx);

    int newstackheadidx = getLink1(plane, idx);//currentstackheadcluster.box.link1;

    if (newstackheadidx >= 0) {
        /*std::cout << "<NMXClusterManager::getClusterFromStack> Attemting to get cluster " << newstackheadidx
                  << " from " << (plane ? "Y" : "X") << std::endl;*/
        setLink2(plane, newstackheadidx, -1);
        //nmx::cluster &newstackheadcluster = buffer.at(newstackheadidx);
        //newstackheadcluster.box.link2 = -1;
    } else
        m_stackTail.at(plane) = -1;

    m_stackHead.at(plane) = newstackheadidx;

    printStack(plane);

    return idx;
}

void NMXClusterManager::returnClusterToStackX(unsigned int idx) {

    returnClusterToStack(0, idx);
}

void NMXClusterManager::returnClusterToStackY(unsigned int idx) {

    returnClusterToStack(1, idx);
}

void NMXClusterManager::returnClusterToStack(unsigned int plane, unsigned int idx) {

    std::cout << "<NMXClusterManager::returnClusterToStack> Returning cluster # " << idx
              << " to plane " << (plane ? "Y" : "X")<< std::endl;

    /*std::cout << "Stack before :\n";
    printStack(plane);*/

    int &stackHead = m_stackHead.at(plane);
    int &stackTail = m_stackTail.at(plane);

    nmx::cluster_buffer &buffer = m_buffer.at(plane);

    if ((stackHead == -1) && (stackTail == -1)) {
        stackHead = idx;
        setLink1(plane, idx, -1);
        //buffer.at(idx).box.link1 = -1;
    } else
        setLink1(plane, stackTail, idx);
        //buffer.at(stackTail).box.link1 = idx;

    setLink1(plane, idx, -1);
    setLink2(plane, idx, stackTail);
    //buffer.at(idx).box.link1 = -1;
    //buffer.at(idx).box.link2 = stackTail;

    stackTail = idx;

    /*std::cout << "Stack after :\n";*/
    printStack(plane);
}

nmx::cluster & NMXClusterManager::getClusterX(unsigned int idx) {

    return getCluster(0, idx);
}

nmx::cluster & NMXClusterManager::getClusterY(unsigned int idx) {

    return getCluster(1, idx);
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

int NMXClusterManager::getLink2(unsigned int plane, unsigned int idx) {

    nmx::cluster &cluster = getCluster(plane, idx);

    nmx::box &box = cluster.box;
    return box.link2;
}

bool NMXClusterManager::setLink1(unsigned int plane, unsigned int idx, int link1) {

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

bool NMXClusterManager::setLink2(unsigned int plane, unsigned int idx, int link2) {

    if (idx == link2) {
        std::cout << "<NMXClusterManager::setLink2> Cannot set link to itself! Plane = " << (plane ? "Y" : "X")
                  << ", idx = " << idx << ", link2 = " << link2 << std::endl;
        return false;
    }

    nmx::cluster &cluster = getCluster(plane, idx);

    nmx::box &box = cluster.box;
    box.link2 = link2;

    return true;
}

void NMXClusterManager::init() {

    for (int plane = 0; plane < 2; plane++) {
        m_stackHead.at(plane) = nmx::NCLUSTERS - 1;
        m_stackTail.at(plane) = 0;
        m_queueHead.at(plane) = -1;
        m_queueTail.at(plane) = -1;

        nmx::cluster_buffer &buffer = m_buffer.at(plane);

        for (int i = 0; i < nmx::NCLUSTERS; i++) {

            nmx::cluster &cluster = buffer.at(i);

            cluster.box.link1 = i - 1;
            cluster.box.link2 = (i + 1 == nmx::NCLUSTERS ? -1 : i + 1);
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