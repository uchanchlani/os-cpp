//
// Created by utkarsh on 11/9/18.
//

#include "fifo_queue.H"
#include "utils.H"

FIFOQueue::FIFOQueue() {
    _size = 0;
    _head_thread = NULL;
    _tail_thread = NULL;
}

void FIFOQueue::push(Thread *thread) {
    thread->next = NULL;

    if(_head_thread == NULL) {
        _head_thread = thread;
        _tail_thread = thread;
    } else {
        _tail_thread->next = thread;
        _tail_thread = _tail_thread->next;
    }
    _size++;
}

Thread *FIFOQueue::pop() {
    if(_head_thread == NULL) {
        return NULL;
    }

    Thread * ret = _head_thread;
    _head_thread = _head_thread->next;
    _size--;

    if(_head_thread == NULL) {
        _tail_thread = NULL;
    }

    return ret;
}

unsigned long FIFOQueue::size() {
    return _size;
}
