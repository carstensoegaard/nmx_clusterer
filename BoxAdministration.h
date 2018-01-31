//
// Created by soegaard on 11/22/17.
//

#ifndef PROJECT_BOXADMINISTRATION_H
#define PROJECT_BOXADMINISTRATION_H

#include <array>

#include "NMXClustererDefinitions.h"

class BoxAdministration {

public:

    BoxAdministration();

    // Stack operations
    int getBoxFromStack();
    void returnBoxToStack(const uint &ibox);

    // Queue operations
    void insertBoxInQueue(const uint &ibox);
    void releaseBoxFromTail();
    void releaseBoxFromHead();
    void releaseBoxFromMiddle(const uint &emptybox);
    void releaseBox(const uint &ibox);

    void updateBox(const int &boxid, const nmx::data_point &point);
    bool checkBox(const int &boxid, const nmx::data_point &point);

    nmx::box &getBox(const int &boxid);

    void printStack();
    void printQueue();
    void printBoxesInQueue();

private:

    int m_stackHead;
    int m_queueHead;
    int m_queueTail;

    std::array<nmx::box, nmx::NBOXES> m_boxList;

    void resetBox(const int &boxid);
    void init();
};

#endif //PROJECT_BOXADMINISTRATION_H
