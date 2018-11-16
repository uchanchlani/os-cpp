/*
    File: kernel.C

    Author: R. Bettati
            Department of Computer Science
            Texas A&M University
    Date  : 17/04/03


    This file has the main entry point to the operating system.

    MAIN FILE FOR MACHINE PROBLEM "KERNEL-LEVEL THREAD MANAGEMENT"

    NOTE: REMEMBER THAT AT THE VERY BEGINNING WE DON'T HAVE A MEMORY MANAGER. 
          OBJECT THEREFORE HAVE TO BE ALLOCATED ON THE STACK. 
          THIS LEADS TO SOME RATHER CONVOLUTED CODE, WHICH WOULD BE MUCH 
          SIMPLER OTHERWISE.
*/

/*--------------------------------------------------------------------------*/
/* DEFINES */
/*--------------------------------------------------------------------------*/

#define GB * (0x1 << 30)
#define MB * (0x1 << 20)
#define KB * (0x1 << 10)
#define KERNEL_POOL_START_FRAME ((2 MB) / Machine::PAGE_SIZE)
#define KERNEL_POOL_SIZE ((2 MB) / Machine::PAGE_SIZE)
#define PROCESS_POOL_START_FRAME ((4 MB) / Machine::PAGE_SIZE)
#define PROCESS_POOL_SIZE ((28 MB) / Machine::PAGE_SIZE)
/* definition of the kernel and process memory pools */

#define MEM_HOLE_START_FRAME ((15 MB) / Machine::PAGE_SIZE)
#define MEM_HOLE_SIZE ((1 MB) / Machine::PAGE_SIZE)
/* we have a 1 MB hole in physical memory starting at address 15 MB */

/* -- COMMENT/UNCOMMENT THE FOLLOWING LINE TO EXCLUDE/INCLUDE SCHEDULER CODE */

#define _USES_SCHEDULER_
/* This macro is defined when we want to force the code below to use
   a scheduler.
   Otherwise, no scheduler is used, and the threads pass control to each
   other in a co-routine fashion.
*/


/* -- UNCOMMENT THE FOLLOWING LINE TO MAKE THREADS TERMINATING */

#define _TERMINATING_FUNCTIONS_
/* This macro is defined when we want the thread functions to return, and so
   terminate their thread.
   Otherwise, the thread functions don't return, and the threads run forever.
*/

/*--------------------------------------------------------------------------*/
/* INCLUDES */
/*--------------------------------------------------------------------------*/

#include "cont_frame_pool.H"
#include "machine.H"         /* LOW-LEVEL STUFF   */
#include "console.H"
#include "gdt.H"
#include "idt.H"             /* EXCEPTION MGMT.   */
#include "irq.H"
#include "exceptions.H"    
#include "interrupts.H"

#include "simple_timer.H"    /* TIMER MANAGEMENT  */

#include "frame_pool.H"      /* MEMORY MANAGEMENT */
#include "mem_pool.H"

#include "thread.H"          /* THREAD MANAGEMENT */

#ifdef _USES_SCHEDULER_
#include "scheduler.H"
#include "fifo_scheduler.H"
#include "rr_timer.H"
#include "rr_scheduler.H"
#include "vm_pool.H"
#include "page_table.H"

#endif

/*--------------------------------------------------------------------------*/
/* MEMORY MANAGEMENT */
/*--------------------------------------------------------------------------*/


typedef unsigned int size_t;

//replace the operator "new"
void * operator new (size_t size, ContFramePool * framePool = NULL) {
    if(framePool == NULL) {
        unsigned long a = PageTable::get_current_page_table_heap()->allocate((unsigned long)size);
        return (void *)a;
    }
    unsigned long frame_no = framePool->get_frames((size >> 12) + (size % Machine::PAGE_SIZE == 0 ? 0 : 1));
    if(frame_no == 0) {
        Console::puts("Curr frame pool out of frames Not a good sign\n");
        assert(false);
    }
    return (void *)(frame_no * Machine::PAGE_SIZE);
}

//replace the operator "new[]"
//not using it as of now but the idea is the same
void * operator new[] (size_t size) {
    unsigned long a = PageTable::get_current_page_table_heap()->allocate((unsigned long)size);
    return (void *)a;
}

bool isKernelMemory(unsigned long addr) {
    return (addr >= 0 MB && addr < 4 MB);
}

//replace the operator "delete"
void operator delete (void * p) {
    if (isKernelMemory((unsigned long)p)) {
        ContFramePool::release_frames(((unsigned long) p) >> PageTable::FRAME_OFFSET);
    } else {
        PageTable::get_current_page_table_heap()->release((unsigned long) p);
    }
}

