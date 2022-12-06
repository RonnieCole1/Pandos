#include "../h/types.h"
#include "../h/const.h"
#include "../h/pcb.h"
#include "../h/asl.h"
#include "../h/scheduler.h"
#include "../h/exceptions.h"
#include "../h/interrupts.h"
#include "/usr/include/umps3/umps/libumps.h"

/********************************** Exception Handling **************************************
 *
 *   Exceptions.c has 2 primary functions: Handle system calls and handle exceptions
 *   via PassUpOrDie. SYSCALL instructions are 4 tuple instruction where the first parameter
 *   refers to the type of system call. In this case, there are 8 types of system calls
 *   avaliable to us. Exceptions.c also handles other kinds of exceptions caused by general
 *   exceptions (like Program Traps) or TLB Refill events (Pagefaults). These are handled via
 *   PassUpOrDie. If our currentProccess allows it, these exceptions are passed up to the 
 *   support layer where they handled. Otherwise, these exceptions are dealt with by killing
 *   the current process.
 *
 *	Authors:
 *      Ronnie Cole
 *      Joe Pinkerton
 *      Joseph Counts
*/

/* ---------------- External Global Variables ---------------- */
extern cpu_t TODStarted;
extern cpu_t currentTOD;
extern int processCnt;
extern int softBlockCnt;
extern pcb_t *readyQue;
extern pcb_t *currentProc;
extern int deviceSema4s[MAXDEVICECNT];
extern int* ClockSema4;

/* ---------------- Helper Functions ---------------- */
HIDDEN void blockingSyscall(int* sem);
HIDDEN void sys2Help(pcb_PTR head);
HIDDEN void passUp(int type);

/************************************************************************
Syscall Handler. We take what is in register a_0 from the BIOSDATAPAGE, 
which is our sysnumber, and check its value from 1 to 8. If our sysNum is 
greater than 8, we pass up or die. For sysNums 1-8, we preform 8 different 
operations:

CREATE PROCESS, TERMINATE PROCESS, PASSEREN, VERHOGEN, WAITIO, GETCPUTIME, 
WAITCLOCK, GETSUPPORTPTR.
	
************************************************************************/
void systemCall() {
    /* Set the a_0 register of the BIOSDATAPAGE to sysNum.*/
    state_PTR caller = (state_PTR) BIOSDATAPAGE;
    int sysNum = caller->s_a0;

    /* Set the a_0 register of the BIOSDATAPAGE to sysNum.*/
    caller = (state_PTR) BIOSDATAPAGE;
    
    /* Check if we are in Kernel mode. If we aren't, jump to passupordie */
    int user = (caller->s_status & USERMOFF);
    if(user != ALLOFF){
	    passUpOrDie(GENERALEXCEPT);
    }
    
    /*Increment caller's program counter by 4*/
    caller->s_pc = caller->s_pc + 4;

    /* Depending on the value of sysNum (1-8) switch to the corresponding SYSCALL type*/
    switch(sysNum) 
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
            Get_SUPPORT_Data(caller);
        break;
        default:
        /* If we reach here, our sysNum is not between 1-8. We passupordie*/
            passUpOrDie(GENERALEXCEPT);
    }
    PANIC();
}

