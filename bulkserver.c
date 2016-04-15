/* based on code from http://gnosis.cx/publish/programming/sockets.html */

/* bulkserver - forked from server.c */

#include <stdio.h>
#include <errno.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <netinet/in.h>
#include <sys/sendfile.h>
#include <fcntl.h>

#define MAXPENDING 5    /* Max connection requests */
#define BUFFSIZE 32
#define BLOCKSIZE 0x10000
#define ZEROCOPY
void Die(char *mess) { perror(mess); exit(1); }

void HandleClient(int sock) {
  char buffer[BUFFSIZE];
  int received = -1;
  long long int respsize,sendsize;
  char* blockptr = malloc(BLOCKSIZE);
  memset(blockptr,0xaa,BLOCKSIZE);

  /* Receive message */
  if ((received = recv(sock, buffer, BUFFSIZE, 0)) < 0) {
    Die("Failed to receive initial bytes from client");
  }
  /* get the client request - for now, just expect a number which is the size of the
     expected response message */
  respsize = strtoll(buffer, NULL,10);
  if (errno) {
      fprintf(stderr, "client request (\"%s\")",buffer);
      Die("Failed to parse client request");
    }
  /* Send bytes and check for more incoming data in loop */
#ifdef ZEROCOPY

  char tmpn[] = "/tmp/XXXXXX" ; 
  int   tmpfd = mkstemp(tmpn);
  ftruncate(tmpfd,respsize);
  long long int sent_bytes = sendfile(sock, tmpfd, 0, respsize);
  if (sent_bytes != respsize) {
    fprintf(stderr,"Failed to send bytes to client (%qd/%qd)\n",sent_bytes,respsize);
    if (errno) {
      perror(NULL);
    }
    exit(1);
  }
  close(tmpfd);
  unlink(tmpn);
  
#else
  while (respsize > 0) {
    sendsize = respsize > BLOCKSIZE ? BLOCKSIZE : respsize;
    respsize -= sendsize;
    /* Send the next block */
    if (send(sock, blockptr, sendsize, 0) != sendsize) {
      Die("Failed to send bytes to client");
    }
  }
#endif
  close(sock);
}



int main(int argc, char *argv[]) {
  int serversock, clientsock;
  struct sockaddr_in echoserver, echoclient;

  if (argc != 2) {
    fprintf(stderr, "USAGE: echoserver <port>\n");
    exit(1);
  }
#ifdef ZEROCOPY
  fprintf(stdout, "using zero copy\n");
#endif
  /* Create the TCP socket */
  if ((serversock = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP)) < 0) {
    Die("Failed to create socket");
  }
  /* Construct the server sockaddr_in structure */
  memset(&echoserver, 0, sizeof(echoserver));       /* Clear struct */
  echoserver.sin_family = AF_INET;                  /* Internet/IP */
  echoserver.sin_addr.s_addr = htonl(INADDR_ANY);   /* Incoming addr */
  echoserver.sin_port = htons(atoi(argv[1]));       /* server port */


  /* Bind the server socket */
  if (bind(serversock, (struct sockaddr *) &echoserver,
                               sizeof(echoserver)) < 0) {
    Die("Failed to bind the server socket");
  }
  /* Listen on the server socket */
  if (listen(serversock, MAXPENDING) < 0) {
    Die("Failed to listen on server socket");
  }


  /* Run until cancelled */
  while (1) {
    unsigned int clientlen = sizeof(echoclient);
    /* Wait for client connection */
    if ((clientsock =
         accept(serversock, (struct sockaddr *) &echoclient,
                &clientlen)) < 0) {
      Die("Failed to accept client connection");
    }
    fprintf(stdout, "Client connected: %s\n",
                    inet_ntoa(echoclient.sin_addr));
    HandleClient(clientsock);
  }
}

