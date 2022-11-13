#include <iostream>
#include "comm.hpp"

int main()
{
    std::string query_string;
    GetQueryString(query_string);
    //std::cerr<<"======================"<<query_string<<std::endl;
    //a=100&b=200
    
    std::string str1;
    std::string str2;
    CutString(query_string, "&", str1, str2);

    std::string name1;
    std::string value1;
    CutString(str1, "=", name1, value1);

    std::string name2;
    std::string value2;
    CutString(str2, "=", name2, value2);

    //1 -> 
    std::cout << name1 << " : " << value1 << std::endl;
    std::cout << name2 << " : " << value2 << std::endl;

    //2
    std::cerr << name1 << " : " << value1 << std::endl;
    std::cerr << name2 << " : " << value2 << std::endl;
    int x = atoi(value1.c_str());
    int y = atoi(value2.c_str());

    //可能向进行某种计算(计算，搜索，登陆等)，想进行某种存储(注册)
    std::cout << "<html>";
    std::cout << "<head><meta charset=\"utf-8\"></head>";
    std::cout << "<body>";
    std::cout << "<h3> " << value1 << " + " << value2 << " = "<< x+y << "</h3>";
    std::cout << "<h3> " << value1 << " - " << value2 << " = "<< x-y << "</h3>";
    std::cout << "<h3> " << value1 << " * " << value2 << " = "<< x*y << "</h3>";
    std::cout << "<h3> " << value1 << " / " << value2 << " = "<< x/y << "</h3>";
    std::cout << "</body>";
    std::cout << "</html>";
    return 0;
}