//replace the operator "delete[]"
//let's not care about this as of now
void operator delete[] (void * p) {
    PageTable::get_current_page_table_heap()->release((unsigned long)p);
}

/*--------------------------------------------------------------------------*/
/* SCHEDULRE and AUXILIARY HAND-OFF FUNCTION FROM CURRENT THREAD TO NEXT */
/*--------------------------------------------------------------------------*/

#ifdef _USES_SCHEDULER_

/* -- A POINTER TO THE SYSTEM SCHEDULER */
Scheduler * SYSTEM_SCHEDULER;

#endif

void pass_on_CPU(Thread * _to_thread) {} // do nothing. we handle this by timer now
void pass_on_CPU_old(Thread * _to_thread) {
  // Hand over CPU from current thread to _to_thread.
  
#ifndef _USES_SCHEDULER_

        /* We don't use a scheduler. Explicitely pass control to the next
           thread in a co-routine fashion. */
	Thread::dispatch_to(_to_thread);

#else

        /* We use a scheduler. Instead of dispatching to the next thread,
           we pre-empt the current thread by putting it onto the ready
           queue and yielding the CPU. */

        SYSTEM_SCHEDULER->resume(Thread::CurrentThread());
        SYSTEM_SCHEDULER->yield();
#endif
}

/*--------------------------------------------------------------------------*/
/* A FEW THREADS (pointer to TCB's and thread functions) */
/*--------------------------------------------------------------------------*/

Thread * thread1;
Thread * thread2;
Thread * thread3;
Thread * thread4;

/* -- THE 4 FUNCTIONS fun1 - fun4 ARE LARGELY IDENTICAL. */

void fun1() {
    Console::puts("Thread: "); Console::puti(Thread::CurrentThread()->ThreadId()); Console::puts("\n");
    Console::puts("FUN 1 INVOKED!\n");

#ifdef _TERMINATING_FUNCTIONS_
    for(int j = 0; j < 10; j++) 
#else
    for(int j = 0;; j++) 
#endif
    {	
        Console::puts("FUN 1 IN BURST["); Console::puti(j); Console::puts("]\n");
        for (int i = 0; i < 10; i++) {
            Console::puts("FUN 1: TICK ["); Console::puti(i); Console::puts("]\n");
        }
        pass_on_CPU(thread2);
    }
}


void fun2() {
    Console::puts("Thread: "); Console::puti(Thread::CurrentThread()->ThreadId()); Console::puts("\n");
    Console::puts("FUN 2 INVOKED!\n");

#ifdef _TERMINATING_FUNCTIONS_
    for(int j = 0; j < 10; j++) 
#else
    for(int j = 0;; j++) 
#endif  
    {		
        Console::puts("FUN 2 IN BURST["); Console::puti(j); Console::puts("]\n");
        for (int i = 0; i < 10; i++) {
            Console::puts("FUN 2: TICK ["); Console::puti(i); Console::puts("]\n");
        }
        pass_on_CPU(thread3);
    }
}

void fun3() {
    Console::puts("Thread: "); Console::puti(Thread::CurrentThread()->ThreadId()); Console::puts("\n");
    Console::puts("FUN 3 INVOKED!\n");

    for(int j = 0;; j++) {
        Console::puts("FUN 3 IN BURST["); Console::puti(j); Console::puts("]\n");
        for (int i = 0; i < 10; i++) {
	    Console::puts("FUN 3: TICK ["); Console::puti(i); Console::puts("]\n");
        }
        pass_on_CPU(thread4);
    }
}

void fun4() {
    Console::puts("Thread: "); Console::puti(Thread::CurrentThread()->ThreadId()); Console::puts("\n");
    Console::puts("FUN 4 INVOKED!\n");

    for(int j = 0;; j++) {
        Console::puts("FUN 4 IN BURST["); Console::puti(j); Console::puts("]\n");
        for (int i = 0; i < 10; i++) {
	    Console::puts("FUN 4: TICK ["); Console::puti(i); Console::puts("]\n");
        }
        pass_on_CPU(thread1);
    }
}

/*--------------------------------------------------------------------------*/
/* MAIN ENTRY INTO THE OS */
/*--------------------------------------------------------------------------*/

