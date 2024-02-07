#include "../inc/pandos_types.h"


//Handler per interrupts generale
void interruptHandler();

//Handler per interrupts sul timer globale del PLT
void pltInterrupts(cpu_t stopTOD);

//Handler per interrupts sull'interval timer
void intervaltimerInterrupts();

//Handler per interrupts sui dispositivi
void deviceInterrupts(int lineNum);