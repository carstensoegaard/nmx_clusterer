//
// Created by soegaard on 2/5/18.
//

#include <iostream>
#include "NMXClusterManager.h"

NMXClusterManager::NMXClusterManager()
        : m_stackHead(nmx::NCLUSTERS-1),
          m_stackTail(0),
          m_queueHead(-1),
          m_queueTail(-1)
{
    init();
}

int NMXClusterManager::getClusterFromStack() {

    if (m_stackHead == -1) {
        std::cout << "<NMXClusterManager::getClusterFromStack> Stack is empty!" << std::endl;
        return m_stackHead;
    }

    int idx = m_stackHead;

    auto currentstackheadcluster = m_buffer.at(m_stackHead);

    int newstackheadidx = currentstackheadcluster.box.link1;

    if (newstackheadidx >= 0) {
        auto newstackheadcluster = m_buffer.at(newstackheadidx);
        newstackheadcluster.box.link2 = -1;
    } else
        m_stackTail = -1;

    m_stackHead = newstackheadidx;

    return idx;
}

void NMXClusterManager::returnClusterToStack(int idx) {

    if ((m_stackHead == -1) && (m_stackTail == -1)) {
        m_stackHead = idx;
        m_buffer.at(idx).box.link1 = -1;
    } else
        m_buffer.at(m_stackTail).box.link1 = idx;

    m_buffer.at(idx).box.link1 = -1;
    m_buffer.at(idx).box.link2 = m_stackTail;

    m_stackTail = idx;
}

void NMXClusterManager::insertClusterInQueue(uint idx) {

    if (m_queueHead == -1)
        m_queueHead = idx;
    else {
        m_buffer.at()
    }
    m_queueTail =

}

void NMXClusterManager::init() {

    for (int i = 0; i < nmx::NCLUSTERS; i++) {

        m_buffer.at(i).box.link1 = i - 1;
        m_buffer.at(i).box.link2 = (i + 1 == nmx::NCLUSTERS ? -1 : i + 1);
    }
}

void NMXClusterManager::printStack() {

    std::cout << "Stack : ";

    int boxid = m_stackHead;

    while (boxid != -1) {

        std::cout << boxid << " ";
        boxid = m_buffer.at(boxid).box.link1;
    }

    std::cout << "\n";
}