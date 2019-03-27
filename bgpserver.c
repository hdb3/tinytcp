
/* bgpserver - forked from echoserver.c */

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

#define MAXPENDING 5    // Max connection requests
#define BUFFSIZE 0x10000

void die(char *mess) { perror(mess); exit(1); }
unsigned char keepalive [19]={ 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0, 19, 4  };
unsigned char marker [16]={ 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff  };
int isMarker (const unsigned char *buf) {
   return ( 0 == memcmp(buf,marker,16));
}

unsigned char * showtype (unsigned char msgtype) {
   switch(msgtype) {
      case 1 : return "OPEN";
          break; 
      case 2 : return "UPDATE";
          break; 
      case 3 : return "NOTIFICATION";
          break; 
      case 4 : return "KEEPALIVE";
          break; 
      default  : return "UNKNOWN";
    }
}

unsigned char *toHex (unsigned char *buf, unsigned int l) {

  unsigned char     hex_str[]= "0123456789abcdef";
  unsigned int      i;
  unsigned char *result;

  if (!(result = (unsigned char *)malloc(l * 2 + 1)))
    return (NULL);

  (result)[l * 2] = 0;

  if (!l)
    return (NULL);

  for (i = 0; i < l; i++)
    {
      (result)[i * 2 + 0] = hex_str[(buf[i] >> 4) & 0x0F];
      (result)[i * 2 + 1] = hex_str[(buf[i]     ) & 0x0F];
    }
  return (result);
}

//int getBGPMessage (int sock) {
//   return 1;
//}

int getBGPMessage (int sock) {
  unsigned char header[19];
  unsigned char buffer[BUFFSIZE];
  int received;

  received = recv(sock, header, 19, 0);
  if (0 == received ) {
    fprintf(stderr, "end of stream\n");
    return 0;
  } else if (received < 0) {
    die("Failed to receive msg  header from peer");
    return 0;
  } else {
    unsigned int pl = header[16] << 8 + header[17];
    unsigned char msgtype = header[18];
    if ((received = recv(sock, header, pl, 0)) != pl) {
      die("Failed to receive msg payload from peer");
    } else {
      unsigned char *hex = toHex (buffer,pl) ;
      fprintf(stderr, "BGP msg type %s length %d received [%s]", showtype(msgtype), pl , hex);
      free(hex);
      return 1;
    }
  }
}


void session(int sock, int fd1 , int fd2) {
  int c;

  // if (c=sendfile(sock, fd1, 0, 0x7ffff000) < 0 ) {
  c=sendfile(sock, fd1, 0, 0x7ffff000);
  if (c < 0 ) {
      die("Failed to send fd1 to peer");
  }

  getBGPMessage (sock); // this is expected to be an Open

  c=sendfile(sock, fd2, 0, 0x7ffff000);
  if (c < 0 ) {
      die("Failed to send fd2 to peer");
  }

  c=send(sock, keepalive, 19, 0);
  if (c < 0 ) {
      die("Failed to send keepalive to peer");
  }

  c=sendfile(sock, fd2, 0, 0x7ffff000);
  if (c < 0 ) {
      die("Failed to send fd2 to peer");
  }

  int res;
  do {
    res = getBGPMessage (sock); // keepalive or updates from now on
  } while (res);
  close(sock);
}


void main(int argc, char *argv[]) {
  int serversock, peersock, fd1,fd2;
  struct sockaddr_in listensocket, peersocket;
  fprintf(stderr, "bgpserver\n");

  if (argc != 3) {
    fprintf(stderr, "USAGE: bgpserver <open_message_file> <update_message_file>\n");
    exit(1);
  }

  if ((fd1 = open(argv[1],O_RDONLY)) < 0) {
    die("Failed to open BGP Open message file");
  }

  if ((fd2 = open(argv[2],O_RDONLY)) < 0) {
    die("Failed to open BGP Update message file");
  }

  if ((serversock = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP)) < 0) {
    die("Failed to create socket");
  }

  memset(&listensocket, 0, sizeof(listensocket));
  listensocket.sin_family = AF_INET;
  listensocket.sin_addr.s_addr = htonl(INADDR_ANY);   // local server addr - wildcard - could be a specific interface
  listensocket.sin_port = htons(179);       // BGP server port

  if (bind(serversock, (struct sockaddr *) &listensocket, sizeof(listensocket)) < 0) {
    die("Failed to bind the server socket");
  }

  if (listen(serversock, MAXPENDING) < 0) {
    die("Failed to listen on server socket");
  }

  while (1) {
    unsigned int peerlen = sizeof(peersocket);
    if ((peersock = accept(serversock, (struct sockaddr *) &peersocket, &peerlen)) < 0) {
      die("Failed to accept peer connection");
    }
    fprintf(stdout, "Peer connected: %s\n", inet_ntoa(peersocket.sin_addr));
    session(peersock,fd1,fd2);
  }
}
