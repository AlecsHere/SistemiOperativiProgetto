#include "../inc/exceptions.h"
#include "../inc/scheduler.h"
#include "../inc/initial.h"


int blockingFlag;

void syscallHandler(){
  state_t* systemState = ((state_t*)BIOSDATAPAGE);

  if((systemState->status & USERPON) != 0){ //Se un servizio privilegiato è chiamato con user mode
    systemState->cause = systemState->cause & (~GETEXECCODE); //Cleaning del registro cause
    systemState->cause = systemState->cause | (PRIVINSTR << CAUSESHIFT); //RI, registered instruction
    trapHandler(); //Invoca trap
  }

  int currentSyscall = systemState->reg_a0; //a0, la syscall da effettuare
  blockingFlag = false;

  switch(currentSyscall){
    case CREATEPROCESS: {
      Create_Process();
      break;
    }
    
    case TERMPROCESS: {
      Terminate_Process();
      break;
    }
    
    case PASSEREN: {
      Passeren();
      break;
    }
    
    case VERHOGEN: {
      Verhogen();
      break;
    }
    
    case DOIO: {
      DO_IO();
      break;
    }
    
    case GETTIME: {
      Get_CPU_Time();
      break;
    }
    
    case CLOCKWAIT: {
      Wait_For_Clock();
      break;
    }
    
    case GETSUPPORTPTR: {
      Get_Support_Data();
      break;
    }
    
    case GETPROCESSID: {
      Get_Process_Id();
      break;
    }
    
    case GETCHILDREN: {
      Get_Children();
      break;
    }
    
    default: {  // SYSCALLs oltre 10
      passUpOrDie(GENERALEXCEPT);
      break;
    }
    
  }

  systemState->pc_epc = systemState->pc_epc + WORDLEN;  //Per evitare chiamate syscall infinite

  if(!blockingFlag){
    contextSwitch();
  }
  else{
    transferState(systemState, &(currentProcess->p_s));
    updateProcTime();
    scheduler();
  }
}

void TLBHandler(){  // tlb exceptions
  passUpOrDie(PGFAULTEXCEPT);
}
void trapHandler(){ // program trap exceptions
  passUpOrDie(GENERALEXCEPT);
}


void passUpOrDie(int tipo){
  if(currentProcess->p_supportStruct == NULL){
    TerminateRecursively(currentProcess);
    outChild(currentProcess);
    scheduler();
  }
  else{ //Se abbiamo una support struct
    transferState(((state_t*)BIOSDATAPAGE), &(currentProcess->p_supportStruct->sup_exceptState[tipo]));
    LDCXT(currentProcess->p_supportStruct->sup_exceptContext[tipo].stackPtr, 
          currentProcess->p_supportStruct->sup_exceptContext[tipo].status, 
          currentProcess->p_supportStruct->sup_exceptContext[tipo].pc);
  }
}

void transferState(state_t* a, state_t* b){
  b->entry_hi = a->entry_hi;
  b->cause = a->cause;
  b->status = a->status;
  b->pc_epc = a->pc_epc;
  for (int i = 0; i < STATE_GPR_LEN; i++)
    b->gpr[i] = a->gpr[i];  //Copia di tutti i reg
  b->hi = a->hi;
  b->lo = a->lo;
}


void blockCurrentProc(int* blockedSem){
  transferState(((state_t*)BIOSDATAPAGE), &(currentProcess->p_s));
  updateProcTime();

  blockingFlag=true;
  insertBlocked(blockedSem, currentProcess);  //Lo aggiungiamo nella lista dei processi bloccati del semaforo
}

void updateProcTime(){
  cpu_t stopTOD;
  STCK(stopTOD);  //Misuriamo la fine del timer TOD
  currentProcess->p_time = currentProcess->p_time + (stopTOD-startTOD); //E aggiungiamo la differenza
}

void P(int* sem){
  if(*sem == 0){
    softBlockCount++;
    blockCurrentProc(sem);  //Blocchiamo il processo se il semaforo è già a 0
  }
  else{
    pcb_t* x = removeBlocked(sem);

    if(x != NULL){
      softBlockCount--;
      insertProcQ(&readyQueue, x); //Lo inseriamo nella readyQueue
    }
    else{
      *sem = *sem-1; //Decrementiamo il valore del semaforo puntato se non ci sono bloccati
    }
  }
}

