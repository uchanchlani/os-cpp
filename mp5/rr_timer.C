//
// Created by utkarsh on 10/20/18.
//

#include "rr_timer.H"
#include "scheduler.H"
#include "thread.H"

extern Scheduler * SYSTEM_SCHEDULER;

void RRTimer::handle_interrupt(REGS *_r) {
    SimpleTimer::handle_interrupt(_r);
    SYSTEM_SCHEDULER->request_handle_interrupt();
    SYSTEM_SCHEDULER->resume(Thread::CurrentThread());
    SYSTEM_SCHEDULER->yield();
}
