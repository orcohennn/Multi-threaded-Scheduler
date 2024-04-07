/** ~~~~~~~~~~~~~~~~~~ Includes ~~~~~~~~~~~ **/

#include <iostream>
#include <setjmp.h>
#include <memory>

/** ~~~~~~~~~~~~~~~~~~ Defines ~~~~~~~~~~~ **/

#define STACK_SIZE 4096 /* stack size per thread (in bytes) */

typedef void (*thread_entry_point)(void);

enum ThreadState{ READY, RUNNING, BLOCKED };

/**
 * The Thread class represents a single thread of execution in a multi-threaded program.
 */
class Thread
{
 private:
  int id;                 // The ID of the thread
  int quantums;           // The number of quantum ticks this thread has received
  ThreadState state;      // The current state of the thread
  char* stack;// The stack used by the thread

 public:
  /**
   * Constructs a new thread object with the specified ID and entry point.
   *
   * @param id The ID of the new thread.
   * @param entry_point The entry point of the new thread.
   */
  Thread(int id, thread_entry_point entry_point);

  sigjmp_buf env;         // The environment buffer used for saving and restoring the thread state
  /**
  * @brief Destructor for the Thread class.
  * This destructor deallocates the memory used by the thread's stack.
  * @param None
  * @return None
  */
  ~Thread();
  /**
   * Determines whether this thread object is equal to the specified thread object.
   *
   * @param other A shared pointer to the other thread object.
   * @return True if the two thread objects have the same ID, false otherwise.
   */
  bool isEqual(std::shared_ptr<Thread> other);

  /**
   * Returns the ID of this thread object.
   *
   * @return The ID of the thread.
   */
  int getId();

  /**
   * Sets the state of this thread object.
   *
   * @param st The new state of the thread.
   */
  void setState(ThreadState st);

  /**
   * Returns the current state of this thread object.
   *
   * @return The current state of the thread.
   */
  ThreadState getState();

  /**
   * Returns the number of quantum ticks this thread has received.
   *
   * @return The number of quantum ticks.
   */
  int getQuantums();

  /**
   * Increments the quantum count for this thread object.
   */
  void incrementQuantum();
};
