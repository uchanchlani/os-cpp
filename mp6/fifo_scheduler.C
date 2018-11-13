//
// Created by utkarsh on 10/18/18.
//

#include "console.H"

#include "fifo_scheduler.H"
#include "utils.H"

typedef unsigned int size_t;
extern void * operator new (size_t size);
extern void * operator new[] (size_t size);
extern void operator delete (void * p);
extern void operator delete[] (void * p);

FIFOScheduler::FIFOScheduler() : Scheduler() {}

void FIFOScheduler::yield() {
    context_switch();
}

void FIFOScheduler::context_switch() {
    if (Thread::CurrentThread() == NULL) return;
    Thread *thread = _ready_queue.pop();
    while(thread != NULL && thread->is_terminated()) {
        terminate(thread);
        thread = _ready_queue.pop();
    }
    if(thread != NULL) {
        Thread::CurrentThread()
                ->dispatch_to(thread);
    }
}

void FIFOScheduler::resume(Thread *_thread) {
    this->add(_thread);
}

void FIFOScheduler::add(Thread *_thread) {
    _ready_queue.push(_thread);
}

void FIFOScheduler::terminate(Thread *_thread) {
    if(Thread::CurrentThread()->equals(_thread)) {
        Console::puts("Marked Thread: ");
        Console::puti(_thread->ThreadId());
        Console::puts(" for deletion\n");
        _thread->mark_for_termination();
        add(_thread);
    } else {
        int thread_id = _thread->ThreadId();
        _thread->clean_up();
        Console::puts("Thread: ");
        Console::puti(thread_id);
        Console::puts(" finally deleted\n");
    }
}
