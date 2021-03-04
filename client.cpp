#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/select.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <errno.h>
#include <netdb.h>
#include <regex.h>
#include <iostream>
#include <signal.h>
/* You will to add includes here */
#define DEBUG
#define PROTOCOL "HELLO 1\n"

void intSignal(int sig)
{
  printf("\n");
  exit(0);
}
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
  if (strlen(Destnick) > 12)
  {
    printf("Error: Name too long. Max 12 character. \n");
    exit(0);
  }
  char expression[] = "[A-Za-z0-9_]";
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
  reti = regexec(&regularexpression, argv[2], matches, &items, 0);
  if (reti)
  {
    printf("Nick is not accepted.\n");
    exit(0);
  }
  regfree(&regularexpression);
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

  char recvBuf[273];
  char sendBuf[260];
  char messageBuf[256];
  char workType[10];
  char otherName[12];
  int bytes;
  fd_set currentSockets;
  fd_set readySockets;
  FD_ZERO(&currentSockets);
  FD_ZERO(&readySockets);
  FD_SET(sockfd, &currentSockets);
  FD_SET(STDIN_FILENO, &currentSockets);
  int fdMax = sockfd;
  int nfds = 0;
  char writeBuf[256];
  signal(SIGINT, intSignal);
  while (true)
  {
    readySockets = currentSockets;
    if (fdMax < sockfd)
    {
      fdMax = sockfd;
    }
    nfds = select(fdMax + 1, &readySockets, NULL, NULL, NULL);
    if (nfds == -1)
    {
      printf("Something went wrong with select\n");
      break;
    }
    if (FD_ISSET(STDIN_FILENO, &readySockets))
    {
      memset(sendBuf, 0, sizeof(sendBuf));
      memset(writeBuf, 0, sizeof(writeBuf));
      std::cin.getline(writeBuf, sizeof(writeBuf));
      std::cin.clear();
      if (strlen(writeBuf) > 256)
      {
        printf("Message too long, try again.\n");
        FD_CLR(STDIN_FILENO, &readySockets);
        break;
      }
      else
      {
        sprintf(sendBuf, "MSG %s", writeBuf);
        send(sockfd, sendBuf, strlen(sendBuf), 0);
        FD_CLR(STDIN_FILENO, &readySockets);
      }
    }
    if (FD_ISSET(sockfd, &readySockets))
    {
      memset(recvBuf, 0, sizeof(recvBuf));
      bytes = recv(sockfd, recvBuf, sizeof(recvBuf), 0);
      if (bytes == -1)
      {
        printf("Failed to recive. \n");
        continue;
      }
      else
      {
        memset(workType, 0, sizeof(workType));
        sscanf(recvBuf, "%s", workType);
      }
      if (strstr(workType, "MSG"))
      {
        memset(messageBuf, 0, sizeof(messageBuf));
        memset(workType, 0, sizeof(workType));
        memset(otherName, 0, sizeof(otherName));
        sscanf(recvBuf, "%s %s %[^\n]", workType, otherName, messageBuf);
        if (strcmp(otherName, Destnick) != 0)
        {
          printf("%s: %s\n", otherName, messageBuf);
        }
      }
      else if (strstr(recvBuf, PROTOCOL) != nullptr)
      {
        memset(sendBuf, 0, sizeof(sendBuf));
        printf("Serverprotocol accepted, Protocol: %s", recvBuf);
        sprintf(sendBuf, "NICK %s", Destnick);
        send(sockfd, sendBuf, strlen(sendBuf), 0);
      }
      else if (strstr(recvBuf, "OK") != nullptr)
      {
        printf("Name accepted\n");
      }
      else if(strstr(recvBuf,"SERVER CLOSED") != nullptr)
      {
        printf("%s", recvBuf);
        exit(0);
      }
      else if(strstr(recvBuf,"ERROR") != nullptr)
      {
        printf("%s",recvBuf);
      }
      else if (strstr(recvBuf, "ERR ") != nullptr)
      {
        printf("%s", recvBuf);
        exit(0);
      }

      FD_CLR(sockfd, &readySockets);
    }
  }
  close(sockfd);
  return 0;
}
