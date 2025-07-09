#include "web_server.h"
using namespace std;

extern void addfd(int epollfd, int fd, bool one_shot);

// 添加信号捕捉
void addsig(int sig, void(handler)(int))
{
    struct sigaction sa;
    memset(&sa, '\0', sizeof(sa));
    sa.sa_handler = handler;
    sigfillset(&sa.sa_mask);
    assert(sigaction(sig, &sa, NULL) != -1);
}

bool WebServer::initSocket()
{
    // 创建监听套接字
    listenfd = socket(PF_INET, SOCK_STREAM, 0);
    int ret = 0;
    struct sockaddr_in address;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_family = AF_INET;
    address.sin_port = htons(port);

    // 端口复用
    int reuse = 1;
    setsockopt(listenfd, SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof(reuse));

    ret = bind(listenfd, (struct sockaddr *)&address, sizeof(address));
    ret = listen(listenfd, 5);
    return true;
}

bool WebServer::initEpoll()
{
    epollfd = epoll_create(5);
    if (epollfd == -1)
        return false;
    addfd(epollfd, listenfd, false);
    return true;
}

bool WebServer::initThreadPool()
{
    // 创建和初始化线程池
    try
    {
        pool = new threadpool<HttpConn>;
    }
    catch (...)
    {
        return false;
    }
    return true;
}

bool WebServer::initDeleteNoActiveConn()
{
    HttpConn::m_epollfd = epollfd;
    delete_no_active_conn.init(TIMESLOT);
    int ret = socketpair(PF_UNIX, SOCK_STREAM, 0, pipefd);
    assert(ret != -1);
    delete_no_active_conn.setnonblocking(pipefd[1]);
    delete_no_active_conn.addfd(epollfd, pipefd[0]);

    delete_no_active_conn.addsig(SIGALRM);
    delete_no_active_conn.addsig(SIGTERM);

    // setitimer代替alarm
    struct itimerval new_value;
    new_value.it_interval.tv_sec = TIMESLOT;
    new_value.it_interval.tv_usec = 0;
    new_value.it_value.tv_sec = 1;
    new_value.it_value.tv_usec = 0;
    setitimer(ITIMER_REAL, &new_value, NULL); // 非阻塞的

    DeleteNoActiveConn::epollfd = epollfd;
    DeleteNoActiveConn::pipefd = pipefd;

    return true;
}

bool WebServer::initMysql()
{
    sql = new Mysql(username, password, database_name);
    HttpConn::init_SQL(sql);
    return true;
}

bool WebServer::initLog()
{
    close_log = 0;
    if (log_write == 1)
        Log::get_instance()->init("./ServerLog", close_log, 2000, 800000, 800);
    else
        Log::get_instance()->init("./ServerLog", close_log, 2000, 800000, 0);
    return true;
}

// 初始化连接进来的客户端以及timer（用来删除非活跃连接）
bool WebServer::addClient()
{
    while (true)
    {
        // 有客户端连接进来
        struct sockaddr_in client_address;
        socklen_t client_addrlength = sizeof(client_address);
        int connfd = accept(listenfd, (struct sockaddr *)&client_address, &client_addrlength);
        cout << connfd << endl;
        if (connfd < 0)
        {
            if (errno == EAGAIN || errno == EWOULDBLOCK)
            {
                // 没有更多连接，这不是错误，继续等待下一个事件
                return true;
            }
            return false;
        }

        if (HttpConn::m_user_count >= MAX_FD)
        {
            // 目前连接数满了
            // 给客户端写一个信息： 服务器正忙
            close(connfd);
            return false;
        }
        // 把http加入到epollfd进行监听
        users[connfd].init(connfd, client_address);
        LOG_INFO("new client fd: %d connect\n", connfd);
        // 设置定时器用来删除非活跃连接
        UtilTimer *timer = new UtilTimer();

        users_timer[connfd].address = client_address;
        users_timer[connfd].sockfd = connfd;

        timer->cb_func = cb_func;
        timer->user_data = &users_timer[connfd];
        time_t cur_time = time(NULL);
        timer->expire = cur_time + 3 * TIMESLOT;

        users_timer[connfd].timer = timer;

        delete_no_active_conn.timer_lst.add_timer(timer);
    }
}

