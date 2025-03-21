/* acm_test.c
 * Tom Trebisky  3-17-2025
 *
 * Send blocks of data to an ACM serial device.
 */

#include <stdio.h>
#include <fcntl.h>
#include <stdlib.h>
#include <unistd.h>
#include <time.h>

// #define TEST_DEVICE	"/dev/ttyACM0"
#define TEST_DEVICE	"/dev/ttyACM1"

static void
error ( char *msg )
{
		fprintf ( stderr, "%s\n", msg );
		exit ( 1 );
}

static void
send ( int fd, int n )
{
		char buf[2048];
		int i;
		int s;

		for ( i=0; i<n; i++ )
			buf[i] = 'B';

		s = write ( fd, buf, n );
		if ( s != n )
			printf ( "Write error: %d (expected %d)\n", s, n );
}

static void
uu_delay ( void )
{
		struct timespec delay;

#ifdef notdef
		// 1 millisecond
		delay.tv_sec = 0;
		delay.tv_nsec = 1000000;
		// 0.2 second (200 ms)
		delay.tv_sec = 0;
		delay.tv_nsec = 200000000;
		// 1 second
		delay.tv_sec = 1;
		delay.tv_nsec = 0;
#endif
		// 1 millisecond
		delay.tv_sec = 0;
		delay.tv_nsec = 1000000;

		nanosleep ( &delay, NULL );
}

int
main ( int argc, char **argv )
{
		int fd;
		int i;
		// int ntest = 500;
		int ntest = 1;
		int size = 200;

		fd = open ( TEST_DEVICE, O_RDWR );
		if ( fd < 0 )
			error ("Cannot open serial device" );
		printf ( "Open\n" );

		// sleep ( 2 );

		// sleep ( 1 );
		for ( i=0; i<ntest; i++ ) {
			// printf ( " %d\n", i );
			// if ( (i%1000) == 0 )
			// 		printf ( ".. %d\n", i );
			send ( fd, size );
			// uu_delay ();
		}
		// sleep ( 1 );

		printf ( "Closing\n" );
		close ( fd );
		printf ( "%d bytes sent\n", ntest*size );
}

/* THE END */
