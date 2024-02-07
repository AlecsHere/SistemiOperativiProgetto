#include "../inc/ash.h"

static semd_t semd_table[MAXPROC]; //SEMD array with a maximun length of MAXPROC
static struct list_head semdFree_h; //List of unused/free SEMD
DEFINE_HASHTABLE(semd_h, 15); //hash table of SEMD currently in use

void initASH(){
    INIT_LIST_HEAD(&semdFree_h);
    for(int i = 0; i<MAXPROC; i++){ //initialize all semd and add them to semdFree_h
        INIT_LIST_HEAD(&semd_table[i].s_freelink);
        INIT_LIST_HEAD(&semd_table[i].s_procq);
        INIT_HLIST_NODE(&semd_table[i].s_link);
        list_add(&semd_table[i].s_freelink, &semdFree_h);
    }
}

int insertBlocked(int *semAdd,pcb_t *p){
    int b;
    struct semd_t * current;
    hash_for_each(semd_h, b, current, s_link){ //check all the semd in use
        if(current->s_key==semAdd){ //if semd is found, add the process to the queue
            p->p_semAdd = semAdd;
            list_add_tail(&p->p_list, &current->s_procq);
            return false;
        }
    }
    //semd not found
    if(list_empty(&semdFree_h)){ //if there are no more semd available return true
        return true;
    }
    //otherwise remove a semd from the free list, add the process to its queue and insert it to the hash table
    semd_t * tmp = list_first_entry(&semdFree_h, struct semd_t, s_freelink);
    tmp->s_key=semAdd;
    p->p_semAdd = semAdd;
    list_add(&p->p_list, &tmp->s_procq);
    hash_add(semd_h,&tmp->s_link,*semAdd);
    list_del(&tmp->s_freelink); //remove semd from the semdFree_h list

    return false;
}

pcb_t* removeBlocked(int *semAdd){
    int b;
    struct semd_t * current;
    hash_for_each(semd_h, b, current, s_link){ //check all the semd in use
        if(current->s_key==semAdd){ //if semd is found, delete and return the first process
            pcb_t * tmp = list_first_entry(&current->s_procq, struct pcb_t, p_list);
            list_del(&tmp->p_list);
            if(list_empty(&current->s_procq)){ //if the queue is now empty, remove semd form the hash table, then add it to the free semd list
                hash_del(&current->s_link);
                list_add(&current->s_freelink, &semdFree_h);
            }
            return tmp;
        }
    }
    //semd not found
    return NULL;
}

pcb_t* outBlocked(pcb_t *p){
    int b;
    struct semd_t * current;
    hash_for_each(semd_h, b, current, s_link){ //check all the semd in use
        if(current->s_key==p->p_semAdd){ //if semd is found, check if the requested process is in the queue
            struct list_head* currentList;
            list_for_each(currentList, &current->s_procq){ //queue check
                if(currentList == &p->p_list){//if the process is found, remove it from the list and return the process
                    list_del(currentList);
                    if(list_empty(&current->s_procq)){ //if queue is now empty, delete semd and add it to the free semd list
                        hash_del(&current->s_link);
                        list_add(&current->s_freelink, &semdFree_h);
                    }
                    return p;
                }
            }
            return NULL; //process requested is not in the queue
        }
    }
    //semd not found
    return NULL;
}

pcb_t* headBlocked(int *semAdd){
    int b;
    struct semd_t * current;
    hash_for_each(semd_h, b, current, s_link){ //check all the semd in use
        if(current->s_key==semAdd){ //if semd is found, return the process queue head
            if(list_empty(&current->s_procq))return NULL; //semd found, but process queue is empty
            struct pcb_t * tmp = list_first_entry(&current->s_procq, struct pcb_t, p_list);
            return tmp;
        }
    }
    //semd not found
    return NULL;
}