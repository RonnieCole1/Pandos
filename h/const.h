#ifndef CONSTS
#define CONSTS

/**************************************************************************** 
 *
 * This header file contains utility constants & macro definitions.
 * 
 ****************************************************************************/

/* Hardware & software constants */
#define PAGESIZE		    4096			/* page size in bytes	*/
#define WORDLEN			    4				  /* word size in bytes	*/

/* define MAXPROC */
#define MAXPROC             20

/* define MAXINT */
#define MAXINT              ((void *)0xFFFFFFFF)

/* define milliseconds */
#define MILLI               100000

/* define CPU burst time */
#define TIMESLICE 		    5000			

/* define MAXDEVICECNT */
#define MAXDEVICECNT        49

/* define top of the Nucleus stack page */ 
#define KERNALSTACK         0x20001000

/* cause register commands */
#define GETEXECCODE         0x0000007C
#define CAUSESHIFT          2

#define TBITS			0x0F

#define REGCONST            0x10000054
#define DEVCONST            0x1000002C

/* status register commands */
#define ALLOFF              0x0
#define USERPON             0x00000008
#define IEPON               0x00000004
#define IEMON               0x00000001
#define IMON                0x0000FF00
#define TEBITON             0x08000000
#define USERMOFF 	    0x00000002

/* timer, timescale, TOD-LO and other bus regs */
#define RAMBASEADDR		    0x10000000
#define RAMBASESIZE		    0x10000004
#define TODLOADDR		    0x1000001C
#define INTERVALTMR		    0x10000020	
#define TIMESCALEADDR	    0x10000024
#define DISABLE             0xFFFFFFFF

/* utility constants */
#define	TRUE			    1
#define	FALSE			    0
#define HIDDEN			    static
#define EOS				    '\0'
#define SUCCESS             0
#define FAILURE             -1

#define NULL 			    ((void *)0xFFFFFFFF)

/* device interrupts */
#define DISKINT			    3
#define FLASHINT 		    4
#define NETWINT 		    5
#define PRNTINT 		    6
#define TERMINT			    7

#define DEVINTNUM		    5		    /* interrupt lines used by devices */
#define DEVPERINT		    8		    /* devices per interrupt line */
#define DEVREGLEN		    4		    /* device register field length in bytes, and regs per dev */	
#define DEVREGSIZE	        16          /* device register size in bytes */

/* device register field number for non-terminal devices */
#define STATUS			    0
#define COMMAND			    1
#define DATA0			    2
#define DATA1			    3

/* device register field number for terminal devices */
#define RECVSTATUS  	    0
#define RECVCOMMAND 	    1
#define TRANSTATUS  	    2
#define TRANCOMMAND 	    3

/* device and line number bits on */
#define FIRST               0x1
#define SECOND              0x2
#define THIRD               0x4
#define FOURTH              0x8
#define FIFTH               0x10
#define SIXTH               0x20
#define SEVENTH             0x40
#define EIGHTH              0x80

/* start of interrupt device bitmap and registers */


/* device common STATUS codes */
#define UNINSTALLED		    0
#define READY			    1
#define BUSY			    3

/* device common COMMAND codes */
#define RESET			    0
#define ACK				    1

/* Memory related constants */
#define KSEG0               0x00000000
#define KSEG1               0x20000000
#define KSEG2               0x40000000
#define KUSEG               0x80000000
#define RAMSTART            0x20000000
#define BIOSDATAPAGE        0x0FFFF000
#define	PASSUPVECTOR	    0x0FFFF900


/* Exceptions related constants */
#define	PGFAULTEXCEPT	    0
#define GENERALEXCEPT	    1
#define INTERRUPTHANDLER    0
#define TLBEXCEPTS          3
#define SYSCALLEXECPTS      8

/* SYSCALL related constaints */
#define CREATEPROCESS       1
#define TERMINATEPROCESS    2
#define PASSEREN            3
#define VERHOGEN            4
#define WAITIO              5
#define GETCPUTIME          6
#define WAITCLOCK           7
#define GETSUPPORTPRT       8

/* TLB Refill related constants*/
#define HIGHENTRY           0x80000000
#define LOWENTRY            0x00000000
#define TLBSTATE            0x0FFFF000


/* operations */
#define	MIN(A,B)		((A) < (B) ? A : B)
#define MAX(A,B)		((A) < (B) ? B : A)
#define	ALIGNED(A)		(((unsigned)A & 0x3) == 0)

/* Psuedo Clock Time*/
#define PCLOCKTIME 100000

/* Macro to load the Interval Timer */
#define LDIT(T)	((* ((cpu_t *) INTERVALTMR)) = (T) * (* ((cpu_t *) TIMESCALEADDR))) 

/* Macro to read the TOD clock */
#define STCK(T) ((T) = ((* ((cpu_t *) TODLOADDR)) / (* ((cpu_t *) TIMESCALEADDR))))

#endif
