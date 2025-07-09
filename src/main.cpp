#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "web_server.h"
using namespace std;

int main(int argc, char *argv[])
{
    int port = 10000;
    if (argc > 1)  
    { 
        // 获取端口号
        int port = atoi(argv[1]);
    }  
    
    string username = "mywebserver";
    string password = "12345678";
    string database_name = "webserver";

    
    // 日志写入方式，默认同步
    int log_write = 0;

    WebServer web_server(port, username, password, database_name, log_write);
    web_server.start();

    return 0;
}