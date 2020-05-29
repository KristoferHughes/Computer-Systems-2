/* myloggerd.c
 * Source file for thread-lab
 * Creates a server to log messages sent from various connections
 * in real time.
 *
 * makefile works
 * Student: Kristofer Hughes
 */
 
#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <stdlib.h>
#include <pthread.h>
#include "message-lib.h"

// forward declarations
int usage( char name[] );
// a function to be executed by each thread
void * recv_log_msgs( void * arg );

// globals
int log_fd; // opened by main() but accessible by each thread
char newBuff[100]; //NEW global var, buffer for file. maybe increase to a higher number depending on message size?
int file_var; //NEW global var, for logfile

void * recv_log_msgs( void * arg ){
		
	int comParing = *((int*)arg);
	
	while((read_msg(comParing, newBuff, 100))> 0){ // loops to receive messages from a connection
		write(log_fd, newBuff, strlen(newBuff));
}

	close_connection(comParing); //close_connection to be more specific, maybe change to just a close for simplicity?
	return NULL; //NOTE TO SELF: keep this at end, required return
}

int usage( char name[] ){
	printf( "Usage:\n" );
	printf( "\t%s <log-file-name> <UDS path>\n", name );
	return 1;
}

int main( int argc, char * argv[] )
{
	if (argc != 3) //make sure argc is not 3
		return usage(argv[0]); //return usage for argv
	
	log_fd = open(argv[1], O_RDWR | O_APPEND | O_CREAT, 0666); // open the log file for appending. maybe increase to 0777?
	int connecting = permit_connections(argv[2]); // permit message connections. spinoff of code in get-messages
	
	if(connecting == -1) //check if the variable for the connection is equal to -1
		return -1; // when accept_next_connection returns -1, terminate the loop. A little out of order in the scheme of things but works!

	pthread_t labthread; //new pthread created!
			
	while(1){ // loop to wait for connection requests;
		
		printf("Holding for a new connection %s\n", argv[2]);

		file_var = accept_next_connection(connecting); 
		pthread_create(&labthread, NULL, recv_log_msgs, &file_var); // launch a new thread that calls recv_log_msgs()
		
	}

	close(connecting); // listener is now closed, now time for the log file to be closed
	close_connection(log_fd); // log file now closed. substitute for just a close function like with previous line?	
	return 0; //required return statement
}
