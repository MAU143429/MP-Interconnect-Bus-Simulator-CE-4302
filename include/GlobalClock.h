#ifndef GLOBAL_CLOCK_H
#define GLOBAL_CLOCK_H

class GlobalClock {
public:
    static void advance();               // Avanza 1 tick
    static void advance(int ticks);      // Avanza cantidad personalizada
    static int now();                    // Retorna el tick actual

private:
    static int tick;                     // Contador global de ticks
};

#endif // GLOBAL_CLOCK_H
