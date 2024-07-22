#ifndef _CLIENT_H

#define _CLIENT_H
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <sys/types.h>          /* See NOTES */
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <sqlite3.h>
#include <signal.h>
#include <time.h>

#include <sys/wait.h>
typedef struct {
	int type;
	char name[32];
	char data[256];
}MSG_T;
/*处理僵尸进程，以防浪费系统资源*/
void handler(int sig);
int connect_tcp_server(const char *ip,short port);
int do_register(int sockfd, MSG_T *msg);
int do_login(int sockfd, MSG_T *msg);
int do_query(int sockfd, MSG_T *msg);
int word_menu(int sockfd);
#endif