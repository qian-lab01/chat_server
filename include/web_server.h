#ifndef WEB_SERVER_H
#define WEB_SERVER_H
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <stdio.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <fcntl.h>
#include <stdlib.h>
#include <sys/epoll.h>
#include <sys/time.h>
#include <iostream>
#include "locker.h"
#include "thread_pool.h"
#include "http_conn.h"
#include "no_active_conn.h"
#include "mysql.h"
#include "log.h"


#define MAX_FD 65536           // 最大的文件描述符个数
#define MAX_EVENT_NUMBER 10000 // 监听的最大的事件数量
const int TIMESLOT = 20;        // 删除非活跃客户端的最小时间
 
class WebServer
{
public:
    WebServer(int port, string username, string password, string database_name, int log_write);
    ~WebServer();
    void start();

private:
    bool initSocket();
    bool initEpoll();
    bool initThreadPool();
    bool initDeleteNoActiveConn();
    bool initMysql();
    bool initLog();
    bool addClient(); // 初始化连接进来的客户端以及timer（用来删除非活跃连接）
    bool dealWithSignal(bool &timeout, bool &stop_server);

    int log_write;
    int close_log;

private:
    int port;
    int listenfd;
    int epollfd; 
    bool stop_server;
    bool timeout;
    int pipefd[2];
    epoll_event events[MAX_EVENT_NUMBER];
    HttpConn *users;
    threadpool<HttpConn> *pool;

    // 删除非活跃连接
    client_data *users_timer;
    DeleteNoActiveConn delete_no_active_conn;

    //数据库
    Mysql *sql;
    string username;
    string password;
    string database_name;
};

#endif