/************************************************************************
System Call 1: When called, a newly populated pcb is placed on the Ready 
Queue and made a child of the Current Process. pcb_t p is the name for 
this new process. To call CREATEPROCESS, the following C code can be used:
    
	int retValue = SYSCALL( CREATEPROCESS, state_t *statep, support_t *supportp, 0);
    	
The first parameter corresponds to register a0 and is not used when created 
process p. 
The second parameter corresponds to register a1, and is used in the copying 
on currentState's state to p.
The third parameter corresponds to register a2, and is used to create the 
support structure of p.
************************************************************************/
void Create_ProcessP(state_PTR caller){

    /* Initialize fields of p */
    pcb_PTR p = allocPcb();
    
    /* Was p successfully created? If not... */
    if(p == NULL) {
    	/* We place -1 (FAILURE) into caller's v0 register */
    	caller->s_v0 = FAILURE;
    }
    /*If p is not NULL (it has been created)*/
    if(p != NULL) { 	
    	
    	/* copy the state of CurrentProc and have p recieve it's contents*/
    	copyState((state_PTR) caller->s_a1, &(p->p_s));
    	
    	/*Support Structure for p is created. Data for this structure is pulled from Register a2 */
        p->p_supportStruct = (support_t *) caller->s_a2;
        
        /* Make p a child of currentProc and also place it on the ReadyQueue */
    	insertChild(currentProc, p);
    	insertProcQ(&readyQue, p);
    	processCnt++;
    	
    	/* We successfully made our process! */
    	caller->s_v0 = SUCCESS;
    }
    /* return CPU to caller */
    myLDST(caller);
}

/************************************************************************
System Call 2: When called, the executing process and all its progeny are 
terminated. To call TERMINATEPROCESS, the following C code can be used:
    
    SYSCALL(TERMINATEPROCESS, 0, 0, 0);
    	
This syscall does not use any of our a registers beyond a0. 
************************************************************************/
void Terminate_Process(){

    /* If the current process has no child... */
    if(emptyChild(currentProc)){
        outChild(currentProc);
        freePcb(currentProc);
        --processCnt;
        
    } else {
        /* recursive call */
        sys2Help(currentProc); 
    }
    scheduler();
}

/************************************************************************
Helper function for TERMINATE PROCESS. This function does the actual 
job of terminating current process and itschildren. 
************************************************************************/
void sys2Help(pcb_PTR head){

    /* Check p has any children. Recursively call until we reach "the youngest" child.
    If we broke from this loop, we have handle different cases for each kind of "child".*/
    while(!emptyChild(head)){
        sys2Help(removeChild(head));
    }

    /* If head as a semaphore address... */
    if(head->p_semAdd != NULL){
        /* Initialize a new semaphore address with the semaphore address of head */
        int* sem = head->p_semAdd;
        outBlocked(head);
        
        /* Check if our semaphore address is in the range between our first
        device's semaphore address and our last device's semaphore address. */
        if(sem >= &(deviceSema4s[0]) && sem <= &(deviceSema4s[MAXDEVICECNT])){
            softBlockCnt--;
        } else{
        /* If we aren't, decrement our semaphore. */
            ++(*sem);              
        }
    
    /* Is head our current proccess? If it is, simply remove it*/
    } else if(head == currentProc){
        /* remove process from its parent */
        outChild(currentProc);
        
    /* Otherwise, remove the head from our ReadyQueue */ 
    } else{
    
        /* remove process from readyQue */
        outProcQ(&readyQue, head);
    }

    /* Free after no more children */
    freePcb(head);
    processCnt--;   
}

/************************************************************************
System Call 3: Preforms a "P" operation or a wait operation. The semaphore 
is decremented and then blocked. To call PASSEREN, the following C code 
can be used:
		
	SYSCALL(PASSEREN, int *semaddr, 0, 0);
		
The second parameter corresponds to register a1, and is used to obtain 
our semaphore.
************************************************************************/
void wait(state_PTR caller) {
    /* obtain our semaphore (sema4). */
    int* sema4 = (int*) caller->s_a1;
    
    /* decrement sema4 */
    (*sema4)--;
    
    /* if our semaphore is less that 0, we block it*/
    if(*sema4 < 0) {
    	/* Block our semaphore and preform all the nessesary operations
    	that come with it */
        copyState(caller, &(currentProc->p_s));
        blockingSyscall(sema4);
    }
    
    /* return CPU to caller */
    myLDST(caller);
}

