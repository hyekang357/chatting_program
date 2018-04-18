#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <arpa/inet.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <netdb.h>
#include <ctype.h>
#include <time.h>
#include <pthread.h>

#include <iostream>
#include <fstream>

#define MAXBUF 1024

using namespace std;

int nClient;
int nFDList [1000];

/*
Main functions of the server:
  1. Listen for connection request from clients
    - when connection request comes in, accept and create a new thread for the next function
  2. Listen for clients' messages and broadcast message to other clients
    - A thread for each client
  3. Server sends typed messages to all the clients
  4. Broad cast the new clients port number to all the connected clients
*/

// Listen for clients' messages and broadcast message to other clients
void *client_handler ( void *ptr ) {

	int first = true;
	int nDesc = *((int*) ptr);
	// printf ("Client Handler Activated for FD %d\n", nDesc);

	char buf [MAXBUF];
	int len;

	while (1) {

		if ((len = read (nDesc, buf, MAXBUF)) > 0) {
      // Broad cast the new clients port number to all the connected clients
			if (first) {
        char * token;
        // perform string manipulation to concatenate ID and CLIENT-PORT
        token = strtok(buf, "-");
        printf("buf: %s\n", buf);

        printf("first token: %s\n", token);
        char clientIDMessage[MAXBUF] = "Client ";
        strcat(clientIDMessage, token);
        strcat(clientIDMessage, ": initiated.");

        printf("%s\n", clientIDMessage);

        // broadcast out the new client ID joined
				for (int k = 0; k < nClient; k++) {
					if (nDesc != nFDList [k]) {
            write (nFDList [k], clientIDMessage, strlen(clientIDMessage) +  1);
          }
				}

        // client-server-port is saved in the config file

        token = strtok (NULL, "-");
        printf("second token: %s\n", token);

        // ofstream configFile;
        // configFile.open ("config", ios::app);
        // configFile << token;
        // configFile.close();

        token = strtok (NULL, "-");
        printf("third token: %s\n", token);

        // broadcast out only the client-port
				for (int k = 0; k < nClient; k++) {
					if (nDesc != nFDList [k])
          usleep(3000000);
					write (nFDList [k], token, strlen(token) +  1);
          usleep(3000000);
				}
				first = false;
			}
		  else {
				printf ("%s\n", buf);
			}
		}
	}
}

// Server sends typed messages to all the clients
void *server_handler ( void *ptr ) {
	printf ("Server Handler Activated\n");
	char readInBuffer [MAXBUF];

	while (1) {
		scanf("%s", readInBuffer);

    char message [MAXBUF] = "Server :";
    strcat(message, readInBuffer);

    // if server reads in exit then send last message to clients and terminate
    if (strcmp("Exit", readInBuffer) == 0 || strcmp("exit", readInBuffer) == 0) {
      char exitMessage[] = "Server:terminated.";
      // broadcast the typed message to all clients
      for (int i = 0; i < nClient; i++) {
        write (nFDList [i], exitMessage, strlen (exitMessage) +  1);
      }
      exit(1);
    } else {
      // broadcast the typed message to all clients
      for (int i = 0; i < nClient; i++) {
        write (nFDList [i], message, strlen (message) +  1);
      }
    }
	}
}

int main (int argc, char* argv [])
{
	char buf [MAXBUF];
	nClient = 0;

	int n, s, len;
	struct sockaddr_in name;
	int nPortNumber = 5002;
	int nClientFileDescriptor = -1;

  // create a new socket object
	// AF_INET: create IP based socket
	// SOCK_STREAM: create TPC based socket
	if ((s = socket (AF_INET, SOCK_STREAM, 0)) < 0) {
		perror ("socket");
		exit (1);
	}

  // create a server address
	memset (&name, 0, sizeof (struct sockaddr_in));
	name.sin_family = AF_INET;
	name.sin_port = htons (nPortNumber);
	len = sizeof (struct sockaddr_in);

	// use a wildcard address
	n = INADDR_ANY;
	memcpy (&name.sin_addr, &n, sizeof (long));

	// assign the address to the socket
	if (bind (s, (struct sockaddr *) &name, len) < 0) {
		perror ("bind");
		exit (1);
	}
  // end of creatint socket with corresponding address

  // server thread: Server sends typed messages to all the clients
	pthread_t server_thread;
	pthread_create(&server_thread, NULL, server_handler, NULL);

  // listen for connections from clients
  while (1) {
		// waiting for a connection
		if (listen (s, 5) < 0) {
			perror ("listen");
			exit (1);
		}



		// accept the connection
		if ((nClientFileDescriptor = accept (s, (struct sockaddr *) &name, (socklen_t*) &len)) < 0) {
			perror ("accept");
			exit (1);
		}
    nFDList [nClient++] = nClientFileDescriptor;

    // Listen for clients' messages and broadcast message to other clients
		pthread_t thread;
		pthread_create(&thread, NULL, client_handler, (void*) &nClientFileDescriptor);
	}

  close (s);
	return 0;

}
