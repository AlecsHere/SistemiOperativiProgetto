#include "ash.h"
#include "ns.h"
#include "pcb.h"

#include "/usr/include/umps3/umps/const.h"
#include "/usr/include/umps3/umps/libumps.h"


/* Variabili globali */
extern int processCount;   //Contatore dei processi iniziati e non ancora terminati
extern int softBlockCount; //Contatore dei processi in stato blocked
extern struct list_head readyQueue;  //Coda dei processi in stato ready
extern pcb_t* currentProcess;  //Puntatore al processo corrente
extern int devSem [(DEVINTNUM*DEVPERINT)+DEVPERINT+1];   //Array di semafori per ogni device + 8 perchè i terminali ne usano 2 a testa + uno per il clock
extern int* devGV [(DEVINTNUM*DEVPERINT)+DEVPERINT];  //Array di puntatori per ogni sub-device, usato per cmdValue

#define clockSem devSem[(DEVINTNUM*DEVPERINT)+DEVPERINT]   //Clocksem è l'ultimo elemento di devSem, semaforo dedicato all'interval timer
extern cpu_t startTOD;   //Misurazione di inizio processo del timer
extern cpu_t stopTOD;  //Misurazione di fine processo del timer

//La funzione main agisce da entry point del programma
int main();
//Gestore di eccezioni che in base al tipo di eccezione richiama il rispettivo handler  
void generalExceptionHandler();