/*
	echoc: a demo of TCP/IP sockets connect

	usage:	client [-h serverhost] [-p port]
*/

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
#include <pthread.h>


#define SERVICE_PORT 5002	// server listening port number
#define MAXBUF 1024

char* CLIENT_ID = NULL;
int CLIENT_SERVER_PORT = 0;
char* CLIENT_SERVER_PORT_CHAR = NULL;
int CLIENT_CLIENT_PORT_INT = 0;
char* CLIENT_CLIENT_PORT = NULL;

/*
Functionalities of the client
1. Client connects to the server
2. Client listens to server
3. Client writes to server
4. Client listens for other clients request and makes connections
5. Client listens to other clients messages
6. Client writes to other clients
*/

int conn(char *host, int port, int fromPort);
void disconn(void);
const char *host = "localhost";

int nClient;
int nFDList [1000];
int serverFD;

// client sends typed messages to all other clients
void *client_write_handler ( void *ptr ) {
	// printf ("Client Write Handler Activated\n");
	char readInBuffer [MAXBUF];

	while (1) {
		scanf("%s", readInBuffer);
    char message [MAXBUF] = "Client ";
    strcat(message, CLIENT_ID);
    strcat(message, ": ");
    strcat(message, readInBuffer);

    // broadcast the typed message to all clients
		for (int i = 0; i < nClient; i++) {
			write (nFDList [i], message, strlen (message) +  1);
		}
    write (serverFD, message, strlen (message) +  1);
	}
}

void *client_listen_handler ( void *ptr ) {
  // if client sends message then print it out

	int nDesc = *((int*) ptr);
	// printf ("Listening to New Client: %d\n", nDesc);
	char buf [MAXBUF];
	int len;

	while(1) {
    // read socket
    if ((len = read (nDesc, buf, MAXBUF)) > 0) {
      printf ("%s\n", buf);
    }
	}
}

void *server_listen_handler ( void *ptr ) {

	int nDesc = *((int*) ptr);
	// printf ("Server Listen Handler Activated for FD %d\n", nDesc);

	char buf [MAXBUF];
	int len;

	while (1) {
		// if server send data through the socket
		if ((len = read (nDesc, buf, MAXBUF)) > 0) {

      // if the data is a port number of the new client
			if (strlen(buf) == 5 && isdigit(buf[0]) && isdigit(buf[1]) &&
				isdigit(buf[2]) && isdigit(buf[3]) && isdigit(buf[4])) {

				int newClientPort = atoi(buf);
				// printf ("Connecting to new Client at port: %d\n", newClientPort);

        // connect to the new client through client port
				int fd; // &&&
        CLIENT_CLIENT_PORT_INT += 1;
				if (!(fd = conn((char*) host, newClientPort, CLIENT_CLIENT_PORT_INT)))    /* connect */
					exit(1);   /* something went wrong */
        // add to list of clients connected
        nFDList [nClient++] = fd;
        // printf("Connecting to port %d at fd: %d\n", newClientPort, fd);

				pthread_t client_listen_thread;
        // create a thread that listen to client, read message, and print out
				pthread_create(&client_listen_thread, NULL, client_listen_handler, (void*) &fd);

			} else {  // if the data is just a message, print it out
					printf ("%s\n", buf);
			}
		}
	}
}

