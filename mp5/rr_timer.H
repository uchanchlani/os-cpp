//
// Created by utkarsh on 10/20/18.
//

#ifndef RR_TIMER_H
#define RR_TIMER_H


#include "simple_timer.H"

class RRTimer : public SimpleTimer {
public:
    RRTimer(int _hz) : SimpleTimer(_hz) {} // fixed quantum of 50msec as of now

    void handle_interrupt(REGS *_r);
};

#endif //RR_TIMER_H
