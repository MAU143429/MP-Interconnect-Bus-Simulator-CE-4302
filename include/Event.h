#ifndef EVENT_H
#define EVENT_H

#include "sms.h"

struct Event {
    SMS message;
    int ready_tick;

    Event(const SMS& msg, int delay_ticks, int current_tick)
        : message(msg), ready_tick(current_tick + delay_ticks) {}
};

#endif
