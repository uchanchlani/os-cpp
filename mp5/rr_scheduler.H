//
// Created by utkarsh on 10/20/18.
//

#ifndef RR_SCHEDULER_H
#define RR_SCHEDULER_H


#include "fifo_scheduler.H"

class RRScheduler : public FIFOScheduler {

public:
    RRScheduler();

    virtual void yield();
};


#endif //RR_SCHEDULER_H
