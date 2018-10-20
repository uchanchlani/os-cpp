//
// Created by utkarsh on 10/20/18.
//

#ifndef RR_SCHEDULER_H
#define RR_SCHEDULER_H


#include "fifo_scheduler.H"

class RRScheduler : public FIFOScheduler {
private:
    void end_of_interrupt();

public:
    RRScheduler();

    virtual void yield();

    virtual void mark_current_thread_started();
};


#endif //RR_SCHEDULER_H
