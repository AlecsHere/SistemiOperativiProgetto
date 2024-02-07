#include "../inc/pandos_types.h"


//Si occupa di implementare un'algoritmo di scheduling FIFO preemptive
void scheduler();
//Sostituisce il processo corrente con il processo dato in input, e ne riprende l'esecuzione effettando un LDST
void contextSwitch();
//Effettua un context switch dopo che imposta il timer del PLT con la timeslice data in input e misura il momento di inizio del processo
void prepareSwitch(int timeSlice);