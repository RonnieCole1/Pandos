#include "../h/pcb.h"
#include "../h/types.h"
#include "../h/const.h"

/************************************ Process Queue ****************************
 *   pcb.c contains datastructures to support the creation and maintainance of process
 *   control blocks. Our PCBs are organized in a doubly, circularly linked list
 *   with a Queue pointer pointed at the tail. This structure is organized as a 
 *   Queue. 
 * 
 *   Authors:
 *      Ronnie Cole
 *      Joe Pinkerton
 *      Joseph Counts
*/

HIDDEN pcb_PTR pcbFree_h;

/* 
    Insert the element pointed to by p onto the pcbFree list.
*/
void freePcb(pcb_PTR p)
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

    /* initialize queue fields by setting to NULL */
    temp->p_next = NULL;
    temp->p_prev = NULL;

    /* initialize tree fields by setting to NULL */
    temp->p_prnt = NULL;
    temp->p_child = NULL;
    temp->p_sibp = NULL;
    temp->p_sibn = NULL;

    /* process status */
    /*temp->p_s = NULL;*/
    temp->p_time = NULL;
    temp->p_semAdd = NULL;

    /* support layer */
    /* temp->p_supportStruct = NULL; */

    return temp;
}

/* 
    Initialize the pcbFree list to contain all the elements of the
    static array of MAXPROC pcbs. This method will be called only
    once during data structure initialization. 
*/
void initPcbs(){
    /* Create an array of pcb_ts. */
    static pcb_t pcbTable[MAXPROC];
    int i;
    pcbFree_h = NULL;
    /* Insert Pcbs from the array pcbTable to our process queue. */
    for(i = 0; i < MAXPROC; i++){
        freePcb(&(pcbTable[i]));
    }
}

/* 
    This method is used to initialize a variable to be tail pointer to a
    process queue.
    Return a pointer to the tail of an empty process queue; i.e. NULL.
*/
pcb_PTR mkEmptyProcQ(){
    return NULL;
}

/* 
    Return TRUE if the queue whose tail is pointed to by tp is empty.
    Return FALSE otherwise.
*/
int emptyProcQ(pcb_t *tp){
    return (tp == NULL);
}

/* 
    Insert the pcb pointed to by p into the process queue whose tailpointer is pointed to by tp. 
    Note the double indirection through tp
    to allow for the possible updating of the tail pointer as well. 
*/

void insertProcQ(pcb_PTR *tp, pcb_PTR p){

    /* Check if our queue is empty */
    if(emptyProcQ(*tp)){
        *tp = p;
        p->p_next = p;
        p->p_prev = p;
    /* If our queue has 1 or more elements */
    } else {
        pcb_PTR temp = *tp;
        *tp = p;
        p->p_next = temp->p_next;
        temp->p_next = p;
        p->p_prev = temp;
        p->p_next->p_prev = p;
    }
}

/* 
    Remove the first (i.e. head) element from the process queue whose
    tail-pointer is pointed to by tp. Return NULL if the process queue
    was initially empty; otherwise return the pointer to the removed element. 
    Update the process queue’s tail pointer if necessary.
*/
pcb_PTR removeProcQ(pcb_PTR *tp){
    /* Check if our queue is empty */
    if(emptyProcQ(*tp)) {
        return NULL;
    } else {
        /* temp is a pointer to the removed element */
        pcb_PTR temp = headProcQ(*tp);
        if((*tp)->p_next == (*tp)){
            *tp = NULL;
        } else {
            (*tp)->p_next = (*tp)->p_next->p_next;
        }
        return temp;
    }
}

/* 
    Remove the pcb pointed to by p from the process queue whose tailpointer is pointed 
    to by tp. Update the process queue’s tail pointer if
    necessary. If the desired entry is not in the indicated queue (an error
    condition), return NULL; otherwise, return p. Note that p can point
    to any element of the process queue.
*/
pcb_PTR outProcQ(pcb_PTR *tp, pcb_PTR p){
    pcb_PTR final;

    /* Return NULL if our queue is empty */
    if ((emptyProcQ(*tp) || (p == NULL)))
    {
        return NULL;
    }

    /* Check if there is a single element. If there is, just remove it. */
    if((*tp)==p)
    {
        return removeProcQ(*tp);
    }

    pcb_PTR temp;

    /* begin to chug through the list looking for our removeable PCB */
    temp = (*tp)->p_next; /* Pointer used to search for p */
    while(temp != (*tp))
    {
        /* If we find the pcb we were looking to remove... */
        if(temp == p)
        {
            /* set all notable fields equal to the one being removed */
            final = temp;
            final->p_prev->p_next = temp->p_next;
            final->p_next->p_prev = temp->p_prev;
            final->p_next = NULL;
            final->p_prev = NULL;
            /* Return p, which is equal to final */
            return final;
        }
        /* step through the queue */
        temp = temp -> p_next;
    }
    /* If we reach this point, we could not find p in the pcb queue */
    return NULL;
}

