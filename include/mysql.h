#ifndef MYSQL_H
#define MYSQL_H

#include <mysql_driver.h>
#include <mysql_connection.h>
#include <cppconn/statement.h>
#include <cppconn/prepared_statement.h>
#include <cppconn/resultset.h>
#include <cppconn/exception.h>
#include <iostream>
#include <string>
#include <nlohmann/json.hpp>
#include <time.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

using json = nlohmann::json;
using namespace std;

class Mysql
{
public:
    Mysql(string user, string pass, string name);

    ~Mysql() {}

    void createDatabase(const string &database_name);

    void createTableAccount(const string &table_name);

    void createTableRecord(const string &table_name);

    bool tableExists(const string &table_name);

    bool isValidTableName(const string &table_name);

    bool insertUser(const string &username, const string &password);

    bool saveRecord(const string &username, const string &question, const string &answer);

    bool getChatRecord(const string &username, string &responseData);

    string queryUserPassword(const string &username);

private:
    const string host = "tcp://127.0.0.1:3306";
    const string username;
    const string password;
    const string database_name;
    sql::Connection *conn;
};

#endif