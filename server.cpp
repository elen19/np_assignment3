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
#include <signal.h>
#include <regex.h>
#include <string>
/* You will to add includes here */
#define DEBUG
#define PROTOCOL "HELLO 1\n"

struct cli
{
  int sockID;
  char cliName[12];
};
std::vector<cli> clients;
void intSignal(int sig)
{
  printf("\n");
  for (size_t i = 0; i < clients.size(); i++)
  {
    send(clients.at((int)i).sockID, "SERVER CLOSED\n", strlen("SERVER CLOSED\n"), 0);
  }
  exit(0);
}

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
  sa.ai_flags = AI_PASSIVE;
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
  freeaddrinfo(si);

  if (listen(sockfd, 5) != 0)
  {
    printf("Failed to listen.\n");
    exit(0);
  }
  char expression[] = "^[A-Za-z0-9_]+$";
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
  len = sizeof(client);
  char recvBuf[260];
  char sendBuf[273];
  char workType[4];
  char messageBuf[256];
  char nameLenght[15];
  fd_set currentSockets;
  fd_set readySockets;
  FD_ZERO(&currentSockets);
  FD_ZERO(&readySockets);
  FD_SET(sockfd, &currentSockets);
  int fdMax = sockfd;
  int nfds = 0;
  int reciver;
  bool sameName = false;
  signal(SIGINT, intSignal);
  while (true)
  {
    readySockets = currentSockets;

    nfds = select(fdMax + 1, &readySockets, NULL, NULL, NULL);
    if (nfds == -1)
    {
      printf("Something went wrong with select.\n");
      printf("%s\n", strerror(errno));
      break;
    }
    for (int i = sockfd; i < fdMax + 1; i++)
    {
      if (FD_ISSET(i, &readySockets))
      {
        if (i == sockfd)
        {
          if ((connfd = accept(sockfd, (struct sockaddr *)&client, (socklen_t *)&len)) == -1)
          {
            continue;
          }
          else
          {
            struct cli newClient;
            newClient.sockID = connfd;
            FD_SET(newClient.sockID, &currentSockets);
            clients.push_back(newClient);
            char buf[sizeof(PROTOCOL)] = PROTOCOL;
            send(connfd, buf, strlen(buf), 0);
            printf("Server protocol %s", buf);
            if (newClient.sockID > fdMax)
            {
              fdMax = newClient.sockID;
            }
          }
        }
        else
        {
          std::string temp = {};
          memset(recvBuf, 0, sizeof(recvBuf));
          reciver = recv(i, recvBuf, sizeof(recvBuf), 0);
          if (reciver <= 0)
          {
            close(i);
            for (size_t j = 0; j < clients.size(); j++)
            {
              if (i == clients[j].sockID)
              {
                clients.erase(clients.begin() + j);
                FD_CLR(i, &currentSockets);
                break;
              }
            }
            continue;
          }
          else
          {
            temp = recvBuf;
            
            while (strlen(recvBuf) > 0)
            {
              if (strstr(recvBuf, "MSG ") != nullptr)
              {
                memset(sendBuf, 0, sizeof(sendBuf));
                memset(workType, 0, sizeof(workType));
                memset(messageBuf, 0, sizeof(messageBuf));
                sscanf(recvBuf, "%s %[^\n]", workType, messageBuf);
                for (size_t j = 0; j < clients.size(); j++)
                {
                  if (i == clients.at(j).sockID)
                  {
                    sprintf(sendBuf, "%s %s %s\n", workType, clients.at(j).cliName, messageBuf);
                    break;
                  }
                }

                for (size_t j = 0; j < clients.size(); j++)
                {
                  send(clients.at(j).sockID, sendBuf, strlen(sendBuf), 0);
                }
                temp.erase(0, strlen(workType) + strlen(messageBuf) +2 );
                strcpy(recvBuf, temp.c_str());
              }
              else if (strstr(recvBuf, "NICK ") != nullptr)
              {
                //Set up a new client space thingy.
                sameName = false;
                for (size_t j = 0; j < clients.size(); j++)
                {
                  if (i == clients.at(j).sockID)
                  {

                    memset(nameLenght, 0, sizeof(nameLenght));
                    sscanf(recvBuf, "%s %[^\n]", workType, nameLenght);
                    if (strlen(nameLenght) > 12)
                    {
                      send(i, "ERROR Name too long! Max 12 characters.\n", strlen("ERROR Name too long! Max 12 characters.\n"), 0);
                      sameName = true;
                    }
                    else
                    {
                      sscanf(recvBuf, "%s %s", workType, clients.at(j).cliName);
                    }
                    for (size_t k = 0; k < clients.size() && !sameName; k++)
                    {
                      if (k != j && strcmp(clients.at(k).cliName, clients.at(j).cliName) == 0 && strlen(clients.at(k).cliName) == strlen(clients.at(j).cliName))
                      {
                        sameName = true;
                        send(i, "ERROR Name is already in use on server.\n", strlen("ERROR Name is already in use on server.\n"), 0);
                      }
                    }
                    if (sameName == false)
                    {

                      reti = regexec(&regularexpression, clients.at(j).cliName, matches, &items, 0);
                      if (reti)
                      {
                        printf("Nick is not accepted.\n");
                        send(i, "ERROR Illegal characters used!\n", strlen("ERROR Illegal characters used!\n"), 0);
                      }
                      else
                      {

                        printf("Clients name is: %s\n", clients.at(j).cliName);
                        printf("Name is allowed.\n");
                        send(i, "OK\n", strlen("OK\n"), 0);
                        break;
                      }
                    }
                    printf("%s\n", nameLenght);
                  }
                }
                temp.erase(0, strlen(workType) + strlen(nameLenght) +2);
                strcpy(recvBuf, temp.c_str());  
              }
              else
              {
                send(i, "ERROR Wrong format on the message sent.\n", strlen("ERROR Wrong format on the message sent.\n"), 0);
              }
            }
          }
        }
        FD_CLR(i, &readySockets);
      }
    }
  }
  regfree(&regularexpression);
  close(sockfd);
  return 0;

#ifdef DEBUG
  printf("Host %s, and port %d.\n", Desthost, port);
#endif
}
