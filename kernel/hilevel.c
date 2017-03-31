#include "hilevel.h"


//TODO Clear up commenting
//TODO Unify Spacing and indenting
//TODO for loops use different integers
//TODO add a way of printing strings from hilevel.c?
//TODO Neaten up stack and pc shit and understand it slightly better
//TODO Give Kill some functionality
//TODO Reset occurs if you input like "fork P3m 4"
//TODO improve naming

// TODO BLocking works def correctly?
// TODO Blocking Queues
// TODO Closing Pipes
// TODO IPC as in cross process communication...
// TODO Dining Philsophers
// TODO Pipes not take negative values

#define PROCESSORS_MAX 17
#define PIPES_MAX 32
#define FD_MAX 1024
#define TIMER_PERIOD 0x00100000
pcb_t pcb[PROCESSORS_MAX], *current = NULL;
pipe_t pipe[PIPES_MAX];
int nextFreeSpace;
// NOTE could this be improved... Do you need currentProcess
int currentProcess = 0;


/* Function that is called by scheduler and returns process with highest priority, taking into account its base priority and time since last execution */

int chooseProcess() {
	int max = -1;
	int max_priority = 0;
	for (size_t i = 0; i < PROCESSORS_MAX; i++) {
		if (pcb[i].alive) {
			// Initially Calculate priority
			pcb[i].priority = (pcb[i].vintage + pcb[i].base);
			if (max_priority < pcb[i].priority) { // If bigger than current max
				max = i; // Update max
				max_priority = pcb[i].priority; // Update max priority
			}
			pcb[i].vintage = pcb[i].vintage + 1; // Increase vintage regardless
		}
	}
	pcb[max].vintage = 0; // Set vintage of max to 0, resetting its age
	return max;
}


/* Scheduling Algorithm that reads in next process from the return of 'priority queue' and Context Switches from current process to the next process */

void scheduler (ctx_t* ctx) {
	PL011_putc( UART0, 's', true );

	int nextProcess = chooseProcess();
	if (nextProcess < 0) {
		PL011_putc(UART0, '#', true); //If something has gone wrong
		PL011_putc(UART0, 'S', true); //If something has gone wrong
	}
	PL011_putc( UART0, '0'+currentProcess, true);
	PL011_putc( UART0, '0'+nextProcess, true);
	memcpy(&pcb[currentProcess].ctx, ctx, sizeof(ctx_t)); // Preserve Current
	memcpy(ctx, &pcb[nextProcess].ctx, sizeof(ctx_t)); // Restore Next
	current = &(pcb[nextProcess]); // Updates current pointer
	currentProcess = nextProcess; // Updates currentProcess
	return;
}


/* Call to main_console and allocates space for console and all user program */

extern void main_console();
extern uint32_t tos_console;
extern uint32_t tos_userSpace;


/* Initialise the process table with entry for the console process */

void hilevel_handler_rst(ctx_t* ctx) {
	PL011_putc( UART0, 'R', true);
	PL011_putc( UART0, '\n', true);
	PL011_putc( UART0, '\n', true);

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
	/* Memset all pcb entries to 0 and ensure they all set to !alive. Initialise console process */

	memset(&pcb[0], 0, sizeof(pcb_t));
	for (int i = 0; i < PROCESSORS_MAX; i++) {
		pcb[i].alive = false;
	}
	pcb[0].pid      = 1; // Set PID to 1, i.e the first process
	pcb[0].ctx.cpsr = 0x50; // USR mode
	pcb[0].ctx.pc   = (uint32_t)(&main_console); // Program Counter to console
	pcb[0].ctx.sp   = (uint32_t)(&tos_console); // Stack pointer to tol_console
	pcb[0].topOfStack = (uint32_t)(&tos_console); // Record top of stack
	pcb[0].alive = true;
	pcb[0].base = 1;
	pcb[0].vintage = 0;

	/* Line below then selects a PCB entry (in this case the 0-th entry), and
	activates it: 'memcpy' copies the execution context to addresses pointed
	at by ctx. Enable IRQs. */

	current = &pcb[0]; memcpy(ctx, &current->ctx, sizeof(ctx_t));
	int_enable_irq();
	return;
}


/* Interrupt handler, that is invoked everytime an IRQ interrupt is raised and needs to be handled. In this case the timer raises the IRQ */

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


