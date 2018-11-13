/*
     File        : blocking_disk.c

     Author      : 
     Modified    : 

     Description : 

*/

/*--------------------------------------------------------------------------*/
/* DEFINES */
/*--------------------------------------------------------------------------*/

    /* -- (none) -- */

/*--------------------------------------------------------------------------*/
/* INCLUDES */
/*--------------------------------------------------------------------------*/

#include "scheduler.H"
#include "assert.H"
#include "utils.H"
#include "console.H"
#include "blocking_disk.H"

/*--------------------------------------------------------------------------*/
/* CONSTRUCTOR */
/*--------------------------------------------------------------------------*/

extern Scheduler * SYSTEM_SCHEDULER;

BlockingDisk::BlockingDisk(DISK_ID _disk_id, unsigned int _size) 
  : SimpleDisk(_disk_id, _size) {
}

/*--------------------------------------------------------------------------*/
/* SIMPLE_DISK FUNCTIONS */
/*--------------------------------------------------------------------------*/

void BlockingDisk::read(unsigned long _block_no, unsigned char * _buf) {
    enter_critical_section();
    SimpleDisk::read(_block_no, _buf);
    exit_critical_section();
}


void BlockingDisk::write(unsigned long _block_no, unsigned char * _buf) {
    enter_critical_section();
    SimpleDisk::write(_block_no, _buf);
    exit_critical_section();
}

void BlockingDisk::enter_critical_section() {
    if (_blocked_queue.size() == 0)
        return;

    if (_disk_status != READY) {
        Console::puts("Thread ");
        Console::puti(Thread::CurrentThread()->ThreadId());
        Console::puts(" blocking the io operation, as device busy\n");
        _blocked_queue.push(Thread::CurrentThread());
        SYSTEM_SCHEDULER->yield();
    } else {
        _disk_status = WAITING_ON_OPERATION;
    }

    Console::puts("Thread ");
    Console::puti(Thread::CurrentThread()->ThreadId());
    Console::puts(" Now doing io operation\n");
}

void BlockingDisk::exit_critical_section() {
    _disk_status = READY;

    Thread * actionable = _blocked_queue.pop();
    if(actionable != NULL) {
        SYSTEM_SCHEDULER->resume(actionable);
        _disk_status = WAITING_ON_OPERATION; // this ensures serializability
    }
}

void BlockingDisk::wait_until_ready() {
    int i = 0;
    while(!is_ready() || i < 2) {
        i++;
        SYSTEM_SCHEDULER->resume(Thread::CurrentThread());
        SYSTEM_SCHEDULER->yield();
    }
}