void V(int* sem){
  if(*sem == 1){
    softBlockCount++;
    blockCurrentProc(sem);  //Blocchiamo il processo se il semaforo è già a 1
  }
  else{
    pcb_t* x = removeBlocked(sem);

    if(x != NULL){
      softBlockCount--;
      insertProcQ(&readyQueue, x);  //Lo inseriamo nella readyQueue
    }
    else{
      *sem = *sem+1; //Alziamo il valore del semaforo puntato se non ci sono bloccati
    }
  }
}


/*  SYSCALLS  */

int Create_Process(){
  pcb_t* x = allocPcb();

  if(x == NULL) {  //Nessun processo creato, ritornare -1
    ((state_t*)BIOSDATAPAGE)->reg_v0 = -1;
    return -1;
  }

  transferState((state_t*)(((state_t*)BIOSDATAPAGE)->reg_a1), &(x->p_s)); //Copiamo lo stato del processo da a1

  support_t* supportStruct = (support_t*)(((state_t*)BIOSDATAPAGE)->reg_a2);  //Prendiamo, se esiste la supportStruct da a2
  x->p_supportStruct = supportStruct;

  //Inseriamo il nuovo processo nella lista dei processi Ready
  processCount++;
  insertProcQ(&readyQueue, x);
  insertChild(currentProcess, x);

  nsd_t* nmsp = (nsd_t*)(((state_t*)BIOSDATAPAGE)->reg_a3);
  if(nmsp != NULL)
    addNamespace(x, nmsp);  //Se presente, prendiamo il namespace dal registro

  ((state_t*)BIOSDATAPAGE)->reg_v0 = x->p_pid; //v0, return con successo
  return x->p_pid;

}

void Terminate_Process(){
  int pid = (((state_t*)BIOSDATAPAGE)->reg_a1); //PID del processo puntato da a1

  if(pid == 0){
    TerminateRecursively(currentProcess);  //Terminiamo il processo chiamante
    outChild(currentProcess);
  } 
  else{
    TerminateRecursively(((pcb_t*)pid));   //Terminiamo il processo in input
    outChild(((pcb_t*)pid));
  }

  scheduler();
}

void TerminateRecursively(pcb_t* p){
  while(!emptyChild(p))
    TerminateRecursively(removeChild(p));  //Terminiamo ogni figlio

  processCount--;
  outProcQ(&readyQueue, p);
  if(outBlocked(p) != NULL)  //Se il processo era bloccato su un semaforo, sistemiamo SoftBlockCount
    softBlockCount--;

  freePcb(p);
}


void Passeren(){
  int* sem = (int*)(((state_t*)BIOSDATAPAGE)->reg_a1);
  P(sem);
}

void Verhogen(){
  int* sem = (int*)(((state_t*)BIOSDATAPAGE)->reg_a1);
  V(sem);
}