int main() {

    GDT::init();
    Console::init();
    IDT::init();
    ExceptionHandler::init_dispatcher();
    IRQ::init();
    InterruptHandler::init_dispatcher();

    /* -- EXAMPLE OF AN EXCEPTION HANDLER -- */

    class DBZ_Handler : public ExceptionHandler {
      public:
      virtual void handle_exception(REGS * _regs) {
        Console::puts("DIVISION BY ZERO!\n");
        for(;;);
      }
    } dbz_handler;

    ExceptionHandler::register_handler(0, &dbz_handler);


    /* -- INITIALIZE MEMORY -- */
    /*    NOTE: We don't have paging enabled in this MP. */
    /*    NOTE2: This is not an exercise in memory management. The implementation
                of the memory management is accordingly *very* primitive! */

    ContFramePool kernel_mem_pool(KERNEL_POOL_START_FRAME,
                                  KERNEL_POOL_SIZE,
                                  0,
                                  0);

    unsigned long n_info_frames =
    ContFramePool::needed_info_frames(PROCESS_POOL_SIZE);

    unsigned long process_mem_pool_info_frame =
            kernel_mem_pool.get_frames(n_info_frames);

    ContFramePool process_mem_pool(PROCESS_POOL_START_FRAME,
                                   PROCESS_POOL_SIZE,
                                   process_mem_pool_info_frame,
                                   n_info_frames);

    /* Take care of the hole in the memory. */
    process_mem_pool.mark_inaccessible(MEM_HOLE_START_FRAME, MEM_HOLE_SIZE);

    /* -- INITIALIZE MEMORY (PAGING) -- */

    /* ---- INSTALL PAGE FAULT HANDLER -- */

    class PageFault_Handler : public ExceptionHandler {
        /* We derive the page fault handler from ExceptionHandler
       and overload the method handle_exception. */
    public:
        virtual void handle_exception(REGS * _regs) {
            PageTable::handle_fault(_regs);
        }
    } pagefault_handler;

    /* ---- Register the page fault handler for exception no. 14
            with the exception dispatcher. */
    ExceptionHandler::register_handler(14, &pagefault_handler);

    /* ---- INITIALIZE THE PAGE TABLE -- */

    Machine::enable_interrupts();

    PageTable::init_paging(&kernel_mem_pool,
                           &process_mem_pool,
                           4 MB);

    PageTable kernel_pt;

    kernel_pt.load();

    PageTable::enable_paging();

    VMPool heap_pool(1 GB, 256 MB, &process_mem_pool, &kernel_pt, true);
    /* -- MEMORY ALLOCATOR IS INITIALIZED. WE CAN USE new/delete! --*/

    /* -- INITIALIZE THE TIMER (we use a very simple timer).-- */

    /* Question: Why do we want a timer? We have it to make sure that 
                 we enable interrupts correctly. If we forget to do it,
                 the timer "dies". */

    RRTimer timer(20); /* timer ticks every 10ms. */
    InterruptHandler::register_handler(0, &timer);
    /* The Timer is implemented as an interrupt handler. */

#ifdef _USES_SCHEDULER_

    /* -- SCHEDULER -- IF YOU HAVE ONE -- */

    SYSTEM_SCHEDULER = new(&kernel_mem_pool) RRScheduler(); // Scheduler goes on the stack

#endif

    /* NOTE: The timer chip starts periodically firing as
             soon as we enable interrupts.
             It is important to install a timer handler, as we
             would get a lot of uncaptured interrupts otherwise. */ 

    /* -- ENABLE INTERRUPTS -- */

    /* -- MOST OF WHAT WE NEED IS SETUP. THE KERNEL CAN START. */

    Console::puts("Hello World!\n");

    /* -- LET'S CREATE SOME THREADS... */

    Thread::init_threading(&kernel_mem_pool, &process_mem_pool);

    Console::puts("CREATING THREAD 1...\n");
    thread1 = new(&kernel_mem_pool) Thread(fun1, 1024);
    Console::puts("DONE\n");

    Console::puts("CREATING THREAD 2...");
    thread2 = new(&kernel_mem_pool) Thread(fun2, 1024);
    Console::puts("DONE\n");

    Console::puts("CREATING THREAD 3...");
    thread3 = new(&kernel_mem_pool) Thread(fun3, 1024);
    Console::puts("DONE\n");

    Console::puts("CREATING THREAD 4...");
    thread4 = new(&kernel_mem_pool) Thread(fun4, 1024);
    Console::puts("DONE\n");

#ifdef _USES_SCHEDULER_

    /* WE ADD thread2 - thread4 TO THE READY QUEUE OF THE SCHEDULER. */

    SYSTEM_SCHEDULER->add(thread2);
    SYSTEM_SCHEDULER->add(thread3);
    SYSTEM_SCHEDULER->add(thread4);

#endif

    /* -- KICK-OFF THREAD1 ... */

    Console::puts("STARTING THREAD 1 ...\n");
    Thread::dispatch_to(thread1);

    /* -- AND ALL THE REST SHOULD FOLLOW ... */

    assert(false); /* WE SHOULD NEVER REACH THIS POINT. */

    /* -- WE DO THE FOLLOWING TO KEEP THE COMPILER HAPPY. */
    return 1;
}
