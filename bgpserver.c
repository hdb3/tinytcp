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
/* MAX_RW_COUNT is defined in the linux headers in fs.h and is INT_MAX & PAGE_CACHE_MASK = 2 * 1024^3 - 4096 */
#define MAX_RW_COUNT 2147479552
void Die(char *mess) { perror(mess); exit(1); }

void getBGPMessage (int sock) {

  if ((received = recv(sock, header, 19, 0)) < 0) {
    Die("Failed to receive initial msg  header from client");
  }
  int pl = buffer[16] << 8 + buffer[17]
  int msgtype = buffer[18]
  if ((received = recv(sock, header, pl, 0)) != pl) {
    Die("Failed to receive initial msg payload from client");
  }

  fprintf(stderr, "BGP msg type %d length %d received", pl , msgtype)
}

void HandleClient(int sock, fd1 , fd2) {
  char header[19];
  char buffer[BUFFSIZE];
  char *tailptr;
  int received = -1;
  long long int respsize,sendsize;
  char* blockptr = malloc(BLOCKSIZE);
  memset(blockptr,0xaa,BLOCKSIZE);

  c=sendfile(sock, fd1, 0, 0x7ffff000)) {
  if (c < 0 ) {
      Die("Failed to send fd1 to client");
  }

  getBGPMessage (sock);

  c=sendfile(sock, fd2, 0, 0x7ffff000)) {
  if (c < 0 ) {
      Die("Failed to send fd2 to client");
  }
  close(sock);
}



int main(int argc, char *argv[]) {
  int serversock, clientsock, fd1,fd2;
  struct sockaddr_in listensocket, clientsocket;
  fprintf(stderr, "bgpserver\n");

  if (argc != 3) {
    fprintf(stderr, "USAGE: bgpserver <open_message_file> <update_message_file>\n");
    exit(1);
  }

  if ((fd1 = open(argv[1])) < 0) {
    Die("Failed to open BGP Open message file");
  }

  if ((fd2 = open(argv[2])) < 0) {
    Die("Failed to open BGP Update message file");
  }

  /* Create the TCP socket */
  if ((serversock = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP)) < 0) {
    Die("Failed to create socket");
  }
  /* Construct the server sockaddr_in structure */
  memset(&listensocket, 0, sizeof(listensocket));       /* Clear struct */
  listensocket.sin_family = AF_INET;                  /* Internet/IP */
  listensocket.sin_addr.s_addr = htonl(INADDR_ANY);   /* Incoming addr - wildcard - could be a specific interface */
  listensocket.sin_port = htons(179);       /* BGP server port */


  /* Bind the server socket */
  if (bind(serversock, (struct sockaddr *) &listensocket,
                               sizeof(listensocket)) < 0) {
    Die("Failed to bind the server socket");
  }
  /* Listen on the server socket */
  if (listen(serversock, MAXPENDING) < 0) {
    Die("Failed to listen on server socket");
  }


  /* Run until cancelled */
  while (1) {
    unsigned int clientlen = sizeof(clientsocket);
    /* Wait for client connection */
    if ((clientsock =
         accept(serversock, (struct sockaddr *) &clientsocket,
                &clientlen)) < 0) {
      Die("Failed to accept client connection");
    }
    fprintf(stdout, "Client connected: %s\n",
                    inet_ntoa(clientsocket.sin_addr));
    HandleClient(clientsock,fd1,fd2);
sys/sendfile.h  }
}

