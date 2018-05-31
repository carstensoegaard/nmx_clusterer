//
// Created by soegaard on 4/25/18.
//

#ifndef PROJECT_NMXQMATRIX_H
#define PROJECT_NMXQMATRIX_H

#include <iostream>

#include "clusterer/include/NMXClustererSettings.h"

namespace nmx {

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

        std::array<unsigned int, 2> getDIM(){

            std::array<unsigned int, 2> ret;
            ret.at(0) = m_dim[0];
            ret.at(1) = m_dim[1];

            return ret;
        };

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

            return -1.;
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

            return -1;
        }

        void reset() {
            for (unsigned int i = 0; i < m_dim[0]; i++) {
                setLink(i, 0, -1);
                for (unsigned int j = 0; j < m_dim[1]; j++) {
                    setQ(i, j, 2.);
                    if (i == 0)
                        setLink(j, 1, -1);
                }
            }
            m_dim[0] = 0;
            m_dim[1] = 0;
        }

    private:
        unsigned int m_dim[2];

        std::array<std::array<int, nmx::DIM_Q_MATRIX>, 2> m_links;
        std::array <std::array<double, nmx::DIM_Q_MATRIX>, nmx::DIM_Q_MATRIX> m_matrix;
    };
}

#endif //PROJECT_NMXQMATRIX_H
