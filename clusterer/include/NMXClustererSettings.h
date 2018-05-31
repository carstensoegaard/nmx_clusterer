//
// Created by soegaard on 11/20/17.
//

#ifndef PROJECT_NMXCLUSTERERSETTINGS_H
#define PROJECT_NMXCLUSTERERSETTINGS_H

#include <cmath>

/*! Various configuring parameters
 *
 */

namespace nmx {

    /*! @name Detector specific parameters.
     *
     * Parameters which relate to the specifications of the detector, either directly or due to the actual
     * configuration.
     */
    ///@{
    /*! Number of strips per plane
     *
     * Number of readout channels per plane per sector in the NMX-detector
     */
    const unsigned int STRIPS_PER_PLANE = 256;

    /*! Number of neighboring bits to consider for merging two clusters.
     *
     * This parameter effective sets the maximum allowed gap in strips for the clustering algorithm.
     * The gap is calculated by \f$ \textrm{Max}_{\textrm{gap}} = 2 \cdot \textrm{INCLUDE\_N\_NEIGHBOURS} +1 \f$
     */
    const unsigned int INCLUDE_N_NEIGHBOURS = 1;

    /*! Maximum cluster time
     *
     * This value sets an upper limit to how long a cluster can be in time. This value is specific to the detector
     * configuration and is related to the drift velocity of the electrons in the detector gas medium.
     */
    const unsigned int MAX_CLUSTER_TIME = static_cast<uint>(30 * 32);
    ///@}



    /*! @name Sorting buffer parameters
     *
     * Parameters which control the coarseness and the length of the sorting buffer. The sorting buffer is used to order
     * data-points with respect to their time-stamps. The sum of the two values cannot exceed 31, since at least one bit
     * is required to control the sliding of the window. The time-stamps are expected to be 32 bits in length.
     */
    ///@{
    /*! The ignored bits.
     *
     * These bits are ignored, hence the more bits, the coarser the sorting of the time-stamps.
     */
    const unsigned int DATA_IGNORE_BITS =  5; // Ignored bits
    /*! The sorting bits.
     *
     * This sets the size of the sorting buffer. The size of the buffer will be 2 to the power of this value.
     */
    const unsigned int DATA_MINOR_BITS  =  7; // Sorting bits
    ///@}


    /*! @name Number of clusters */
    ///@{
    /*! Set the number of available clusters.
     *
     * This number should be set large enough, that a failed cluster request rarely or never occurs.
     * See NMXClusterer::getFailedClusterRequests. The transfer buffer between clustering and paring is set to the same
     * size.
     */
    const unsigned int NCLUSTERS = 30;
    ///@}

    /*! @name Q-matrix parameters
     *
     * The pairing algorithm requires the calculation of the so-called Q-matrix in order to determine whether the
     * total ernergies of the two clusters match adequately.
     */
    ///@{
    /*! Size of the Q-matrix.
     *
     * To avoid dynamic memory allocation, the maximum size of the matrix is set at compile-time. Set it lrage enough,
     * that is is never too small.
     */
    const unsigned int DIM_Q_MATRIX = 15;
    /*! Threshold for total energy match
     *
     * Meaningful values of Q are in the range ]0,2]
     * Lower value = stricter requirements. 2 = no requirement.
     */
    const double DELTA_Q = 2.0;
    ///@}

    /*! @name Paring buffer paramaters
     *
     * Parameters for the pairng buffer. Currently set at the same values as the sorting buffer. Given their own names
     * for easily independent configuration if required.
     */
    ///@{
    /*! The ignored bits.
     *
     * These bits are ignored, hence the more bits, the coarser the sorting of the clusters.
     */
    const unsigned int CLUSTER_IGNORE_BITS = DATA_IGNORE_BITS;

    /*! The sorting bits.
     *
     * This sets the size of the pairing buffer. The size of the buffer will be 2 to the power of this value.
     */
    const unsigned int CLUSTER_MINOR_BITS  = DATA_MINOR_BITS;
    ///@}
}

namespace nmx {


    /*! @name Derived quantities for the sorting buffer
     *
     * Quantities for the sorting buffer, derived from the configuration variables.
     */
    ///@{
    /*! Size of the sorting buffer.
     *
     * This calculates the size of the sorting buffer
     */
    const uint32_t DATA_MAX_MINOR  = 1 << DATA_MINOR_BITS;

    /*! Mask for retrieving the index
     *
     * This mask is used to retrieve the index for which the data-point should be inserted in the sorting buffer.
     */
    const uint32_t DATA_MINOR_BITMASK  = DATA_MAX_MINOR  - 1;

    /*! Number of boxes.
     *
     * This calculates the number of boxes necessary for the algorithm. This number os the maximum ever required
     * during a run.
     */
    const unsigned int NBOXES = STRIPS_PER_PLANE/(2*INCLUDE_N_NEIGHBOURS+1)+1;
    /// @}

    /*! @name Derived quantities for the pairing algorithm
     *
     */
    ///@{
    /*! Size of the pairing buffer.
     *
     * This calculates the size of the pairing buffer
     */
    const uint32_t CLUSTER_MAX_MINOR  = 1 << CLUSTER_MINOR_BITS;

    /*! Mask for retrieving the index
     *
     * This mask is used to retrieve the index for which the cluster should be inserted in the pairing buffer.
     */
    const uint32_t CLUSTER_MINOR_BITMASK  = CLUSTER_MAX_MINOR  - 1;
    ///@}
}

#endif //PROJECT_NMXCLUSTERERSETTINGS_H
