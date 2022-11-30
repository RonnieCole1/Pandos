#include "../h/types.h"
#include "../h/const.h"
#include "../h/pcb.h"
#include "../h/asl.h"
#include "../h/scheduler.h"
#include "../h/exceptions.h"
#include "../h/interrupts.h"
#include "/usr/include/umps3/umps/libumps.h"

/********************************** Exception Handling ****************************
 *
 *   WIP!
 * 
 *   Authors:
 *      Ronnie Cole
 *      Joe Pinkerton
 *      Joseph Counts
*/

/* global variables from scheduler.c */
extern cpu_t TODStarted;
extern cpu_t currentTOD;

/* global variables from initial.c */
extern int processCnt;
extern int softBlockCnt;
extern pcb_t *readyQue;
extern pcb_t *currentProc;
extern int deviceSema4s[MAXDEVICECNT];

/* Syscall Handler. We take what is in register a_0 from the BIOSDATAPAGE, which is our sysnumber, and check its value from 1 to 8. If our
sysNum is greater than 8, we pass up or die.*/
void systemCall() {
    /* local variables */
    state_PTR caller;
    int request;

    /* Set the a_0 register of the BIOSDATAPAGE to sysNum.*/
    caller = (state_PTR) BIOSDATAPAGE;
    request = caller->s_a0;

    caller->s_pc = caller->s_pc + 4;
    caller->s_t9 = caller->s_pc + 4;

    switch(request) 
    {
        case CREATEPROCESS:
            Create_ProcessP(caller);
        break;
        case TERMINATEPROCESS:
            Terminate_Process();
        break;
        case PASSEREN:
            wait(caller);
        break;
        case VERHOGEN:
            signal(caller);
        break;
        case WAITIO:
            Wait_for_IO_Device(caller);
        break;
        case GETCPUTIME:
            Get_CPU_Time(caller);
        break;
        case WAITCLOCK:
            Wait_For_Clock(caller);
        break;
        case GETSUPPORTPRT:
            Get_SUPPORT_Data();
        break;
        default:
        /* If we reach here, our sysNum is not between 1-8. We passupordie*/
            passUpOrDie(GENERALEXCEPT);
    }
    PANIC();
}

/* 
    System Call 1: When called, a newly populated pcb is placed on the Ready Queue and made a child
    of the Current Process. pcb_t p is the name for this new process, and its fields obtained from registers a1 and a2.
    Its cpu time is initialized to 0, and its semaphore address is set to NULL since it is in the ready
    state. 
*/
void Create_ProcessP(state_PTR caller){
    /* Initialize fields of p */
    pcb_PTR p = allocPcb();
    if(p == NULL) {
    	caller->s_v0 = FAILURE;
    }
    if(p != NULL) {
    	insertChild(currentProc, p);
    	insertProcQ(&readyQue, p);
    	copyState((state_PTR) caller->s_a1, &(p->p_s));
        p->p_supportStruct = (support_t *) caller->s_a2;
    	processCnt++;
    	p->p_time = 0;
    	p->p_semAdd = NULL;
    	caller->s_v0 = SUCCESS;
    }
    /* return CPU to caller */
    myLDST(caller);
}

/* 
    System Call 2: When called, the executing process and all its progeny are terminated. 
*/
void Terminate_Process(){
    if(emptyChild(currentProc)){        /* current process has no child */
        outChild(currentProc);
        freePcb(currentProc);
        --processCnt;
    } else {
        /* recursive call */
        sys2Help(currentProc);
    }
    currentProc = NULL;                 /* no current process anymore */
    scheduler();
}

/* Recursively removes all the children of head */
void sys2Help(pcb_PTR head){
    while(!emptyChild(head)){
        sys2Help(removeChild(head));
    }

    if(head->p_semAdd != NULL){
        int* sem = head->p_semAdd;
        outBlocked(head);
        if(sem >= &(deviceSema4s[0]) && sem <= &(deviceSema4s[MAXDEVICECNT - 1])){
            softBlockCnt--;
        } else{
            ++(*sem);                   /* increment semaphore */
        }
    } else if(head == currentProc){
        /* remove process from its parent */
        outChild(currentProc);
    } else{
        /* remove process from readyQue */
        outProcQ(&readyQue, head);
    }

    /* free after no more children */
    freePcb(head);
    processCnt--;
}

