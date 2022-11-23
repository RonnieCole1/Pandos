#include "../h/pcb.h"
#include "../h/asl.h"
#include "../h/types.h"
#include "../h/const.h"
#include "../h/exceptions.h"
#include "../h/scheduler.h"
#include "../h/interrupts.h"
#include "../h/initial.h"
#include "/usr/include/umps3/umps/libumps.h"

/* ---------------- External Global Variables ---------------- */
extern int processCnt;
extern int softBlockCnt;
extern pcb_t *readyQue;
extern pcb_t *currentProc;
extern int deviceSema4s[MAXDEVICECNT];
extern cpu_t TODStarted;
extern cpu_t currentTOD;

/* ---------------- External Helper Functions ---------------- */
extern int getDeviceSemaphore(int dnum, int linenum);
extern cpu_t getAccumulatedTime(cpu_t current, cpu_t started);

/* ---------------- Helper Functions ---------------- */
HIDDEN void interruptChecker(unsigned int causeOfInterrupt);
HIDDEN void returnControltoCurrentProc();
HIDDEN void deviceInterruptsHNDLER(int interruptedDeviceType);
HIDDEN void PLTHandler();
HIDDEN void interruptChecker(causeOfInterrupt);
HIDDEN int terminalInterruptHNDLER(volatile devregarea_t *devReg);
HIDDEN void PseudoHandler();

/* ---------------- Global Variables ---------------- */
int dsema4;
cpu_t timeLeft;
unsigned int causeOfInterrupt;
unsigned int status;




/************************************************************************
	From genExceptionHandler, we arive at interruptHandler() when the value
	of our Cause.ExeCode is 0. Global variables are initialized and we check
	the following:
		
		1. If our interrupt was caused by our timers.
		
		2. If we have a device interrupt.
		
	We handle each whichever interrupt we've recieved and return to our
	current process.
************************************************************************/
void interruptHandler() {
	
/* ---------------- Set Global Variables ---------------- */
	/* Pull the cause register from our BIOSDATAPAGE */
	causeOfInterrupt = ((state_PTR) BIOSDATAPAGE)->s_cause;
	/* Obtain the Timer value from CPU0 */
	timeLeft = getTIMER();
	/* Initialize the device semaphore to 0 */
	dsema4 = 0;
	/* Load TOD clock onto currentTOD */
	STCK(currentTOD);
	/* Initialize status to 0. We will use this later */
	status = 0;
	
/* ---------------- Check Interrupt Type ---------------- */	
	/* Input the value of our cause register and check if the type
	of interrupt we encountered was the result of our Timeslice expiring or
	our Pseudo Clock. If our timers are not to blame,
	we now check if and what devices may have caused the interrupt. 
	These are our "non-timer interrupts". When we discover which device/timer
	is to blame, we pass the responsibility of handling these interrupts to
	their respective helper functions. */
	interruptChecker(causeOfInterrupt);	
	
/* ----------- Return Control to Current Process ----------- */	
	/* After checking and preforming whatever operations from either
	handler, we want to return to our current process. */
	if(currentProc != NULL) {
		returnControltoCurrentProc();
	} else {
		/* If we've arrived here, then we have no current process
		to return to. Wait! */
		WAIT();
	}
}




/***********************************************************************
	Based on the cause register of our BIOSDATAPAGE and
	a select handful of constants, we determine which device/timer
	caused our interrupt.
************************************************************************/
void interruptChecker(unsigned int causeOfInterrupt) {
/* ----------- Timer Interrupt Checkers ----------- */	
	/* If our PLT Timer is to blame, pass to PLT Handler*/
	if((causeOfInterrupt & PLTINT) != 0) {
		PLTHNDLER();
	}
	/* If our Pseudo/Interval Timer is to blame, pass to Pseudo Handler*/
	if((causeOfInterrupt & PSUINT) != 0) {
		PseudoHNDLER();
	}

/* ----------- Device Interrupt Checkers ----------- */	
	/* If our Disk is to blame, pass to device Interrupts Handler
	with the parameter "DISK"*/
	if((causeOfInterrupt & DISKINT) != 0) {
		deviceInterruptsHNDLER(DISK);
	}	
	/* If our Flash is to blame, pass to device Interrupts Handler
	with the parameter "FLASH"*/
	if((causeOfInterrupt & FLASHINT) != 0) {
		deviceInterruptsHNDLER(FLASH);
	}	
	/* If our Printer is to blame, pass to device Interrupts Handler
	with the parameter "PRINTER"*/
	if((causeOfInterrupt & PRNTINT) != 0) {
		deviceInterruptsHNDLER(PRINTER);
	}	
	/* If our Terminal is to blame, pass to device Interrupts Handler
	with the parameter "TERMINAL"*/
	if((causeOfInterrupt & TERMINT) != 0) {
		deviceInterruptsHNDLER(TERMINAL);
	}
}




/***********************************************************************
	Return control to the current process. Assuming there is a
	current process.
************************************************************************/
void returnControltoCurrentProc() {
	/* Set current process's time to the time we accumulated since
	we started handling this interrupt*/
	currentProc->p_time = getAccumulatedTime(currentTOD, TODStarted);
	/* Copy over our state from the BIOSDATAPAGE to our current process's
	state */
	copyState(&(currentProc->p_s),(state_PTR) BIOSDATAPAGE);
	/* Store the time of currentProc's start in TODstarted*/
	STCK(TODStarted);
	/* Obtain the Timer value from CPU0 */
	setTIMER(timeLeft);
	/* Load the state of CurrentProc */
    	myLDST(&(currentProc->p_s));
}




