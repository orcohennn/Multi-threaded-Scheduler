/** ~~~~~~~~~~~~~~~~~~ Includes ~~~~~~~~~~~ **/
#include <string>
#include <iostream>
#include "uthreads.h"
#include <queue>
#include <signal.h>
#include <sys/time.h>
#include <memory>
#include <map>
#include "thread.h"

/** ~~~~~~~~~~~~~~~~~~ Defines ~~~~~~~~~~~ **/
#define FAILURE -1
#define SUCCESS 0
#define ANOTHER_THREAD_JUMP 1
#define MIL 1000000
#define ERR_LIB_FORMAT "thread library error: "
#define ERR_SYS_FORMAT "system error: "
#define INIT_ERR "Init error, quantum isn't positive!"
#define SPAWN_ERR "Spawn error, max threads or invalid entry_point!"
#define TERMINATE_ERR "terminate error, invalid thread id"
#define BLOCK_ERR "Block error, illegal tid!"
#define RESUME_ERR "Resume error, illegal tid!"
#define SIGACTION_ERR "sigaction error."
#define SETTIMER_ERR "settimer error."
#define SIGADDSET_ERR "sigaddset error."
#define SIGPROCMASK_ERR "sigprocmask error."
#define QUANTUM_ERR "quantum error, invalid thread id"
#define SLEEP_ERR "sleep error, main thread is illegal"
#define BAD_ALLOC_ERR "bad alloc"

/** readyQueue - a vector of shared pointers to threads that are ready to be executed */
std::deque<std::shared_ptr<Thread>> readyQueue;

/** threadsVector - a vector of shared pointers to threads */
std::vector<std::shared_ptr<Thread>> threadsVector (100, nullptr);
std::priority_queue<int, std::vector<int>, std::greater<int> > minValidId;

/** sleepMap - A map with integer key and value to store the amount of time a
 * thread should sleep before it resumes execution. */
std::map<int, int> sleep_map;

/** A shared pointer to the Thread that running now, initially set to  nullptr */
std::shared_ptr<Thread> running_thread = nullptr;

struct sigaction sa = {};
struct itimerval timer;
sigset_t blockedSigSet;
int totalQuantums;
bool is_blocked = false;

/** ~~~~~~~~~~~~~~~~~~ Helper functions ~~~~~~~~~~~ **/

void removeFromReady (const std::shared_ptr<Thread>& thread);
int tidCheck (int tid, std::string msg, int floor_tid);
void timerInitialize (int usecs);
int uthread_create (thread_entry_point entry_point);
void sleepsQuantumUpdate();

/**
uthread_get_tid - gets the ID of the currently running thread
@return the ID of the running thread
*/
int uthread_get_tid ()
{
  return running_thread->getId ();
}

/**
err_lib_print - prints an error message for a library error
@param err_text: the text of the error message
@return void
*/
void err_lib_print (std::string err_text)
{
  std::cerr << ERR_LIB_FORMAT << err_text << '\n';
}

/**
err_sys_print - prints an error message for a system error and exits the program
@param err_text: the text of the error message
@return void
*/
void err_sys_print (std::string err_text)
{
  std::cerr << ERR_SYS_FORMAT << err_text << '\n';
  exit (1);
}

/**
block_signals_helper - helper function to block signals
@return EXIT_SUCCESS if the signals were successfully blocked, FAILURE otherwise
*/
int block_signals_helper()
{
  if (!is_blocked)
  {
    if (sigprocmask (SIG_BLOCK, &blockedSigSet, nullptr))
    {
      err_sys_print (SIGPROCMASK_ERR);
      return FAILURE;
    }
    is_blocked = true;
  }
  return EXIT_SUCCESS;
}

/**
unblock_signals_helper - helper function to unblock signals
@return EXIT_SUCCESS if the signals were successfully unblocked, FAILURE otherwise
*/
int unblock_signals_helper(){
  if(is_blocked)
  {
    if (sigprocmask (SIG_UNBLOCK, &blockedSigSet, nullptr))
    {
      err_sys_print (SIGPROCMASK_ERR);
      return FAILURE;
    }
    is_blocked = false;
  }
    return EXIT_SUCCESS;
}

/**
readyQueueIsFull - checks if the ready queue is full
@return true if the ready queue is full, false otherwise
*/
bool readyQueueIsFull ()
{
  return readyQueue.size () == MAX_THREAD_NUM -1;
}

/**
minValidIdInitialize - initializes the minimum valid thread ID queue
@return void
*/
void minValidIdInitialize ()
{
  for (int i = 0; i < 100; i++)
  {
    minValidId.push (i);
  }
}

