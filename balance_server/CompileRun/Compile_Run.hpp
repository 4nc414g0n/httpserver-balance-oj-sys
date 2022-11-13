#pragma once

#include <cstring>
#include <iostream>
#include <json/json.h>
using std::cout;
using std::endl;

#include "../../comm/Log.hpp"
#include "../../comm/Util.hpp"
#include "Compiler.hpp"
#include "Runner.hpp"

namespace ns_compile_run{
    using namespace ns_log;
    using namespace ns_util;
    using namespace ns_complier;
    using namespace ns_runner;
    class CompileRun{
        public:
            CompileRun(){}
            ~CompileRun(){}
            static void Start(const std::string& inJson, std::string *outJson)//outJson输出型
            {
                //inJson:  {"code": "#include <iost...", "input": "123", "cpuLimit": "1", "memLimit": "1024"}
                //outJson: {"status": "", "message": "", "stdout": "", "stderr", ""}
            //解析字符串inJson为对应的Key / Value
                Json::Value InValue;//用于injson的反序列化
                Json::Value outValue;//用于outJson序列化
                Json::Reader reader;//用于读取，将字符串或者文件输入流(inJson)转换为Json::Value对象的
                reader.parse(inJson, InValue);//反序列化
                
                std::string code = InValue["code"].asString();//提取代码部分
                std::string input = InValue["input"].asString();//提取代码部分
                int cpuLimit = InValue["cpuLimit"].asInt();//时间复杂度
                int memLimit = InValue["memLimit"].asInt();//空间复杂度

            //goto语句到flag位置间不能定义变量
                int status = 0;//错误标志，统一处理outValue的序列化
                int runCode = 0;//Runner运行返回值
                std::string fileName="";//用于形成唯一文件名

                if(code.size()==0)//用户代码为空
                {
                    status = -1;
                    goto END;
                }
                fileName = CompileRunUtil::UniqueFileName();//形成唯一文件名
                if(!CommonUtil::Write2File(code, CompileUtil::File2Src(fileName))){//写入文件失败
                    status = -2;     
                    goto END;
                }
                if(!Compiler::Compile(fileName)){
                    //编译失败
                    status = -3;
                    goto END;
                }
                runCode = Runner::Run(fileName, cpuLimit, memLimit);
                if(runCode < 0){//内部错误(open出错...)
                    status = -4;
                    goto END;
                }
                else if(runCode > 0){//信号中断
                    status = runCode;
                    goto END;
                }
                else
                    status = 0;
            END:
            //统一处理outValue的序列化
                //outJson: {"status": "", "message": "", "stdout": "", "stderr", ""}
                outValue["status"] = status;
                outValue["message"] = CompileRunUtil::Status2Desc(status, fileName);
                if(status == 0){//成功
                    std::string out;
                    CommonUtil::ReadFromFile(&out, RunUtil::File2Stdout(fileName), true);//true保留换行符
                    outValue["stdout"] = out;
                    std::string err;
                    CommonUtil::ReadFromFile(&err, RunUtil::File2Stderr(fileName), true);
                    outValue["stderr"] = err;
                }
                Json::StyledWriter writer;
                *outJson = writer.write(outValue);

                //移除tmp下的所有文件
                CompileRunUtil::RemoveTmpFile(fileName);
            }
    };
}