/* Returns either the no. of chars requested or amount of chars currently in the buffer: Whichever value is less */

int minNoChars(int buflim, int n) {
	if (buflim < n) {
		n = buflim;
		// TODO change this if too frequent
		// Prints if it will read less bytes than requested
		PL011_putc( UART0, '#', true );
		PL011_putc( UART0, 'r', true );
	}
	return n;
}


/* 'hilevel_handler_svc' is invoked by 'lolevel_handler_svc' every time a reset
interrupt is raised and needs to be handled. Recall that the first two
arguments:
- A pointer to the preserved execution context (on the SVC mode stack, i.e
the value of sp)
- the immediate operand of the svc instruction that raised the interrupt
being handled: this is used as the system call identifier
The function uses the system call identifier to decide how the interrupt
should be handled, and so what the kernel-facing side of the system call
API should do... */

void hilevel_handler_svc(ctx_t* ctx, uint32_t id, fildes_t* fildes) {

	switch( id ) {
		case 0x00 : { // 0x00 => yield()
			scheduler( ctx ); // Choose next program
			break;
		}
		case 0x01 : { // 0x01 => write( fd, x, n )
			// Write n bytes from x to the file descriptor fd
			int   fd = ( int   )( ctx->gpr[ 0 ] );
			char*  x = ( char* )( ctx->gpr[ 1 ] );
			int    n = ( int   )( ctx->gpr[ 2 ] );
			if (fd < 3) { // If normal write fd == 0-2
				for( int i = 0; i < n; i++ ) {
					PL011_putc( UART0, *x++, true );
				}
				ctx->gpr[ 0 ] = n; // Return number of bytes written
			}
			else { // Else if fd points to pipe and is required to write to it
				// Get current pipe from pipe descriptor
				int currentPipe = current->fildes[fd].pipeNo - 1;
				// Number of chars to write
				int n1  = n;
				// Number of chars in buffer
				int numChar = pipe[currentPipe].noChars;
				// Space left in buffer
				int spaceLeft = (BUFFER_MAX - numChar);
				// If writing more than is left in buffer...
				if (n1 > spaceLeft) {
					// If there is not enough space to right print '#W'
					PL011_putc( UART0, '#', true );
					PL011_putc( UART0, 'W', true );
					ctx->gpr[ 0 ] = -1; // Return -1 to show unsuccessful
					break;
				}
				// Copy x into the buffer
				memcpy(&(pipe[currentPipe].buffer[numChar]), x, n1);
				// Update number of chars in buffer
				pipe[currentPipe].noChars = (numChar + n1);
				ctx->gpr[ 0 ] = n1; // Return number of bytes written
			}
			break;
		}
		case 0x02 : { // 0x03 => read( fd, x, n)
			// read n bytes into x from the file descriptor fd;
			int   fd = ( int   )( ctx->gpr[ 0 ] );
			char*  x = ( char* )( ctx->gpr[ 1 ] );
			int    n = ( int   )( ctx->gpr[ 2 ] );
			if (fd < 3) {// If normal read fd == 0-2
				for( int i = 0; i < n; i++ ) {
					*x++ = PL011_getc( UART0, true );
				}
				ctx->gpr[ 0 ] = n; // Return number of bytes read
			}
			else { // Else if fd points to pipe and is required to read from it
				// Get current pipe from pipe descriptor
				int currentPipe = current->fildes[fd].pipeNo - 1;
				//Ensure chars requested are smaller than amount in buffer, or just take as many as there are
				int n2 = minNoChars(pipe[currentPipe].noChars, n);
				if (n2 < 0 ) { // If buffer empty print '#R'
					PL011_putc( UART0, '#', true );
					PL011_putc( UART0, 'R', true );
					ctx->gpr[ 0 ] = -1; // Return -1 to show unsuccessful
				}
				// Copy from buffer to x
				memcpy(x, &(pipe[currentPipe].buffer[0]), n2);
				//Move remaining chars down the buffer
				memcpy(&(pipe[currentPipe].buffer[0]), &(pipe[currentPipe].buffer[n2]), n2);
				//Update no. of chars
				pipe[currentPipe].noChars = (pipe[currentPipe].noChars - n2);
				ctx->gpr[ 0 ] = n2; // Return number of bytes read
			}
			break;
		}
		case 0x03 : { // 0x03 => fork()
			/* Iterate through the pcb and set nextFreeSpace to the next !alive process */
			int nextFreeSpace;
			for (int i = 0; i < PROCESSORS_MAX; i++) {
				if (!pcb[i].alive) {
					nextFreeSpace = i;
					break;
				}
			}
			memset(&pcb[nextFreeSpace], 0, sizeof(pcb_t)); // Set space to 0
			memcpy(&pcb[nextFreeSpace].ctx, ctx, sizeof(ctx_t)); // Copy ctx
			pcb[nextFreeSpace].pid      = nextFreeSpace+1; // Set Pid
			pcb[nextFreeSpace].ctx.cpsr = 0x50; // USR mode
			pcb[nextFreeSpace].topOfStack = (uint32_t)(&(tos_userSpace) - ((nextFreeSpace-1) * 0x00001000)); // '-' as stack goes downwards
			uint32_t distance = current->topOfStack - ctx->sp; // Distance to calculate how much to memcpy
			pcb[nextFreeSpace].ctx.sp = pcb[nextFreeSpace].topOfStack - distance; // Set stack pointer to point at bottom of memory stack
			memcpy((uint32_t*)pcb[nextFreeSpace].ctx.sp, (uint32_t*)ctx->sp, distance); // Size from address, so that sp not topOfStack

			// Copies FDs from fork
			for (int j = 3; j < FD_MAX; j++) {
				if ((current->fildes[j].fdActive)) {
					pcb[nextFreeSpace].fildes[j] = current->fildes[j];
				}
			}
			// Copies Priority if one is available
			pcb[nextFreeSpace].base = current->base-1;

			ctx->gpr[0] = (nextFreeSpace + 1); // Set parent's return pid to that of childs
			pcb[nextFreeSpace].ctx.gpr[0] = 0; // Set child's return pid to 0
			pcb[nextFreeSpace].alive = true; // Set program to alive
			break;
		}
		case 0x04 : { // 0x04 => exit( int x )
			/* Exit a process, the next process will replace and overwrite this one so doesn't require setting everything to 0... (in fork) */
			int success = (int)ctx->gpr[0];
			if (success == 0) { // When called with argument '0'
				current->alive = false; // Set alive to false
				scheduler(ctx); // Call scheduler to choose next process
			}
			else { // If exit is called with an argument that is != 0
				PL011_putc( UART0, '#', true ); // Raise '#X' for failure
				PL011_putc( UART0, 'X', true );
				PL011_putc( UART0, '0'+success, true );
			}
			break;
		}

		case 0x05 : { // 0x05 => exec( const void* x )
			/* This is called from the child prgram sets the pc and sp all to new location where child is, copies current context into ctx and set current to this nextFreeSpace */
			current->ctx.pc   = ctx->gpr[0];
			current->ctx.cpsr = 0x50;
			current->ctx.sp   = (uint32_t)(&(tos_userSpace) - ((current->pid-2) * 0x00001000));
			memcpy(ctx, &current->ctx, sizeof(ctx_t));
			// TODO do i need this current = &pcb[nextFreeSpace];
			break;
		}
		case 0x07 : { // 0x07 => setpri( int pid, int x)
			/* This is called from the Parent Process, as the Parent Process calls fork before the child process does... This allows the Parent to set the Child's Priority before it is executed. This means that if a high priority is chosen the process is immediately run and doesn't have to wait for exec to be called. */
			pcb[((int)ctx->gpr[0]-1)].vintage = 0; // Process's age since last execution
			pcb[((int)ctx->gpr[0]-1)].base = (int)ctx->gpr[1]; // Base priority
			pcb[((int)ctx->gpr[0]-1)].priority = (int)ctx->gpr[1];
			break;
		}
		case 0x08 : { // 0x08 => pipes( int fd[2] )
			/* Make Pipe, take in Fd's and return them once completed and pointing towards correct pipe */
			int nextPipe;
			int* fd = (int*)ctx->gpr[0]; // Take in File Descriptors
			for (int i = 0; i < PIPES_MAX; i++) {
				if (!pipe[i].pipeActive) {
					nextPipe = i; // Select next non active pipe as NextPipe
					break;
				}
				if (i == PIPES_MAX-1) { // If no more pipes available
					ctx->gpr[0] = -1;
					PL011_putc( UART0, '#', true );
					PL011_putc( UART0, 'P', true );
					return;
				}
			}
			pipe[nextPipe].pipeActive = true; // Set pipe as Active
			pipe[nextPipe].noChars = 0; // Pipe begins with no chars in buffer
			pipe[nextPipe].noFds = 0; // Pipe begins with no File Descriptors pointing towards it
			pipe[nextPipe].pipeID = nextPipe + 1; // Pipe Id is 1 for first pipe similar to that of process ID
			bool doneRead = false;
			bool doneBoth = false;
			for (int j = 3; j < FD_MAX; j++) {
				// Iterates though and creates two File Descriptors at next available space
				if (!(current->fildes[j].fdActive)) {
					if (doneRead) {
						// Write File Descriptor
						fd[1] = j;
						current->fildes[j].pipeNo = nextPipe+1; // FD's pipe
						current->fildes[j].isRead = false; // FD is write
						current->fildes[j].fdActive = true; // FD is now active
						// Increase number of Fd's pointing to pipe
						pipe[nextPipe].noFds = pipe[nextPipe].noFds + 1;
						doneBoth = true;
						break;
					} else {
						// Read File Descriptor
						fd[0] = j;
						current->fildes[j].pipeNo = nextPipe+1; // FD's pipe
						current->fildes[j].isRead = true; // FD is read
						current->fildes[j].fdActive = true; // FD is now active
						// Increase number of Fd's pointing to pipe
						pipe[nextPipe].noFds = pipe[nextPipe].noFds + 1;
						doneRead = true;

					}


				}

			}
			if(!doneBoth) { // If Fd havent been intialised properly
				ctx->gpr[0] = -1;
				PL011_putc( UART0, '#', true );
				PL011_putc( UART0, 'F', true );

			} else { // Return FDs
				ctx->gpr[0] = 0;
			}
			break;
		}
		case 0x09 : { // 0x09 => close( int fd )
			/* Take in file descriptor to be removed, remove it, and update number of Fds attached to the specific pipe. Once all Fds are removed from pipe, set pipe to inactive and memset the buffer of the pipe to 0 */
			int fd = (int)ctx->gpr[0]; // Take in Fd from SYS_CLOSE
			int pipeToClose = current->fildes[fd].pipeNo - 1;
			if (!current->fildes[fd].fdActive) { // If Fd is not found
				PL011_putc( UART0, '#', true );
				PL011_putc( UART0, 'N', true );
				PL011_putc( UART0, 'O', true );
				PL011_putc( UART0, 'F', true );
				PL011_putc( UART0, 'D', true );
				ctx->gpr[0] = -1; // Return -1 to show unsuccessful
			}
			else {
				current->fildes[fd].fdActive = false; // Set it to inactive
				fd = 0; // Set it to 0
				int numberOfFds = 0;
				for (int j = 3; j < FD_MAX; j++) {
					if ((current->fildes[j].fdActive)) {
						// Recalculate number of Fds associted with pipe
						numberOfFds = numberOfFds + 1;
					}
				}
				pipe[pipeToClose].noFds = numberOfFds; // Update pipe struct
				if (pipe[pipeToClose].noFds == 0) { // If no more Fds...
					pipe[pipeToClose].pipeActive = false; // Close Pipe
					PL011_putc( UART0, 'C', true );
					PL011_putc( UART0, 'h', true );
					PL011_putc( UART0, '0'+pipe[pipeToClose].noChars, true );
					// Set buffer back to 0s
					memset(&(pipe[pipeToClose].buffer[0]), 0, 1024);
					pipe[pipeToClose].noChars = 0; // Set number of chars to 0
					pipe[pipeToClose].pipeID = 0; // Set Pipe ID back to 0
					PL011_putc( UART0, 'P', true ); // Success of Closing Pipe
					PL011_putc( UART0, 'i', true );
					PL011_putc( UART0, 'C', true );
					PL011_putc( UART0, 'l', true );
					PL011_putc( UART0, 's', true );
					PL011_putc( UART0, 'd', true );
				}
				ctx->gpr[0] = 0; // Success of Removing Fd
			}
			break;
		}
		default   : { // 0x?? => unknown/unsupported
			break;
		}
	}
	return;
}