/**
removeFromReady - removes a thread from the ready queue
@param thread: a shared pointer to the thread to be removed from the ready queue
@return void
*/
void removeFromReady (const std::shared_ptr<Thread>& thread)
{
  for (unsigned long i = 0; i < readyQueue.size (); i++)
  {
    if (readyQueue[i]->isEqual (thread))
    {
      readyQueue.erase (readyQueue.begin () + i);
    }
  }
}

/**
tidCheck - checks if the given thread ID is valid
@param tid: the thread ID to check
@param msg: a message to print if the thread ID is invalid
@param floor_tid: the minimum valid thread ID
@return SUCCESS if the thread ID is valid, FAILURE otherwise
*/
int tidCheck (int tid, std::string msg, int floor_tid)
{
  std::shared_ptr<Thread> thread = nullptr;
  if (tid < floor_tid || tid > 99)
  {
    err_lib_print (msg);
    return FAILURE;
  }
  if (threadsVector[tid] != nullptr)
  {
      thread = threadsVector[tid];
  }
  if (thread == nullptr)
  {
    err_lib_print (msg);
    return FAILURE;
  }
  return SUCCESS;
}

/**
uthread_create - creates a new thread with the given entry point
@param entry_point: the function to execute when the thread is created
@return the ID of the new thread, or FAILURE if the creation failed
*/
int uthread_create (thread_entry_point entry_point)
{
  int threadId = minValidId.top ();
  minValidId.pop ();
  std::shared_ptr<Thread> newtThread;
  try{
      newtThread = (std::make_shared<Thread> (threadId, entry_point));
  }
  catch(std::bad_alloc &e) {
    err_sys_print (BAD_ALLOC_ERR);
  }
  if (entry_point == nullptr)
  {
    running_thread = newtThread;
    newtThread->setState (RUNNING);
  }
  else
  { readyQueue.push_back (newtThread);
  }
  threadsVector[threadId] = newtThread;
  return threadId;
}

/**
 * Clears all data structures used by the thread scheduler.
 * This includes the ready queue, the vector of all threads, and the sleep map.
 */
void Clear_database()
{
  readyQueue.clear();
  threadsVector.clear();
  sleep_map.clear();
}


/**
 * Switches execution from the current running thread to the next thread in the ready queue.
 *
 * @param to_block A boolean indicating whether the current thread should be blocked.
 * @param to_sleep A boolean indicating whether the current thread should be put to sleep.
 */
void jumpToThread (bool to_block, bool to_sleep)
{
  sleepsQuantumUpdate();
  totalQuantums++;

  if (readyQueue.empty())
  {
    running_thread->incrementQuantum();
    return;
  }
  if (running_thread != nullptr && !to_block)
  {
    running_thread->setState(READY);
    if (!to_sleep){
      readyQueue.push_back (running_thread);
    }
  }
  if (to_block){
    running_thread->setState(BLOCKED);
  }

  std::shared_ptr<Thread> &nextThread = readyQueue.front ();
  readyQueue.pop_front ();
  nextThread->setState (RUNNING);
  running_thread = nextThread;
  running_thread->incrementQuantum();
  if (setitimer (ITIMER_VIRTUAL, &timer, NULL) < 0)
  {
    err_sys_print (SETTIMER_ERR);
  }
  siglongjmp (running_thread->env, ANOTHER_THREAD_JUMP);

}

/**
 * Signal handler function for the virtual timer.
 * This function switches execution from the current running thread to the next thread in the ready queue.
 *
 * @param sig The signal number.
 */
void timer_handler (int sig)
{
  int ret_val = sigsetjmp(running_thread->env, ANOTHER_THREAD_JUMP);
  if (ret_val == 0)
  {
    jumpToThread(false, false);
  }
}

/**
timerInitialize - initializes the timer that determines the time quantum for each thread
@param usecs: the time in microseconds for each quantum
@return void
*/
void timerInitialize (int quantum_usecs)
{
  // Install timer_handler as the signal handler for SIGVTALRM.
  sa.sa_handler = &timer_handler;
  if (sigaction (SIGVTALRM, &sa, NULL) < 0)
  {
    err_sys_print (SIGACTION_ERR);
  }
  if (sigaddset (&blockedSigSet, SIGVTALRM) < 0)
  {
    err_sys_print (SIGADDSET_ERR);
  }

  timer.it_value.tv_sec = quantum_usecs/MIL;
  timer.it_value.tv_usec = quantum_usecs%MIL;

  timer.it_interval.tv_sec = quantum_usecs/MIL;
  timer.it_interval.tv_usec = quantum_usecs%MIL;

  if (setitimer (ITIMER_VIRTUAL, &timer, NULL) < 0)
  {
    err_sys_print (SETTIMER_ERR);
  }

}

