//
// Created by soegaard on 2/5/18.
//

#include <iostream>
#include <thread>

#include "NMXClusterManager.h"

NMXClusterManager::NMXClusterManager()
        : m_verboseLevel(0)
{
    reset();
}

int NMXClusterManager::getClusterFromStack(unsigned int plane) {

    bool firstYield = true;

    while (m_stackHead.at(plane) == -1) {
        m_nFailedClusterRequests++;
        if (firstYield) {
            std::cout << "<NMXClusterManager::getClusterFromStack> Stack " << (plane ? "Y" : "X")
                      << " empty - yielding thread!" << std::endl;
            firstYield = false;
        }
        std::this_thread::yield();
    }

    if (!firstYield)
        std::cout << "<NMXClusterManager::getClusterFromStack> Thread released!" << std::endl;

    m_mutex[plane].lock();

    if (m_verboseLevel > 2)
        std::cout << "<NMXClusterManager::gerClusterFromStack> Getting cluster from stack "
                  << (plane ? "Y" : "X") << "!" << std::endl;

    if (m_verboseLevel > 2) {
        std::cout << "Before:\n";
        std::cout << "Stack-head = " << m_stackHead.at(plane) << ", stack-tail = " << m_stackTail.at(plane) << std::endl;
        printStack(plane);
    }

    int idx = m_stackHead.at(plane);
    m_stackHead.at(plane) = m_buffer.at(plane).at(idx).box.link1; // Set stack-head to link1

    if (m_stackHead.at(plane) < 0) // Is the stack empty ?
        m_stackTail.at(plane) = -1; // Indicate this by setting the tail to -1

    // Decouple cluster from stack (set link1 to -1)
    m_buffer.at(plane).at(idx).box.link1 = -1;

    if (m_verboseLevel > 2) {
        std::cout << "After:\n";
        std::cout << "Stack-head = " << m_stackHead.at(plane) << ", stack-tail = " << m_stackTail.at(plane) << std::endl;
        printStack(plane);
    }

    m_mutex[plane].unlock();

    m_verboseLevel = 0;

    return idx;
}

void NMXClusterManager::returnClusterToStack(unsigned int plane, unsigned int idx) {

    if (m_verboseLevel > 0) {
        std::cout << "<NMXClusterManager::returnClusterToStack> Returning cluster # " << idx
                  << " to plane " << (plane ? "Y" : "X") << std::endl;
        std::cout << "Before:\n";
        printStack(plane);
    }

    m_mutex[plane].lock();
    int &stackHead = m_stackHead.at(plane);
    int &stackTail = m_stackTail.at(plane);

    if (m_verboseLevel > 0)
        std::cout << "<NMXClusterManager::returnClusterToStack> StackHead = " << stackHead << ", stackTail = "
                  << stackTail << std::endl;

    if (stackTail >= 0) // Stack is not empty
        m_buffer.at(plane).at(stackTail).box.link1 = idx; // Set link1 of the stack-tail to the new index.
    else // Stack is empty
        stackHead = idx; // Set the stack-head to the new index

    m_buffer.at(plane).at(idx).box.link1 = -1;

    stackTail = idx; // Set the tail to the new idx
    m_mutex[plane].unlock();

    if (m_verboseLevel > 0) {
        std::cout << "After:\n";
        printStack(plane);
    }
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

void NMXClusterManager::reset() {

    for (int plane = 0; plane < 2; plane++) {
        m_stackHead.at(plane) = nmx::NCLUSTERS - 1;
        m_stackTail.at(plane) = 0;

        nmx::cluster_buffer &buffer = m_buffer.at(plane);

        for (unsigned int i = 0; i < nmx::NCLUSTERS; i++) {

            nmx::cluster &cluster = buffer.at(i);

            cluster.npoints = 0;

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