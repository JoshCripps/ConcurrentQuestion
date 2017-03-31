#include "P7.h"

int is_prim2( uint32_t x ) {
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
void main_P7() {
    int fd[2];

    for( int i = 0; i < 50; i++ ) {
        write( STDOUT_FILENO, "P7", 2 );

        uint32_t lo = 1 <<  8;
        uint32_t hi = 1 << 16;

        for( uint32_t x = lo; x < hi; x++ ) {
            int r = is_prim2( x );
        }
    }
    for (int a = 0; a < 50; a++) {
        char yay[100];
        int m = read( fd[0], yay, 5);
        if (m < 0) {
            write( STDOUT_FILENO, "#NOTHING ON PIPE, YIELDING. ", 28 );
            yield();
            //continue;
        }
        write( STDOUT_FILENO, "Rdn: ", 5 );
        write( STDOUT_FILENO, yay, m );
        write( STDOUT_FILENO, " ", 1 );
    }
    for (int i = 0; i < 2; i++) {
        close( fd[i] );
    }
    exit( EXIT_SUCCESS );
}
