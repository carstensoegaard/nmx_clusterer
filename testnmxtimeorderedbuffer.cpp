//
// Created by soegaard on 2/9/18.
//

#include <iostream>

#include "NMXTimeOrderedBuffer.h"

void readOutput(NMXTimeOrderedBuffer<int> &tob) {

    while (1) {

        buffer<int> buf = tob.getNextProcessed();

        std::cout << "Got " << buf.npoints << " points\n";

        for (uint i = 0; i < buf.npoints; i++) {

            int val = buf.data.at(i);

            std::cout << "Cluster idx = " << val << std::endl;
        }
    }
}

int main() {

    NMXTimeOrderedBuffer<int> tob;

    std::thread t1 = std::thread(readOutput, std::ref(tob));

    for (int i = 0; i < 20; i++) {
        std::cout << "i = " << i << std::endl;
        tob.insert(i, i);
        tob.insert(i*10, i);

    }

    t1.join();

    return 1;
}