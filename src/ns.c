#include "../inc/ns.h"

/*Array contenente i vari namespace descriptor. 
Per fase uno, utilizzeremo soltanto il namespace base, ma l'implementazione permetterà l'aggiunta di ulteriori namespaces per le successive fasi*/
static nsd_t type_nsd[NS_TYPE_MAX][MAXPROC] = {0};

/*Lista di NSD di type libero/inutilizzato, categorizzate per type*/
static struct list_head type_nsFree_h[NS_TYPE_MAX] = {0}; 

/*Lista di NSD di type attivi, categorizzate per type*/
static struct list_head type_nsList_h[NS_TYPE_MAX] = {0}; 

void initNamespaces(){
    for (int i = 0; i < NS_TYPE_MAX; i++){
        /*Inizializzazione liste*/
        INIT_LIST_HEAD(&type_nsFree_h[i]);
        INIT_LIST_HEAD(&type_nsList_h[i]);
        for (int j = 0; j < MAXPROC; j++)
            list_add(&type_nsd[i][j].n_link, &type_nsFree_h[i]); /*Aggiunta del namespace default di i-esimo tipo nella lista dei namespace liberi*/
    }
}

nsd_t *getNamespace(pcb_t *p, int type){
    if(p->namespaces[type]==NULL){ /*Il namespace non è stato allocato*/
        return NULL;
    }
    return p->namespaces[type];
}

int addNamespace(pcb_t *p,nsd_t *ns){
    struct list_head *pos;  /*Puntatore utilizzato come list_head iterante per list_for_each*/

    if(p==NULL || ns->n_type>NS_TYPE_MAX || &ns->n_link==NULL) /*Nel caso vi sia un errore, ritorna FALSE*/
        return FALSE;
    p->namespaces[ns->n_type]=ns;

    list_for_each(pos, &p->p_child)  /*Estraiamo ogni figlio dal padre*/
        container_of(pos,struct pcb_t, p_sib)->namespaces[ns->n_type]=ns; /*Assegnamo ad ogni figlio il namespace ns*/
        
    return TRUE;
}


nsd_t *allocNamespace(int type){
    if (list_empty(&type_nsFree_h[type])||type>NS_TYPE_MAX)  /*Controlliamo se abbiamo possibilità di allocare*/
        return NULL;
    list_move(type_nsFree_h[type].next, &type_nsList_h[type]); /*Allocazione del namespace*/
    return list_first_entry(&type_nsList_h[type], struct nsd_t, n_link);
    
}

void freeNamespace(nsd_t *ns){
    list_move(&ns->n_link, &type_nsFree_h[ns->n_type]);
}