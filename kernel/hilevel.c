#include "hilevel.h"




/* Define Process Tabel. taken from lab-3_q, define array of pcb_t instances for use as the process
table, one entry for each user process, with a pointer into this table
maintained so it is clear which PCB is active (the process currently
running)
* Understand *pointers
*/
#define PROCESSORS_MAX 17
//#define TIMER_PERIOD 0x00100000
pcb_t pcb[PROCESSORS_MAX], *current = NULL;
int nextFreeSpace;
int currentUsedSpace = 0;
int nextUsedSpace;



/* Iterate through the pcb and return the index of the first available space in the array */
int getNextSpace() {
	for (int i = 0; i < PROCESSORS_MAX; i++) {
		if (pcb[i].spaceAvailable) {
			return i;
		}
	}
}

// DOES CONSOLE HAVE A PRIORITY?
int chooseProcess() {
	PL011_putc( UART0, ' ', true );
	int max = -1;
	int max_priority = 0;
	for (size_t i = 0; i < PROCESSORS_MAX; i++) {
		if (!(pcb[i].spaceAvailable)) {
			//Initially calculate priority
			pcb[i].priority = (pcb[i].vintage + pcb[i].base);

			if (max_priority < pcb[i].priority) {
				//If bigger than current max, replace it, and set vintage to 0
				max = i;
				max_priority = pcb[i].priority;
			}
			pcb[i].vintage = pcb[i].vintage + 1;
			 PL011_putc( UART0, 'P', true );
			 PL011_putc( UART0, '0'+i, true );
			 PL011_putc( UART0, '=', true );
			 PL011_putc( UART0, '0'+pcb[i].priority, true );
		}
	}
	pcb[max].vintage = 0;
	PL011_putc( UART0, ' ', true );
	return max;
}

/* Scheduler function (algorithm). Currently special purpose for 3 user
programs. It checks which process is active, and performs a context
switch to suspend it and resume the next one in a simple round-robin
scheduling. 'Memcpy' is used to copy the associated execution contexts
into place, before updating 'current' to refelct new active PCB.
* Understand &pointers
* PCB?
*/



//Check if process is alive and wants to be run

void scheduler (ctx_t* ctx) {
	PL011_putc( UART0, 's', true );

	int nextProcess = chooseProcess();
	if (nextProcess < 0) {
		PL011_putc(UART0, '#', true);
	}
	PL011_putc( UART0, '0'+currentUsedSpace, true);
	PL011_putc( UART0, '0'+nextProcess, true);
	memcpy(&pcb[currentUsedSpace].ctx, ctx, sizeof(ctx_t));
	memcpy(ctx, &pcb[nextProcess].ctx, sizeof(ctx_t)); //restore
	current = &(pcb[nextProcess]);
	currentUsedSpace = nextProcess;



	//Call next one and link round when hitting sizeof&pcb... acll //fork when necessary

		/*for(int i = currentUsedSpace; i < (currentUsedSpace+PROCESSORS_MAX); i++){
			//If next space is used, switch to that space
			if(!(pcb[(i+1)%PROCESSORS_MAX].spaceAvailable)) {
				PL011_putc( UART0, '0'+currentUsedSpace, true);
				nextUsedSpace = ((i+1)%PROCESSORS_MAX);
				PL011_putc( UART0, '0'+nextUsedSpace, true);
				memcpy(&pcb[currentUsedSpace].ctx, ctx, sizeof(ctx_t)); //preserve P3
				memcpy(ctx, &pcb[nextUsedSpace].ctx, sizeof(ctx_t)); //restore  P3
				//current = &(pcb[i+1%sizeof(pcb)]);
				current = &(pcb[nextUsedSpace]);
				currentUsedSpace=nextUsedSpace;
				break;
			}
		}*/

	return;
}

/* These are the main function for each user program
Extern allows calls to functions from header files
tos_P3/4/5 function is called in image.ld and allocates space for functions
*/


extern void main_console();
extern uint32_t tos_console;
extern uint32_t tos_userSpace;


/* Initialise the process table by copying information into two PCBs, one for
each user program; in each case the PCB is first zero'd by memset and then
components such as the program counter value are initialised (to the address
of the entry point, e.g main_P3)
*/

