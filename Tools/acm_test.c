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

#include <termios.h>

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

void
dump_buf ( char *buf, int n )
{
		int i;

		for ( i=0; i<n; i++ ) {
			if ( i > 0 )
				printf ( " " );
			printf ( "%02x", buf[i] );
		}
		printf ( "\n" );
}

void
write_test ( int fd )
{
		int i;
		// int ntest = 500;
		// int ntest = 10 * 1024 * 21;
		// int ntest = 1024 * 21;
		int ntest = 1;
		int size = 512;

		printf ( "Sending %d packets of %d bytes to %s\n",
			ntest, size, TEST_DEVICE );

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
		printf ( "%d bytes sent\n", ntest*size );
}

#define RBUF_SIZE	64*1024

char rbuf[RBUF_SIZE];

void
read_test ( int fd )
{
		int n;
		int limit = 100;

		for ( ;; ) {
			// n = read ( fd, rbuf, RBUF_SIZE );
			n = read ( fd, rbuf, limit );
			// printf ( "Read returns: %d %s\n", n, rbuf );
			printf ( "Read returns: %d ", n );
			dump_buf ( rbuf, n );
			if ( n <= 0 )
				break;
		}
}

int
open_usb ( char *dev )
{
		int fd;
		struct termios tm;

		fd = open ( TEST_DEVICE, O_RDWR );
		if ( fd < 0 ) {
			fprintf ( stderr, "Serial device: %s\n", dev );
			error ("Cannot open serial device" );
		}

		tcgetattr ( fd, &tm );
		// tm.c_lflag &= ~ECHO;
		cfmakeraw ( &tm );
		tcsetattr ( fd, 0, &tm );

		return fd;
}

int
main ( int argc, char **argv )
{
		int fd;

		fd = open_usb ( TEST_DEVICE );

		// write_test ( fd );
		read_test ( fd );

		printf ( "Closing\n" );
		close ( fd );
}

/* THE END */
