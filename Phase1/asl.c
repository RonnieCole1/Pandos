#include "../h/asl.h"
#include "../h/pcb.h"
#include "pcb.c"
#include "../h/types.h"
#include "../h/const.h"

HIDDEN semd_t *semd_h, *semdFree_h;

static semd_t semdTable[MAXPROC];

int insertBlocked(int *semAdd, pcb_PTR p){
    p->s_procQ = 
    p->p_next = p;
    p->p_semAdd = semAdd;
    if(){
        
    }
    mkEmptyProcQ(p->s_procQ);
}

pcb_t *removeBlocked(int *semAdd){
    if(search(semAdd) == FALSE){
        return NULL;
    } else {
        removeProcQ(s_procQ->p_next);
        if(emptyProcQ(s_procQ)){
            semd_t temp;
            temp->s_next = semdFree_h;
        }
    }
}

pcb_t *outBlocked(pcb_t *p){
    if(search(p) == FALSE){
        return NULL;
    } else {
        return p;
    }
}

pcb_t *headBlocked(int *semAdd){
    if(search(semAdd) == FALSE){
        return NULL;
    }
    return headProcQ(semAdd);
}

void initASL(){
    int i;
    for(i = 0; i < MAXPROC; i++){
        semdFree_h = &(semdTable[i]);
    }
}

int search(semd_t *s){
    int i;
    for(i = 0; i < MAXPROC; i++){
        return ((*s)->s_semAdd == &(semdTable[i]));
    }
}
