#include <sys/types.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <math.h>
#include <arpa/inet.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <errno.h>
#include <unistd.h>
#include <syslog.h>
#include <string.h>
#include <stdio.h>

#include <gps.h>

#define PORT_NUMBER "12345"
#define MAX_BUFFER_SIZE 1024

void getLine( char *& buffer );
int createConnection( const char *hostname, const char *port );	// port number as string

static struct gps_data_t gpsData;

int main( int argc, char **argv )
{
	// Enforce proper command line arguments.
	if( argc != 2 )
	{
		fprintf( stderr, "Usage: %s <hostname>\n", argv[0] );
		exit( EXIT_FAILURE );
	}

	// Connect to remote host.
	int sockfd = createConnection( argv[1], PORT_NUMBER );
	if( sockfd < 0 )
	{
		fprintf( stderr, "Couldn't connect to %s.\n", argv[1] );
		exit( EXIT_FAILURE );
	}
	else
	{
		printf( "Connected to %s\n.", argv[0] );
	}

	// Connect to gpsd.
	if( gps_open( "localhost", DEFAULT_GPSD_PORT, &gpsData ) != 0 )
	{
		fprintf( stderr, "Couldn't connect to gpsd, errno = %d, %s.\n", errno, gps_errstr( errno ) );
		exit( EXIT_FAILURE );
	}
	else
	{
		printf( "Connected to gpsd.\n" );
	}

	// Register for updates from gpsd.
	gps_stream( &gpsData, WATCH_ENABLE | WATCH_JSON, NULL );

	printf( "Waiting for gps lock." );
	for(;;)
	{
		if( !gps_waiting( &gpsData, 5000000 ) )
		{
			fprintf( stderr, "GPS fix timed out.\n" );
			exit( EXIT_FAILURE );
		}
		else
		{
			if( gps_read( &gpsData ) == -1 )
			{
				fprintf( stderr, "gps_read() error, errno = %d\n", errno );
			}
			else
			{
				if( isnan( gpsData.fix.latitude ) || isnan( gpsData.fix.longitude ) )
				{
					fprintf( stderr, "Bad GPS fix.\n" );
				}
				else
				{
					printf( "Latitude: %f\n", gpsData.fix.latitude );
					printf( "Longitude: %f\n", gpsData.fix.longitude );
				}
			}
		}
	}

	close( sockfd );
	return 0;
}

void getLine( char *& buffer )
{
	unsigned int size = MAX_BUFFER_SIZE;
	unsigned int i = 0;
	int c;
	buffer = (char *)malloc( MAX_BUFFER_SIZE );

	do
	{
		c = fgetc( stdin );
		if( c != EOF && c != '\n' )
		{
			buffer[i++] = (char)c;
		}
		if( i >= size - 1 )
		{
			size *= 2;
			buffer = (char *)realloc( buffer, size );
		}
	} while( c != EOF && c != '\n' );

	if( i == 0 )
	{
		free( buffer );
		buffer = NULL;
	}
	else
	{
		buffer[i] = '\0';
	}
}

int createConnection( const char *hostname, const char *port )
{
	int sockfd;
	struct addrinfo hints;
	struct addrinfo *servinfo;

	memset( &hints, 0, sizeof( hints ) );
	hints.ai_family = AF_UNSPEC;
	hints.ai_socktype = SOCK_STREAM;

	if( getaddrinfo( hostname, port, &hints, &servinfo ) != 0 )
	{
		fprintf( stderr, "getaddrinfo() failure.\n" );
		return -1;
	}

	// Connect to the first result possible.
	struct addrinfo *p = NULL;
	for( p = servinfo; p != NULL; p = p->ai_next )
	{
		sockfd = socket( p->ai_family, p->ai_socktype, p->ai_protocol );
		if( sockfd == -1 )
		{
			continue;
		}

		if( connect( sockfd, p->ai_addr, p->ai_addrlen ) == -1 )
		{
			continue;
		}

		break;
	}

	if( p == NULL )
	{
		fprintf( stderr, "connect() failure.\n" );
		return -1;
	}

	freeaddrinfo( servinfo );

	return sockfd;
}

