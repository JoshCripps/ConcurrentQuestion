#include "P3.h"

int is_prime( uint32_t x ) {
  if ( !( x & 1 ) || ( x < 2 ) ) {
    return ( x == 2 );
  }

  for( uint32_t d = 3; ( d * d ) <= x ; d += 2 ) {
    if( !( x % d ) ) {
      return 0;
    }
  }

  return 1;
}


void main_P3() {

  for( int i = 0; i < 50; i++ ) {
    write( STDOUT_FILENO, "P3", 2 );


    uint32_t lo = 1 <<  8;
    uint32_t hi = 1 << 16;

    for( uint32_t x = lo; x < hi; x++ ) {
      int r = is_prime( x );
    }
  }

  /*Time to test pipes*/
  int fd[2];
  if (pipes(fd) < 0){
      write( STDOUT_FILENO, "#NO MORE PIPES", 14 );
  }
  else {
      char yay[100];
      int n = write( fd[1], "abcde", 5);
      if (n < 0 ) {
          write( STDOUT_FILENO, "#NO MORE SPACE ON PIPE, YIELDING.", 33 );
          yield();
      }
      int m = read( fd[0], yay, 5);
      if (m < 0) {
          write( STDOUT_FILENO, "#NOTHING ON PIPE, YIELDING.", 27 );
          yield();

      }

      write( STDOUT_FILENO, "Reading: ", 9 );
      write( STDOUT_FILENO, yay, m );
  }

  exit( EXIT_SUCCESS );
}
