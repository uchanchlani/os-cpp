//
// Created by utkarsh on 10/18/18.
//

#include "console.H"

#include "fifo_scheduler.H"
#include "utils.H"


Thread * FIFOScheduler::_head_thread = NULL;
Thread * FIFOScheduler::_tail_thread = NULL;

Thread *FIFOScheduler::pop() {
    if(_head_thread == NULL) return NULL;
    Thread * popped = _head_thread;
    _head_thread = _head_thread->next;
    if(_head_thread == NULL) {
        _tail_thread = NULL;
    }
    return popped;
}

FIFOScheduler::FIFOScheduler() : Scheduler() {

}

void FIFOScheduler::yield() {
    if(!Machine::interrupts_enabled()) {
        Machine::enable_interrupts(); // trick if we need to run fifo even after the introduction of rr scheduler
    }
    context_switch();
}

void FIFOScheduler::context_switch() {
    if (Thread::CurrentThread() == NULL) return;
    Thread *thread = pop();
    while(thread != NULL && thread->is_terminated()) {
        terminate(thread);
        thread = pop();
    }
    if(thread != NULL) { // What if I only have one thread in the system and
                         // I cannot dispatch to any other thread. I will choose to do no dispatching at all.
                         // Interesting things will happen if the only thread in the system terminates
                         // In that case the instruction pointer will point to a garbage value (start of the stack of the terminated thread)
                         // And will break the operating system completely.
                         // I will choose to break the os as of now. Ideally the os should make sure it is running at least 1 os thread which does nothing
                         // and simply yields back the cpu, in cases like these. But this causes one extra context switch for a thread which basically does nothing and degrades the performance
                         //
                         // The point I'm trying to make is this is a complicated thing which requires further understanding for me and I cannot do as of now, so coming back
                         // I choose to break the system as of now.
        Thread::CurrentThread()
                ->dispatch_to(thread);
    }
}

void FIFOScheduler::resume(Thread *_thread) {
    this->add(_thread);
}

void FIFOScheduler::add(Thread *_thread) {
    if (_head_thread == NULL) {
        _head_thread = _thread;
        _tail_thread = _head_thread;
    } else {
        _tail_thread->next = _thread;
        _tail_thread = _tail_thread->next;
    }
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
        Thread::clean_up(_thread);
        Console::puts("Thread: ");
        Console::puti(thread_id);
        Console::puts(" finally deleted\n");
    }
}
