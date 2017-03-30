#include "hilevel.h"


//TODO Clear up commenting
//TODO Unify Spacing and indenting
//TODO for loops use different integers
//TODO add a way of printing strings from hilevel.c?
//TODO Neaten up stack and pc shit and understand it slightly better
//TODO Give Kill some functionality
//TODO Reset occurs if you input like "fork P3m 4"

// TODO BLocking works def correctly?
// TODO Blocking Queues
// TODO Closing Pipes
// TODO IPC as in cross process communication...
// TODO Dining Philsophers
// TODO Pipes not take negative values

/* Define Process Table. taken from lab-3_q, define array of pcb_t instances for use as the process
table, one entry for each user process, with a pointer into this table
maintained so it is clear which PCB is active (the process currently
running)
* Understand *pointers
*/
#define PROCESSORS_MAX 17
#define PIPES_MAX 32
#define FD_MAX 1024
#define TIMER_PERIOD 0x00100000
pcb_t pcb[PROCESSORS_MAX], *current = NULL;
pipe_t pipe[PIPES_MAX];
int nextFreeSpace
// TODO check if this necessary
int currentProcess = 0;

// DOES CONSOLE HAVE A PRIORITY?
int chooseProcess() {
	//PL011_putc( UART0, ' ', true );
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
			//  PL011_putc( UART0, 'P', true );
			//  PL011_putc( UART0, '0'+i, true );
			//  PL011_putc( UART0, '=', true );
			//  PL011_putc( UART0, '0'+pcb[i].priority, true );
		}
	}
	pcb[max].vintage = 0;
	//PL011_putc( UART0, ' ', true );
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
		PL011_putc(UART0, '#', true); //If something has gone wrong
		PL011_putc(UART0, 'S', true); //If something has gone wrong
	}
	PL011_putc( UART0, '0'+currentProcess, true);
	PL011_putc( UART0, '0'+nextProcess, true);
	memcpy(&pcb[currentProcess].ctx, ctx, sizeof(ctx_t));
	memcpy(ctx, &pcb[nextProcess].ctx, sizeof(ctx_t)); //restore
	current = &(pcb[nextProcess]);
	currentProcess = nextProcess;



	//Call next one and link round when hitting sizeof&pcb... acll //fork when necessary

	/*for(int i = currentProcess; i < (currentProcess+PROCESSORS_MAX); i++){
	//If next space is used, switch to that space
	if(!(pcb[(i+1)%PROCESSORS_MAX].spaceAvailable)) {
	PL011_putc( UART0, '0'+currentProcess, true);
	nextUsedSpace = ((i+1)%PROCESSORS_MAX);
	PL011_putc( UART0, '0'+nextUsedSpace, true);
	memcpy(&pcb[currentProcess].ctx, ctx, sizeof(ctx_t)); //preserve P3
	memcpy(ctx, &pcb[nextUsedSpace].ctx, sizeof(ctx_t)); //restore  P3
	//current = &(pcb[i+1%sizeof(pcb)]);
	current = &(pcb[nextUsedSpace]);
	currentProcess=nextUsedSpace;
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

	TIMER0->Timer1Load  = TIMER_PERIOD; // select period = 2^20 ticks ~= 1 sec
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
	pcb[0].topOfStack = (uint32_t)(&tos_console);
	pcb[0].spaceAvailable = false;
	pcb[0].base = 1;
	pcb[0].vintage = 0;

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

//Returns either the no. of chars requested or amount of chars in buffer... Whichever is less
int minNoChars(int buflim, int n) {
	if (buflim < n) {
		n = buflim;
		// TODO change this if too frequent
		PL011_putc( UART0, '#', true );
		PL011_putc( UART0, 'r', true );
	}
	return n;
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

void hilevel_handler_svc(ctx_t* ctx, uint32_t id, fildes_t* fildes) {

	switch( id ) {
		case 0x00 : { // 0x00 => yield()
			scheduler( ctx );
			break;
		}
		case 0x01 : { // 0x01 => write( fd, x, n )
			// write n bytes from x to   the file descriptor fd; return bytes written
			int   fd = ( int   )( ctx->gpr[ 0 ] );
			char*  x = ( char* )( ctx->gpr[ 1 ] );
			int    n = ( int   )( ctx->gpr[ 2 ] );
			if (fd < 3) {

				for( int i = 0; i < n; i++ ) {
					PL011_putc( UART0, *x++, true );
				}
				//Give back n to show how many bytes were written
				ctx->gpr[ 0 ] = n;
			}
			else {
				//Get current pipe from pipe descriptor
				int currentPipe = current->fildes[fd].pipeNo - 1;
				PL011_putc( UART0, 'N', true );
				PL011_putc( UART0, '0'+currentPipe, true );
				// no. of chars to write
				int n1  = n;
				// no. of chars in buffer
				int numChar = pipe[currentPipe].noChars;
				// space left in buffer
				int spaceLeft = (BUFFER_MAX - numChar);
				// If writing more than is left in buffer...
				if (n1 > spaceLeft) {
					// Only write as much as possible and "#W" - THAT ISNT LEGIT I DONT THINK
					//n1 = spaceLeft;
					PL011_putc( UART0, '#', true );
					PL011_putc( UART0, 'W', true );
					ctx->gpr[ 0 ] = -1;
					break;
				}
				memcpy(&(pipe[currentPipe].buffer[numChar]), x, n1);
				// Update number of chars in buffer
				pipe[currentPipe].noChars = (numChar + n1);
				//Give back n to show how many bytes were written
				ctx->gpr[ 0 ] = n1;
			}
			break;

		}
		case 0x02 : { // 0x03 => read( fd, x, n)
			// read  n bytes into x from the file descriptor fd; return bytes read
			int   fd = ( int   )( ctx->gpr[ 0 ] );
			char*  x = ( char* )( ctx->gpr[ 1 ] );
			int    n = ( int   )( ctx->gpr[ 2 ] );
			if (fd < 3) {
				for( int i = 0; i < n; i++ ) {
					*x++ = PL011_getc( UART0, true );
				}
				ctx->gpr[ 0 ] = n;
			}
			else {

				// * Double check about & on memcpy with pointers
				//Get current pipe from pipe descriptor

				int currentPipe = current->fildes[fd].pipeNo - 1;
				// PL011_putc( UART0, 'N', true );
				// PL011_putc( UART0, '0'+currentPipe, true );
				//Ensure chars requested are smaller than amount in buffer, or just take as many as there is
				int n2 = minNoChars(pipe[currentPipe].noChars, n);
				if (n2 < 0 ) {
					PL011_putc( UART0, '#', true );
					PL011_putc( UART0, 'R', true );
					ctx->gpr[ 0 ] = -1;

				}
				//copy from buffer to x
				memcpy(x, &(pipe[currentPipe].buffer[0]), n2);
				//Update no. of chars
				//Move chars down the buffer
				memcpy(&(pipe[currentPipe].buffer[0]), &(pipe[currentPipe].buffer[n2]), n2);
				pipe[currentPipe].noChars = (pipe[currentPipe].noChars - n2);
				ctx->gpr[ 0 ] = n2;
			}
			break;

		}
		case 0x03 : { // 0x03 => fork()

			/* Iterate through the pcb and set nextFreeSpacehe first available space in the array */
			// THIS MIGHT BE INCORRECT
			int nextFreeSpace;
			for (int i = 0; i < PROCESSORS_MAX; i++) {
				if (pcb[i].spaceAvailable) {
					nextFreeSpace = i;
					break;
				}
			}

			//should this be 0? like eventually should be changed no?
			memset(&pcb[nextFreeSpace], 0, sizeof(pcb_t));
			memcpy(&pcb[nextFreeSpace].ctx, ctx, sizeof(ctx_t)); //Preservey
			pcb[nextFreeSpace].pid      = nextFreeSpace+1;
			pcb[nextFreeSpace].ctx.cpsr = 0x50;
			//pcb[nextFreeSpace].ctx.pc   = (uint32_t)(&main_console);
			//May be a '-'?
			//* Is the plus 1 nessary... different on exec line i believe so get them the same
			pcb[nextFreeSpace].topOfStack = (uint32_t)(&(tos_userSpace) - ((nextFreeSpace-1) * 0x00001000));
			uint32_t distance = current->topOfStack - ctx->sp;
			pcb[nextFreeSpace].ctx.sp = pcb[nextFreeSpace].topOfStack - distance;
			//Size from address, so sp not topOfStack
			memcpy((uint32_t*)pcb[nextFreeSpace].ctx.sp, (uint32_t*)ctx->sp, distance);

			ctx->gpr[0] = (nextFreeSpace + 1);
			pcb[nextFreeSpace].ctx.gpr[0] = 0;
			pcb[nextFreeSpace].spaceAvailable = false;
			break;
		}
		case 0x04 : { /* 0x04 => exit( int x )
			When called with argument '1'  set the spaceAvailable to true, so procses is no longer recognised... and calls scheduler so a new process will be automatically selected.
			Next process will replace and overwrite this one so doesn't require setting everything to 0...
			* Set argument for if int x isnt 1?
			*/
			current->spaceAvailable = true;
			scheduler(ctx);
			break;
		}

		case 0x05 : { /* 0x05 => exec( const void* x )
			This is called from the child prgram sets the pc and sp all to new location where child is, copies current context into ctx and set current to this nextFreeSpace
			* sp line maybe a bit dodge.... not totally sure how all this works
			*/
			current->ctx.pc   = ctx->gpr[0];
			current->ctx.cpsr = 0x50;
			//Is this line pointless, Aleena redefines pointless in a profound manner
			current->ctx.sp   = (uint32_t)(&(tos_userSpace) - ((current->pid-2) * 0x00001000));
			memcpy(ctx, &current->ctx, sizeof(ctx_t));
			// current = &pcb[nextFreeSpace];
			break;
		}
		case 0x07 : { /* 0x07 => setpri( int pid, int x)
			This is called from the Parent Process, as the Parent Process calls fork before the child process does... This allows the Parent to set the Child's Priority before it is executed. This means that if a high priority is chosen the process is immediately run and doesn't have to wait for exec to be called.
			*Really double check that that is the case... as can't remember the situation and why it didn't work before and may have just been a fuck up on my behalf...
			*/

			pcb[((int)ctx->gpr[0]-1)].vintage = 0;
			pcb[((int)ctx->gpr[0]-1)].base = (int)ctx->gpr[1];
			pcb[((int)ctx->gpr[0]-1)].priority = (int)ctx->gpr[1];

			//current = &pcb[nextFreeSpace];
			break;
		}
		case 0x08 : { /* 0x08 => pipes( int fd[2] ) */
			// Make pipe
			// Remember pipe index
			// 2 file descriptor in current process, both point to pipe one is read, one is write
			// Fd starts from 3


			//THINK THIS IS DONE?
			int nextPipe;
			int* fd = (int*)ctx->gpr[0];

			for (int i = 0; i < PIPES_MAX; i++) {
				if (!pipe[i].pipeActive) {
					nextPipe = i;
					break;
				}
				if (i ==PIPES_MAX-1) {
					ctx->gpr[0] = -1;
					PL011_putc( UART0, '#', true );
					PL011_putc( UART0, 'P', true );
					return;
				}
			}

			pipe[nextPipe].pipeActive = true;
			pipe[nextPipe].noChars = 0;
			pipe[nextPipe].noFds = 0;
			pipe[nextPipe].pipeID = nextPipe + 1;

			bool doneRead = false;
			bool doneBoth = false;
			for (int j = 3; j < FD_MAX; j++) {
				// Iterates though and creates two File Descriptors at next available space
				if (!(current->fildes[j].fdActive)) {
					if (doneRead) {
						//WRITE
						fd[1] = j;
						//ctx->gpr[1] = j;
						current->fildes[j].pipeNo = nextPipe+1;

						current->fildes[j].isRead = false;
						current->fildes[j].fdActive = true;
						pipe[nextPipe].noFds = pipe[nextPipe].noFds + 1;
						doneBoth = true;
						break;
					} else {
						//READ
						fd[0] = j;
						//ctx->gpr[0] = j;
						current->fildes[j].pipeNo = nextPipe+1;
						current->fildes[j].isRead = true;
						current->fildes[j].fdActive = true;
						pipe[nextPipe].noFds = pipe[nextPipe].noFds + 1;
						doneRead = true;

					}


				}

			}
			if(!doneBoth) {
				ctx->gpr[0] = -1;
				PL011_putc( UART0, '#', true );
				PL011_putc( UART0, 'F', true );

			} else {
				ctx->gpr[0] = 0;
			}
			break;
		}
		case 0x09 : {
			//Take in file descriptor Understand which one it is...
			// Remove that file descriptor
			// '-' no of file desccriptors left
			// Unactivate pipe when done
			int fd = (int)ctx->gpr[0];

			int pipeToClose = current->fildes[fd].pipeNo - 1;
			// PL011_putc( UART0, 'p', true );
			// PL011_putc( UART0, 'i', true );
			// PL011_putc( UART0, 'p', true );
			// PL011_putc( UART0, 'e', true );
			// PL011_putc( UART0, '0'+current->fildes[fd].pipeNo, true );
			if (!current->fildes[fd].fdActive) {
				PL011_putc( UART0, '#', true );
				PL011_putc( UART0, 'N', true );
				PL011_putc( UART0, 'O', true );
				PL011_putc( UART0, 'F', true );
				PL011_putc( UART0, 'D', true );
				ctx->gpr[0] = -1;

			}
			else {
				current->fildes[fd].fdActive = false;
				fd = 0;
				int numberOfFds = 0;
				for (int j = 3; j < FD_MAX; j++) {
					if ((current->fildes[j].fdActive)) {
						numberOfFds = numberOfFds + 1;
					}
				}
				pipe[pipeToClose].noFds = numberOfFds;
				if (pipe[pipeToClose].noFds == 0) {
					pipe[pipeToClose].pipeActive = false;
					PL011_putc( UART0, 'C', true );
					PL011_putc( UART0, 'h', true );
					PL011_putc( UART0, '0'+pipe[pipeToClose].noChars, true );
					// FIX THIS LINE
					memcpy(pipe[pipeToClose].buffer[0], 0, 1024/*sizeof(pipe[pipeToClose].buffer[pipe[pipeToClose].noChars])*/);
					pipe[pipeToClose].noChars = 0;
					pipe[pipeToClose].pipeID = 0;
					PL011_putc( UART0, 'P', true );
					PL011_putc( UART0, 'i', true );
					PL011_putc( UART0, 'C', true );
					PL011_putc( UART0, 'l', true );
					PL011_putc( UART0, 's', true );
					PL011_putc( UART0, 'd', true );
				}
				ctx->gpr[0] = 0;
			}

			break;
		}
		default   : { // 0x?? => unknown/unsupported
			break;
		}
	}

	return;
}
