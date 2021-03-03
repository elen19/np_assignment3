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
#include <sys/select.h>
#include <vector>
/* You will to add includes here */
#define DEBUG
#define PROTOCOL "HELLO 1\n"

struct cli
{
  int sockID;
  struct sockaddr_in addr;
  char cliName[12];
};
std::vector<cli> clients;

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
  char recvBuf[260];
  char sendBuf[273];
  char workType[4];
  char messageBuf[256];
  fd_set currentSockets;
  fd_set readySockets;
  FD_ZERO(&currentSockets);
  FD_ZERO(&readySockets);
  FD_SET(sockfd, &currentSockets);
  FD_SET(STDIN_FILENO, &currentSockets);
  int fdMax = sockfd;
  int nfds = 0;
  while (true)
  {
    readySockets = currentSockets;
    for (size_t i = 0; i < clients.size(); i++)
    {
      if (fdMax < clients.at(i).sockID)
      {
        fdMax = clients.at(i).sockID;
      }
    }
    if (fdMax < sockfd)
    {
      fdMax = sockfd;
    }
    nfds = select(fdMax + 1, &readySockets, NULL, NULL, NULL);
    if (nfds == -1)
    {
      printf("Something went wrong with select");
      break;
    }
    if (FD_ISSET(sockfd, &readySockets))
    {
      if ((connfd = accept(sockfd, (struct sockaddr *)&client, (socklen_t *)&len)) == -1)
      {
        continue;
      }
      else
      {
        struct cli newClient;
        newClient.sockID = connfd;
        newClient.addr = client;
        FD_SET(newClient.sockID, &currentSockets);
        clients.push_back(newClient);
        char buf[sizeof(PROTOCOL)] = PROTOCOL;
        send(connfd, buf, strlen(buf), 0);
        printf("Server protocol %s", buf);
      }
      FD_CLR(sockfd,&readySockets);
    }
    for (size_t i = 0; i < clients.size(); i++)
    {
      if (FD_ISSET(clients.at(i).sockID, &readySockets))
      {
        memset(recvBuf, 0, sizeof(recvBuf));
        if (recv(clients.at(i).sockID, recvBuf, sizeof(recvBuf), 0) == -1)
        {
          continue;
        }
        else if (strstr(recvBuf, "MSG ") != nullptr)
        {
          memset(sendBuf, 0, sizeof(sendBuf));
          memset(workType,0,sizeof(workType));
          memset(messageBuf,0,sizeof(messageBuf));
          sscanf(recvBuf,"%s %[^\n]",workType,messageBuf);
          sprintf(sendBuf, "%s %s %s",workType, clients.at(i).cliName, messageBuf);
          for (size_t j = 0; j < clients.size(); j++)
          {
            if (j != i)
            {
              send(clients.at(j).sockID, sendBuf, strlen(sendBuf), 0);
            }
          }
        }
        else if (strstr(recvBuf, "NICK ") != nullptr)
        {
          //Set up a new client space thingy.
          sscanf(recvBuf,"%s %s", workType, clients.at(i).cliName);
          printf("Clients name is: %s\n", clients.at(i).cliName);
          printf("Name is allowed.\n");
        }
        FD_CLR(clients.at(i).sockID,&readySockets);
      }
    }
  }
  close(sockfd);
  return 0;

#ifdef DEBUG
  printf("Host %s, and port %d.\n", Desthost, port);
#endif
}
