#ifndef NO_ACTIVE_CONN_H
#define NO_ACTIVE_CONN_H

#include <stdio.h>
#include <time.h>
#include <arpa/inet.h>
#include "http_conn.h"

#define BUFFER_SIZE 64
class UtilTimer; // 前向声明

struct client_data
{
    sockaddr_in address;
    int sockfd;
    UtilTimer *timer;
};

class UtilTimer
{
public:
    UtilTimer *prev, *next;
    time_t expire;
    void (*cb_func)(client_data *);
    client_data *user_data;
    UtilTimer() : prev(nullptr), next(nullptr), expire(0), cb_func(nullptr), user_data(nullptr) {}
};

// expire升序的双链表
class SortTimerList
{
public:
    SortTimerList();
    ~SortTimerList();
    void add_timer(UtilTimer *timer);
    void delete_timer(UtilTimer *timer);
    void modify_timer(UtilTimer *timer);
    void tick();

private:
    UtilTimer *dummy_head;
    UtilTimer *dummy_tail;
    int size = 0;
};

class DeleteNoActiveConn
{
public:
    DeleteNoActiveConn() {}

    ~DeleteNoActiveConn() {}

    void init(int timer_slot);

    void setnonblocking(int fd);

    void addfd(int epollfd, int fd);

    static void sig_handler(int sig);

    void addsig(int sig);

    void timer_handler();

public:
    int timer_slot;
    static int *pipefd;
    static int epollfd;
    SortTimerList timer_lst;
};

// 从epollfd取消监听并关闭句柄
void cb_func(client_data *user_data); 

#endif