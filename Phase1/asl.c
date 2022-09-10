#include "../h/asl.h"
#include "../h/pcb.h"
#include "pcb.c"
#include "../h/types.h"
#include "../h/const.h"

/*
    asl.c contains the Active Semaphore list, which consists of a Free Semaphore list and an Active Semaphore list.
    Each semaphore has an address (semAdd) and a process queue associated with it. A semaphore is active if there is at
    least one pcb on the process queue associated with it.
    The semaphore list is a sorted NULL-terminated single, linearly lnked list of semaphore descriptors whose
    head is pointed to by the variable semd_h. The list semd_h points to will represent out Active Semaphore List (ASL).
    We will keep the ASL using s_semAdd as a sort key. The list will be sorted in acending order.
    The Active Semaphore list will also contain two dummy nodes. One with semAdd = 0 and another with semAdd = MAXINT.
    These dummy nodes will handle edge cases.
*/

HIDDEN semd_t *semd_h, *semdFree_h;


static semd_t semdTable[MAXPROC];       /* A static list of semaphore descriptors. */

/*
    Insert the pcb pointed to by p at the tail of the process queue associated with the semaphore whose physical address is semAdd
    and set the semaphore address of p to semAdd. If the semaphore is currently not active (i.e. there is no descriptor for it in the
    ASL), akkicated a bew descriptor from the semdFree list, insert it in the ASL (at the appropriate position), initialize all of the fields
    (i.e. set s_semAdd to semAdd, and s_procq to mkEmptyProcQ()), and proceed as aboe. If a new semaphore descriptor needs to be allocated
    and the smdFree list is empty, return TRUE. In all other cases return FALSE.
*/
int insertBlocked(int *semAdd, pcb_PTR p){
    if(search(semAdd)){
       p->p_semAdd = semAdd;
       insertProcQ(p->p_semAdd, p);
    } else {
        /* Allocate sem descriptor from FreeList */
        
    }
}

/*
    Search the ASL for a descriptor of this semaphore. If none is found, return Null; otherwise,
    remove the first (i.e. head) pcb from the process queue of the found semaphore secriptor and return
    a pointer to it. If the process queue for this semaphore becomes empty (emptyProcQ(s_procq) is TRUE),
    remove the seaphore descriptor from the ASL and return it to the semdFree list.
*/
pcb_t *removeBlocked(int *semAdd){
    if(search(semAdd) == FALSE){
        return NULL;
    } else {
        semd_t *temp;
        *temp->s_procQ;
        removeProcQ(temp);
        if(emptyProcQ(temp)){
            semd_t *temp2;
            temp2->s_next = semdFree_h;
        }
    }
}

/*
    Remove the pcb pointed to by p from the process queue associated with p's semaphore (p -> p_semAdd)
    on the ASL. If pcb pointed to by p does not appear in the process queue associated with p's semaphore,
    which is an error condition, return NULL; otherwise, return p.
*/
pcb_t *outBlocked(pcb_t *p){
    if(search(p) == FALSE){
        return NULL;
    } else {
        return p;
    }
}

/*
    Return a pointer to the pcb that is at the head of the process queue associated with the semaphore semAdd. Return NULL
    if semAdd is not found on the ASL or if the process queue associated with semAdd is empty.
*/
pcb_t *headBlocked(int *semAdd){
    if(search(semAdd) == FALSE){
        return NULL;
    }
    return headProcQ(semAdd);
}

/*
    Initialize the semdFree list to contain all the elements of the array 
    static semd_t semdTable[MAXPROC]
    This method will be only called once during data structure initialization.
*/
void initASL(){
    int i;
    for(i = 0; i < MAXPROC; i++){
        semdFree_h = &(semdTable[i]);
    }
}

/* Helper function */
int search(semd_t *s){
    int i;
    for(i = 0; i < MAXPROC; i++){
        return ((s)->s_semAdd == &(semdTable[i]));
    }
}
