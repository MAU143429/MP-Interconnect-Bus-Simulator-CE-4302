#include "../include/GlobalClock.h"

int GlobalClock::tick = 0;

void GlobalClock::advance() {
    ++tick;
}

void GlobalClock::advance(int ticks) {
    tick += ticks;
}

int GlobalClock::now() {
    return tick;
}
