#include <iostream>
#include <string>
#include "comm.hpp"
#include "mysql.h"

bool InsertSql(std::string sql)
{
    MYSQL *conn = mysql_init(nullptr);
    mysql_set_character_set(conn, "utf8");
    if(nullptr == mysql_real_connect(conn, "127.0.0.1", "root", "1234567890", "user", 3306, nullptr, 0)){
        std::cerr << "connect error!" << std::endl;
        return 1;
    }
    std::cerr << "connect success" << std::endl;

    std::cerr << "query : " << sql <<std::endl;
    int ret = mysql_query(conn, sql.c_str());
    std::cerr << "result: " << ret << std::endl;
    mysql_close(conn);
    return true;
}

int main()
{
    std::string query_string;
    //from http
    if(GetQueryString(query_string)){
        std::cerr << query_string << std::endl;
        //数据处理？
        //name=tom&passwd=111111

        std::string name;
        std::string passwd;

        CutString(query_string, "&", name, passwd);

        std::string _name;
        std::string sql_name;
        CutString(name, "=", _name, sql_name);

        std::string _passwd;
        std::string sql_passwd;
        CutString(passwd, "=", _passwd, sql_passwd);

        std::string sql = "insert into user(name, passwd) values (\'";
        sql += sql_name;
        sql += "\',\'";
        sql += sql_passwd;
        sql += "\')";

        //插入数据库
        if(InsertSql(sql)){
            std::cout << "<html>";
            std::cout << "<head><meta charset=\"utf-8\"></head>";
            std::cout << "<body><h1>注册成功!</h1></body>";
        }
    }
    return 0;
}





