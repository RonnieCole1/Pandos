#include "../h/pcb.h"
#include "../h/asl.h"
#include "../h/types.h"
#include "../h/const.h"
#include "../h/initial.h"
#include "../h/scheduler.h"
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

/* global variables maintaining time usage*/
extern cpu_t TODStarted;
extern cpu_t currentTOD;

/* global variables from initial.c */
extern int processCnt;
extern int softBlockCnt;
extern pcb_t *readyQue;
extern pcb_t *currentProc;
extern int deviceSema4s[MAXDEVICECNT];


/* Syscall Handler. We take what is in register a_0 from the BIOSDATAPAGE, which is our sysnumber, and check its value from 1 to 8. 
If our sysNum is greater than 8, we pass up or die. For sysNums 1-8, we preform 8 different operations:

	CREATE PROCESS, TERMINATE PROCESS, PASSEREN, VERHOGEN, WAITIO, GETCPUTIME, WAITCLOCK, GETSUPPORTPTR.
	
*/
void systemCall() {

    /* Set the a_0 register of the BIOSDATAPAGE to sysNum.*/
    state_PTR caller = (state_PTR) BIOSDATAPAGE;
    int sysNum = caller->s_a0;

    /* Move all the registers and fields from currentProc's state to caller */
    copyState(caller, &(currentProc->p_s));
    /*Increment caller's program counter by 4*/
    caller->s_pc = caller->s_pc + 4;
	
	/* Depending on the value of sysNum (1-8) switch to the corresponding SYSCALL type*/
    switch(sysNum) 
    {
        case CREATEPROCESS: Create_ProcessP(caller);
        break;
        
        case TERMINATEPROCESS: Terminate_Process();
        break;
        
        case PASSEREN: wait(caller);
        break;
        
        case VERHOGEN: signal(caller);
        break;
        
        case WAITIO: Wait_for_IO_Device(caller);
        break;
        
        case GETCPUTIME: Get_CPU_Time(caller);
        break;
        
        case WAITCLOCK: Wait_For_Clock(caller);
        break;
        
        case GETSUPPORTPRT: Get_SUPPORT_Data(caller);
        break;
        
        default:
        /* If we reach here, our sysNum is not between 1-8. We passupordie*/
            passUpOrDie(GENERALEXCEPT);
    }
}

/* 
    System Call 1: When called, a newly populated pcb is placed on the Ready Queue and made a child
    of the Current Process. pcb_t p is the name for this new process. To call CREATEPROCESS, the following C code
    can be used:
    
    	int retValue = SYSCALL( CREATEPROCESS, state_t *statep, support_t *supportp, 0);
    	
    The first parameter corresponds to register a0 and is not used when created process p. 
    The second parameter corresponds to register a1, and is used in the copying on currentState's state to p.
    The third parameter corresponds to register a2, and is used to create the support structure of p.
*/
void Create_ProcessP(state_PTR caller){
    /* Initialize fields of p */
    pcb_PTR p = allocPcb();  

    /* Was p successfully created? If not... */
    if (p == NULL){
    	/* We place -1 (FAILURE) into currentProc's state's v0 register */
        currentProc->p_s.s_v0 = FAILURE;
	/*If p is not NULL (it has been created)*/
    }
    /* If we have created a process... */
    if (p != NULL) {
    	
    	
    	/* copy the state of CurrentProc and have p recieve it's contents*/
    	copyState(&(p->p_s), (state_PTR) currentProc->p_s.s_a1);
	
    	/*Support Structure for p is created. Data for this structure is pulled from Register a2 */
    	support_t *currentProcSupport;
    	currentProcSupport = (support_t*) currentProc->p_s.s_a2;
    	
    	/* Set the support structure of currentProc to p's support structure*/
	p->p_supportStruct = currentProcSupport;
	
	/* Make p a child of currentProc and also place it on the ReadyQueue */
    	insertChild(currentProc, p);
    	insertProcQ(&readyQue, p);
    	processCnt++;
    	
    	/* We successfully made our process! */
    	currentProc->p_s.s_v0 = SUCCESS;
    }

    /* Load the state of current Process's state  */
   myLDST(caller);
}

/* 
    System Call 2: When called, the executing process and all its progeny are terminated. To call TERMINATEPROCESS, 
    the following C code can be used:
    
    	SYSCALL(TERMINATEPROCESS, 0, 0, 0);
    	
    This syscall does not use any of our a registers beyond a0. 
*/
void Terminate_Process(){
    /* Call sysCall2Helper on CurrentProc. CurrentProc and its children will all die*/
    sysCall2Helper(currentProc);
    /* Now that CurrentProc and it's children are dead, call scheduler to push a new process into our queue*/
    scheduler();
}

