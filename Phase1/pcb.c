#include "pcb.h"

HIDDEN pcb_t *pcbFree_h;

/* process control block type */
typedef struct pcb_t{
    /* process queue fields */
    struct pcb_t    *p_next,    /* pointer to next entry */
                    *p_prev,    /* pointer to prev entry */

    /* process tree fields */
                    *p_prnt,    /* pointer to parent */
                    *p_child,   /* pointer to 1st child */
                    *p_sib;     /* pointer to sibling */

    /* process status information */
    state_t         p_s         /*processor state */
    cpu_t           p_time;     /* cpu time used by proc */
    int             *p_semAdd;  /* pointer to sema4 on which process blocked */

    /* support layer information */
    support_t       *p_supportStruct;
                                /* ptr to support struct */
} pcb_t;

/* add pcb on free list */
void freePcb(pcb_t *p)
{
    p->p_next = pcbFree_h;
    pcbFree_h = p;
}

/*initialize the PCB */
pcb_PTR allocPcb()
{
    if(pcbFree_h == NULL)
    {
        return NULL;
    }
    pcb_PTR temp;
    temp = pcbFree_h;
    pcbFree_h = pcbFree_h->p_next;

    /* queue fields */
    temp->p_next = NULL;
    temp->p_prev = NULL;

    /* tree fields */
    temp->p_prnt = NULL;
    temp->p_child = NULL;
    temp->p_sib = NULL;

    /* process status */
    temp->p_s = NULL;
    temp->p_time = NULL;
    temp->p_semAdd = NULL;

    /* support layer */
    temp->p_supportStruct = NULL;

    return temp;
}

/* initialize pcbFree List */
void initPcbs(){
    static pcb_t pcbTable[MAXPROC];
    for(int i = 0; i < MAXPROC; i++){
        allocPcb(&pcbTable[i]);
    }
}

pcb_t *mkEmptyProcQ(){
    return null;
}

int emptyProcQ(pcb_t *tp){
    return tp == null;
}

insertProcQ(pcb_t **tp, pcb_t *p){
    /* empty queue case */
    if(emptyProcQ(tp)){
        *tp = p;
        p->p_next = p;
        p->p_prev = p;
        return;
    } else {
        p->p_next = *tp;
        p->p_prev = *tp;
        return;
    }
}

pcb_t *removeProcQ(pcb_t **tp){
    if(emptyProcQ(tp)) {
        return NULL;
    } else {
        pcb_t temp = p->p_prev;
        *tp = temp;
        return;
    }
}

/* take a PCB out of the queue of some TP and then returns it */
pcb_PTR outProcQ(pcb_PTR *tp, pcb_PTR p){
    pcb_PTR final;
    /* empty case */
    if ((emptyProcQ(*tp) || (p == NULL)))
    {
        return NULL;
    }
    /* single case */
    if((*tp)==p)
    {
        return removeProcQ(*tp);
    }
    pcb_PTR temp;
    /* begin to chug through the list looking for our removeable PCB */
    temp = (*tp)->p_next;
    while(temp != (*tp))
    {
        if(temp == p)
        {
            /* set all notable fields equal to the one being removed */
            final = temp;
            final->p_prev->p_next = temp->p_next;
            final->p_next->p_prev = temp->p_prev;
            final->p_next = NULL;
            final->p_prev = NULL;
            return final;
        }
        temp = temp -> p_next;
    }
    return NULL
}

pcb_t *headProc(pcb_t *tp) {
    if(emptyProcQ(tp)) {
        return NULL;
    }
    return (tp -> p_next);
}

int emptyChild(pcb_t *p) {
    return(p -> p_child = NULL);
}

void insertChild (pcb_t *prnt, pcb_t *p){
    p -> p_prnt = prnt;
    prnt -> p_child = p;
}
        
pcb_t *removeChild(pcb_t){
    p->p_child = null;
    if(p->p_prnt == null){
        return null;
    } else{
        return(p->p_child);
    }
}
        
pcb_t *outchild(pcb_t *p){
    p->p_prnt = null;
    if(p->p_prnt == null){
        return null;
    } else{
        return p;
    }
}
