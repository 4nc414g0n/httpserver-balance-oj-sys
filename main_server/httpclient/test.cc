#include <iostream>
#include "HttpClient.hpp"
#include <json/json.h>
int main()
{
    using namespace ns_httpclient;
    HttpClient httpclient("127.0.0.1", 8081);
    
    std::string in_json;
    Json::Value in_value;
    //R"()", raw string
    in_value["code"] = R"(
    #include<iostream>
    int main()
    {
        std::cout << "test" << std::endl;
        return 0;
    }
    )";
    in_value["input"] = "";
    in_value["cpuLimit"] = 1;
    in_value["memLimit"] = 10240*30;
    Json::FastWriter writer;
    in_json = writer.write(in_value);

    std::string out_json;
    std::string path = "/compile_run.json";
    std::string type = "application/json;charset=utf-8";
    if(httpclient.GetoutJson(path, in_json, type, &out_json))
        cout<<out_json<<endl;

}