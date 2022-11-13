#pragma once 
#include <iostream>
#include <cstring>
using std::cout;
using std::end;

#include <sys/types.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <fcntl.h>

#include "../../comm/Util.hpp"
#include "../../comm/Log.hpp"



namespace ns_complier{

    using namespace ns_util;
    using namespace ns_log;

    class Compiler{
        public:
            Compiler(){}
            ~Compiler(){}

            //源文件source: ./tmp/fileName.cc
            //编译出错error : ./tmp/fileName.stderr
            //可执行文件exe   : ./tmp/fileName.exe
            static bool Compile(std::string &fileName)//compile成功与否
            {
                //创建子进程
                pid_t pid = fork();
                if(pid == 0){//child
                    umask(0);//严谨清零unamsk
                    int errorFd = open(CompileUtil::File2Error(fileName).c_str(), O_CREAT|O_WRONLY, 0644);//0644权限(用户读写,组用户和其它用户只读)
                    if(errorFd<0){
                        LOG(LOG_LEVEL_WARNING)<<"Open File ["<<CompileUtil::File2Error(fileName)<<"] Failed..."<<endl;
                        exit(1);
                    }
                    dup2(errorFd, 2);//stderr也就是2 重定向到open的fd(errorFd)
                    //execlp程序替换调用g++
                    execlp("g++", "g++", "-o", CompileUtil::File2Exe(fileName).c_str(), //HEADER宏，定义了就不包含header.hpp
                            CompileUtil::File2Src(fileName).c_str(),"-DHEADER", "-std=c++11",nullptr);//参数nullptr结束
                    LOG(LOG_LEVEL_ERROR)<<"g++ Compile Failed...(Check on execlp()args)"<<endl;
                    exit(2);
                }
                else if(pid < 0){//error
                    LOG(LOG_LEVEL_ERROR)<<"Create Child Failed..."<<endl;
                    return false;
                }
                else{//father
                    waitpid(pid, nullptr, 0);
                    //.exe文件存在
                    if(CommonUtil::FileExists(CompileUtil::File2Exe(fileName))){
                        LOG(LOG_LEVEL_INFO)<<".exe File ["<<CompileUtil::File2Exe(fileName)<<"] Exists Compile Success..."<<endl;
                        return true;
                    }
                    LOG(LOG_LEVEL_INFO)<<".exe File ["<<CompileUtil::File2Exe(fileName)<<"] Not Exists Compile Failed..."<<endl;
                    return false;
                }
            }
    };
}