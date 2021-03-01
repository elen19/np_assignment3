#include <stdio.h>
#include <stdlib.h>
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
#include <regex.h>
/* You will to add includes here */
#define DEBUG
#define PROTOCOL "HELLO 1\n"
int main(int argc, char *argv[])
{

  /* Do more magic */
  if (argc != 2)
  {
    printf("Wrong format IP:PORT\n");
    exit(0);
  }
  char delim[] = ":";
  char *Desthost = strtok(argv[1], delim);
  char *Destport = strtok(NULL, delim);
  if (Desthost == NULL || Destport == NULL)
  {
    printf("Wrong format\n");
    exit(0);
  }
  int port = atoi(Destport);
  int sockfd, len, connfd;
  struct sockaddr_in client;

  addrinfo sa, *si, *p;
  memset(&sa, 0, sizeof(sa));
  sa.ai_family = AF_UNSPEC;
  sa.ai_socktype = SOCK_STREAM;
  struct timeval tv;
  tv.tv_sec = 5;
  tv.tv_usec = 0;
  if (int rv = getaddrinfo(Desthost, Destport, &sa, &si) != 0)
  {
    fprintf(stderr, "%s\n", gai_strerror(rv));
    exit(0);
  }

  for (p = si; p != NULL; p = p->ai_next)
  {
    if ((sockfd = socket(p->ai_family, p->ai_socktype, p->ai_protocol)) == -1)
    {
      continue;
    }
    if ((bind(sockfd, p->ai_addr, p->ai_addrlen)) != 0)
    {
      printf("Failed to bind.\n");
      close(sockfd);
      continue;
    }
    break;
  }
  if (p == NULL)
  {
    printf("Couldn't create/bind socket.\n");
    exit(0);
  }
  setsockopt(sockfd, SOL_SOCKET, SO_RCVTIMEO, (const char *)&tv, sizeof(tv));
  freeaddrinfo(si);

  if (listen(sockfd, 5) != 0)
  {
    printf("Failed to listen.\n");
    exit(0);
  }
  len = sizeof(client);
  char recvBuf[256];
  char sendBuf[256];
  bool clientIsActive = false;

  char expression[] = "^[A-Za-z_]+$";
  regex_t regularexpression;
  int reti;
  reti = regcomp(&regularexpression, expression, REG_EXTENDED);
  if (reti)
  {
    fprintf(stderr, "Could not compile regex.\n");
    exit(1);
  }

  int matches = 0;
  regmatch_t items;
  while (true)
  {
    if (clientIsActive == false)
    {
      if ((connfd = accept(sockfd, (struct sockaddr *)&client, (socklen_t *)&len)) == -1)
      {
        continue;
      }
      else
      {
        char buf[sizeof(PROTOCOL)] = PROTOCOL;
        send(connfd, buf, strlen(buf), 0);
        printf("Server protocol %s", buf);
      }
    }
    memset(recvBuf, 0, sizeof(recvBuf));
    memset(sendBuf, 0, sizeof(sendBuf));
    if (recv(connfd, recvBuf, sizeof(recvBuf), 0) == -1)
    {
      continue;
    }
    else if (strstr(recvBuf, "NICK"))
    {
      //Set up a new client space thingy.
      reti = regexec(&regularexpression, recvBuf, matches, &items, 0);
      if (!reti)
      {
        printf("Nick is accepted.\n");
      }
      else
      {
        printf("Nick is not accepted.\n");
      }
      printf("%s", recvBuf);
    }
  }
  regfree(&regularexpression);
  close(sockfd);
  return 0;

#ifdef DEBUG
  printf("Host %s, and port %d.\n", Desthost, port);
#endif
}