# Scheduler Class

The `Scheduler` class manages the scheduling and execution of threads in a multi-threaded environment. This README provides an overview of the key methods and functionalities of the `Scheduler` class.

## Overview

The `Scheduler` class is responsible for:
- Maintaining a list of threads and their states.
- Handling thread scheduling and execution.
- Supporting operations such as thread creation, termination, blocking, and resuming.
- Implementing scheduling algorithms such as Round-Robin (RR).

## Key Methods

### `update_deque()`
- Updates the deque of ready threads by selecting the next thread to run.

### `jump_to_threads_helper(bool sleep, bool block, bool terminate)`
- Helper method for jumping to threads based on their state and specified actions.
- Handles cases such as thread sleep, block, and termination.

### `find_next_id_available()`
- Finds the next available thread ID for thread creation.

### `terminate_thread(int tid)`
- Terminates the thread with the specified thread ID.
- Removes the thread from the scheduler and adjusts its state accordingly.

### `block_ready_thread(int tid)`
- Blocks the thread with the specified thread ID.
- Moves the thread from the ready state to the blocked state.

### `add_thread(sp_thread &thread)`
- Adds a new thread to the scheduler and sets its initial state to ready.

### `thread_found(int tid)`
- Finds a thread by its thread ID and returns a shared pointer to it.

### `resume_thread(int tid)`
- Resumes the execution of a blocked thread with the specified thread ID.
- Moves the thread from the blocked state to the ready state.

### `sleeping_threads_update()`
- Updates the state of sleeping threads.
- Decrements the sleep time of sleeping threads and moves them to the ready state when their sleep time expires.

### `remove_from_sleep(int tid)`
- Removes a thread from the sleeping state.

### `Clear()`
- Clears all data structures and resets the scheduler to its initial state.

## Round-Robin (RR) Implementation

The Round-Robin scheduling algorithm is implemented in the `Scheduler` class.
- Each thread is given a fixed time quantum to execute before being preempted.
- Threads are scheduled in a circular manner, allowing each thread to execute for a specified time slice.
- If a thread's time quantum expires, it is moved to the end of the ready queue, and the next thread is scheduled.

## Usage

To use the `Scheduler` class, create an instance of the class and use its methods to manage threads and scheduling.

Example:
```cpp
Scheduler scheduler;
// Add threads, perform scheduling operations, etc.
