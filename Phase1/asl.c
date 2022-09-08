#include "asl.h"
#include "pcb.h"
#include "pcb.c"

HIDDEN semd_t *semd_h, *semdFree_h;

/* semaphore descriptor type */
typedef struct semd_t{
    struct  semd_t  *s_next;        /* next element on the ASL */
    int             *s_semAdd;      /* pointer to the semaphore */
    pcb_t           *s_procQ;       /* tail pointer to a process queue */
} semd_t;

static semd_t semdTable[MAXPROC];
semd_t->s_semAdd = 0;
semd_t->s_semAdd = FFFFFFFF;

int insertBlocked(int *semAdd, pcb_t *p){
    p->p_next = p;
    p->p_semAdd = semAdd;
    if(){
        
    }
}

void initASL(){
    for(int i = 0; i < MAXPROC; i++){

    }
}


