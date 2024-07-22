#include "server.h"

#define DATABASE "my.db"

// 通信双方的信息结构体


enum MSGTYPE{
    REGISTER,
    LOGIN,
    QUERY,//查询
    HISTORY
};

int do_register(int sockfd, MSG_T *msg, sqlite3 *db)
{
    char sql[128]={0};
    char *errmsg=NULL;
   // sprintf(sql,"insert into user values('%s','%s');",msg->name,msg->data);
    sprintf(sql, "insert into user values('%s', '%s');", msg->name, msg->data);
    if(sqlite3_exec(db,sql,NULL,NULL,&errmsg) != SQLITE_OK)
    {
        perror(errmsg);
        memset(msg->data, 0, strlen(msg->data));
		sprintf(msg->data, "user(%s) already exist.!", msg->name);
    }
    else
    {
        printf("client user(%s) register success\n", msg->name);
		memset(msg->data, 0, strlen(msg->data));
		strcpy(msg->data, "register ok !");
    }
    //返回相关信息
    if (send(sockfd, msg, sizeof(MSG_T), 0) < 0)
	{
		printf("fail to send\n");
		return 0;
	}
    return 1;
}
int do_login(int sockfd, MSG_T *msg, sqlite3 *db)
{
    char sql[128] = {0};
	char *errmsg = NULL, **result = NULL;
    int nrow;//有多少条记录
    int ncolumn;//每条记录有多少信息
    printf("%s,%d\n",__func__,__LINE__);
   // sprintf(sql,"select * from user where name='%s and passwd='%s';", msg->name,msg->data);
   sprintf(sql, "select * from user where name='%s' and password='%s';", msg->name, msg->data);
    //if(sqlite3_get_table(db,sql,&result,&nrow,&ncolumn,&errmsg)!=SQLITE_OK)
    if (sqlite3_get_table(db, sql, &result, &nrow, &ncolumn, &errmsg) != SQLITE_OK)
    {
        perror(errmsg);
      //  printf('get table failed');
    }
    else
    {
        printf("sqlite3 get table ok! \n");
    }
  
    if(1 == nrow)
    {
        printf("client user(%s) login success\n",msg->name);
        memset(msg->data, 0, strlen(msg->data));
		strcpy(msg->data, "OK");
    }
    else //用户名或密码错误
	{
		printf("client user(%s) login fail! \n", msg->name);
		memset(msg->data, 0, strlen(msg->data));
		strcpy(msg->data, "user or passwd wrong! \n");
	}
    //返回应答
	if (send(sockfd, msg, sizeof(MSG_T), 0) < 0)
	{
		printf("fail to send\n");
		return 0;
	}
    printf("%d %d\n",nrow,ncolumn);
    printf("%s,%d\n",__func__,__LINE__);
	return 1;
}