/*
	Helper function for TERMINATE PROCESS. This function does the actual job of terminating current process and its
	children. 
*/
void sysCall2Helper(pcb_PTR p) {

	/* Create temporary pointers to aid in the traversel of our tree */
    pcb_PTR tempPointer;
    int *temp;
    
    /* Check p has any children. Recursively call until we reach "the youngest" child */
    while (!emptyChild(p)) {
    	sysCall2Helper(removeChild(p));
    } /* If we broke from this loop, we have handle different cases for each kind of "child".*/
    
    /* Is p our current proccess? If it is, simply remove it*/
    if (p == currentProc) {
    	outChild(p);
    }
    
    if (p != currentProc) {
    	tempPointer = outBlocked(p);
    	
    	if (tempPointer != NULL) {
    		tempPointer = p->p_semAdd;
    		
    		if(temp >= &deviceSema4s[0] && temp <= &deviceSema4s[MAXDEVICECNT-1]) {
    			softBlockCnt--;
    		}
    	}
    }
}

/* 
	System Call 3: Preforms a "P" operation or a wait operation. The semaphore is decremented and then blocked.
	To call PASSEREN, the following C code can be used:
		
		SYSCALL(PASSEREN, int *semaddr, 0, 0);
		
    	The second parameter corresponds to register a1, and is used to obtain our semaphore.
*/
void wait(state_PTR caller)
{
	/* obtain our semaphore (sema4) from currentProc's state*/
    int sema4 = (int*) currentProc->p_s.s_a1;
    /*decrement sema4 */
    sema4--;
    /* if our semaphore is less that 0, we block it*/
    if(sema4 < 0) {
        BlockedSYS(sema4);
    }
    myLDST(caller);
}

/* 
	System Call 4: Preforms a "V" operation or a signal operation. The semaphore is incremented
	and is unblocked/placed into the ReadyQue. To call VERHOGEN, the following C code can be used:
		
		SYSCALL(VERHOGEN, int *semaddr, 0, 0);
		
    	The second parameter corresponds to register a1, and is used to obtain our semaphore.
*/
void signal(state_PTR caller)
{
	/* obtain our semaphore (sema4) from currentProc's state*/
    int sema4 = (int*) currentProc->p_s.s_a1;
    /*increment sema4 */
    sema4++;
    /*If our sema4 is less than 0, we unblock our semaphore and place the corresponding
    process into our readyQue*/
    if(sema4 <= 0) {
        pcb_t *p = removeBlocked(&(sema4));
        insertProcQ(&readyQue, p);
    }
    myLDST(caller);
}

/* 
	System Call 5: When an I/O operation is initiated, the initiating process is blocked until the I/O completes.
	WAITIO is called every time a process initiates an I/O operation. From the start of the I/O operation, the current
	process transitions from the running state into a blocked state. To call WAITIO, the following C code can be used:
		
		SYSCALL(WAITIO, int intlNo, int dnum, int waitForTermRead);
		
	The second parameter corresponds to a1, which is our interupt line number.
	The third parameter corresponds to a2, which is our device number.
	The forth parameter corresponds to a3, which indicates if we are waiting for a terminal read operation. This is
	read as either TRUE or FALSE.
*/
void Wait_for_IO_Device(state_PTR caller){
	/* Obtain nessesary values from registers a1,a2,a3.*/
    int lineNumber = currentProc->p_s.s_a1;
    int dnum = currentProc->p_s.s_a2;
    int waitForTermRead = currentProc->p_s.s_a3;
    
    /* In order to preform the nessesary P operation on our device semaphore, we obtain it...*/
    dnum = dnum + (lineNumber - DISKINT) * DEVPERINT;
    
    /*Check if our device semaphore is waiting for TRO and if it is equal to TERMINT*/
    if ((dnum == TERMINT) && (waitForTermRead)) {
    	/*If so, we increment our device semaphore by DEVPRINT*/
        dnum += DEVPERINT;
    }
    
    /* Decrement our device semaphore as expected for a P operation*/
    deviceSema4s[dnum]--;
    if (deviceSema4s[dnum] < 0) {
    	softBlockCnt++;
    	BlockedSYS(&(deviceSema4s[dnum]));
    }
    myLDST(caller);
}

