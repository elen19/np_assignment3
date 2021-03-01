#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <errno.h>
#include <netdb.h>

/* You will to add includes here */
#define DEBUG
#define PROTOCOL "HELLO 1\n"

int main(int argc, char *argv[])
{

  /* Do magic */
  if (argc != 3)
  {
    printf("Wrong format IP:PORT Nickname\n");
    exit(0);
  }

  char delim[] = ":";
  char *Desthost = strtok(argv[1], delim);
  char *Destport = strtok(NULL, delim);
  char *Destnick = strtok(argv[2], delim);

  if (Desthost == NULL || Destport == NULL || Destnick == NULL) 
  {
    printf("Wrong format\n");
    exit(0);
  }
  if(strlen(Destnick)>12)
  {
    printf("Error: Name too long. Max 12 character. \n");
    exit(0);
  }
  //int port = atoi(Destport);
  addrinfo sa, *si, *p;
  memset(&sa, 0, sizeof(sa));
  sa.ai_family = AF_INET;
  sa.ai_socktype = SOCK_STREAM;
  struct timeval tv;
  tv.tv_sec = 5;
  tv.tv_usec = 0;
  if (int rv = getaddrinfo(Desthost, Destport, &sa, &si) != 0)
  {
    fprintf(stderr, "%s\n", gai_strerror(rv));
    exit(0);
  }

  int sockfd;
  for (p = si; p != NULL; p = p->ai_next)
  {
    if ((sockfd = socket(p->ai_family, p->ai_socktype, p->ai_protocol)) == -1)
    {
      continue;
    }
    if ((connect(sockfd, p->ai_addr, p->ai_addrlen)) != 0)
    {
      close(sockfd);
      printf("Couldn't connect to server.\n");
      continue;
    }
    break;
  }
  if (p == NULL)
  {
    printf("Couldn't create connect.\n");
    exit(0);
  }
  setsockopt(sockfd, SOL_SOCKET, SO_RCVTIMEO, (const char *)&tv, sizeof(tv));
  freeaddrinfo(si);

  char recvBuf[256];
  char sendBuf[256];
  int bytes;
  while (true)
  {
    bytes = recv(sockfd, recvBuf, sizeof(recvBuf), 0);
    if (bytes == -1)
    {
      printf("Failed to recive. \n");
      continue;
    }
    else if (strstr(recvBuf, PROTOCOL) != nullptr)
    {
      printf("Serverprotocol accepted, Protocol: %s", recvBuf);
      sprintf(sendBuf,"NICK%s ",Destnick);
      send(sockfd, sendBuf, strlen(sendBuf), 0);
    }
  }
  close(sockfd);
  return 0;
}
