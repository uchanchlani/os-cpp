//
// Created by utkarsh on 11/9/18.
//

#ifndef FIFO_QUEUE_H
#define FIFO_QUEUE_H

#include <thread.H>

class FIFOQueue {
private:
    Thread *_head_thread;
    Thread *_tail_thread;
    unsigned long _size;

public:
    FIFOQueue();

    void push(Thread * thread);
    Thread * pop();
    unsigned long size();
};


#endif //FIFO_QUEUE_H
