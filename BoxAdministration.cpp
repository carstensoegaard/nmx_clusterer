//
// Created by soegaard on 11/22/17.
//

#include <iostream>
#include "BoxAdministration.h"

BoxAdministration::BoxAdministration()
        : m_stackHead(nmx::NBOXES-1),
          m_queueHead(-1),
          m_queueTail(-1)
{
    init();
}

int BoxAdministration::getBoxFromStack() {

    if (m_stackHead < 0) {
        std::cerr << "<BoxAdministration::getBoxFromStack> Stack is empty!\n";
        return -1;
    }

    int newbox = m_stackHead;

    m_stackHead = m_boxList.at(m_stackHead).link1;

    return newbox;
}

void BoxAdministration::returnBoxToStack(unsigned int ibox) {

    if (ibox > nmx::NBOXES-1)
        std::cerr << "<BoxAdminitration::returnBoxToStack> Box " << ibox << " is out of range [0,"
                  << nmx::NBOXES -1 << "]\n";

    m_boxList[ibox].link1 = m_stackHead;
    m_stackHead = ibox;
}

void BoxAdministration::insertBoxInQueue(unsigned int ibox) {

    if (ibox > nmx::NBOXES-1)
        std::cerr << "<BoxAdminitration::insertBoxInQueue> Box " << ibox << " is out of range [0,"
                  << nmx::NBOXES -1 << "]\n";


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

void BoxAdministration::releaseBox(unsigned int ibox) {

    if (ibox > nmx::NBOXES-1)
        std::cerr << "<BoxAdminitration::releaseBox> Box " << ibox << " is out of range [0,"
                  << nmx::NBOXES -1 << "]\n";

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

void BoxAdministration::releaseBoxFromMiddle(unsigned int emptyBox) {

    int leftBox  = m_boxList.at(emptyBox).link2;
    int rightBox = m_boxList.at(emptyBox).link1;
    m_boxList[leftBox].link1  = rightBox;
    m_boxList[rightBox].link2 = leftBox;
}

void BoxAdministration::releaseBoxFromTail() {

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

void BoxAdministration::updateBox(unsigned int boxid, const nmx::data_point &point) {

    if (boxid > nmx::NBOXES-1)
        std::cerr << "<BoxAdminitration::updateBox> Box " << boxid << " is out of range [0,"
                  << nmx::NBOXES -1 << "]\n";

    auto &box = m_boxList.at(boxid);

    if (point.time < box.min_time)
        box.min_time = point.time;
    if (point.time > box.max_time)
        box.max_time = point.time;
    if (point.strip < box.min_strip)
        box.min_strip = point.strip;
    if (point.strip > box.max_strip)
        box.max_strip = point.strip;

    box.chargesum += point.charge;
    if (point.charge > box.maxcharge)
        box.maxcharge = point.charge;
}

bool BoxAdministration::checkBox(unsigned int boxid, const nmx::data_point &point) {

    if (boxid > nmx::NBOXES-1)
        std::cerr << "<BoxAdminitration::checkBox> Box " << boxid << " is out of range [0,"
                  << nmx::NBOXES -1 << "]\n";

    auto box = m_boxList.at(static_cast<uint>(boxid));

    uint time_diff = std::abs(static_cast<int>(point.time) - static_cast<int>(box.max_time));

    if (time_diff > nmx::MAX_CLUSTER_TIME)
        return true;

    return false;
}

inline void BoxAdministration::resetBox(unsigned int boxid) {

    if (boxid > nmx::NBOXES-1)
        std::cerr << "<BoxAdminitration::resetBox> Box " << boxid << " is out of range [0,"
                  << nmx::NBOXES -1 << "]\n";

    m_boxList.at(boxid).min_strip = UINT32_MAX;
    m_boxList.at(boxid).max_strip = 0;
    m_boxList.at(boxid).min_time = UINT32_MAX;
    m_boxList.at(boxid).max_time = 0;
    m_boxList.at(boxid).chargesum = 0;
    m_boxList.at(boxid).maxcharge = 0;
}

nmx::box& BoxAdministration::getBox(unsigned int boxid) {

    if (boxid > nmx::NBOXES-1)
        std::cerr << "<BoxAdminitration::getBox> Box " << boxid << " is out of range [0,"
                  << nmx::NBOXES -1 << "]\n";

    return m_boxList.at(boxid);
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
        std::cout << "        Link 1 = " << box.link1 << std::endl;
        std::cout << "        Link 2 = " << box.link2 << std::endl;
        boxid = box.link2;
    }

    std::cout << "\n";
}
