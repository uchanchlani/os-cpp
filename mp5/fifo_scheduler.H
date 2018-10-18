//
// Created by utkarsh on 10/18/18.
//

#ifndef FIFO_SCHEDULER_H
#define FIFO_SCHEDULER_H

#include "scheduler.H"
#include "thread.H"

class FIFOScheduler : public Scheduler {
private:
    static Thread *_head_thread;
    static Thread *_tail_thread;

    Thread * pop();

public:
    FIFOScheduler();

    void yield();

    void resume(Thread *_thread);

    void add(Thread *_thread);

    void terminate(Thread *_thread);
};

#endif //FIFO_SCHEDULER_H