void hilevel_handler_rst(ctx_t* ctx) {
	PL011_putc( UART0, 'R', true);
	PL011_putc( UART0, '\n', true);
	PL011_putc( UART0, '\n', true);


	// Timer stuff

	TIMER0->Timer1Load  = 0x00100000; // select period = 2^20 ticks ~= 1 sec
	TIMER0->Timer1Ctrl  = 0x00000002; // select 32-bit   timer
	TIMER0->Timer1Ctrl |= 0x00000040; // select periodic timer
	TIMER0->Timer1Ctrl |= 0x00000020; // enable          timer interrupt
	TIMER0->Timer1Ctrl |= 0x00000080; // enable          timer

	GICC0->PMR          = 0x000000F0; // unmask all            interrupts
	GICD0->ISENABLER1  |= 0x00000010; // enable timer          interrupt
	GICC0->CTLR         = 0x00000001; // enable GIC interface
	GICD0->CTLR         = 0x00000001; // enable GIC distributor


	/* Initialise PCBs... CPSR value of 0x50 means the processor is switched 	into the USR mode, with IRQ interrupts enabled, and the PC and SP values match the entry point and top of stack.
	*/


	memset(&pcb[0], 0, sizeof(pcb_t));
	for (int i = 0; i < PROCESSORS_MAX; i++) {
		pcb[i].spaceAvailable = true;
	}
	pcb[0].pid      = 1;
	pcb[0].ctx.cpsr = 0x50;
	pcb[0].ctx.pc   = (uint32_t)(&main_console);
	pcb[0].ctx.sp   = (uint32_t)(&tos_console);
	pcb[0].spaceAvailable = false;
	pcb[0].base = 1;
	pcb[0].vintage = 0;

	//
	// memset(&pcb[0], 0, sizeof(pcb_t));
	// pcb[0].pid      = 1;
	// pcb[0].ctx.cpsr = 0x50;
	// pcb[0].ctx.pc   = (uint32_t)(&main_P3);
	// pcb[0].ctx.sp   = (uint32_t)(&tos_P3);
	//
	// memset(&pcb[1], 0, sizeof(pcb_t));
	// pcb[1].pid      = 2;
	// pcb[1].ctx.cpsr = 0x50;
	// pcb[1].ctx.pc   = (uint32_t)(&main_P4);
	// pcb[1].ctx.sp   = (uint32_t)(&tos_P4);
	//
	// memset(&pcb[2], 0, sizeof(pcb_t));
	// pcb[2].pid      = 3;
	// pcb[2].ctx.cpsr = 0x50;
	// pcb[2].ctx.pc   = (uint32_t)(&main_P5);
	// pcb[2].ctx.sp   = (uint32_t)(&tos_P5);

	/* Line below then selects a PCB entry (in this case the 0-th entry), and
	activates it: 'memcpy' copies the execution context to addresses pointed
	at by ctx (as allocated in 'lolevel_handler_rst'). Note that the specific
	value used to initialise CPSR means each process will execute in USR
	mode, with IRQ interrupts enabled. As such, there is no explicit call to
	int_enable_irq as was required in worksheet #2...
	*/

	current = &pcb[0]; memcpy(ctx, &current->ctx, sizeof(ctx_t));
	int_enable_irq();
	return;
}


//Understand and rewrite comments
void hilevel_handler_irq(ctx_t* ctx) {

	// Step 2: read the interrupt identifier so we know the source.

	uint32_t id = GICC0->IAR;

	// Step 4: handle the interrupt, then clear (or reset) the source.

	if( id == GIC_SOURCE_TIMER0 ) {
		PL011_putc( UART0, 'T', true ); TIMER0->Timer1IntClr = 0x01;
		scheduler(ctx);

	}

	// Step 5: write the interrupt identifier to signal we're done.

	GICC0->EOIR = id;

	return;
}

/* 'hilevel_handler_svc' is invoked by 'lolevel_handler_svc' every time a reset
interrupt is raised and needs to be handled. Recall that it is passed two
arguments:
- A pointer to the preserved execution context (on the SVC mode stack, i.e
the value of sp)
- the immediate operand of the svc instruction that raised the interrupt
being handled: this is used as the system call identifier
The function uses the system call identifier to decide how the interrupt
should be handled, and so what the kernel-facing side of the system call
API should do...
The first case 0x00 deals with the yield call: when one of the user processes invokes yield as defined in libc.c, the resulting supervisor call interrupt is evetually handled by this case. The semantics of this system call are that the process scheduler should be invoked, i.e, that the currently executing user process has yielded control of the processor (thus permiiting the other to execute).
The second case 0x01 deals with the write call: when one of the user processes invokes write as defined in libc.c, the resulting supervisor call interrupt is eventually handled by this case. The semantics of this system call are that some n-element sequence of bytes pointed to by x are written to file descriptor fd; the kernel ignores fd, and simply writes those bytes to the PL011_t instance UART0.
* Third case read...
* uint32_t id.. is it the id of the current program?
*/