/* 
    Return a pointer to the first pcb from the process queue whose tail
    is pointed to by tp. Do not remove this pcbfrom the process queue.
    Return NULL if the process queue is empty. 
*/
pcb_t *headProcQ(pcb_t *tp) {
    /* Check if the queue is empty */
    if(emptyProcQ(tp)) {
        return NULL;
    }
    /* If it is not empty, take advantage of the pcb queue structure to simpy step
    from the tail pointer to the next node, which represents the head. */
    return tp->p_next;
}

/************************************ Process Tree ****************************
 *   pcb.c also contains a process tree, or a tree of process queues. This tree is
 *   organized so that a parent pcb contains a pointer to a NULL terminated doubly,
 *   linearly linked list of its child pcbs. Each child process has a pointer to its
 *   parent pcb and possibly the next child pcb of its parent. For this data structure,
 *   we created fields p_sibn and p_sibp to give us access to both the previous and 
 *   next siblings of node p.
 *  
*/

/* 
    Return TRUE if the pcb pointed to by p has no children. Return
    FALSE otherwise. 
*/
int emptyChild (pcb_PTR p){
    return ((p -> p_child) == NULL);
}

/*
    Make the pcb pointed to by p a child of the pcb pointed to by prnt.
*/
void insertChild (pcb_PTR prnt, pcb_PTR p) {
    /* Check if the node has no children. */
    if (emptyChild(prnt)) {
        prnt -> p_child = p;
        p -> p_prnt = prnt;
        p -> p_sibn = NULL;
        p -> p_sibp = NULL;
        return;
    }
    /* If prnt already has children, make p a child and and prnts other children
    siblings of p */
    else {
        prnt -> p_child -> p_sibp = p; 
        p -> p_sibn = prnt -> p_child;
        p -> p_sibp = NULL;
        prnt -> p_child = p;
        p -> p_prnt = prnt;
        return;
    }
}

/* 
    Make the first child of the pcb pointed to by p no longer a child of
    p. Return NULL if initially there were no children of p. Otherwise,
    return a pointer to this removed first child pcb. 
*/
pcb_PTR removeChild (pcb_PTR p) {
    pcb_PTR tempChild;
    /* Check if p has any children. If not return NULL */
    if (emptyChild(p)) {
        return NULL;
    }
    /* If p has children... */
    tempChild = p -> p_child;
    /* and if p also has siblings... */
    if (p -> p_child -> p_sibn != NULL) {
        /* make p's second child its first child and
        set the previous first child's fields to NULL. */
        p -> p_child  = p -> p_child  -> p_sibn;
        p -> p_child -> p_sibp == NULL;
        tempChild -> p_prnt = NULL;
        tempChild -> p_sibn = NULL;
        tempChild -> p_sibp = NULL;
        /*return a pointer to this removed child */
        return tempChild;
    }
    /* then p only has one child, set that child's fields to NULL */
    else {
        p -> p_child = NULL;
        tempChild -> p_prnt = NULL;
        tempChild -> p_sibn = NULL;
        tempChild -> p_sibp = NULL;
        /*return a pointer to this removed child */
        return tempChild;
    }
}

/* 
    Make the pcb pointed to by p no longer the child of its parent. If
    the pcb pointed to by p has no parent, return NULL; otherwise, return
    p. Note that the element pointed to by p need not be the first child of
    its parent. 
*/       
pcb_PTR outChild (pcb_PTR p){
    /* Check if p has a parent or if p is NULL */
    if (p -> p_prnt == NULL || p == NULL) { 
        /* Just return NULL */
        return NULL;    
    }
    /* if p is the only child */
    if (p -> p_prnt -> p_child == p) { 
        return removeChild(p -> p_prnt);
    }
    /* p is the last child */
    if (p -> p_sibn == NULL) {
        p -> p_sibp -> p_sibn = NULL;
        p -> p_prnt = NULL;
        return p;
    }
    /* p is neither the first child or the last child */
    if (p -> p_sibp != NULL ||p -> p_sibn != NULL) {
        p -> p_sibn -> p_sibp = p -> p_sibp;
        p -> p_sibp -> p_sibn = p -> p_sibn;
        p -> p_prnt = NULL;
        return p;
    }        
    
    return NULL;
}