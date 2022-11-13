#pragma once 
#include <iostream>
#include <cstring>

#include <sys/types.h>
#include <unistd.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/resource.h>

#include "../../comm/Util.hpp"
#include "../../comm/Log.hpp"



namespace ns_runner{

    using namespace ns_log;
    using namespace ns_util;

    class Runner{
        public:
            Runner(){}
            ~Runner(){}

            //1. 程序正常运行完毕
            //2. 程序崩溃
            //标准输入: ./tmp/fileName.stdin
            //标准输出: ./tmp/fileName.stdout
            //标准错误: ./tmp/fileName.stderr

            //OJ: 设置CPU运行时间限制cpuLimit，内存使用限制memLimit (上层业务逻辑传入的针对不同题目的 时间&空间限制)
            //int setrlimit(int resource, const struct rlimit *rlp);
            //返回值>0: 对应错误码()
            //返回值=0: 正常退出
            //返回值<0: 内部错误(open出错...)
            static int Run(std::string& fileName, int cpuLimit, int memLimit)//返回signal
            {
                std::string exeFile = CompileUtil::File2Exe(fileName);
                std::string stdinFile = RunUtil::File2Stdin(fileName);
                std::string stdoutFile = RunUtil::File2Stdout(fileName);
                std::string stderrFile = RunUtil::File2Stderr(fileName);

                //打开文件
                umask(0);
                //int exeFd = open(exeFile.c_str(), O_CREAT|O_RDONLY, 0644);
                int stdinFd = open(stdinFile.c_str(), O_CREAT|O_RDONLY, 0644);//0644权限(用户读写,组用户和其它用户只读)
                int stdoutFd = open(stdoutFile.c_str(), O_CREAT|O_WRONLY, 0644);
                int stderrFd = open(stderrFile.c_str(), O_CREAT|O_WRONLY, 0644);
                if(stderrFd<0 || stdinFd<0 || stdoutFd<0){
                    if(stdinFd<0)
                        LOG(LOG_LEVEL_ERROR)<<"Open File ["<<RunUtil::File2Stdin(fileName)<<"] Failed..."<<endl;
                    if(stdoutFd<0) 
                        LOG(LOG_LEVEL_ERROR)<<"Open File ["<<RunUtil::File2Stdout(fileName)<<"] Failed..."<<endl;
                    if(stderrFd<0)
                        LOG(LOG_LEVEL_ERROR)<<"Open File ["<<RunUtil::File2Stderr(fileName)<<"] Failed..."<<endl;
                    return -1;
                }

                //创建子进程进行exec*
                pid_t pid = fork();
                if(pid==0){//child
                    dup2(stdinFd, 0);//stdinFd->stdin
                    dup2(stdoutFd, 1);//stdoutFd->stdout
                    dup2(stderrFd, 2);//stderrFd->stderr
                    if(errno == EBADF || errno== EINTR || errno ==EIO){
                        LOG(LOG_LEVEL_WARNING)<<"dup2 failed"<<endl;
                        return -2;
                    }
                    
                    RunUtil::SetProcLimit(cpuLimit, memLimit);//设置execl程序资源限制
                                                             //(特别注意：memLimit设太小会导致execl段错误)
                    execl(exeFile.c_str(), exeFile.c_str(), nullptr);
                    LOG(LOG_LEVEL_ERROR)<<"Run ["<<exeFile<<"] Failed...(Check on execl() args)"<<endl;
                    exit(1); 
                }
                else if(pid < 0){//error
                    close(stdinFd);
                    close(stdoutFd);
                    close(stderrFd);
                    LOG(LOG_LEVEL_ERROR)<<"Fork Child Failed..."<<endl;
                    return -3;
                }
                else{//father
                    close(stdinFd);
                    close(stdoutFd);
                    close(stderrFd);
                    int status=0;
                    pid_t ret = waitpid(pid, &status, 0);
                    if(ret == pid){
                        if(WIFEXITED(status)){//WIFEXITED(status) 若此值为非0 表明进程正常结束
                            if(WEXITSTATUS(status) == 0){
                                LOG(LOG_LEVEL_INFO) << "Run Success..." <<endl;
                            }
                            else{
                                LOG(LOG_LEVEL_ERROR) << "Child Error Exit("<<WEXITSTATUS(status)<<")" <<endl;
                            }
                        }
                        else{
                            LOG(LOG_LEVEL_ERROR) << "Child Error, Killed By Signal: " <<(status & 0x7F)<<endl;
                        }
                    }
                    //程序运行异常，status & 0x7F获取信号
                    return (status & 0x7F);//进程控制(第7位对应了退出信号)
                }
            }
    };
}