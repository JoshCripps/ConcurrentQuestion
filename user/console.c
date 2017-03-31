#include "console.h"

/* The following functions are special-case versions of a) writing,
 * and b) reading a string from the UART (the latter case returning
 * once a carriage return character has been read, or an overall
 * limit reached).
 */
const void* addr;


void puts( char* x, int n ) {
  for( int i = 0; i < n; i++ ) {
    PL011_putc( UART1, x[ i ], true );
  }
}

void gets( char* x, int n ) {
  for( int i = 0; i < n; i++ ) {
    x[ i ] = PL011_getc( UART1, true );

    if( x[ i ] == '\x0A' ) {
      x[ i ] = '\x00'; break;
    }
  }
}

/* Since we lack a *real* loader (as a result of lacking a storage
 * medium to store program images), the following approximates one:
 * given a program name, from the set of programs statically linked
 * into the kernel image, it returns a pointer to the entry point.
 */

extern void main_P3();
extern void main_P4();
extern void main_P5();
extern void main_P6();

void* load( char* x ) {
  if     ( 0 == strcmp( x, "P3" ) ) {
    return &main_P3;
  }
  else if( 0 == strcmp( x, "P4" ) ) {
    return &main_P4;
  }
  else if( 0 == strcmp( x, "P5" ) ) {
    return &main_P5;
  }
  else if( 0 == strcmp( x, "P6" ) ) {
    return &main_P6;
  }


  return NULL;
}

/* The behaviour of the console process can be summarised as an
 * (infinite) loop over three main steps, namely
 *
 * 1. write a command prompt then read a command,
 * 2. split the command into space-separated tokens using strtok,
 * 3. execute whatever steps the command dictates.
 */

 int getPriority( char* x ) {
     if (x == NULL) {
         return 1;
     }
    int y = atoi( x );

    if (y < 0) {
        puts( "Giving priority 1 as input was not recognised.\n", 47);
        return 1;

    }
    else return y;
}

void main_console() {
  char* p, x[ 1024 ];
  PL011_putc( UART0, 'c', true );

  while( 1 ) {
    puts( "shell$ ", 7 ); gets( x, 1024 ); p = strtok( x, " " );

    if     ( 0 == strcmp( p, "fork" ) ) {
      pid_t pid = fork();

      if( 0 == pid ) {
        // child
        //Andrew says returning from exec shouldnt return - scary = sorted
        exec( addr );
    } else {
    // parent
        //check pid is right?
        addr = load( strtok( NULL, " " ) );
        int prio = getPriority( strtok( NULL, " " ) );
        setpri(pid, prio);
    }


}
    else if( 0 == strcmp( p, "kill" ) ) {
      pid_t pid = atoi( strtok( NULL, " " ) );
      int   s   = atoi( strtok( NULL, " " ) );

      kill( pid, s );
    }
    else {
      puts( "unknown command\n", 16 );
    }
  }

  exit( EXIT_SUCCESS );
}
