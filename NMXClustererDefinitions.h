//
// Created by soegaard on 10/24/17.
//
#ifndef NMX_CLUSTER_DEFINITIONS_H
#define NMX_CLUSTER_DEFINITIONS_H

#include <iostream>
#include <array>

#include "NMXClustererSettings.h"


namespace nmx {

    // A data-point for either X or Y plane
    struct data_point {
        uint32_t strip;
        uint32_t charge;
        uint32_t time;
    };

    // Define an array of length NROWS
    typedef std::array<uint32_t, MAX_MINOR> row_array;
    // Define an array of length CHANNELS_PER_PLANE
    typedef std::array<int32_t, STRIPS_PER_PLANE> col_array;

    typedef std::array<data_point, STRIPS_PER_PLANE> buffer_data;

    struct buffer {
        uint npoints;
        buffer_data data;
    };

    struct idx_buffer {
        int nidx;
        std::array<int, 100> data;
    };

    struct clusterParingEntry {

        std::array<int, 2> queue;
        std::array<unsigned int, 2> queueLength;
    };

    class Qmatrix {

    public:
        Qmatrix() {
            m_dim[0] = nmx::DIM_Q_MATRIX;
            m_dim[1] = nmx::DIM_Q_MATRIX;

            reset();
        }

        void setDIM(unsigned int dimI, unsigned int dimJ) {
            if (dimI > nmx::DIM_Q_MATRIX) {
                std::cout << "<Qmatix::setDIM> i > " << nmx::DIM_Q_MATRIX << std::endl;
                dimI = nmx::DIM_Q_MATRIX;
            }
            if (dimJ > nmx::DIM_Q_MATRIX) {
                std::cout << "<Qmatix::setDIM> j > " << nmx::DIM_Q_MATRIX << std::endl;
                dimJ = nmx::DIM_Q_MATRIX;
            }
            m_dim[0] = dimI;
            m_dim[1] = dimJ;
        }

        void setQ(unsigned int i, unsigned int j, double val) {
            if ((i >= m_dim[0]) || (j >= m_dim[1]))
                std::cout << "<Qmatrix::set> Invalid indices i,j = " << i << "," << j << std::endl;
            else
                m_matrix.at(i).at(j) = val;
        }

        double at(unsigned int i, unsigned int j) const {
            if ((i >= m_dim[0]) || (j >= m_dim[1]))
                std::cout << "<Qmatrix::set> Invalid indices i,j = " << i << "," << j << std::endl;
            else
                return m_matrix.at(i).at(j);
        }

        void setLink(unsigned int idx, unsigned int plane, int cluster_idx) {
            if (idx >= m_dim[plane])
                std::cout << "<Qmatrix::setLink> Invalid idx = " << idx << " on " << (plane ? "Y\n" : "X\n");
            else
                m_links.at(plane).at(idx) = cluster_idx;
        }

        int getLink(unsigned int idx, unsigned int plane) const {
            if (idx >= m_dim[plane])
                std::cout << "<Qmatrix::getLink> Invalid idx = " << idx << " on " << (plane ? "Y\n" : "X\n");
            else
                return m_links.at(plane).at(idx);
        }

        void reset() {
            for (unsigned int i = 0; i < m_dim[0]; i++) {
                setLink(i, 0, -1);
                for (unsigned int j = 0; j < m_dim[1]; j++) {
                    setQ(i,j,2.);
                    if (i == 0)
                        setLink(j, 1, -1);
                }
            }
            m_dim[0] = 0;
            m_dim[1] = 0;
        }

    private:
        unsigned int m_dim[2];

        std::array<std::array<int, nmx::DIM_Q_MATRIX> ,2> m_links;
        std::array<std::array<double, nmx::DIM_Q_MATRIX>, nmx::DIM_Q_MATRIX> m_matrix;
    };

    typedef std::array<buffer, MAX_MINOR> tbuffer;
    typedef std::array<tbuffer, 2> time_ordered_buffer;

    // Cluster formation element definitions

    struct box {
        uint32_t min_strip;
        uint32_t max_strip;
        uint32_t min_time;
        uint32_t max_time;
        uint64_t chargesum;
        uint64_t maxcharge;
        int link1;
        int link2;
    };

    typedef std::array<data_point, STRIPS_PER_PLANE> cluster_points;

    struct cluster {
        nmx::box box;
        uint npoints;
        cluster_points data;
    };

    typedef std::array<cluster, NCLUSTERS> cluster_buffer;

    struct clusterPair {
        int x_idx;
        int y_idx;
    };

    struct pairBuffer {
        uint64_t npairs;
        std::array<clusterPair, 100> pairs;
    };
}

#endif //NMX_CLUSTER_DEFINITIONS_H
