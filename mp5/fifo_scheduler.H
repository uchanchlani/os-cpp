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

    virtual void yield();

    virtual void resume(Thread *_thread);

    virtual void add(Thread *_thread);

    virtual void terminate(Thread *_thread);

protected:
    void context_switch();
};

#endif //FIFO_SCHEDULER_H
