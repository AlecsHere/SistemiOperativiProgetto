#include "../inc/initial.h"
#include "../inc/scheduler.h"
#include "../inc/exceptions.h"
#include "../inc/interrupts.h"
#include "/usr/include/umps3/umps/cp0.h"


extern void test();
extern void uTLB_RefillHandler();

cpu_t startTOD;
cpu_t stopTOD;
int processCount;
int softBlockCount;
int devSem [(DEVINTNUM*DEVPERINT)+DEVPERINT+1];
int* devGV [(DEVINTNUM*DEVPERINT)+DEVPERINT];
struct list_head readyQueue;
pcb_t* currentProcess;

int main(){
    /*Inizializza strutture dati*/
    initASH();
    initNamespaces();
    initPcbs();

    /*Inizializza variabili globali*/
    processCount = 0;
    softBlockCount = 0;
    mkEmptyProcQ(&readyQueue);   //Inizializzazione readyQueue
    currentProcess = NULL;
    for (int i=0; i<=((DEVINTNUM*DEVPERINT)+DEVPERINT); i++) devSem[i] = 0;

    /*Inizializza passupvector*/
    passupvector_t* passUpVector = (passupvector_t*)PASSUPVECTOR;
    passUpVector->tlb_refill_handler = (memaddr)uTLB_RefillHandler;
    passUpVector->tlb_refill_stackPtr = KERNELSTACK;    //Indirizzo puntato = 0x20001000
    passUpVector->exception_handler = (memaddr)generalExceptionHandler;
    passUpVector->exception_stackPtr = KERNELSTACK;

    LDIT(PSECOND);  //Intial timer impostato a 100ms
    memaddr ramTop;
    RAMTOP(ramTop); //Impostiamo ramTop come il nostro indirizzo di RAMTOP


    pcb_t * process = allocPcb();   //Allochiamo il processo usato per il test

    process -> p_s.status = ALLOFF | IEPON | TEBITON;   //OR tra i bits per dare i permessi. Interrupt e Terminal.
    process -> p_s.reg_sp = ramTop;  //Impostiamo lo stack pointer del processo su RamTop
    process -> p_s.pc_epc = (memaddr)test;
    process -> p_s.reg_t9 = (memaddr)test;    //pc e reg_t9 puntano alla memoria di funzione test

    processCount++; //=1
    insertProcQ(&readyQueue, process); //Lo inseriamo nella readyQueue
    
    scheduler();    //Passaggio del controllo allo scheduler

    
    return 0;
}


//Handler di tutte le eccezioni che non siano TLB-Refills
void generalExceptionHandler(){
    int exceptionType = CAUSE_GET_EXCCODE((((state_t*)BIOSDATAPAGE)->cause));

    if(exceptionType == IOINTERRUPTS)
        interruptHandler();
    else if(exceptionType <= TLBINVLDS)
        TLBHandler();
    else if (exceptionType == SYSEXCEPTION)
        syscallHandler();
    else
        trapHandler();

}