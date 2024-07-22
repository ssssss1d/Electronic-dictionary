#ifndef _SERVER_H

#define _SERVER_H
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

void do_work(int acceptfd,sqlite3 *db);
/*处理僵尸进程，以防浪费系统资源*/
void handler(int sig);
/*初始化服务端套接字*/
int init_tcp_server_socket(const char *ip,short port);
int do_register(int sockfd, MSG_T *msg, sqlite3 *db);
int do_login(int sockfd, MSG_T *msg, sqlite3 *db);
int do_query(int sockfd, MSG_T *msg, sqlite3 *db);
int do_history(int sockfd, MSG_T *msg, sqlite3 *db);
int do_searchword(MSG_T *msg, char *word);
void get_date(char *data);
int history_callback(void* arg,int colCount,char** colValue,char** colName);
#endif