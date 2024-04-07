#include "thread.h"
#include <signal.h>
#include <memory>

#ifdef __x86_64__
/* code for 64 bit Intel arch */

typedef unsigned long address_t;
#define JB_SP 6
#define JB_PC 7

/* A translation is required when using an address of a variable.
   Use this as a black box in your code. */
address_t translate_address (address_t addr)
{
  address_t ret;
  asm volatile("xor    %%fs:0x30,%0\n"
               "rol    $0x11,%0\n"
  : "=g" (ret)
  : "0" (addr));
  return ret;
}

#else
/* code for 32 bit Intel arch */

typedef unsigned int address_t;
#define JB_SP 4
#define JB_PC 5


/* A translation is required when using an address of a variable.
   Use this as a black box in your code. */
address_t translate_address(address_t addr)
{
    address_t ret;
    asm volatile("xor    %%gs:0x18,%0\n"
                 "rol    $0x9,%0\n"
    : "=g" (ret)
    : "0" (addr));
    return ret;
}
#endif



/** ~~~~~~~~~~~~~~~~~~ Thread Class ~~~~~~~~~~~ **/

Thread::Thread (int id, thread_entry_point entry_point)
{
  this->id = id;
  this->state = READY;
  this->stack = new char[STACK_SIZE];
  sigsetjmp(env, 1);
  if (id != 0)
  {
    this->quantums = 0;
    address_t sp = (address_t) stack + STACK_SIZE - sizeof (address_t);
    address_t pc = (address_t) entry_point;
    (env->__jmpbuf)[JB_SP] = translate_address (sp);
    (env->__jmpbuf)[JB_PC] = translate_address (pc);
  }
  else
  { this->quantums = 1; }
  sigemptyset (&env->__saved_mask);
}

/** ~~~~~~~~~~~~~~~~~~ Methods ~~~~~~~~~~~ **/

bool Thread::isEqual (std::shared_ptr<Thread> other)
{
  return other->id == this->id;
}


void Thread::incrementQuantum ()
{ this->quantums++; }

/** ~~~~~~~~~~~~~~~~~~ Getters ~~~~~~~~~~~ **/

int Thread::getId ()
{
  return this->id;
}

void Thread::setState (ThreadState st)
{
  this->state = st;
}

ThreadState Thread::getState ()
{
  return this->state;
}

int Thread::getQuantums ()
{ return this->quantums; }

Thread::~Thread()
{
    delete[] stack;
}