/************************************************************************ 
System Call 4: Preforms a "V" operation or a signal operation. The semaphore 
is incremented and isunblocked/placed into the ReadyQue. To call VERHOGEN, 
the following C code can be used:
		
	SYSCALL(VERHOGEN, int *semaddr, 0, 0);
		
The second parameter corresponds to register a1, and is used to obtain 
our semaphore.
************************************************************************/
void signal(state_PTR caller){
    /* Obtain our semaphore (sema4). */
    int* sema4 = (int*) caller->s_a1;
    
    /* Increment sema4 */
    (*sema4)++;  
    
    /*If our sema4 is less than 0, we unblock our semaphore and place the corresponding
    process into our readyQue*/
    if((*sema4) <= 0) {
    
    	/* Retrieve the process with our sema4 address */
        pcb_PTR p = removeBlocked(sema4);
        /* add to ready queue */
        insertProcQ(&(readyQue), p);
    }
    
    /* return control to caller */
    myLDST(caller);
}

/************************************************************************ 
System Call 5: When an I/O operation is initiated, the initiating process 
is blocked until the I/O completes. WAITIO is called every time a process 
initiates an I/O operation. From the start of the I/O operation, the current
process transitions from the running state into a blocked state. To call 
WAITIO, the following C code can be used:
		
	SYSCALL(WAITIO, int intlNo, int dnum, int waitForTermRead);
		
The second parameter corresponds to a1, which is our interupt line number.
The third parameter corresponds to a2, which is our device number.
The forth parameter corresponds to a3, which indicates if we are waiting
for a terminal read operation. This is read as either TRUE or FALSE.
************************************************************************/
void Wait_for_IO_Device(state_PTR caller){

/* ---------------- Initialize Local Variables ---------------- */
    int lineNum, deviceNumber, read, index, *sem;
    copyState(caller, &(currentProc->p_s));
    lineNum = caller->s_a1;
    deviceNumber = caller->s_a2;
    read = caller->s_a3;

    /* Find our semaphore address  */
    index = DEVPERINT * (lineNum - 3 + read) + deviceNumber;

    /* Locate the semaphore device in our semaphore list and decrement */
    sem = &(deviceSema4s[index]);
    (*sem)--;

    if((*sem) < 0){
        blockingSyscall(sem);
    }
}

/************************************************************************ 
System Call 6: Requests a processes' accumulated time. This accumulated
time is placed in our v0 register. To call GETCPUTIME, the following C 
code can be used:
	
	SYSCALL(GETCPUTIME, 0, 0, 0);
************************************************************************/
int Get_CPU_Time(state_PTR caller){

    /* Copy the state from our current process to caller */
    copyState(caller, &(currentProc->p_s));
    
    /* Get the total CPU time that elapsed since process started */
    cpu_t TODStopped;
    STCK(TODStopped);
    currentProc -> p_time = currentProc -> p_time + (TODStopped - TODStarted);
    
    /* Store accumulated time into our v0 register */
    caller->s_v0 = currentProc->p_time;

    /* Update start time */
    STCK(TODStarted);
    
    myLDST(caller);
}

/************************************************************************ 
System Call 7: Preforms a V operation on our pseudo clock semaphore. 
This will block our pseudo clock. To call WAITCLOCK, the following 
C code can be used:
		
	SYSCALL (WAITCLOCK, 0, 0, 0);
************************************************************************/
void Wait_For_Clock(state_PTR caller){

    /* Copy the state from our current process to caller */
    copyState(caller, &(currentProc->p_s));
    /* Decrement our clock semaphore */
    (*ClockSema4)--;
    
    if((*ClockSema4)<0){
    	blockingSyscall(&(deviceSema4s[MAXDEVICECNT-1]));
    }
}

/************************************************************************
System Call 8: This system call obtains the support structure of the 
current process. To call GETSUPPORTPTR, the following C code can be used:
		
	support_t *sPtr = SYSCALL(GETSUPPORTPTR, 0, 0, 0);
************************************************************************/
void Get_SUPPORT_Data(state_PTR caller)
{

    /* Copy the state from our current process to caller */
    copyState(caller, &(currentProc->p_s));
    /* Place currentProc's support structure into v0*/
    currentProc->p_s.s_v0 =(int) currentProc->p_supportStruct;
    myLDST(&currentProc->p_s);
}