/***********************************************************************
	When handling device interrupts, there are a number of steps we
	take. We calculate the address for the interrupted device's
	device register. We find the device number of the device causing
	the interrupt. We obtain the device's semaphore.
	
	If this was a terminal device interrupt, there is an additonal step
	that is handled in another helper function. In either case, we
	save the status code from this device and preform a V operation
	on the device's semaphore. We then return the status code in V0.
************************************************************************/
void deviceInterruptsHNDLER(int interruptedDeviceType) {

/* ---------------- Local Variables ---------------- */
	unsigned int bitMap;
	int dnum;
	volatile devregarea_t *devReg;
	int dINTNUM;
	pcb_PTR temp;
	
/* ---------------- Set Local Variables ---------------- */
	/* Initialize dnum to 0 */
	dnum = 0;
	/* Set devReg to RAMBASEADDR in preperation for our bitMap */
	devReg = (devregarea_t*) RAMBASEADDR;
	/* dINTNUM will be the device's index in interrupt_dev */
	dINTNUM = (interruptedDeviceType - DISK);
	/* Finally set our bitMap */
	bitMap = devReg->interrupt_dev[dINTNUM];
	/* Used for V'ing our device semaphore. Start at NULL */
	temp = NULL;
	
	
/* ---------------- Find Device Number ---------------- */
	/* Based on the value of bitMap and the address of
	the device, we check if it is on. If it is, we have
	found our device. */
	if((bitMap & D0) != 0){
        	dnum = 0;
    	}else if((bitMap & D1) != 0){
        	dnum = 1;
    	}else if((bitMap & D2) != 0){
        	dnum = 2;
    	}else if((bitMap & D3) != 0){
        	dnum = 3;
    	}else if((bitMap & D4) != 0){
        	dnum = 4;
    	}else if((bitMap & D5) != 0){
        	dnum = 5;
    	}else if((bitMap & D6) != 0){
        	dnum = 6;
    	}else{
        	dnum = 7;
    	}
/* -------------------------------------------------- */	

	/* Calculate the address for this device semaphore */
	dsema4 = getDeviceSemaphore(dnum, interruptedDeviceType);
	
	/* Check if the interrupted device was Terminal */
	if(interruptedDeviceType == TERMINAL){
		/* If it was, we obtain our status code from
		our helper function */
		status = terminalInterruptHNDLER(devReg);
	}
	/* If it this was not a Terminal interrupt...*/
	if(interruptedDeviceType != TERMINAL){
		/* set our status code to our device's status*/
		status = (devReg->devreg[dsema4]).d_status;
		/* Acknowledge the device! */
        	devReg->devreg[dsema4].d_command = ACK;
	}
	
	
	deviceSema4s[dsema4] += 1;
        
        /* V our device semaphore, and place our status code into v0*/
        if(deviceSema4s[dsema4] <= 0){
        	temp = removeBlocked(&(deviceSema4s[dsema4]));
        	temp->p_s.s_v0 = status;
        	insertProcQ(&readyQue, temp);
        	--softBlockCnt;
    	}
	
	/* If our current process is NULL, we need to call our scheduler
	and get a new one going */
    	if(currentProc == NULL){
       		scheduler();
    	}
}




/***********************************************************************
	
************************************************************************/
void PLTHNDLER() {
	
	currentProc->p_time = getAccumulatedTime(currentTOD, TODStarted);
	
	copyState(&(currentProc->p_s), (state_PTR) BIOSDATAPAGE);
	
	insertProcQ(&readyQue, currentProc);
	
	scheduler();
}




/***********************************************************************
	
************************************************************************/
void PseudoHNDLER() {

	pcb_PTR process;
	
	LDIT(PCLOCKTIME);
        process = removeBlocked(&(deviceSema4s[MAXDEVICECNT-1]));
        
        while(process != NULL){
            insertProcQ(&readyQue, process);
            
            process = removeBlocked(&(deviceSema4s[MAXDEVICECNT-1]));
            softBlockCnt--;
            
        }
}




/***********************************************************************
	
************************************************************************/
int terminalInterruptHNDLER(volatile devregarea_t *devReg) {
	if((devReg->devreg[dsema4].d_data0 & 0x0F) != READY){
            status = devReg->devreg[dsema4].d_data0;
            devReg->devreg[dsema4].d_data1 = ACK;
        } else{
            status = devReg->devreg[dsema4].d_status;
            devReg->devreg[dsema4].d_command = ACK;
            dsema4 = dsema4 + DEVPERINT;
        }
}

    
    
      
/***********************************************************************
	Parameters are pointers to both a "paste" state and a "copy"
	state. The fields from "copy" are pasted into the "paste" state
	aswell as all of the associated registers with that state.
************************************************************************/
void copyState(state_PTR paste, state_PTR copy) {
    int i;
    copy->s_entryHI = paste->s_entryHI;
    copy->s_cause = paste->s_cause;
    copy->s_status = paste->s_status;
    copy->s_pc = paste->s_pc;
    for (i = 0; i < STATEREGNUM; i++) {
        copy->s_reg[i] = paste->s_reg[i];
    }
}