int main(int argc, char **argv) {

  extern char *optarg;
	extern int optind;
	int c, err = 0;
	char *prompt = 0;
	int port = SERVICE_PORT;	/* default: whatever is in port.h */
	// const char *host = "localhost";	/* default: this host */

  nClient = 0;
  CLIENT_ID = argv[1];
	CLIENT_SERVER_PORT = atoi(argv[2]);
	CLIENT_SERVER_PORT_CHAR = argv[2];
	CLIENT_CLIENT_PORT = argv[3];
	CLIENT_CLIENT_PORT_INT = atoi(argv[3]);

	// printf("arguments: %s, %d, %s\n", CLIENT_ID, CLIENT_SERVER_PORT, CLIENT_CLIENT_PORT);
	// printf("connecting to server at %s, port %d\n", host, port);

  // client connects to the server
  // connecting to host at port from client_server_port
	if (!(serverFD = conn((char*) host, port, CLIENT_SERVER_PORT)))
		exit(1);   /* something went wrong */

  // create thread to listen to server
  pthread_t server_listen_thread;
	pthread_create(&server_listen_thread, NULL, server_listen_handler, (void*) &serverFD);

  // write to server the client-client port (and client ID)
	// so that server can broadcast to connect to other clients
  // format of the message
    // "CLIENTID-CLIENTCLIENTPORT-CLIENTSERVERPORT"
  char clientInfo[MAXBUF];
  strcpy(clientInfo, CLIENT_ID);
  strcat(clientInfo, "-");
  strcat(clientInfo, CLIENT_CLIENT_PORT);
	strcat(clientInfo, "-");
	strcat(clientInfo, CLIENT_SERVER_PORT_CHAR);	

  write (serverFD, clientInfo, strlen (clientInfo) +  1);

  // create socket for other clients to connect to
  int n, s, len;
	struct sockaddr_in name;
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
	name.sin_port = htons (CLIENT_CLIENT_PORT_INT);
	len = sizeof (struct sockaddr_in);

	// use a wildcard address
	n = INADDR_ANY;
	memcpy (&name.sin_addr, &n, sizeof (long));

  // printf("Create socket with CLIENT_CLIENT_PORT_INT: %d\n", CLIENT_CLIENT_PORT_INT);
	// assign the address to the socket
	if (bind (s, (struct sockaddr *) &name, len) < 0) {
		perror ("bind");
		exit (1);
	}
  // end of creating socket with client-client port

  // create thread to read message from cmd and send to other clients. and server?
  pthread_t client_write_thread;
	pthread_create(&client_write_thread, NULL, client_write_handler, NULL);


  while(1) {

    // waiting for a connection
    // printf("Listening on CLIENT_CLIENT_PORT_INT: %d\n", CLIENT_CLIENT_PORT_INT);
		if (listen (s, 5) < 0) {
			perror ("listen");
			exit (1);
		}

    // accept the connection
		if ((nClientFileDescriptor = accept (s, (struct sockaddr *) &name, (socklen_t*) &len)) < 0) {
			perror ("accept");
			exit (1);
		}

    // add client to the list
    nFDList [nClient++] = nClientFileDescriptor;

    // printf("Creating new thread for fd: %d", nClientFileDescriptor);
    // create a thread that listens to the client, reads message, and prints out
    pthread_t thread;
		pthread_create(&thread, NULL, client_listen_handler, (void*) &nClientFileDescriptor);


  }

  close (s);
  return 0;
}

///////////////// Helper functions //////////////////////
int conn(char *host, int port, int fromPort)
{

  int fd;
	struct hostent *hp;	/* host information */
	unsigned int alen;	/* address length when we get the port number */
	struct sockaddr_in myaddr;	/* our address */
	struct sockaddr_in servaddr;	/* server address */

	// printf("conn(host=\"%s\", port=\"%d\")\n", host, port);

	/* get a tcp/ip socket */
	/* We do this as we did it for the server */
	/* request the Internet address protocol */
	/* and a reliable 2-way byte stream */

	if ((fd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
		perror("cannot create socket");
		return 0;
	}

	/* bind to an arbitrary return address */
	/* because this is the client side, we don't care about the */
	/* address since no application will connect here  --- */
	/* INADDR_ANY is the IP address and 0 is the socket */
	/* htonl converts a long integer (e.g. address) to a network */
	/* representation (agreed-upon byte ordering */

	memset((char *)&myaddr, 0, sizeof(myaddr));
	myaddr.sin_family = AF_INET;
	myaddr.sin_addr.s_addr = htonl(INADDR_ANY);
	myaddr.sin_port = htons(fromPort);		//use given port number from arguments

	if (bind(fd, (struct sockaddr *)&myaddr, sizeof(myaddr)) < 0) {
		perror("bind failed");
		return 0;
	}

	/* this part is for debugging only - get the port # that the operating */
	/* system allocated for us. */
        alen = sizeof(myaddr);
        if (getsockname(fd, (struct sockaddr *)&myaddr, &alen) < 0) {
                perror("getsockname failed");
                return 0;
        }

	/* fill in the server's address and data */
	/* htons() converts a short integer to a network representation */
	memset((char*)&servaddr, 0, sizeof(servaddr));
	servaddr.sin_family = AF_INET;
	servaddr.sin_port = htons(port);

	/* look up the address of the server given its name */
	hp = gethostbyname(host);
	if (!hp) {
		fprintf(stderr, "could not obtain address of %s\n", host);
		return 0;
	}

	/* put the host's address into the server address structure */
	memcpy((void *)&servaddr.sin_addr, hp->h_addr_list[0], hp->h_length);

	/* connect to server */

	if (connect(fd, (struct sockaddr *)&servaddr, sizeof(servaddr)) < 0) {
		perror("connect failed");
		return 0;
	}
	return fd;
}