/* 
	System Call 6: Requests a processes' accumulated time. This accumulated time is placed in our v0 register.
	To call GETCPUTIME, the following C code can be used:
	
		SYSCALL(GETCPUTIME, 0, 0, 0);
*/
int Get_CPU_Time(state_PTR caller){
    /* Obtain CurrentTOD clock time */
    STCK(currentTOD);
    /* Find the difference in between TOD clocks to find accumulated time IN ADDITION to currentProc's CPU time*/
    currentTOD = (currentTOD - TODStarted) + currentProc->p_time;
    /* Send up our current time to v0 */
    currentProc->p_s.s_v0 = currentTOD;
    myLDST(caller);
}

/* 
	System Call 7: Preforms a V operation on our pseudo clock semaphore. This will block our pseudo clock.
	To call WAITCLOCK, the following C code can be used:
		
		SYSCALL (WAITCLOCK, 0, 0, 0);
*/
void Wait_For_Clock(state_PTR caller){
	/*Define out pseudo clock as semPClock, which is located in the address of the 49th device semphore*/
    int semPClock = (int) &(deviceSema4s[MAXDEVICECNT-1]);
    /* Decrement this semaphore just as we would with a V operation */
    (semPClock)--;
    /* If semPClock is less than 0, block it. */
    if (semPClock < 0) {
    	softBlockCnt++;
    	BlockedSYS(&semPClock);
    }
    myLDST(caller);
}

/* 
	System Call 8: This system call obtains the support structure of the current process. To call GETSUPPORTPTR,
	the following C code can be used:
		
		support_t *sPtr = SYSCALL(GETSUPPORTPTR, 0, 0, 0);
*/
void Get_SUPPORT_Data(state_PTR caller)
{
	/* Place currentProc's support structure into v0*/
    currentProc->p_s.s_v0 = (int) currentProc->p_supportStruct;
	myLDST(caller);
}


/*
	Program Trap Exceptions are handled via passUpOrDie. Program Traps occur when a user requests a
	privileged service while in user-mode. If the support level is avaliable, this is handled in our 
	support layer.
*/
void programTRPHNDLR() {
    passUpOrDie(GENERALEXCEPT);
}

/*
	TLB TrapHandler are handled via passUpOrDie. Triggered when no matching entries are found in the TLB
	when preforming address translation. If the support level is avaliable, this is handled in our support 
	layer.
*/
void TLB_TrapHandler() {
    passUpOrDie(PGFAULTEXCEPT);
}

/* 
	PassUp Or Die: Exceptions labed 9 and above, ProgramTraps, and TLB Exceptions will all be handled in pass
	up or die, where we first check the support structure of currentProc. If a support structure is not avaliable,
	we terminate the current process and all its children. If we do find a support structre, the exception is passed
	up to the support layer. Parameter "type" refers to whether the type of exception is being handled is caused by a
	Pagefault (PGFAULTEXCEPT) or is just a general exception (GENERALEXCEPT). 
*/
void passUpOrDie(int type) {
    if(currentProc->p_supportStruct == NULL) {
        Terminate_Process();
    } else{
        passUp(type);
    }
}

/* 
	Helper function for Sys3, Sys5, and Sys7. This helper blocks the semaphore placed into parameter sema4B
	and writes currentProc's accumulated CPU time into its p_time field. After which, our  currentProc is set
	to NULL and our scheduler is called.
*/
void BlockedSYS(int *sema4B)
{
 	/* Find the accumulated CPU time and write it in currentProc's time field*/
    STCK(currentTOD);
    currentProc->p_time = (currentTOD - TODStarted) + currentProc->p_time;
    
    /* Block the sema4 */
    insertBlocked(sema4B, currentProc);
    
    /* set Current Process to NULL */
    currentProc = NULL;
    
    /* Call scheduler to get another Process going */
    scheduler();
}
/* 
	The Nucleus passes up the handing of exceptions to the Support Level. We copy the exception state from
	the BIOSDATAPAGE to a location that is accessible to the support layer (sup_exceptContext[type]). Parameter 
	"type" refers to whether the type of exception is being handled is caused by a Pagefault (PGFAULTEXCEPT) 
	or is just a general exception (GENERALEXCEPT). 
*/
void passUp(int type) {
	/*Copy the state saved in our BIOSDATAPAGE to the support exception state*/
    copyState(&currentProc->p_supportStruct->sup_exceptContext[type], (state_PTR) BIOSDATAPAGE);
    /*Load context with the fields of current process' support structure's stack pointer, status,
    and program counter, all of which was obtained from our BIOSDATAPAGE*/
    LDCXT(currentProc->p_supportStruct->sup_exceptContext[type].c_stackPtr,
    currentProc->p_supportStruct->sup_exceptContext[type].c_status,
    currentProc->p_supportStruct->sup_exceptContext[type].c_pc);
}