/* System Call 3: Preforms a "P" operation or a wait operation. The semaphore is decremented
and then blocked.*/
void wait(state_PTR caller)
{
    int* sema4 = (int*) caller->s_a1;
    (*sema4)--;
    if(*sema4 < 0) {
        copyState(caller, &(currentProc->p_s));
        cpu_t TODStopped;
	STCK(TODStopped);
	currentProc -> p_time = currentProc -> p_time + (TODStopped - TODStarted);
	insertBlocked(sema4, currentProc);
        currentProc = NULL;
        scheduler();
    }
    myLDST(caller);
}

/* System Call 4: Preforms a "V" operation or a signal operation. The semaphore is incremented
and is unblocked/placed into the ReadyQue.*/
void signal(state_PTR caller){
    int* sema4 = (int*) caller->s_a1;
    (*sema4)++;   /* increment semaphore */
    if((*sema4) <= 0) {
        pcb_PTR p = removeBlocked(sema4);
        if(p != NULL){
            /* add to ready queue */
            insertProcQ(&(readyQue), p);
        }
    }
    /* return control to caller */
    myLDST(caller);
}

/* Sys5 */
void Wait_for_IO_Device(state_PTR caller){
    int lineNum, deviceNumber, read, index, *sem;
    copyState(caller, &(currentProc->p_s));
    lineNum = caller->s_a1;
    deviceNumber = caller->s_a2;
    read = caller->s_a3;    /* terminal read/write */

    index = DEVPERINT * (lineNum - 3 + read) + deviceNumber;

    sem = &(deviceSema4s[index]);
    (*sem)--;

    if((*sem) < 0){
        insertBlocked(sem, currentProc);
        softBlockCnt++;
        cpu_t TODStopped;
	STCK(TODStopped);
	currentProc -> p_time = currentProc -> p_time + (TODStopped - TODStarted);
        currentProc = NULL;
        scheduler();
    }
}

/* Sys6 */
int Get_CPU_Time(state_PTR caller){


	copyState(caller, &(currentProc->p_s));
    /* get current time, subtract from global start time */
    cpu_t TODStopped;
    STCK(TODStopped);
    currentProc -> p_time = currentProc -> p_time + (TODStopped - TODStarted);
    caller->s_v0 = currentProc->p_time;

    /*update start time */
    STCK(TODStarted);
    myLDST(caller);
}

/* Sys7 */
void Wait_For_Clock(state_PTR caller){
    int* semPClock = (int*) &(deviceSema4s[MAXDEVICECNT-1]);
    (*semPClock)--;
    insertBlocked(semPClock, currentProc);
    copyState(caller, &(currentProc->p_s));     /* store state back in currentProc */
    softBlockCnt++;
    currentProc = NULL;
    scheduler();
}

/* Sys8 */
void Get_SUPPORT_Data()
{
    return currentProc->p_supportStruct;
}

void programTRPHNDLR() {
    passUpOrDie(GENERALEXCEPT);
}

void TLB_TrapHandler() {
    passUpOrDie(PGFAULTEXCEPT);
}

/* 
    Passup Or Die 
*/
void passUpOrDie(int reason) {
    if(currentProc->p_supportStruct == NULL) {
        Terminate_Process();
        scheduler();
    } else{
        passUp(reason);
    }
}

void passUp(int type) {
    /*Copy the state saved in our BIOSDATAPAGE to the support exception state*/
    copyState(((state_PTR) BIOSDATAPAGE), &currentProc->p_supportStruct->sup_exceptState[type]);
    /*Load context with the fields of current process' support structure's stack pointer, status,
    and program counter, all of which was obtained from our BIOSDATAPAGE*/
    LDCXT(currentProc->p_supportStruct->sup_exceptContext[type].c_stackPtr,
    currentProc->p_supportStruct->sup_exceptContext[type].c_status,
    currentProc->p_supportStruct->sup_exceptContext[type].c_pc);
}
