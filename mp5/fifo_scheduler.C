//
// Created by utkarsh on 10/18/18.
//

#include "fifo_scheduler.H"
#include "utils.H"

Thread * FIFOScheduler::_head_thread = NULL;

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
    Thread::CurrentThread()
                ->dispatch_to(pop());
}

void FIFOScheduler::resume(Thread *_thread) {
    this->add(_thread);
}

void FIFOScheduler::add(Thread *_thread) {
    if (_head_thread == NULL) {
        _head_thread = _thread;
        _tail_thread = _head_thread;
    }
    _tail_thread->next = _thread;
    _tail_thread = _tail_thread->next;
}

void FIFOScheduler::terminate(Thread *_thread) {
//    if (_head_thread->equals(_thread)) {
//        _head_thread = _head_thread->next;
//    }
    Scheduler::terminate(_thread);
}
