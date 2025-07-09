#include <stdio.h>
#include <time.h>
#include <arpa/inet.h>
#include "no_active_conn.h"
#include "http_conn.h"

SortTimerList::SortTimerList()
{
    dummy_head = new UtilTimer();
    dummy_tail = new UtilTimer();
    dummy_head->next = dummy_tail;
    dummy_tail->prev = dummy_head;
}

SortTimerList::~SortTimerList()
{
    UtilTimer *cur = dummy_head->next;
    while (cur != dummy_tail)
    {
        UtilTimer *tmp = cur;
        cur = cur->next;
        delete tmp;
    }
    delete dummy_head;
    delete dummy_tail;
}

void SortTimerList::add_timer(UtilTimer *timer)
{
    if (!timer) return;
      
    UtilTimer *pre = dummy_head;
    UtilTimer *cur = dummy_head->next;

    while (cur->expire != 0 && timer->expire >= cur->expire)
    {
        pre = cur;
        cur = cur->next;
    }
    
    pre->next = timer;
    timer->next = cur;
    timer->prev = pre;
    timer->next->prev = timer;
    size++;
    
}

void SortTimerList::delete_timer(UtilTimer *timer)
{
    UtilTimer *pre = timer->prev;
    pre->next = timer->next;
    timer->next->prev = pre;
    delete timer;  
    size--;
}

void SortTimerList::modify_timer(UtilTimer *timer)
{
    UtilTimer *pre = timer->prev;
    pre->next = timer->next;
    timer->next->prev = pre;
    add_timer(timer);
    size--;
}

void SortTimerList::tick()
{
    if (size == 0)
        return;
    printf("time tick\n");
    time_t cur_time = time(NULL); 
    UtilTimer *cur_head = dummy_head->next;
    while (cur_head->expire != 0 && cur_time >= cur_head->expire)
    {
        cur_head->cb_func(cur_head->user_data);
        dummy_head->next = cur_head->next;
        cur_head->next->prev = dummy_head;
        UtilTimer *tmp = cur_head;
        cur_head = cur_head->next;
        delete tmp;
        size--;
    } 
}

void DeleteNoActiveConn::setnonblocking(int fd) {
    int opt = fcntl(fd, F_GETFL);
    opt |= O_NONBLOCK;
    fcntl(fd, F_SETFL, opt);
}

void DeleteNoActiveConn::addfd(int epollfd, int fd) {
    epoll_event event;
    event.data.fd = fd; 
    event.events = EPOLLIN | EPOLLET;
    epoll_ctl(epollfd, EPOLL_CTL_ADD, fd, &event);
    setnonblocking(fd);
}

void DeleteNoActiveConn::sig_handler(int sig)
{
    int save_errno = errno;
    int msg = sig;
    send(pipefd[1], (char*)&msg, 1, 0);
    errno = save_errno;
}

void DeleteNoActiveConn::addsig(int sig) {
    struct sigaction sa;
    memset(&sa, '\0', sizeof(sa));
    sa.sa_handler = sig_handler;
    sa.sa_flags |= SA_RESTART;
    sigfillset(&sa.sa_mask);
    assert(sigaction(sig, &sa, NULL)!=-1);
} 

void DeleteNoActiveConn::timer_handler() {
    timer_lst.tick();
}

void DeleteNoActiveConn::init(int timer_slot) {
    timer_slot = timer_slot;
    timer_lst = SortTimerList();
}

int *DeleteNoActiveConn::pipefd = 0;
int DeleteNoActiveConn::epollfd = 0;

void cb_func(client_data *user_data) {
    epoll_ctl(DeleteNoActiveConn::epollfd, EPOLL_CTL_DEL, user_data->sockfd, 0);
    assert(user_data);
    close(user_data->sockfd);
    HttpConn::m_user_count--;
    printf("close fd %d\n", user_data->sockfd);
}