int do_searchword(MSG_T *msg, char *word)
{
    //读取文件 行数据(一行一行读取),对比要查询的单词
    //如果成功，该函数返回相同的 str 参数。
    //如果到达文件末尾或者没有读取到任何字符，str 的内容保持不变，并返回一个空指针。
    //如果发生错误，返回一个空指针。
    FILE *fp=NULL;
    int len=strlen(word);
    char row_data[512]={'\0'};
    int res=0;
    char *p;

    if((fp=fopen("dict.txt","r"))==NULL)
    {
         perror("fail to open dict.txt.\n");
        return -1;
    }
    while(fgets(row_data,512,fp)!=NULL)
    {
        res=strncmp(row_data,word,len);//每行对比前word_len个字节
        if(res !=0)
            continue;
        if(row_data[len] != ' ')//单词跟注释之间没有空格，说明只是子串
            continue;
        p = row_data + len;
        
        while (*p == ' ')// 找到了单词,跳过所有的空格
            p++;
        strcpy(msg->data, p);
        fclose(fp);
        return 1;
    }
    fclose(fp);
    return 0; //文件比对结束未找到单词
}
void get_date(char *data)
{
    time_t rowtime; 
    struct tm *info;
    time(&rowtime);
 
    //进行时间格式转换
    info = localtime(&rowtime);
 
    sprintf(data, "%d-%d-%d %d:%d:%d", info->tm_year + 1900, info->tm_mon + 1, info->tm_mday,
                                       info->tm_hour, info->tm_min, info->tm_sec);
    printf("get date is : %s\n", data);     
}
int do_query(int sockfd, MSG_T *msg, sqlite3 *db)
{
    char sql[128] = {0};
	char word[64] = {0};
	int found = 0;
    char date[128] = {0};
    char *errmsg;

    printf("\n");

    strcpy(word,msg->data);//取出要查询的单词
    found=do_searchword(msg,word);//开始在文件中查询该单词
    if(1 == found)//若找到 插入历史记录表中
    {
        get_date(date);
        sprintf(sql,"insert into record values('%s','%s','%s')",msg->name,date,word);
        if (sqlite3_exec(db, sql, NULL, NULL, &errmsg) != SQLITE_OK)
	    {
	        perror(errmsg);
	        return -1;
	    }
	    else
	    {
	        printf("sqlite3 insert record done.\n");
	    }
    }
    else if (found == 0)//没有找到
    {
        memset(msg->data, 0, strlen(msg->data));
        strcpy(msg->data, "Not found!\n");
    }
    else if (found == -1)//dict.txt文件打开失败
    {
        memset(msg->data, 0, strlen(msg->data));
        strcpy(msg->data, "fail to open dict.txt.");
    }
 
	//将查询的结果发送给客户端
	send(sockfd, msg, sizeof(MSG_T), 0);
    
    return 0;
}
int history_callback(void* arg,int colCount,char** colValue,char** colName)
{
    int acceptfd;
	MSG_T msg;
 
	acceptfd = *((int *)arg);
	sprintf(msg.data, "%s , %s", colValue[1], colValue[2]);
	send(acceptfd, &msg, sizeof(MSG_T), 0);
 
	return 0;
}
int do_history(int sockfd, MSG_T *msg, sqlite3 *db)
{
    char sql[128] = {0};
	char *errmsg;
    sprintf(sql,"select * from record where name ='%s'",msg->name);
    if(sqlite3_exec(db,sql,history_callback,(void *)&sockfd,&errmsg) != SQLITE_OK)
    {
        perror(errmsg);
    }
    else
    printf("sqlite3 query record done.\n");
}
void do_work(int acceptfd,sqlite3 *db)
{
    MSG_T recv_msg;
    // 默认是阻塞式接收
	// 如果客户端断开连接 或是 主动发送close关闭连接,recv会返回0
	// recv的返回值：<0 出错; =0 连接关闭; >0 接收到数据大小
    while(recv(acceptfd,&recv_msg,sizeof(MSG_T),0) > 0)
    {
        switch (recv_msg.type)
        {
        case REGISTER:
        do_register(acceptfd,&recv_msg,db);
            break;
        case LOGIN:
        do_login(acceptfd,&recv_msg,db);
            break;
        case QUERY:
        do_query(acceptfd,&recv_msg,db);
            break;
        case HISTORY:
        do_history(acceptfd,&recv_msg,db);
            break;    
        default:
            printf("error data msg");
        }
    }
}
void handler(int sig)
{
    switch (sig)
    {
    case SIGCHLD://说明有子进程结束了
        waitpid(-1,NULL,1);//回收退出子进程的资源，防止成为僵尸进程
        break;
    
    default:
        break;
    }
}
int init_tcp_server_socket(const char *ip,short port)
{
     int sockfd ;//套接字描述符，对于服务端而言，该描述符用来监听和建立连接
    //1，创建套接字
    sockfd = socket(AF_INET,//tcp属于 IPV4协议族
            SOCK_STREAM,//流式套接字专门用于tcp
            0   //不知名的私有协议(指的应用层协议)
        );
    if(-1==sockfd)
    {
        perror("创建套接字失败");
        return -1;
    }

    //2，初始化服务端自己的套接字地址
    struct sockaddr_in server_addr;//服务端自己的套接字地址  ip + 端口
                                    //服务端程序在哪里运行就是哪个ip地址
                                    //端口号 随意(但是不能是知名应用的端口)，也不能跟本机其他程序端口冲突
    memset(&server_addr,0,sizeof(server_addr));//把整个结构体内存全部置0
    server_addr.sin_family = AF_INET;//IPV4协议族
    server_addr.sin_port = htons(port);//指定端口为 10000
    inet_aton(ip,&(server_addr.sin_addr));//指定ip地址

    //3，绑定套接字地址
    int r = bind(sockfd,(struct sockaddr*)&server_addr,sizeof(server_addr));
    if(-1 == r)
    {
        perror("绑定套接字地址失败");
        close(sockfd);
        return -1;
    }

    //4，监听套接字，最大数量为10
    r = listen(sockfd,10);
    if(-1 == r)
    {
        perror("监听套接字失败");
        close(sockfd);
        return -1;
    }
    return sockfd;
}
int main(int argc,char *argv[])
{
    if(argc != 3)
    {
        perror("argc error");
        return -1;
    }
    
    sqlite3 *db=NULL;
    char *errmsg=NULL;
    char sql[128]={0};
    pid_t pid;
    int acceptfd;//用于通信的文件描述符
    /*进行数据库相关操作*/
    if(sqlite3_open(DATABASE,&db)!=SQLITE_OK)
    {
        perror(sqlite3_errmsg(db));
        return -1;
    }
    printf("%s,%d\n",__func__,__LINE__);
    printf("sqlite3 open %s success.\n", DATABASE);
    sprintf(sql,"create table if not exists user(name text primary key,password text);");
    if(sqlite3_exec(db,sql,NULL,NULL,&errmsg) !=SQLITE_OK)//用户表不存在则创建
    {
        perror(errmsg);
        return -1;
    }
    memset(sql,0,sizeof(errmsg));
    sprintf(sql,"create table if not exists record(name text,data text,word text);");
    if(sqlite3_exec(db,sql,NULL,NULL,&errmsg)!=SQLITE_OK)//记录表不存在则创建
    {
        perror(errmsg);
        return -1;
    }
    /*初始化服务端套接字，监听客户端请求，同时设置并发服务器*/
    int sockfd = init_tcp_server_socket(argv[1],atoi(argv[2]));
    if(-1==sockfd)
    {
        printf("初始化tcp服务端失败\n");
        return -1;
    }

    signal(SIGCHLD,handler);

    while(1)
    {
         struct sockaddr_in client_addr;
        int len = sizeof(client_addr);
        acceptfd = accept(sockfd,//是否有客户端向当前套接字描述符发起连接请求，如果没有会阻塞
                (struct sockaddr*)&client_addr,//传入client_addr地址，如果accept成功返回了，那么client_addr
                                                //保存了客户端的套接字地址
                &len
            );
        //printf("%s函数的第%d行\n",__func__,__LINE__);
        printf("客户端连接成功了，它的ip为:%s,端口为:%hu\n",inet_ntoa(client_addr.sin_addr),ntohs(client_addr.sin_port));

        //创建子进程和已经连接成功的客户端通信，父进程继续处理其他客户端的连接请求
        pid_t pid = fork();
        if(-1==pid)
        {
            perror("create pid failed\n");
            close(acceptfd);
            close(sockfd);
            return -1;
        }
        else if(pid == 0)//子进程,处理客户端与数据库相关信息
        {
            do_work(acceptfd,db);
           // communication(acceptfd);
            exit(0);
        }

        //父进程继续循环执行 accept
        //wait(NULL); //绝对不能在此wait，因为wait会阻塞，父进程就无法继续响应其他客户端的连接
        //应该捕捉SIGCHLD信号，在信号处理函数中 wait回收子进程资源，防止子进程成为僵尸进程

    }
}