#include "../inc/pandos_types.h"


extern int blockingFlag;   //flag per le SYSCALLs bloccanti

//Handler per le operazioni SYSCALLs
void syscallHandler();
//Handler per le eccezioni TLB
void TLBHandler();
//Handler per le program traps
void trapHandler();

//Passa il controllo alla supportstruct del processo, se presente, o lo termina
void passUpOrDie(int exceptionType);
//Copia i parametri del primo stato input in quelli del secondo stato input
void transferState(state_t* a, state_t* b);

//Salva lo stato, aggiorna il tempo di esecuzione e blocca il processo corrente sul semaforo
void blockCurrentProc(int* blockedSem);
//Aggiorna il tempo di esecuzione del processo
void updateProcTime();
//Termina il sottoalbero dei figli del processo input
void TerminateRecursively(pcb_t* p);

void P(int* sem);   //Operazione P su sem
void V(int* sem);   //Operazione V su sem


//SYSCALLS

int Create_Process();   //1
void Terminate_Process();   //2
void Passeren();    //3
void Verhogen();    //4
int DO_IO();   //5
int Get_CPU_Time();    //6
int Wait_For_Clock();  //7
support_t* Get_Support_Data();    //8
int Get_Process_Id();  //9
int Get_Children();    //10