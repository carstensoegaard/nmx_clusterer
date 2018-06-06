//
// Created by soegaard on 1/26/18.
//

#ifndef PROJECT_NMXCLUSTERERDEBUGINFO_H
#define PROJECT_NMXCLUSTERERDEBUGINFO_H

#include <zconf.h>
#include <iostream>
#include <iomanip>

#include "../../clusterer/include/NMXClustererDefinitions.h"
#include "../../clusterer/include/NMXBoxAdministration.h"
#include "../../clusterer/include/NMXClusterManager.h"

namespace nmx {


    static bool checkD(uint d, std::string s) {

        if (d > nmx::DATA_MAX_MINOR) {
            std::string sout("\n<");
            sout.append(s);
            sout.append("> Cannot move ");
            sout.append(std::to_string(d));
            sout.append(" entries to clusterer !\nMax is ");
            sout.append(std::to_string(nmx::DATA_MAX_MINOR));
            sout.append(" FATAL ERROR");
            std::cout << sout;
            return false;
        }

        return true;
    }

    //*****************************************************************************************************************

    static bool checkPoint(const nmx::DataPoint& point, std::string s) {

        if (point.strip >= nmx::STRIPS_PER_PLANE) {
            std::cerr << "<" << s << "> Strip # " << point.strip << " is larger than " << nmx::STRIPS_PER_PLANE - 1
                      << std::endl;
            std::cerr << "Omitting point!\n";

            return false;
        }

        return true;
    }

    //*****************************************************************************************************************

    static bool checkMinorTime(uint32_t minortime, std::string s) {

        if (minortime >= nmx::DATA_MAX_MINOR) {
            std::cerr << "<" << s << "> Minor-time = " << minortime << " max minor-time = " << nmx::DATA_MAX_MINOR - 1
                      << "\n";
            std::cerr << " *** Point will not be added to the DataBuffer! ***\n";
            return false;
        }

        return true;
    }

    //*****************************************************************************************************************

    static void printPoint(const nmx::DataPoint &point) {

        std::cout << "Point : S = " << point.strip << " C = " << point.charge << " T = " << point.time << std::endl;
    }

    //*****************************************************************************************************************

    static void printTimeOrderedBuffer(const nmx::time_ordered_buffer& time_ordered_buffer,
                                       const nmx::dataColumn_t& SortQ) {

        std::cout << "Time ordered DataBuffer :\n";

        for (uint idx = 0; idx < nmx::DATA_MAX_MINOR; idx++) {

            nmx::dataBufferColumn_t tbuf = time_ordered_buffer.at(SortQ.at(idx));
            auto buf = tbuf.at(idx);

            if (buf.nPoints == 0)
                continue;

            std::cout << "Index " << idx << std::endl;

            std::cout << "Strip  ";
            for (uint ientry = 0; ientry < buf.nPoints; ientry++) {

                auto point = buf.data.at(ientry);
                std::cout << std::setw(5) << point.strip;
            }

            std::cout << "\nTime   ";
            for (uint ientry = 0; ientry < buf.nPoints; ientry++) {

                auto point = buf.data.at(ientry);
                std::cout << std::setw(5) << point.time;
            }

            std::cout << "\nCharge ";
            for (uint ientry = 0; ientry < buf.nPoints; ientry++) {

                auto point = buf.data.at(ientry);
                std::cout << std::setw(5) << point.charge;
            }
            std::cout << "\n";
        }
    }

    //*****************************************************************************************************************

    static void printBox(const nmx::Box &box) {

        std::cout << "Strips [" << box.min_strip << ", " << box.max_strip << "]\n";
        std::cout << "Time   [" << box.min_time << ", " << box.max_time << "]\n";
    }

    //*****************************************************************************************************************

    static void printBox(int boxid, NMXBoxAdministration *boxes) {

        std::cout << "Box-id " << boxid << std::endl;

        nmx::Box box = boxes->getBox(boxid);

        printBox(box);
    }


    //*****************************************************************************************************************

    static void printMajorTimeBuffer(const nmx::dataColumn_t& majortime_buffer) {

        std::cout <<"\nMajorTimeBuffer:\n";

        for (uint idx = 0; idx < nmx::DATA_MAX_MINOR; idx++) {
            if (idx%32 == 0)
                std::cout << "\nIdx : ";
            std::cout << std::setw(4) << idx << " ";
        }
        for (uint idx = 0; idx < nmx::DATA_MAX_MINOR; idx++) {
            if (idx%32 == 0)
                std::cout << "\nVal : ";
            std::cout << std::setw(4) << majortime_buffer.at(idx) << " ";
        }
        std::cout << "\n";
    }

    //*****************************************************************************************************************

    static void checkI1(const nmx::dataColumn_t& majortime_buffer, const uint i1) {

        bool ok = true;

        uint shifts = 0;

        for (uint idx = 0; idx < nmx::DATA_MINOR_BITMASK - 1; idx++) {
            if (majortime_buffer.at(idx) < majortime_buffer.at(idx + 1)) {
                std::cout << "Wrong order in B2 DataBuffer\n";
                ok = false;
            }
            if (majortime_buffer.at(idx) > majortime_buffer.at(idx + 1)) {
                shifts++;
                if (idx != i1) {
                    std::cout << "i1 at wrong location! i1 is at " << i1 << " should be at " << idx << std::endl;
                    ok = false;
                }
            }
        }

        if (shifts > 1) {
            std::cout << "Too many shifts " << shifts << std::endl;
            ok = false;
        }

        if (!ok) {
            printMajorTimeBuffer(majortime_buffer);
            throw 1;
        }
    }

    //*****************************************************************************************************************

    static void printMask(nmx::dataRow_t mask) {

        std::cout <<"\nMask:\n";

        for (uint idx = 0; idx < mask.size(); idx++) {
            if (idx%32 == 0)
                std::cout << "\nIdx : ";
            std::cout << std::setw(4) << idx << " ";
        }
        for (uint idx = 0; idx < mask.size(); idx++) {
            if (idx%32 == 0)
                std::cout << "\nVal : ";
            std::cout << std::setw(4) << mask.at(idx) << " ";
        }
        std::cout << "\n";
    }

    //*****************************************************************************************************************

    static void printQueue(unsigned int plane, int idx, NMXClusterManager &manager) {

        if (idx < 0) {
            std::cout << "Empty!\n";
            return;
        }

        std::cout << (plane ? "Y" : "X") << "-queue: ";

        while (idx >= 0) {
            std::cout << idx << " -> ";
            int newIdx = manager.getLink1(plane, idx);
            if (newIdx == idx) {
                std::cout << "Cluster " << idx << " is linked to itself!" << std::endl;
                break;
            }
            idx =  newIdx;
        }
        std::cout << "\n";
    }
}

#endif //PROJECT_NMXCLUSTERERDEBUGINFO_H

