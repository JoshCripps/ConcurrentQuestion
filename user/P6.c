#include "P6.h"

int is_prime2( uint32_t x ) {
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


void main_P6() {
    bool readSuccessful = false;
    bool writeSuccessful = false;
    /*Time to test pipes*/
    /* Simple example where a pipe is set up and is writen to and read from... Its written to twice as often as its read from, so will yield when it can no longer write and wait until more is read off the buffer before continuing. When it has shown evidence of the blocking behaviour it closes the pipe and sets the buffer back to 0 which can then be reused by another process... */
    int fd[2];
    if (pipes(fd) < 0) {
        write( STDOUT_FILENO, "#NO MORE PIPES", 14 );
    }
    else {
        while (!readSuccessful || !writeSuccessful) {
            for( int i = 0; i < 50; i++ ) {
                write( STDOUT_FILENO, "P6", 2 );


                uint32_t lo = 1 <<  8;
                uint32_t hi = 1 << 16;

                for( uint32_t x = lo; x < hi; x++ ) {
                    int r = is_prime2( x );
                }
            }

            for (int a = 0; a < 450; a++) {
                char yay[100];
                int n = write( fd[1], "abcde", 5);
                write( STDOUT_FILENO, "Wtn.. ", 6 );
                if (n < 0 ) {
                    write( STDOUT_FILENO, "#NO MORE SPACE ON PIPE, YIELDING. ", 34 );
                    yield();
                    //continue;
                }
                else { writeSuccessful = true; }
                if (a%2 == 0) {
                    int m = read( fd[0], yay, 5);
                    if (m < 0) {
                        write( STDOUT_FILENO, "#NOTHING ON PIPE, YIELDING. ", 28 );
                        yield();
                        //continue;
                    }
                    else { readSuccessful = true; }
                    write( STDOUT_FILENO, "Rdn: ", 5 );
                    write( STDOUT_FILENO, yay, m );
                    write( STDOUT_FILENO, " ", 1 );
                }
            }
            for (int i = 0; i < 2; i++) {
                close( fd[i] );
            }
        }


        exit( EXIT_SUCCESS );
    }
}