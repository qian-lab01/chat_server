#ifndef LST_TIMER
#define LST_TIMER

#include <stdio.h>
#include <time.h>
#include <arpa/inet.h>

#define BUFFER_SIZE 64
class util_timer;   // 前向声明

struct client_data {
    sockaddr_in address;
    int sockfd;
    char buffer[BUFFER_SIZE];
    util_timer *timer;
};

class util_timer{
public:
    util_timer *prev, *next;
    time_t expire;
    void (*cb_func)(client_data*);
    client_data *user_data;
    util_timer(): prev(nullptr), next(nullptr), expire(0), cb_func(nullptr), user_data(nullptr) {}
};


class sort_timer_list {
public:
    sort_timer_list() {
        dummy_head = new util_timer();
        dummy_tail = new util_timer();
        dummy_head->next = dummy_tail;
        dummy_tail->prev = dummy_head;
    }

    ~sort_timer_list() {
        util_timer *cur = dummy_head->next;
        while (cur!=dummy_tail) {
            util_timer *tmp = cur;
            cur = cur->next;
            delete tmp;
        }
        delete dummy_head;
        delete dummy_tail;
    }

    void add_timer(util_timer *timer) {
        if (!timer) return;
        util_timer *pre = dummy_head;
        util_timer *cur = dummy_head->next;
        while (cur!=dummy_tail && timer->expire >= cur->expire) 
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

    void delete_timer(util_timer *timer) {
        util_timer *pre = timer->prev;
        pre->next = timer->next;
        timer->next->prev = pre;
        delete timer;
        size--;
    }

    void modify_timer(util_timer *timer) {
        util_timer *pre = timer->prev;
        pre->next = timer->next;
        timer->next->prev = pre;
        add_timer(timer);
        size--;
    }

    void tick() {
        if (size==0) return;
        printf("time tick\n");
        time_t cur_time = time(NULL);
        util_timer *cur_head = dummy_head->next;
        int count = 0;
        while (cur_head->expire!=0 && cur_time >= cur_head->expire) {
            cur_head->cb_func(cur_head->user_data);
            dummy_head->next = cur_head->next;
            cur_head->next->prev = dummy_head;
            util_timer *tmp = cur_head;
            cur_head = cur_head->next;
            delete tmp;
            size--;
        }
    }
private:
    util_timer *dummy_head;
    util_timer *dummy_tail;
    int size=0;
};
#endif