bool WebServer::dealWithSignal(bool &timeout, bool &stop_server)
{
    int sig;
    char signals[1024];
    int ret = recv(pipefd[0], signals, sizeof(signals), 0);
    if (ret == 0)
    {
        return false;
    }
    else if (ret == -1 && errno != EAGAIN && errno != EWOULDBLOCK) {
        return false;
    }
    for (int i = 0; i < ret; i++)
    {
        switch (signals[i])
        {
        case SIGALRM:
        {
            timeout = true;
            break;
        }
        case SIGTERM:
        {
            stop_server = true;
        }
        }
    }

    return true;
}

WebServer::WebServer(int port, string username, string password, string database_name, int log_write) : port(port), username(username), password(password), database_name(database_name), log_write(log_write)
{
    // 创建一个数组 用于保存所有的客户端信息
    users = new HttpConn[MAX_FD];
    users_timer = new client_data[MAX_FD];
    // 对SIGPIE信号进行处理
    addsig(SIGPIPE, SIG_IGN);
    if (!initSocket())
    {
        printf("initSocket is error");
        throw exception();
    }

    if (!initEpoll())
    {
        printf("initEpoll is error");
        throw exception();
    }

    if (!initThreadPool())
    {
        printf("initThreadPool is error");
        throw exception();
    }

    if (!initDeleteNoActiveConn())
    {
        printf("initDeleteNoActiveConn is error");
        throw exception();
    }

    if (!initMysql())
    {
        printf("initDeleteNoActiveConn is error");
        throw exception();
    }

    if (!initLog())
    {
        printf("initLog is error");
        throw exception();
    }
}

WebServer::~WebServer()
{
    close(epollfd);
    close(listenfd);
    close(pipefd[0]);
    close(pipefd[1]);
    delete[] users;
    delete[] users_timer;
    delete pool;
}

void WebServer::start()
{
    while (!stop_server)
    {

        int number = epoll_wait(epollfd, events, MAX_EVENT_NUMBER, -1);
        if ((number < 0) && (errno != EINTR))
        {
            LOG_ERROR("epoll failuer\n");
            break;
        }

        for (int i = 0; i < number; i++)
        {
            int sockfd = events[i].data.fd;
            if (sockfd == listenfd)
            {

                // 将新的客户的数据初始化,放入数组中,并放入定时器中
                if (!addClient())
                {
                    LOG_ERROR("add client fail!\n");
                }
            }
            else if (events[i].events & (EPOLLRDHUP | EPOLLHUP | EPOLLERR))
            {
                UtilTimer *timer = users_timer[sockfd].timer;
                // 对方异常断开或错误等事件
                users[sockfd].close_conn();
                cb_func(&users_timer[sockfd]);
                delete_no_active_conn.timer_lst.delete_timer(timer);
                LOG_INFO("closed client fd %d\n", users_timer[sockfd].sockfd);
            }
            else if (sockfd == pipefd[0] && events[i].events & EPOLLIN)
            {
                if (!dealWithSignal(timeout, stop_server))
                {
                    LOG_ERROR("deal with signal failure\n");
                }
            }
            else if (events[i].events & EPOLLIN)
            {
                UtilTimer *timer = users_timer[sockfd].timer;
                // 一次性把全部数据读完
                if (users[sockfd].read())
                {
                    LOG_INFO("deal with the client (%s)\n", inet_ntoa(users[sockfd].get_address()->sin_addr));
                    pool->append(users + sockfd);
                    // 刷新服务器的存续时间
                    time_t cur_time = time(NULL);
                    timer->expire = cur_time + 3 * TIMESLOT;
                    delete_no_active_conn.timer_lst.modify_timer(timer);
                }
                else
                {
                    LOG_INFO("read client data failure!\n");
                    users[sockfd].close_conn();
                    // 删除对应的timerlist表
                    cb_func(&users_timer[sockfd]);
                    if (timer)
                    {
                        delete_no_active_conn.timer_lst.delete_timer(timer);
                    }
                }
            }
            else if (events[i].events & EPOLLOUT)
            {
                LOG_INFO("send data to the client (%s)\n", inet_ntoa(users[sockfd].get_address()->sin_addr));
                UtilTimer *timer = users_timer[sockfd].timer;
                // 一次性把全部数据写完
                if (users[sockfd].write())
                {
                    time_t cur_time = time(NULL);
                    timer->expire = cur_time + 3 * TIMESLOT;
                    delete_no_active_conn.timer_lst.modify_timer(timer);
                }
                else
                {
                    users[sockfd].close_conn();
                }
            } 
        }

        if (timeout)
        {
            delete_no_active_conn.timer_handler();
            timeout = false;
        }
    }
}