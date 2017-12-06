//
// Created by soegaard on 11/22/17.
//

#include <iostream>
#include "BoxAdministration.h"
#include "NMXClustererDefinitions.h"

BoxAdministration::BoxAdministration()
        : m_stackHead(nmx::NBOXES-1),
          m_queueHead(-1),
          m_queueTail(-1)
{

    init();
}

int BoxAdministration::getBoxFromStack() {

    int newbox = m_stackHead;

    m_stackHead = m_boxList.at(m_stackHead).link1;

    return newbox;
}

void BoxAdministration::returnBoxToStack(const uint &ibox) {

    m_boxList[ibox].link1 = m_stackHead;
    m_stackHead = ibox;
}

void BoxAdministration::insertBoxInQueue(const uint &ibox) {

    if (m_queueHead > -1) {
        m_boxList[ibox].link1 = m_queueHead;
        m_boxList[m_queueHead].link2 = ibox;
        m_boxList[ibox].link2 = -1;
        m_queueHead = ibox;
    } else  {
        m_queueHead = ibox;
        m_queueTail = ibox;
        m_boxList[ibox].link1 = -1;
        m_boxList[ibox].link2 = -1;
    }
}

void BoxAdministration::releaseBox(const uint &ibox) {

    int emptyBox = ibox;

    if (emptyBox != m_queueHead && emptyBox != m_queueTail)  {
        releaseBoxFromMiddle(ibox);

    } else {
        if (emptyBox == m_queueHead)
            releaseBoxFromHead();
        if (emptyBox == m_queueTail)
            releaseBoxFromTail();
    }

    resetBox(ibox);

    returnBoxToStack(ibox);
}

void BoxAdministration::releaseBoxFromMiddle(const uint &emptyBox) {

    int leftBox  = m_boxList.at(emptyBox).link2;
    int rightBox = m_boxList.at(emptyBox).link1;
    m_boxList[leftBox].link1  = rightBox;
    m_boxList[rightBox].link2 = leftBox;
}


void BoxAdministration::releaseBoxFromTail() {

    // Be aware !!!
    // empty box may be negative

    int emptyBox = m_queueTail;
    m_queueTail = m_boxList[emptyBox].link2;
    if (m_boxList[emptyBox].link2 > -1)
        m_boxList[m_queueTail].link1 = -1;
}

void BoxAdministration::releaseBoxFromHead() {

    // Be aware !!!
    // empty box may be negative

    int emptyBox = m_queueHead;
    m_queueHead = m_boxList[emptyBox].link1;
    if (m_boxList[emptyBox].link1 > -1)
        m_boxList[m_queueHead].link2 =- 1;
}



void BoxAdministration::updateBox(const int &boxid, const uint32_t strip, const uint32_t time) {

    auto &box = m_boxList.at(boxid);

    if (time < box.min_time)
        box.min_time = time;
    if (time > box.max_time)
        box.max_time = time;
    if (strip < box.min_strip)
        box.min_strip = strip;
    if (strip > box.max_strip)
        box.max_strip = strip;
}

bool BoxAdministration::checkBox(const int &boxid, const nmx::data_point &point) {

    //std::cout << "Checking box " << (uint)boxid << std::endl;

    auto box = m_boxList.at(boxid);

    //std::cout << "Data.time = " << point.time << ", box.max_time = " << box.max_time << std::endl;
    int time_diff = std::abs(static_cast<int>(point.time) - static_cast<int>(box.max_time));
    //std::cout << "Time diff " << time_diff;

    if (time_diff > nmx::MAX_CLUSTER_TIME) {
      //  std::cout << " larger than " << nmx::MAX_CLUSTER_TIME << std::endl;
        return true;
    }

    //std::cout << " smaller than " << nmx::MAX_CLUSTER_TIME << std::endl;

    return false;
}

inline void BoxAdministration::resetBox(const int &boxid) {

    m_boxList.at(boxid).min_strip = UINT32_MAX;
    m_boxList.at(boxid).max_strip = 0;
    m_boxList.at(boxid).min_time = UINT32_MAX;
    m_boxList.at(boxid).max_time = 0;
}

void BoxAdministration::init() {

    for (uint i = 0; i < nmx::NBOXES; i++) {

        resetBox(i);

        m_boxList.at(i).link1 = i - 1;
    }
}

void BoxAdministration::printStack() {

    std::cout << "Stack : ";

    int boxid = m_stackHead;

    while (boxid != -1) {

        std::cout << boxid << " ";
        boxid = getBox(boxid).link1;
    }

    std::cout << "\n";
}

void BoxAdministration::printQueue() {

    std::cout << "Queue : ";

    int boxid = m_queueTail;

    while (boxid != -1) {

        std::cout << boxid << " ";
        boxid = getBox(boxid).link2;

    }

    std::cout << "\n";
}

void BoxAdministration::printBoxesInQueue() {

    std::cout << "Boxes in queue : \n";

    int boxid = m_queueTail;

    while (boxid != -1) {

        nmx::box box = getBox(boxid);

        std::cout << "Box " << boxid << " :\n";
        std::cout << "        Strips [" << box.min_strip << ", " << box.max_strip << "]\n";
        std::cout << "        Time   [" << box.min_time << ", " << box.max_time << "]\n";
        boxid = box.link2;
    }

    std::cout << "\n";
}