void hilevel_handler_svc(ctx_t* ctx, uint32_t id) {

	switch( id ) {
		case 0x00 : { // 0x00 => yield()
			scheduler( ctx );
			break;
		}
		case 0x01 : { // 0x01 => write( fd, x, n )
			int   fd = ( int   )( ctx->gpr[ 0 ] );
			char*  x = ( char* )( ctx->gpr[ 1 ] );
			int    n = ( int   )( ctx->gpr[ 2 ] );

			for( int i = 0; i < n; i++ ) {
				PL011_putc( UART0, *x++, true );
			}

			ctx->gpr[ 0 ] = n;
			break;
		}
		case 0x03 : { // 0x04 => fork()
			nextFreeSpace = getNextSpace();

			//should this be 0? like eventually should be changed no?
			memset(&pcb[nextFreeSpace], 0, sizeof(pcb_t));
			memcpy(&pcb[nextFreeSpace].ctx, ctx, sizeof(ctx_t)); //Preservey
			pcb[nextFreeSpace].pid      = nextFreeSpace+1;
			pcb[nextFreeSpace].ctx.cpsr = 0x50;
			//pcb[nextFreeSpace].ctx.pc   = (uint32_t)(&main_console);
			//May be a '-'?
			pcb[nextFreeSpace].ctx.sp   = (uint32_t)(&(tos_userSpace) + ((nextFreeSpace+1) * 0x00001000));
			pcb[nextFreeSpace].spaceAvailable = false;

			ctx->gpr[0] = (nextFreeSpace + 1);
			pcb[nextFreeSpace].ctx.gpr[0] = 0;
			break;
		}
		case 0x04 : {
			/* Perform normal termination, invoke call-backs, flush then close
			open files, pass exit status to parent process (via wait)... in lib.c
			it takes in int x, assign it to r0, make system call exit then return
			*/
			// it says exit ( EXIT_SUCCESS ) which == 0
			//Want to return pc to previous Program
			// * WHY does 0 matter?
			pcb[nextFreeSpace].spaceAvailable = true;
			scheduler(ctx);

			break;
		}

		case 0x05 : {
			//Program Counter line and something else
			//uint32_t address = (ctx->gpr[0]);
			//ctx->pc = address;
			current->ctx.pc   = ctx->gpr[0];
			//pcb[nextFreeSpace].vintage = 0;
			//(int) might be unnessary...
			//pcb[nextFreeSpace].base = (int)ctx->gpr[1];
			//pcb[nextFreeSpace].priority = (int)ctx->gpr[1];
			current->ctx.cpsr = 0x50;
			//Is this line pointless, Aleena redefines pointless in a profound manner
			current->ctx.sp   = (uint32_t)(&(tos_userSpace) + ((current->pid) * 0x00001000));
			memcpy(ctx, &current->ctx, sizeof(ctx_t));
			// PL011_putc( UART0, 'P', true );
			// PL011_putc( UART0, 'r', true );
			// PL011_putc( UART0, 'i', true );
			// PL011_putc( UART0, '0'+pcb[nextFreeSpace].base, true );
			current = &pcb[nextFreeSpace];
			break;
		}
		case 0x07 : {
			//Program Counter line and something else
			//uint32_t address = (ctx->gpr[0]);
			//ctx->pc = address;
			//(int) might be unnessary...
			PL011_putc( UART0, 'S', true );
			PL011_putc( UART0, 'E', true );
			PL011_putc( UART0, 'T', true );
			pcb[((int)ctx->gpr[0]-1)].vintage = 0;
			pcb[((int)ctx->gpr[0]-1)].base = (int)ctx->gpr[1];
			pcb[((int)ctx->gpr[0]-1)].priority = (int)ctx->gpr[1];

			//memcpy(ctx, &current->ctx, sizeof(ctx_t));
			 PL011_putc( UART0, 'P', true );
			 PL011_putc( UART0, 'r', true );
			 PL011_putc( UART0, 'i', true );
			 PL011_putc( UART0, '0'+((int)ctx->gpr[0]), true );
			 PL011_putc( UART0, '0'+pcb[((int)ctx->gpr[0]-1)].priority, true );

			 //current = &pcb[nextFreeSpace];
			break;
		}
		//
		default   : { // 0x?? => unknown/unsupported
			break;
		}
	}

	return;
}
