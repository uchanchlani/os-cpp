//
// Created by utkarsh on 10/20/18.
//

#include "interrupts.H"
#include "rr_scheduler.H"

void RRScheduler::end_of_interrupt() {
    if(is_interrupt_occured()) {
        InterruptHandler::end_of_interrupt(0);
        handled_interrupt();
    }
}

RRScheduler::RRScheduler() : FIFOScheduler() {
    set_handle_timer_interrupt();
}

void RRScheduler::yield() {
    context_switch();
    end_of_interrupt();
}

void RRScheduler::mark_current_thread_started() {
    Scheduler::mark_current_thread_started();
    end_of_interrupt();
}

