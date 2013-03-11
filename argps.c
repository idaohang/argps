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

	// Connect to gpsd.
	static struct gps_data_t *GPS = NULL;
	if( gps_open( "127.0.0.1", "2947", GPS ) < 0 )
	{
		fprintf( stderr, "Couldn't connect to gpsd.\n" );
		exit( EXIT_FAILURE );
	}

	// Register for updates from gpsd.
	gps_stream( GPS, WATCH_ENABLE | WATCH_JSON, NULL );

	printf( "Waiting for gps lock." );
	while( GPS->status == 0 )
	{
		if( gps_waiting( GPS, 500 ) )
		{
			// Error check the GPS data read.
			if( gps_read( GPS ) == -1 )
			{
				fprintf( stderr, "gps_read() error, terminating.\n" );
				gps_stream( GPS, WATCH_DISABLE, NULL );
				gps_close( GPS );
				close( sockfd );
				exit( EXIT_FAILURE );
			}
			else
			{
				// Status > 0 means data is present.
				if( GPS->status > 0 )
				{
					if( isnan( GPS->fix.latitude ) || isnan( GPS->fix.longitude ) )
					{
						fprintf( stderr, "Couldn't get GPS fix.\n" );
						gps_stream( GPS, WATCH_DISABLE, NULL );
						gps_close( GPS );
						close( sockfd );
						exit( EXIT_FAILURE );
					}
					else
					{
						printf( "\n" );
						printf( "latitude: %f\n", GPS->fix.latitude );
						printf( "longitude: %f\n", GPS->fix.longitude );
					}
				}
				else
				{
					printf( "." );
				}
			}
		}
		// If gps_waiting() returns false, gpsd has shut the stream down (done
		// periodically). Re-open the stream to continue reading GPS data.
		else
		{
			gps_stream( GPS, WATCH_ENABLE | WATCH_JSON, NULL );
		}

		// Sleep so we're not sending too much data.
		sleep( 1 );
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

