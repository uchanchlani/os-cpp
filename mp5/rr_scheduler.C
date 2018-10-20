//
// Created by utkarsh on 10/20/18.
//

#include "interrupts.H"
#include "rr_scheduler.H"

RRScheduler::RRScheduler() : FIFOScheduler() {
    handles_timer_interrupt = true;
}

void RRScheduler::yield() {
    FIFOScheduler::yield();
    if(is_interrupt_occured()) {
        InterruptHandler::end_of_interrupt(0);
        handled_interrupt();
    }
}