/************************************************************************
Program Trap Exceptions are handled via passUpOrDie. Program Traps occur 
when a user requests a privileged service while in user-mode. If the 
support level is avaliable, this is handled in our support layer.
************************************************************************/
void programTRPHNDLR() {
    passUpOrDie(GENERALEXCEPT);
}

/************************************************************************
TLB TrapHandler are handled via passUpOrDie. Triggered when no matching 
entries are found in the TLB when preforming address translation. If 
the support level is avaliable, this is handled in our support layer.
************************************************************************/
void TLB_TrapHandler() {
    passUpOrDie(PGFAULTEXCEPT);
}

/************************************************************************
PassUp Or Die: Exceptions labed 9 and above, ProgramTraps, and TLB 
Exceptions will all be handled in pass up or die, where we first check 
the support structure of currentProc. If a support structure is not avaliable, 
we terminate the current process and all its children. If we do find a 
support structre, the exception is passed up to the support layer. Parameter 
"type" refers to whether the type of exception is being handled is caused by a
pagefault (PGFAULTEXCEPT) or is just a general exception (GENERALEXCEPT). 
************************************************************************/
void passUpOrDie(int reason) {
    if(currentProc->p_supportStruct == NULL) {
        Terminate_Process();
    } else{
        passUp(reason);
    }
}

/************************************************************************ 
The Nucleus passes up the handing of exceptions to the Support Level. 
We copy the exception state from the BIOSDATAPAGE to a location that 
is accessible to the support layer (sup_exceptContext[type]). Parameter 
"type" refers to whether the type of exception is being handled is caused 
by a Pagefault (PGFAULTEXCEPT) or is just a general exception (GENERALEXCEPT). 
************************************************************************/
void passUp(int type) {
    /*Copy the state saved in our BIOSDATAPAGE to the support exception state*/
    copyState(((state_PTR) BIOSDATAPAGE), &currentProc->p_supportStruct->sup_exceptState[type]);
    /*Load context with the fields of current process' support structure's stack pointer, status,
    and program counter, all of which was obtained from our BIOSDATAPAGE*/
    LDCXT(currentProc->p_supportStruct->sup_exceptContext[type].c_stackPtr,
    currentProc->p_supportStruct->sup_exceptContext[type].c_status,
    currentProc->p_supportStruct->sup_exceptContext[type].c_pc);
}

/************************************************************************
Helper function for Sys3, Sys5, and Sys7. This helper blocks the semaphore
placed into parameter sema4B and writes currentProc's accumulated CPU time 
into its p_time field. After which, our  currentProc is set to NULL and 
our scheduler is called.
************************************************************************/
void blockingSyscall(int* bsema4) {
	
	/* Block our semaphore */
	insertBlocked(bsema4, currentProc);
        softBlockCnt++;
        
        /* Stop our TOD clock and use it to find accumulated time */
        cpu_t TODStopped;
	STCK(TODStopped);
	
	/* Find the accumulated CPU time and write it in currentProc's time field*/
	currentProc -> p_time = currentProc -> p_time + (TODStopped - TODStarted);
	
	/* Set current Process to NULL and call scheduler */
        currentProc = NULL;
        scheduler();
}

void semaphoreHelper(int sysNum, int* inputSema4) {

	switch(sysNum)
	{
		case PASSEREN:
		(*inputSema4)--;
		if(*inputSema4 < 0) {
			blockingSyscall(inputSema4);
		}
		break;
	
		case VERHOGEN:
		(*inputSema4)++;
		if((*inputSema4) <= 0) {
        		pcb_PTR p = removeBlocked(inputSema4);
            		/* add to ready queue */
            		insertProcQ(&(readyQue), p);
    		}
		break;
		
		case WAITCLOCK:
		(*ClockSema4)--;
		if(*ClockSema4 < 0) {
			blockingSyscall(&(deviceSema4s[MAXDEVICECNT-1]));
		}
		break;	
	}
	
}

