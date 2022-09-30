#include "../h/asl.h"
#include "../h/pcb.h"
#include "../h/types.h"
#include "../h/const.h"

/***************************************** Active Semaphore List *****************************************
 *   asl.c contains the Active Semaphore list, which consists of a Free Semaphore list and an Active Semaphore list.
 *   Each semaphore has an address (semAdd) and a process queue associated with it. A semaphore is active if there is at
 *   least one pcb on the process queue associated with it.
 *   The semaphore list is a sorted NULL-terminated single, linearly lnked list of semaphore descriptors whose
 *   head is pointed to by the variable semd_h. The list semd_h points to will represent out Active Semaphore List (ASL).
 *   We will keep the ASL using s_semAdd as a sort key. The list will be sorted in acending order.
 *   The Active Semaphore list will also contain two dummy nodes. One with semAdd = 0 and another with semAdd = MAXINT.
 *   These dummy nodes will handle edge cases.
 * 
 *   Authors:
 *      Ronnie Cole
 *      Joe Pinkerton
 *      Joseph Counts
*/

HIDDEN semd_t *semd_h, *semdFree_h;
semd_t *search(int *semAdd);
void freeSEMD(semd_PTR sem);
semd_t *freeLIST();

/*
    Insert the pcb pointed to by p at the tail of the process queue associated with the semaphore whose physical address is semAdd
    and set the semaphore address of p to semAdd. If the semaphore is currently not active (i.e. there is no descriptor for it in the
    ASL), allocate a new descriptor from the semdFree list, insert it in the ASL (at the appropriate position), initialize all of the fields
    (i.e. set s_semAdd to semAdd, and s_procq to mkEmptyProcQ()), and proceed as above. If a new semaphore descriptor needs to be allocated
    and the smdFree list is empty, return TRUE. In all other cases return FALSE.
*/
int insertBlocked(int *semAdd, pcb_PTR p){
    semd_t *temp = search(semAdd);
    if(temp->s_next->s_semAdd == semAdd){
       p->p_semAdd = semAdd;
       insertProcQ(&(temp->s_next->s_procQ), p);
       return FALSE;
    } else {
    /* Allocate descriptor from FreeList */
        semd_t *insrt = freeLIST();
        if(insrt == NULL){
            return TRUE;
        }
        insrt -> s_next=temp->s_next;
        temp -> s_next = insrt;
        insrt -> s_procQ = mkEmptyProcQ();
        insertProcQ(&(insrt -> s_procQ), p);
        p -> p_semAdd = semAdd;
        insrt -> s_semAdd = semAdd;
        return FALSE;
    }
}

/*
    Search the ASL for a descriptor of this semaphore. If none is found, return Null; otherwise,
    remove the first (i.e. head) pcb from the process queue of the found semaphore secriptor and return
    a pointer to it. If the process queue for this semaphore becomes empty (emptyProcQ(s_procq) is TRUE),
    remove the seaphore descriptor from the ASL and return it to the semdFree list.
*/
pcb_t *removeBlocked(int *semAdd){
    semd_t *temp = search(semAdd);
    if(temp->s_next->s_semAdd == semAdd){
        pcb_PTR temp1 = removeProcQ(&(temp->s_next->s_procQ));
        if(emptyProcQ(temp->s_next->s_procQ)){
            semd_t *temp2 = temp->s_next;
            temp->s_next = temp->s_next->s_next;
            freeSEMD(temp2);
        }
        temp1->p_semAdd = NULL;
        return temp1;
    }
    return NULL;
}

/*
    Remove the pcb pointed to by p from the process queue associated with p's semaphore (p -> p_semAdd)
    on the ASL. If pcb pointed to by p does not appear in the process queue associated with p's semaphore,
    which is an error condition, return NULL; otherwise, return p.
*/
pcb_t *outBlocked(pcb_t *p){
    semd_t *temp = search(p->p_semAdd);
    if(temp->s_next->s_semAdd == p->p_semAdd){
        pcb_PTR temp1 = outProcQ(&(temp->s_next->s_procQ), p);
        if(emptyProcQ(temp->s_next->s_procQ)){
            semd_t *temp2 = temp->s_next;
            temp->s_next = temp->s_next->s_next;
            freeSEMD(temp2);
        }
        return temp1;
    }
    return NULL;
}

/*
    Return a pointer to the pcb that is at the head of the process queue associated with the semaphore semAdd. Return NULL
    if semAdd is not found on the ASL or if the process queue associated with semAdd is empty.
*/
pcb_t *headBlocked(int *semAdd){
    semd_t *temp = search(semAdd);
    if(temp->s_next->s_semAdd == semAdd){
        return headProcQ(temp->s_next->s_procQ);
    }
    return NULL;
}

/*
    Initialize the semdFree list to contain all the elements of the array 
    static semd_t semdTable[MAXPROC]
    This method will be only called once during data structure initialization.
*/
void initASL(){
    static semd_t semdTable[MAXPROC + 2];                           /* A static list of semaphore descriptors with 2 dummy nodes for the head and tail. */
    semdFree_h = NULL;
    semd_h = NULL;

    int i;
    for(i = 0; i < MAXPROC; i++){
        freeSEMD(&(semdTable[i]));
    }

    semd_h = &(semdTable[MAXPROC + 1]);
    semd_h -> s_next = NULL;
    semd_h -> s_semAdd = MAXINT;
    semd_h -> s_procQ = NULL;

    (semdTable[MAXPROC]).s_next = semd_h;
    semd_h = &(semdTable[MAXPROC]);
    semd_h -> s_semAdd = 0;
    semd_h -> s_procQ = NULL;
}


/*
    Search for a Semaphore Descriptor *semAdd. If it's semaphore address matches the semaphore
    address for semAdd. If it doesn't, next the next node.
*/
semd_t *search(int *semAdd){
	semd_t *temp = semd_h;
	while(semAdd > temp->s_next->s_semAdd){
		temp = temp->s_next;
	}
	return temp;
}

/* 
    Insert the element pointed to by sem onto the semdFree list.
*/
void freeSEMD(semd_PTR sem){
    if(semdFree_h == NULL){
        semdFree_h = sem;
        semdFree_h -> s_next = NULL;
    }
    else {
        sem->s_next = semdFree_h;
        semdFree_h = sem;
    }
}

/*
    Removes a semaphore from the free list
*/
semd_t *freeLIST(){
    semd_t *temp = semdFree_h;
    if(semdFree_h == NULL)
    {
        return NULL;   
    }
    semdFree_h = semdFree_h -> s_next;
    temp -> s_next = NULL;
    temp -> s_semAdd = NULL;
    temp->s_procQ = mkEmptyProcQ();
    return temp;
}