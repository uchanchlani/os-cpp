//
// Created by utkarsh on 10/18/18.
//

#ifndef FIFO_SCHEDULER_H
#define FIFO_SCHEDULER_H

#include "scheduler.H"
#include "thread.H"
#include "fifo_queue.H"

class FIFOScheduler : public Scheduler {
private:
    static FIFOQueue _ready_queue;

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
