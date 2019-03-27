
/* bgpserver - forked from peeraddr.c */

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
#define SOCKADDRSZ (sizeof (struct sockaddr_in))

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
    unsigned int pl = header[17] << 8 + header[16];
    unsigned char msgtype = header[18];
    if ((received = recv(sock, header, pl, 0)) != pl) {
      die("Failed to receive msg payload from peer");
    } else if (!isMarker(header)) {
      die("Failed to find BGP marker in msg header from peer");
    } else {
      unsigned char *hex = toHex (buffer,pl) ;
      fprintf(stderr, "BGP msg type %s length %d received [%s]\n", showtype(msgtype), pl , hex);
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
  struct sockaddr_in peeraddr;

  fprintf(stderr, "bgpc\n");
  if (3 > argc) {
      fprintf(stderr, "USAGE: bgpc <open_message_file> <update_message_file> {IP address}\n");
      exit(1);
  }
  if ((fd1 = open(argv[1],O_RDONLY)) < 0) {
    die("Failed to open BGP Open message file");
  }

  if ((fd2 = open(argv[2],O_RDONLY)) < 0) {
    die("Failed to open BGP Update message file");
  }

  if (3 == argc) { // server mode.....

    memset(&peeraddr, 0, SOCKADDRSZ );
    peeraddr.sin_family = AF_INET;
    peeraddr.sin_addr.s_addr = htonl(INADDR_ANY);   // local server addr - wildcard - could be a specific interface
    peeraddr.sin_port = htons(179);       // BGP server port
  
    if ((serversock = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP)) < 0) {
      die("Failed to create socket");
    }
  
    int reuse = 1;
    if (setsockopt(serversock, SOL_SOCKET, SO_REUSEADDR, (const char*)&reuse, sizeof(reuse)) < 0)
      die("Failed to set server socket option SO_REUSEADDR");

    if (setsockopt(serversock, SOL_SOCKET, SO_REUSEPORT, (const char*)&reuse, sizeof(reuse)) < 0)
      die("Failed to set server socket option SO_REUSEPORT");

    if (bind(serversock, (struct sockaddr *) &peeraddr, SOCKADDRSZ ) < 0) {
      die("Failed to bind the server socket");
    }
  
    if (listen(serversock, MAXPENDING) < 0) {
      die("Failed to listen on server socket");
    }
  
    while (1) {
      unsigned int addrsize;
      fprintf(stderr, "waiting for connection\n");
      if ((peersock = accept(serversock, (struct sockaddr *) &peeraddr, &addrsize )) < 0) {
        die("Failed to accept peer connection");
      }
      if ( addrsize != SOCKADDRSZ || AF_INET != peeraddr.sin_family) {
        die("bad sockaddr");
      }
      fprintf(stderr, "Peer connected: %s\n", inet_ntoa(peeraddr.sin_addr));
      session(peersock,fd1,fd2);
    }
  } else { // client mode
      fprintf(stderr, "Connecting to: %s\n", argv[3]);
      memset(&peeraddr, 0, SOCKADDRSZ );
      peeraddr.sin_family = AF_INET;
      peeraddr.sin_addr.s_addr = inet_addr(argv[3]);
      peeraddr.sin_port = htons(179);
      if ((peersock = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP)) < 0) {
        die("Failed to create socket");
      } else if (connect(peersock, (struct sockaddr *) &peeraddr, SOCKADDRSZ ) < 0) {
        die("Failed to connect with peer");
      } else {
          fprintf(stderr, "Peer connected: %s\n", argv[3]);
          session(peersock,fd1,fd2);
      }
  }
}
