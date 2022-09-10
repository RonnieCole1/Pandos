#include "pcb.h"
#include "types.h"
#include "const.h"

HIDDEN pcb_t *pcbFree_h;



/*
    Insert the element pointed to by p onto the pcbFree list.
*/
void freePcb(pcb_t *p)
{
    p->p_next = pcbFree_h;
    pcbFree_h = p;
}

/*
    Return NULL if the pcbFree list is empty. Otherwise,
    remove an element from the pcbFree list, provide initial values for ALL of the pcb
    fields (i.e. NULL and/or 0) and then return a pointer to the removed element. pcbs
    get reused, so it is important that no previous value persist in a pcb when it gets
    reallocated.
*/
pcb_PTR allocPcb()
{
    if(pcbFree_h == NULL)
    {
        return NULL;
    }
    pcb_PTR temp;
    temp = pcbFree_h;
    pcbFree_h = pcbFree_h->p_next;

    /* 
        Initialize the queue fields by setting them to NULL.
    */
    temp->p_next = NULL;
    temp->p_prev = NULL;

    /* 
        Initialize the tree fields by setting them to NULL.
    */
    temp->p_prnt = NULL;
    temp->p_child = NULL;
    temp->p_sib = NULL;

    /* 
        Initialize the status of our processes by setting them to Null.
    */
    temp->p_s = NULL;
    temp->p_time = NULL;
    temp->p_semAdd = NULL;

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
    } else {
        p->p_next = *tp;
        p->p_prev = *tp;
    }
}

pcb_t *removeProcQ(pcb_t **tp){
    if(emptyProcQ(tp)) {
        return NULL;
    } else {
        pcb_t temp = p->p_prev;
        *tp = temp;
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