/**
sleepsQuantumUpdate - updates the sleep time of each thread in the sleep queue
 and wakes up the threads that finish their times
@return void
*/
void sleepsQuantumUpdate()
{
  auto it = sleep_map.begin();
  while (it != sleep_map.end()) {
    if (it->second == 0)
    {
      std::shared_ptr<Thread> &weakup_thread = threadsVector[it->first];
      if(weakup_thread->getState() != BLOCKED){
        weakup_thread->setState(READY);
        readyQueue.push_back (weakup_thread);
      }
      it = sleep_map.erase (it);
    }
    else
    {
      it->second --;
      ++it;
    }
  }
}

/**
Removes an entry from the sleep_map based on the given thread ID.
@param tid Thread ID to remove from the sleep_map
*/
void removefromSleeps(int tid){
  auto it = sleep_map.begin();
  while (it != sleep_map.end())
  {
    if (it->first == tid){
      it = sleep_map.erase (it);
    }
    else{
      ++it;
    }
  }
}


/** ~~~~~~~~~~~~~~~~~~ Library functions ~~~~~~~~~~~ **/

int uthread_init (int quantum_usecs)
{
  if (quantum_usecs <= 0)
  {
    err_lib_print (INIT_ERR);
    return FAILURE;
  }
  minValidIdInitialize ();
  timerInitialize (quantum_usecs);
  uthread_create (nullptr);
  totalQuantums = 1;
  return SUCCESS;

}

int uthread_spawn (thread_entry_point entry_point)
{
  block_signals_helper();
  if (entry_point == nullptr || readyQueueIsFull ())
  {
    err_lib_print (SPAWN_ERR);
    unblock_signals_helper();
    return FAILURE;
  }
  int id = uthread_create(entry_point);
  unblock_signals_helper();
  return id;
}

int uthread_terminate (int tid)
{
  block_signals_helper();
  if (tid == 0)
  {
    unblock_signals_helper();
    Clear_database();
    exit (0);
  }

  if (tidCheck (tid, TERMINATE_ERR, 0) == FAILURE)
  { unblock_signals_helper();
    return FAILURE; }
  std::shared_ptr<Thread> thread = threadsVector[tid];


  threadsVector[tid] = nullptr;
  minValidId.push (tid);
  if (thread->isEqual (running_thread))
  {
    running_thread = nullptr;
    unblock_signals_helper();
    jumpToThread(false, false);
  }
  else
  {
    removeFromReady (thread);
  }
  removefromSleeps(tid);

  unblock_signals_helper();
  return SUCCESS;
}

int uthread_block (int tid)
{
  if (tidCheck (tid, BLOCK_ERR, 1) == FAILURE)
  { return FAILURE; }
  if (threadsVector[tid]->getState() == BLOCKED){return SUCCESS; }
  block_signals_helper();

  std::shared_ptr<Thread> &thread = threadsVector[tid];

  if (thread->isEqual (running_thread))
  {
    unblock_signals_helper();
    int ret_val = sigsetjmp(running_thread->env, ANOTHER_THREAD_JUMP);
    if (ret_val == 0)
    {
      jumpToThread(true, false);
    }
  }
  else
  {
    thread->setState(BLOCKED);
    removeFromReady(thread);
    unblock_signals_helper();
  }

  return SUCCESS;
}

int uthread_resume (int tid)
{
  if (tidCheck (tid, RESUME_ERR, 0) == FAILURE)
  {
    return FAILURE; }
  block_signals_helper();
  std::shared_ptr<Thread> &thread = threadsVector[tid];

  if (thread->getState () == BLOCKED)
  {
    if (sleep_map.count(tid) ==0) {
      readyQueue.push_back (thread);
    }
    thread->setState (READY);
  }
  unblock_signals_helper();
  return SUCCESS;
}

int uthread_sleep (int num_quantums)
{

  if (running_thread->getId() ==0){
    err_lib_print (SLEEP_ERR);
    return FAILURE;
  }
  block_signals_helper();
  sleep_map[running_thread->getId()] = num_quantums;
  unblock_signals_helper();
  int ret_val = sigsetjmp(running_thread->env, ANOTHER_THREAD_JUMP);
  if (ret_val == 0)
  {
    running_thread = nullptr;
    jumpToThread(false, true);
  }
  return SUCCESS;
}

int uthread_get_total_quantums()
{
  return totalQuantums;
}

int uthread_get_quantums(int tid){
  block_signals_helper();
  if (tidCheck (tid, QUANTUM_ERR , 0) == FAILURE)
    { return FAILURE; }
  unblock_signals_helper();
  return threadsVector[tid]->getQuantums();
}

