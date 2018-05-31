//
// Created by soegaard on 11/22/17.
//

#ifndef PROJECT_BOXADMINISTRATION_H
#define PROJECT_BOXADMINISTRATION_H

#include <array>

#include "clusterer/include/NMXClustererDefinitions.h"

class BoxAdministration {

public:

    BoxAdministration();

    // Stack operations
    int getBoxFromStack();
    void returnBoxToStack(unsigned int ibox);

    // Queue operations
    void insertBoxInQueue(unsigned int ibox);
    void releaseBoxFromTail();
    void releaseBoxFromHead();
    void releaseBoxFromMiddle(unsigned int emptybox);
    void releaseBox(unsigned int ibox);

    void updateBox(unsigned int boxid, const nmx::data_point &point);
    bool checkBox(unsigned int boxid, const nmx::data_point &point);

    nmx::box &getBox(unsigned int boxid);

    int getQueueTail() { return m_queueTail; }

    void printStack();
    void printQueue();
    void printBoxesInQueue();

private:

    int m_stackHead;
    int m_queueHead;
    int m_queueTail;

    std::array<nmx::box, nmx::NBOXES> m_boxList;

    void resetBox(unsigned int boxid);
    void init();
};

#endif //PROJECT_BOXADMINISTRATION_H
