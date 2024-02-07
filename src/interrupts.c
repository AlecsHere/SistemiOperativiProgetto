#include "../inc/interrupts.h"
#include "../inc/exceptions.h"
#include "../inc/scheduler.h"
#include "../inc/initial.h"


void interruptHandler(){
  STCK(stopTOD);  //Fermiamo il TOD
  cpu_t timeLeft = getTIMER();  //Salviamo la timeslice da reinserire una volta ripreso il controllo
  
  unsigned int interruptCause = ((state_t*)BIOSDATAPAGE)->cause;  //Codice di causa interrupt

  if((interruptCause & LOCALTIMERINT) != 0){
    pltInterrupts(stopTOD);
  }
  else if((interruptCause & TIMERINTERRUPT) != 0){
    intervaltimerInterrupts();
  }
  else if((interruptCause & DISKINTERRUPT) != 0){
    deviceInterrupts(DISKINT);
  }
  else if((interruptCause & FLASHINTERRUPT) != 0){
    deviceInterrupts(FLASHINT);
  }
  else if((interruptCause & PRINTINTERRUPT) != 0){
    deviceInterrupts(PRNTINT);
  }
  else if((interruptCause & TERMINTERRUPT) != 0){
    deviceInterrupts(TERMINT);
  }

  prepareSwitch(timeLeft);
}


void pltInterrupts(cpu_t stopTOD){
  setTIMER(TIMESLICE * (*(cpu_t*)TIMESCALEADDR));
  transferState(((state_t*)BIOSDATAPAGE), &(currentProcess->p_s));
  currentProcess->p_time = currentProcess->p_time + (stopTOD-startTOD); //Aggiorniamo manualmente poiché non consideriamo il tempo dell'interrupt
  
  insertProcQ(&readyQueue, currentProcess);

  scheduler();
}


void intervaltimerInterrupts(){
  while(clockSem == 0){ // Sblocchiamo tutti i processi bloccati sul clockSem
    V(&clockSem);
  }

  clockSem = 0;
  LDIT(PSECOND);
}


void deviceInterrupts(int lineNum){
  int deviceNum = 0;
  int line = lineNum-DISKINT; //Non consideriamo le prime 3 linee 'non-device'
  int deviceStatus = 0; //Status del device

  volatile devregarea_t* busreg = (devregarea_t*)RAMBASEADDR; //Volatile evita possibili errori data la natura hardware del bus
  unsigned int interruptBitMap = busreg->interrupt_dev[line];

  //Troviamo il device number sulla linea
  if((interruptBitMap & DEV0ON) != 0) deviceNum = 0;
  else if((interruptBitMap & DEV1ON) != 0) deviceNum = 1;
  else if((interruptBitMap & DEV2ON) != 0) deviceNum = 2;
  else if((interruptBitMap & DEV3ON) != 0) deviceNum = 3;
  else if((interruptBitMap & DEV4ON) != 0) deviceNum = 4;
  else if((interruptBitMap & DEV5ON) != 0) deviceNum = 5;
  else if((interruptBitMap & DEV6ON) != 0) deviceNum = 6;
  else if((interruptBitMap & DEV7ON) != 0) deviceNum = 7;

  //L'indice da usare nell'array devSem
  int semDevNum = ((line * DEVPERINT) + deviceNum);

  if(lineNum != 7){ //Se stiamo operando su un non-terminale
    deviceStatus = busreg->devreg[line][deviceNum].dtp.status;
    busreg->devreg[line][deviceNum].dtp.command = ACK; //Diamo comando di ACK
  }
  else{           //Se stiamo operando su un terminale
    unsigned int deviceTransmStatus = busreg->devreg[line][deviceNum].term.transm_status;  //Status di trasmissione

    if(deviceTransmStatus > 1){ //Caso trasmissione, sopra a 1 vuol dire che ha eseguito
      semDevNum+=DEVPERINT; //i transmit sono locati sulla 'linea 8'
      deviceStatus = deviceTransmStatus;
      busreg->devreg[line][deviceNum].term.transm_command = ACK; //Diamo comando di ACK
    } 
    else{                       //Caso ricezione
      deviceStatus = busreg->devreg[line][deviceNum].term.recv_status;  //Status di ricezione
      busreg->devreg[line][deviceNum].term.recv_command = ACK; //Diamo comando di ACK
    }

  }
  V(&(devSem[semDevNum]));  //Operazione V sul semaforo devsem, sbloccante

  *devGV[semDevNum] = deviceStatus;  //Status del device salvato globalmente, per inserirlo in cmdValue
}