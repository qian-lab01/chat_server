#include "mysql.h"

Mysql::Mysql(string user, string pass, string name) : username(user), password(pass), database_name(name)
{
    sql::mysql::MySQL_Driver *driver = sql::mysql::get_mysql_driver_instance();
    try
    {
        conn = driver->connect(host, username, password);
    }
    catch (sql::SQLException &e)
    {
        cerr << "MySQL connection failed!" << database_name << endl;
    }
    try
    {
        conn->setSchema(database_name);
    }
    catch (sql::SQLException &e)
    {
        cerr << "Database does not exist, creating database: " << database_name << endl;
        createDatabase(database_name);
        conn->setSchema(database_name);
    }
}

void Mysql::createDatabase(const string &database_name)
{
    try
    {
        sql::Statement *stat = conn->createStatement();
        stat->execute("create table if not exits " + database_name + "");
        std::cout << "Database created or already exists: " << database_name << std::endl;
        delete stat;
    }
    catch (sql::SQLException &e)
    {
        std::cerr << "Error creating table: " << e.what() << std::endl;
    }
}

void Mysql::createTableAccount(const string &table_name)
{
    try
    {
        sql::Statement *stat = conn->createStatement();
        stat->execute("create table if not exists " + table_name +
                      " (id int primary key auto_increment, "
                      "username varchar(255) not null unique, "
                      "password varchar(255) not null)");
        std::cout << "user table created successfully" << endl;
        delete stat;
    }
    catch (sql::SQLException &e)
    {
        std::cerr << "Error creating database: " << e.what() << std::endl;
    }
}

void Mysql::createTableRecord(const string &table_name)
{
    try
    {
        sql::Statement *stat = conn->createStatement();
        stat->execute("create table if not exists " + table_name +
                      " (id int primary key auto_increment, "
                      "question text not null, "
                      "answer text not null, "
                      "time DATETIME not null) DEFAULT CHARSET=utf8mb4");
        cout << "user table created successfully" << endl;
        delete stat;
    }
    catch (sql::SQLException &e)
    {
        std::cerr << "Error creating database: " << e.what() << std::endl;
    }
}

bool Mysql::tableExists(const string &table_name)
{
    try
    {
        sql::PreparedStatement *pstmt = conn->prepareStatement("SHOW TABLES LIKE " + table_name);
        sql::ResultSet *res = pstmt->executeQuery();
        bool exists = res->next();
        delete res;
        delete pstmt;
        return exists;
    }
    catch (sql::SQLException &e)
    {
        std::cerr << "Error checking table existence: " << e.what() << std::endl;
        return false;
    }
}

bool Mysql::insertUser(const string &username, const string &password)
{
    if (!tableExists("user_table"))
    {
        createTableAccount("user_table");
    }
    try
    {
        sql::PreparedStatement *pre_stat = conn->prepareStatement("insert into user_table (username, password) values (?, ?)");
        pre_stat->setString(1, username);
        pre_stat->setString(2, password);
        pre_stat->execute();
        std::cout << "Inserted user: " << username << std::endl;
        delete pre_stat;
        return true;
    }
    catch (sql::SQLException &e)
    {
        if (e.getErrorCode() == 1062)
        { // MySQL error code for duplicate entry
            std::cerr << "Error inserting user: Username " << username << " already exists." << std::endl;
        }
        else
            std::cerr << "Error inserting user: " << e.what() << std::endl;
        return false;
    }
}

bool Mysql::isValidTableName(const string &table_name)
{
    if (table_name.empty() || table_name.length() > 64)
    {
        return false;
    }
    for (char c : table_name)
    {
        if (!isalnum(c) && c != '_')
        {
            return false;
        }
    }
    return true;
}

bool Mysql::saveRecord(const string &username, const string &question, const string &answer)
{
    string table_name = username + "_record";

    if (!isValidTableName(table_name))
    {
        cout << "Invalid table name: " << table_name << endl;
        return false;
    }

    if (!tableExists(table_name))
    {
        createTableRecord(table_name);
    }

    time_t t = time(NULL);
    struct tm *tm_info = localtime(&t);
    char time_str[26];
    strftime(time_str, 26, "%Y-%m-%d %H:%M:%S", tm_info);
    string time_now(time_str);
    try
    {
        string st = "insert into " + table_name + " (question, answer, time) values (?, ?, ?)";
        sql::PreparedStatement *pre_stat = conn->prepareStatement(st);
        pre_stat->setString(1, question);
        pre_stat->setString(2, answer);
        pre_stat->setString(3, time_now);
        cout << answer << endl;
        pre_stat->execute();
        cout << "Inserted userRecord: " << time_now << endl;
        delete pre_stat;
        return true;
    }
    catch (sql::SQLException &e)
    {
        cout << "SQL Error: " << e.what()
             << " (MySQL error code: " << e.getErrorCode() << ")" << endl;
        return false;
    }
}