int DO_IO(){
  volatile devreg_t* cmdAddr = (devreg_t*)(((state_t*)BIOSDATAPAGE)->reg_a1);
  int* cmdValue = (int*)(((state_t*)BIOSDATAPAGE)->reg_a2); //in 0 status, in 1 command

  /*
    Valori ricavati dall'equazione: cmdAddr = 0x10000054 + ((lineNum-3)* 0x80) + (devNum * 0x10)
  */

  int lineNum = (((int)cmdAddr-0x10000054)/0x80)+3; //Calcolo del lineNum da cmdAddr
  int line = lineNum-DISKINT; //Non consideriamo le prime 3 linee

  int devNum = ((int)cmdAddr-0x10000054-(line*0x80))/0x10;  //Calcolo del devNum da cmdAddr
  int semDevNum = (line*DEVPERINT)+devNum;  //Usato nell'array di semafori. Da aumentare se siamo in transmit del terminale

  volatile devregarea_t* busreg = (devregarea_t*)RAMBASEADDR; //Volatile evita possibili errori data la natura hardware del bus
  
  if(lineNum!=7){ //Caso device normale
    busreg->devreg[line][devNum].dtp.command = cmdValue[1]; //Eseguiamo il comando

    P(&(devSem[semDevNum]));  //Operazione P sul semaforo, bloccante.

    if(busreg->devreg[line][devNum].dtp.status == 0){
      ((state_t*)BIOSDATAPAGE)->reg_v0 = -1;  //v0, return fallito
      return -1;
    }

  }
  else{  //Caso terminale
    int* baseAddr = (int*)(0x10000054 + (line*0x80) + (devNum*0x10));  //Indirizzo base

    if((baseAddr+TRANSMITCHAR) == (int*)cmdAddr){  //Siamo in transmit. +2.
      semDevNum += DEVPERINT;  //Transmit in 'linea 8'

      busreg->devreg[line][devNum].term.transm_command = cmdValue[1];

      P(&(devSem[semDevNum]));  //Operazione P sul semaforo, bloccante.
      
      if(busreg->devreg[line][devNum].term.transm_status == 0){
        ((state_t*)BIOSDATAPAGE)->reg_v0 = -1;  //v0, return fallito
        return -1;
      }
    }
    else{ //Siamo in recieve
      busreg->devreg[line][devNum].term.recv_command = cmdValue[1];

      P(&(devSem[semDevNum]));  //Operazione P sul semaforo, bloccante.

      if(busreg->devreg[line][devNum].term.recv_status == 0){
        ((state_t*)BIOSDATAPAGE)->reg_v0 = -1;  //v0, return fallito
        return -1;
      }
    }

  }
  
  devGV[semDevNum] = cmdValue; //Puntiamo allo stesso indirizzo di memoria di cmdValue
  ((state_t*)BIOSDATAPAGE)->reg_v0 = 0; //v0, return con successo
  return 0;
}

int Get_CPU_Time(){
  updateProcTime();

  ((state_t*)BIOSDATAPAGE)->reg_v0 = currentProcess->p_time;
  return currentProcess->p_time;
}

int Wait_For_Clock(){
  P(&clockSem); //Operazione P sul semaforo clockSem.
  return 0;
}


support_t* Get_Support_Data(){
  if(currentProcess->p_supportStruct != NULL){
    ((state_t*)BIOSDATAPAGE)->reg_v0 = (unsigned int)currentProcess->p_supportStruct;
    return currentProcess->p_supportStruct;
  }
  else{
    ((state_t*)BIOSDATAPAGE)->reg_v0 = NULL;
    return NULL;
  }
}

int Get_Process_Id(){
  int parent = ((state_t*)BIOSDATAPAGE)->reg_a1;  //Verifichiamo se prendere il pid del processo padre o del processo corrente

  if(parent == 0){  //Caso in cui prendiamo il PID del processo corrente
    ((state_t*)BIOSDATAPAGE)->reg_v0 = currentProcess->p_pid;
    return currentProcess->p_pid;
  }
  else{
    if(getNamespace(currentProcess->p_parent, NS_PID) == getNamespace(currentProcess, NS_PID)){ //Caso genitore con stesso namespace
      ((state_t*)BIOSDATAPAGE)->reg_v0 = currentProcess->p_parent->p_pid;
      return currentProcess->p_parent->p_pid;
    }
    else{ //Caso namespaces diversi
      ((state_t*)BIOSDATAPAGE)->reg_v0 = 0;
      return 0;
    }
  }

}

int Get_Children(){
  int* children = (int*)(((state_t*)BIOSDATAPAGE)->reg_a1); //Array dei figli passati
  int size = ((state_t*)BIOSDATAPAGE)->reg_a2;

  if(emptyChild(currentProcess)){ //Caso nessun figlio
    ((state_t*)BIOSDATAPAGE)->reg_v0 = 0;
    return 0;
  }

  int childc = 0; //Contatore per figli da ritornare
  int childi = 0; //Contatore per array children

  struct list_head* ite;
  list_for_each(ite, &(currentProcess->p_child)){
    if(getNamespace(container_of(ite, struct pcb_t, p_sib), NS_PID) == getNamespace(currentProcess, NS_PID)){ //Se il figlio ha lo stesso namespace del currentprocess   
      
      if(childi<size){  //Se possibile, inseriamo il figlio nell'array children
        children[childi] = container_of(ite, struct pcb_t, p_sib)->p_pid;  //Inseriamo il pid del figlio dentro l'array children
        childi++;
      }

      childc++; //Aumentiamo il contatore da ritornare per ogni figlio dello stesso namespace
    }
  }

  ((state_t*)BIOSDATAPAGE)->reg_v0 = childc;
  return childc;
}