#include "client.h"
// 通信双方的信息结构体


enum MSGTYPE{
    REGISTER,
    LOGIN,
    QUERY,//查询
    HISTORY,
    QUIT
};
int sockfd;
void handler(int sig)
{
    switch (sig)
    {
    case SIGINT://说明有子进程结束了
       // waitpid(-1,NULL,1);//回收退出子进程的资源，防止成为僵尸进程
        close(sockfd);
        exit(0);
        break;
    
    default:
        break;
    }
}
int connect_tcp_server(const char *ip,short port)
{
    int sockfd ;
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
    //3，向服务端发送连接请求
    struct sockaddr_in server_addr;//服务端的套接字地址  ip + 端口
                                    //服务端程序在哪里运行就是哪个ip地址
                                    //端口号 随意(但是不能是知名应用的端口)，也不能跟本机其他程序端口冲突
    memset(&server_addr,0,sizeof(server_addr));//把整个结构体内存全部置0
    server_addr.sin_family = AF_INET;//IPV4协议族
    server_addr.sin_port = htons(port);//指定端口为 10000
    inet_aton(ip,&(server_addr.sin_addr));//指定ip地址
    int r = connect(sockfd,(struct sockaddr*)&server_addr,sizeof(server_addr));//第二个参数需要指定服务端的套接字地址
    if(-1==r)
    {
        perror("连接失败");
        close(sockfd);
        return -1;
    }

    return sockfd;
}
// 注册
int do_register(int sockfd, MSG_T *msg)
{
	printf("register ... \n");
	
	memset(msg, 0, sizeof(MSG_T));
	msg->type = REGISTER;
	printf("input name:"); 
    scanf("%s", msg->name);
     getchar();
	printf("input passwd:");
     scanf("%s", msg->data); 
     getchar();
	
	if (send(sockfd, msg, sizeof(MSG_T), 0) < 0)
	{
		printf("fail to send register msg.\n");
		return -1;
	}
	
	if (recv(sockfd, msg, sizeof(MSG_T), 0) < 0)
	{
		printf("fail to register.\n");
		return -1;
	}
	
	//ok !  或者 user already exist.
	printf("%s\n", msg->data);
	
	return 1;
}
int do_login(int sockfd, MSG_T *msg)
{
	printf("login ...\n");
	
	memset(msg, 0, sizeof(MSG_T));
	msg->type = LOGIN;
	printf("input name:"); scanf("%s", msg->name); getchar();
	printf("input passwd:"); scanf("%s", msg->data); getchar();
	
	if (send(sockfd, msg, sizeof(MSG_T), 0) < 0)
	{
		printf("fail to send register msg.\n");
		return -1;
	}
	
	if (recv(sockfd, msg, sizeof(MSG_T), 0) < 0)
	{
		printf("fail to login.\n");
		return -1;
	}
	
	//登录成功
	if (strncmp(msg->data, "OK", 3) == 0)
	{
		printf("login ok! \n");
		return 1;
	}
	else
	{
		printf("%s\n", msg->data);
	}
	
	return 0;
}
// 单词查询
int do_query(int sockfd, MSG_T *msg)
{
	printf("query ...\n");
	
	msg->type = QUERY;
	while(1)
	{
		printf("input word:"); scanf("%s", msg->data); getchar();
		
		// 输入是"#"表示退出本次查询
		if (strncmp(msg->data, "#", 1) == 0)
			break;
		
		//将要查询的单词发送给服务器
		if (send(sockfd, msg, sizeof(MSG_T), 0) < 0)
		{
			printf("fail to send.\n");
			return -1;
		}
		
		//等待服务器，传递回来的单次的注释信息
		if (recv(sockfd, msg, sizeof(MSG_T), 0) < 0)
		{
			printf("fail to recv.\n");
			return -1;
		}
		printf("%s\n", msg->data);
	}
	
	return 1;
}
// 历史记录查询
int do_history(int sockfd, MSG_T *msg)
{
	printf("history ...\n");
	
	msg->type = HISTORY;
 
	//将消息发送给服务器
	if (send(sockfd, msg, sizeof(MSG_T), 0) < 0)
	{
		printf("fail to send.\n");
		return -1;
	}
		
	while(1)
	{
		//等待服务器，传递回来的单次的注释信息
		if (recv(sockfd, msg, sizeof(MSG_T), 0) < 0)
		{
			printf("fail to recv.\n");
			return -1;
		}
 
		if (msg->data[0] == '\0')
		    break;
 
		//输出历史记录信息    
		printf("%s\n", msg->data);
	}
	
	return 1;
 
}
int word_menu(int sockfd)
{
    while(1)
	{
		printf("***********************************************\n");
		printf("*****1.query_word  2.history_record  3.quit****\n");
		printf("***********************************************\n");
		printf("please choose:");
		
		int input_nbr = 0;
        MSG_T send_msg;
		scanf("%d", &input_nbr);
		getchar();//回收垃圾字符
		
		
		switch (input_nbr)
		{
			case 1:
				do_query(sockfd, &send_msg);
				break;
				
			case 2:
				do_history(sockfd, &send_msg);
				break;
				
			case 3:
				close(sockfd);
				exit(0);
				break;
			
			default:
				printf("Invalid data cmd. \n");
				break;
		}
	}
	
	return 0;
}
int main(int argc,char *argv[])
{
    if(argc != 3)
    {
        perror("argc error");
        return -1;
    }
    signal(SIGINT,handler);
    int input_nbr;
    MSG_T send_msg;
    int sockfd = connect_tcp_server(argv[1],atoi(argv[2]));
    if(sockfd == -1)
    {
        return -1;
    }
    while(1)
    {
        printf("***********************************************\n");
		printf("*******  1.register   2.login   3.quit  *******\n");
		printf("***********************************************\n");
		printf("please choose:");
        scanf("%d", &input_nbr);
		getchar();//回收垃圾字符
        switch (input_nbr)// 一级菜单
		{
			case 1:
				do_register(sockfd, &send_msg);
				break;
				
			case 2:
				if (do_login(sockfd, &send_msg) == 1)
				{
					word_menu(sockfd);
				}
				break;
				
			case 3:
				close(sockfd);
				exit(0);
				break;
			
			default:
				printf("Invalid data cmd. \n");
				break;
		}
    }
}