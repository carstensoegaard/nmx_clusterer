//
// Created by soegaard on 11/22/17.
//

#ifndef PROJECT_BOXADMINISTRATION_H
#define PROJECT_BOXADMINISTRATION_H

#include <array>

#include "../include/NMXClustererDefinitions.h"

/*! Administration of nmx::Box
 *
 * This class contains and manages boxes. It holds a queue of nmx::Box. Provides cluster index munders for the
 * clusterer.
 */
class NMXBoxAdministration {

public:

    /*! Constructor */

    NMXBoxAdministration();

    /*! @name Stack operations
     *
     *
     */
    ///@{
    /*! Get a free box from the stack
     *
     * Gets the index of the first free box on the stack and removes it from the stack
     *
     * @return index of the free box
     */
    int getBoxFromStack();

    /*! Retun a box to the stack
     *
     * Once a box is no longer in use, the now free box must be returned to the stack.
     *
     * @param ibox The index of the now free box
     */
    void returnBoxToStack(unsigned int ibox);
    ///@}

    /*! @name Queue operations */
    ///@{
    /*! Insert a box in the queue
     *
     * Insert a box into the queue. The box will reside in the queue while the cluster is being built
     * @param ibox Index of the box to be inserted in the queue
     */
    void insertBoxInQueue(unsigned int ibox);
    /*! Release the box from the tail of the queue
     *
     * Freed boxes must be realeas from the queue in order to return to the stack. This function removes the box at
     * the tail of the queue.
     */
    void releaseBoxFromTail();
    /*! Release the box from the tail of the queue
     *
     * Freed boxes must be realeas from the queue in order to return to the stack. This function removes the box at
     * the head of the queue.
     */
    void releaseBoxFromHead();
    /*! Release the box from the tail of the queue
     *
     * Freed boxes must be realeas from the queue in order to return to the stack. This function removes the specified
     * box from the queue.
     *
     * @param emptybox The index of the box to be released from the queue
     */
    void releaseBoxFromMiddle(unsigned int emptybox);
    /*! Realase a box from the queue.
     *
     * This function findes the correct method to release the box.
     *
     * @param ibox the index of the box to be removed
     */
    void releaseBox(unsigned int ibox);
    ///@}
    /*! Update the content of the box, if necessary. */
    void updateBox(unsigned int boxid, const nmx::DataPoint &point);
    /*! Check the box
     *
     * Check the time-span of the box.
     * @param boxid Index of the box to be checked
     * @param point nmx::dataPoint to be checked against
     * @return True if the maximum time-span is exceeded.
     */
    bool checkBox(unsigned int boxid, const nmx::DataPoint &point);
    /*! Get a reference to a box.
     *
     * @param boxid The index of the box
     * @return Reference to the box
     */
    nmx::Box &getBox(unsigned int boxid);

    /*! Get the tail of the queue.
     *
     * @return index of the queue tail
     */
    int getQueueTail() { return m_queueTail; }

    /*! @name Debugging fuctions */
    ///@{
    /*! Print the current stack. */
    void printStack();
    /*! Print the current queue. */
    void printQueue();
    /*! Print the contents of the boxes in the current queue */
    void printBoxesInQueue();
    ///@}

private:

    int m_stackHead; /*!< Index of the stack-head. */
    int m_queueHead; /*!< Index of the queue head. */
    int m_queueTail; /*!< Index of the queue tail. */

    std::array<nmx::Box, nmx::NBOXES> m_boxList; /*!< nmx::Box container */

    /*! Reset the box. */
    void resetBox(unsigned int boxid);
    /*! Initialize the class. */
    void init();
};

#endif //PROJECT_BOXADMINISTRATION_H
