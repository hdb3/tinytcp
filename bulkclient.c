/* based on code from
http://gnosis.cx/publish/programming/sockets.html
*/

 
#include <stdio.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <netinet/in.h>

#define BUFFSIZE 0x10000
void Die(char *mess) { perror(mess); exit(1); }


int main(int argc, char *argv[]) {
  int sock;
  struct sockaddr_in echoserver;
  char buffer[BUFFSIZE];
  unsigned int echolen;
  long long int received;
  ssize_t bytes;

  if (argc != 4) {
    fprintf(stderr, "USAGE: bulkclient <server_ip> <port> <data-length-requested>\n");
    exit(1);
  }
  /* Create the TCP socket */
  if ((sock = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP)) < 0) {
    Die("Failed to create socket");
  }
 
 
  /* Construct the server sockaddr_in structure */
  memset(&echoserver, 0, sizeof(echoserver));       /* Clear struct */
  echoserver.sin_family = AF_INET;                  /* Internet/IP */
  echoserver.sin_addr.s_addr = inet_addr(argv[1]);  /* IP address */
  echoserver.sin_port = htons(atoi(argv[2]));       /* server port */
  /* Establish connection */
  if (connect(sock,
              (struct sockaddr *) &echoserver,
              sizeof(echoserver)) < 0) {
    Die("Failed to connect with server");
  }
        
 
  /* Send the word to the server */
  echolen = strlen(argv[3]);
  if (send(sock, argv[3], echolen, 0) != echolen) {
    Die("Mismatch in number of sent bytes");
  }
  /* Receive data back from the server */
  fprintf(stdout, "Received: ");
  received = 0;
  do {
    bytes = recv(sock, buffer, BUFFSIZE, 0);
    received += bytes;
  } while (bytes > 0);
        

 fprintf(stdout, " %lld\n",received);
 close(sock);
 exit(0);
}
