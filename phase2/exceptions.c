#include "../h/pcb.h"
#include "../h/asl.h"
#include "../h/types.h"
#include "../h/const.h"
#include "../h/initial.h"
#include "../h/scheduler.h"
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

void SYSCALL() {
    switch(SYSNUM) 
    {
        case CREATEPROCESS:
            Create_ProcessP(caller);
            break;
        case TERMINATEPROCESS:
            Terminate_Process();
            break;
        case PASSEREN:
            wait(sema4);
            break;
        case VERHOGEN:
            signal(sema4);
            break;
        case WAITIO:
            Wait_for_IO_Device();
            break;
        case GETCPUTIME:
            Get_CPU_Time(p);
            break;
        case WAITCLOCK:
            Wait_For_Clock();
            break;
        case GETSUPPORTPRT:
            void Get_SUPPORT_Data();
            break;
        default:
            passUpOrDie(caller);
    }
    if(SYSNUM > GETSUPPORTPRT) {
        passUpOrDie(currentProc, GENERALEXCEPT);
    }
}

/* 
    System Call 1: When called, a newly populated pcb is placed on the Ready Queue and made a child
    of the Current Process. pcb_t p is the name for this new process, and its fields obtained from registers a1 and a2.
    Its cpu time is initialized to 0, and its semaphore address is set to NULL since it is in the ready
    state. 
*/
void Create_ProcessP(state_t *caller){
    /* Initialize fields of p */
    pcb_t *p;
    p->p_s = s_a1;
    p->p_supportStruct = s_a2;
    /* Make p a child of currentProc and also place it on the ReadyQueue */
    insertProcQ(readyQue, p);
    insertChild(currentProc, p);
    processCnt++;
    /* Set cpu time to 0 and semAdd to NULL */
    p->p_time = 0;
    p->p_semAdd = NULL;
}

/* System Call 2: When called, the executing process and all its progeny are terminated. */
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
    --processCnt;
}

/* System Call 3: Preforms a "P" operation or a wait operation. The semaphore is decremented
and then blocked.*/
pcb_t *wait(sema4)
{
    sema4--;
    if(sema4 < 0){
        pcb_t *p = removeProcQ(&(sema4));
        insertBlocked(&sema4, currentProc);
    }
    BlockedSYS(p);
}

/* System Call 4: Preforms a "V" operation or a signal operation. The semaphore is incremented
and is unblocked/placed into the ReadyQue.*/
pcb_t *signal(sema4)
{
    sema4++;
    if(sema4 <= 0){
        pcb_PTR temp = removeBlocked(&sema4);
        insertProcQ(&readyQue, temp);
    }
    return BlockedSYS(temp);
}

/* Sys5 */
void Wait_for_IO_Device()
{
    BlockedSYS(currentProc);
    /*SYSCALL(PASSEREN, iosema4, 0, 0); Need Helper Function here*/
    /*Need to handle subdevices*/
}

/* Sys6 */
int Get_CPU_Time(pcb_t *p)
{
    accumulatedTime = currentProc.p_time;
}

/* Sys7 */
void Wait_For_Clock()
{
    /* Define pseudoClockSema4 */
    pseudoClockSema4--;
    /* Handle this in a helper function */
    if(pseudoClockSema4 < 0)
    {
        pcb_t *p = removeProcQ(&(pseudoClockSema4));
        insertBlocked(&(pseudoClockSema4),p);
    }
    BlockedSYS(currentProc);
}

/* Sys8 */
void Get_SUPPORT_Data()
{
    return currentProc->p_supportStruct;
}

/*Used for syscalls that block*/
void BlockedSYS(pcb_t *p)
{
    p.p_s.s_pc = p.p_s.s_pc + 4;
    p->p_s.s_status = ALLOFF | IEPON | IMON | TEBITON;
    p->p_time = p->p_time + intervaltimer;
    insertBlocked(currentProc);
    scheduler();
}

void programTRPHNDLR() {
    passUpOrDie(currentProc, GENERALEXCEPT);
}

void uTLB_RefillHandler() {
    passUpOrDie(currentProc, PGFAULTEXCEPT);
}

/* Passup Or Die */

void passUpOrDie(pcb_t currProc, int ExeptInt) {
    if(currProc->p_supportStruct == NULL) {
        Terminate_Process();
    }
    if(currProc->p_supportStruct != NULL) {
        passUp(currProc, ExeptInt);
    }
}

void passUp(pcb_t currProc, int ExeptInt) {
    state_PTR tempstate = (state_t *) BIOSDATAPAGE->s_cause & GETEXECCODE;
    currProc->p_supportStruct->sup_exceptState[ExeptInt] = tempstate;
    context_t exceptContext;
    exceptContext.c_stackPtr = currProc->p_supportStruct->sup_exceptState[ExeptInt].c_stackPtr;
    exceptContext.c_status = currProc->p_supportStruct->sup_exceptState[ExeptInt].c_status;
    exceptContext.c_pc = currProc->p_supportStruct->sup_exceptState[ExeptInt].c_pc;
    LDCXT(exceptContext);
}

void uTLB_RefillHandler () {
    setENTRYHI(0x80000000);
    setENTRYLO(0x00000000);
    TLBWR();
    LDST ((state_PTR) 0x0FFFF000);
}