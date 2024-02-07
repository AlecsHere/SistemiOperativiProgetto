#include "../inc/scheduler.h"
#include "../inc/initial.h"


void scheduler(){
  pcb_t* p = removeProcQ(&readyQueue);  //Prendiamo il primo processo pronto e lo eseguiamo
  
  if (p != NULL){
    STCK(startTOD);
    setTIMER(TIMESLICE);  //Carico di 'timeslice' ms nel PLT
    currentProcess=p;
    LDST(&(currentProcess->p_s));
  }
  else{ //ReadyQueue vuota
    if (processCount == 0)  // Se non ci sono processi, spegiamo la macchina. Fine programma.
      HALT();
    else{
      if (softBlockCount > 0){  //Se tutti i processi sono bloccati, aspetta.
        currentProcess = NULL;

        setTIMER((unsigned int)0xFFFFFFFF); //PLT in 'pausa' con tempo indefinito
        unsigned int status = (IECON | IMON); //Inserisco maschera la quale attiva gli interrupts
        setSTATUS(status);

        while(softBlockCount > 0 && emptyProcQ(&readyQueue)){
          WAIT();
        }
      }
      else{
        PANIC(); // Con processcount>0 e softBlockCount=0, siamo in stato di deadlock.
      }
    }
    
  }
}

void prepareSwitch(int timeSlice){
  STCK(startTOD); //Misuriamo l'inizio del timer TOD
  setTIMER(timeSlice);  //Carico di 'timeslice' ms nel PLT
  contextSwitch();
}
void contextSwitch(){
  if(currentProcess!=NULL){
    LDST(((state_t*)BIOSDATAPAGE)); // Carichiamo lo stato del processore per farlo ripartire
  }
  else{
    scheduler();
  }
}