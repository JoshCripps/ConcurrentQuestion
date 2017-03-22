#include "hilevel.h"




/* Define Process Tabel. taken from lab-3_q, define array of pcb_t instances for use as the process
  table, one entry for each user process, with a pointer into this table
  maintained so it is clear which PCB is active (the process currently
  running)
  * Understand *pointers
*/

pcb_t pcb[3], *current = NULL;

/* Scheduler function (algorithm). Currently special purpose for 3 user
   programs. It checks which process is active, and performs a context
   switch to suspend it and resume the next one in a simple round-robin
   scheduling. 'Memcpy' is used to copy the associated execution contexts
   into place, before updating 'current' to refelct new active PCB.
   * Understand &pointers
   * PCB?
*/

void scheduler (ctx_t* ctx) {
	if (current == &pcb[0]) {
		memcpy(&pcb[0].ctx, ctx, sizeof(ctx_t)); //preserve P3
        memcpy(ctx, &pcb[1].ctx, sizeof(ctx_t)); //restore  P3
        current = &pcb[1];
    }
    else if (current == &pcb[1]) {
		memcpy(&pcb[1].ctx, ctx, sizeof(ctx_t)); //preserve P4
        memcpy(ctx, &pcb[2].ctx, sizeof(ctx_t)); //restore  P4
        current = &pcb[2];
    }
    else if (current == &pcb[2]) {
		memcpy(&pcb[2].ctx, ctx, sizeof(ctx_t)); //preserve P5
        memcpy(ctx, &pcb[0].ctx, sizeof(ctx_t)); //restore  P5
        current = &pcb[0];
    }
    return;
}

/* These are the main function for each user program
   Extern allows calls to functions from header files
   tos_P3/4/5 function is called in image.ld and allocates space for functions
*/


extern void main_console();
extern void main_P3();
extern uint32_t tos_P3;
extern void main_P4();
extern uint32_t tos_P4;
extern void main_P5();
extern uint32_t tos_P5;

/* Initialise the process table by copying information into two PCBs, one for
   each user program; in each case the PCB is first zero'd by memset and then
   components such as the program counter value are initialised (to the address
   of the entry point, e.g main_P3)
*/

void hilevel_handler_rst(ctx_t* ctx) {


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
    pcb[0].pid      = 1;
    pcb[0].ctx.cpsr = 0x50;
    pcb[0].ctx.pc   = (uint32_t)(&main_P3);
    pcb[0].ctx.sp   = (uint32_t)(&tos_P3);

    memset(&pcb[1], 0, sizeof(pcb_t));
    pcb[1].pid      = 2;
    pcb[1].ctx.cpsr = 0x50;
    pcb[1].ctx.pc   = (uint32_t)(&main_P4);
    pcb[1].ctx.sp   = (uint32_t)(&tos_P4);

    memset(&pcb[2], 0, sizeof(pcb_t));
    pcb[2].pid      = 3;
    pcb[2].ctx.cpsr = 0x50;
    pcb[2].ctx.pc   = (uint32_t)(&main_P5);
    pcb[2].ctx.sp   = (uint32_t)(&tos_P5);

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
	  scheduler(ctx);
      PL011_putc( UART0, 'T', true ); TIMER0->Timer1IntClr = 0x01;

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
      case 0x02 : { // 0x02 => read( fd, x, n )
      int   fd = ( int   )( ctx->gpr[ 0 ] );
      char*  x = ( char* )( ctx->gpr[ 1 ] );
      int    n = ( int   )( ctx->gpr[ 2 ] );

      for( int i = 0; i < n; i++ ) {
        *x++ = PL011_getc( UART0, true );

      }


      ctx->gpr[ 0 ] = n;
      break;
    }

      default   : { // 0x?? => unknown/unsupported
        break;
      }
    }

    return;
}
