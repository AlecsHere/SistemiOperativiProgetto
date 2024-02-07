#include "../inc/pcb.h"

//lista dei PCB che sono liberi o inutilizzati.
static struct list_head pcbFree_h;
//array di PCB con dimensione massima MAXPROC.
static pcb_t pcbFree_table[MAXPROC];


//FUNZIONI AUSILIARI

//inizializza i campi della struttura state_t a 0
static void initStatoProcessore(state_t s){
    s.entry_hi = 0;
	s.cause = 0;
	s.status = 0;
	s.pc_epc = 0;
    for (int i = 0; i <STATE_GPR_LEN; i++){
        s.gpr[i] = 0;
    }
	s.hi = 0;
	s.lo = 0;
}

//verifica se l'elemento x si trova nella lista con testa head
static int isInList(struct list_head* x, struct list_head* head){
    struct list_head* ite;      //iteratore

    list_for_each(ite, head){   //controllo che x sia nella lista
        if(ite == x){
            return true;
        }
    }

    return false;
}




void initPcbs(){
    INIT_LIST_HEAD(&pcbFree_h);
    for (int i=0; i<MAXPROC; i++)
        list_add(&pcbFree_table[i].p_list, &pcbFree_h);
}

void freePcb(pcb_t * p){
    if (p == NULL) return;

    //libero il pcb aggiungendolo nella pcbFree
    list_add(&p->p_list, &pcbFree_h);
}

pcb_t *allocPcb(){
    if (list_empty(&pcbFree_h)) return NULL;

    else{
        //prendo il primo pcb libero e lo rimuovo da pcbFree
        pcb_t *x = list_first_entry(&pcbFree_h, pcb_t, p_list); 
        list_del(&x->p_list);

        //inizializzo tutti i suoi campi
        if (x!=NULL){
            x->p_list.prev = x->p_list.next = NULL;
            x->p_parent = NULL;
            INIT_LIST_HEAD(&x->p_child);
            x->p_sib.prev = x->p_sib.next = NULL;
            initStatoProcessore(x->p_s);    //per inizializzare p_s
            x->p_time = 0;
            x->p_semAdd = NULL;
            for(int i = 0; i<NS_TYPE_MAX; i++){
                x->namespaces[i]=NULL;
            }

            //aggiunte nuove:
            x->p_supportStruct = NULL;
            x->p_pid = (int)x;    //indirizzo di x
        }
        return x;
    }
}


void mkEmptyProcQ(struct list_head *head){
    INIT_LIST_HEAD(head);
}

int emptyProcQ(struct list_head *head){
    return list_empty(head);
}


void insertProcQ(struct list_head* head, pcb_t* p){
    if (head == NULL || p == NULL)  return;

    //aggiungo il pcb in coda alla lista
    list_add_tail(&p->p_list, head);
}

pcb_t* headProcQ(struct list_head* head){
    if (list_empty(head))   return NULL;
    else                    return list_first_entry(head, pcb_t, p_list);
}


pcb_t* removeProcQ(struct list_head* head){
    if (list_empty(head))   return NULL;

    //prendo il primo elemento della lista con headProcQ e lo rimuovo
    pcb_t* x = headProcQ(head);
    list_del(&x->p_list);
    return x;
}

pcb_t* outProcQ(struct list_head* head, pcb_t *p){
    if (list_empty(head))  return NULL;

    //controllo se il pcb è nella lista; se non lo è, return NULL
    if(!isInList(&p->p_list, head)) return NULL;

    list_del(&p->p_list);
    return p;
}


int emptyChild(pcb_t *p){
    if (p == NULL)  return true;
    return list_empty(&p->p_child); //true se non ha figli, false altrimenti
}

void insertChild(pcb_t *prnt, pcb_t *p){
    if (prnt == NULL || p == NULL)   return;

    //p diventa figlio di prnt, assieme ai suoi fratelli
    p->p_parent = prnt;
    list_add_tail(&p->p_sib, &prnt->p_child);
}

pcb_t* removeChild(pcb_t* p){
    if (p == NULL || list_empty(&p->p_child))   return NULL;

    //rimuovo il primo figlio del pcb
    pcb_t * x = list_first_entry(&p->p_child, pcb_t, p_sib);
    list_del(&x->p_sib);
    return x;
}

pcb_t* outChild(pcb_t* p){
    if (p == NULL || p->p_parent == NULL)  return NULL;

    //rimuovo il processo dalla lista dei fratelli, e cancello il suo parent
    list_del(&p->p_sib);
    p->p_parent = NULL;
    return p;
}