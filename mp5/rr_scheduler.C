//
// Created by utkarsh on 10/20/18.
//

#include "interrupts.H"
#include "rr_scheduler.H"

RRScheduler::RRScheduler() : FIFOScheduler() {
    set_handle_timer_interrupt();
}

void RRScheduler::yield() {
    FIFOScheduler::yield();
    if(is_interrupt_occured()) {
        InterruptHandler::end_of_interrupt(0);
        handled_interrupt();
    }
}