bool Mysql::getChatRecord(const string &username, string &responseData)
{
    try
    {
        std::string tableName = username + "_record";
        sql::Statement *stmt = conn->createStatement();

        // 查询聊天记录（按时间排序）
        std::string query = "SELECT question, answer, time FROM " + tableName + " ORDER BY time DESC";
        std::unique_ptr<sql::ResultSet> res(stmt->executeQuery(query));

        // 构建 JSON 响应
        json jsonResponse;
        jsonResponse["success"] = true;
        jsonResponse["message"] = "Chat records retrieved successfully";

        // 存储聊天记录的数组
        json records = json::array();

        // 遍历结果集
        while (res->next())
        {
            json record;
            record["question"] = res->getString("question");
            record["answer"] = res->getString("answer");
            record["time"] = res->getString("time");
            records.push_back(record);
        }

        jsonResponse["records"] = records;
        responseData = jsonResponse.dump();
        delete stmt;
        return true;
    }
    catch (sql::SQLException &e)
    {
        std::cerr << "SQL Error: " << e.what()
                  << " (MySQL error code: " << e.getErrorCode() << ")" << std::endl;
        return false;
    }
}

// bool Mysql::getChatRecord(const string &username)
// {
//     string table_name = username + "_record";

//     // 执行SQL查询
//     sql::Statement *stat = conn->createStatement();
//     sql::ResultSet *res = stat->executeQuery("SELECT * FROM " + table_name);

//     // 创建输出文件（使用POSIX系统调用）
//     int fd = open("output.txt", O_WRONLY | O_CREAT | O_TRUNC, 0644);
//     if (fd == -1)
//     {
//         cerr << "无法创建输出文件!" << endl;
//         delete res;
//         delete stat;
//         delete conn;
//         return false;
//     }

//     // 获取列数
//     sql::ResultSetMetaData *metaData = res->getMetaData();
//     int columnCount = metaData->getColumnCount();

//     // 写入表头（列名）
//     for (int i = 1; i <= columnCount; i++)
//     {
//         string columnName = metaData->getColumnName(i);
//         write(fd, columnName.c_str(), columnName.length());

//         if (i < columnCount)
//         {
//             const char tab = '\t';
//             write(fd, &tab, 1);
//         }
//     }
//     const char newline = '\n';
//     write(fd, &newline, 1);

//     // 写入数据行
//     while (res->next())
//     {
//         for (int i = 1; i <= columnCount; i++)
//         {
//             // 处理NULL值
//             if (res->isNull(i))
//             {
//                 const char *nullStr = "NULL";
//                 write(fd, nullStr, strlen(nullStr));
//             }
//             else
//             {
//                 string value = res->getString(i);
//                 write(fd, value.c_str(), value.length());
//             }

//             if (i < columnCount)
//             {
//                 write(fd, "\t", 1);
//             }
//         }
//         write(fd, "\n", 1);
//     }

//     // 释放资源
//     close(fd);
//     delete res;
//     delete stat;
//     delete conn;

//     cout << "数据导出完成！文件位置: output.txt" << endl;
//     return true;
// }

string Mysql::queryUserPassword(const string &username)
{
    try
    {
        sql::PreparedStatement *pre_stat = conn->prepareStatement("select password from user_table where username = ?");
        pre_stat->setString(1, username);
        sql::ResultSet *res = pre_stat->executeQuery();
        if (res->next())
        {
            string pw = res->getString("password");
            cout << "password is " << pw << endl;
            delete pre_stat;
            delete res;
            return pw;
        }
        else
        {
            delete pre_stat;
            delete res;
            return "NULL";
        }
    }
    catch (sql::SQLException &e)
    {
        std::cerr << "Error querying user password: " << e.what() << std::endl;
        return "NULL";
    }
}