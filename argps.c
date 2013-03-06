#include <sys/types.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
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

#define PORT_NUMBER "12345"
#define MAX_BUFFER_SIZE 1024

void getLine( char *& buffer );

int main( int argc, char **argv )
{
	int sockfd;
	struct addrinfo hints;
	struct addrinfo *servinfo;

	memset( &hints, 0, sizeof( hints ) );
	hints.ai_family = AF_UNSPEC;
	hints.ai_socktype = SOCK_STREAM;

	if( getaddrinfo( argv[1], PORT_NUMBER, &hints, &servinfo ) != 0 )
	{
		fprintf( stderr, "getaddrinfo() failure.\n" );
		exit( EXIT_FAILURE );
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
		exit( EXIT_FAILURE );
	}

	freeaddrinfo( servinfo );

	while( 1 )
	{
		char *buffer = NULL;
		printf( ">> " );
		getLine( buffer );

		int numSent;
		if( ( numSent = send( sockfd, buffer, strlen( buffer ) + 1, 0 ) ) == -1 )
		{
			fprintf( stderr, "send() failure.\n" );
			exit( EXIT_FAILURE );
		}

		free( buffer );
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

	buffer[i] = '\0';
}

