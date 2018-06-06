//
// Created by soegaard on 2/9/18.
//

#include <iostream>

#include "NMXTimeOrderedBuffer.h"

void readOutput(NMXTimeOrderedBuffer &tob) {

    while (1) {

        nmx::idx_buffer buf = tob.getNextSorted();

        std::cout << "Got " << buf.nidx << " points\n";

        for (uint i = 0; i < buf.nidx; i++) {

            int val = buf.data.at(i);

            std::cout << "Cluster idx = " << val << std::endl;
        }
    }
}

int main() {

    NMXTimeOrderedBuffer tob;

    std::thread t1 = std::thread(readOutput, std::ref(tob));

    for (int i = 0; i < 20; i++) {
        std::cout << "i = " << i << std::endl;
        tob.insert(i, i);
        tob.insert(i*10, i);

    }

    t1.join();

    return 1;
}