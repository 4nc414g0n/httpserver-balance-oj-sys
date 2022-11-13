
#include <iostream>
using std::cout;
using std::endl;

#include <json/json.h>

#include "Compiler.hpp"
#include "Runner.hpp"
#include "Compile_Run.hpp"

int main()
{
    using namespace ns_compile_run;
    //in_json: {"code": "#include...", "input": "","cpu_limit": 1, "mem_limit": 10240}
    //out_json: {"status":"0", "reason":"","stdout":"","stderr":"",}
    //通过http让client上传一个json string
    //下面的工作，充当客户端请求的json串
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
    //std::cout << in_json << std::endl;
    //这个是将来给客户端返回的json串
    std::string out_json;
    CompileRun::Start(in_json, &out_json);

    std::cout << out_json << std::endl;
}