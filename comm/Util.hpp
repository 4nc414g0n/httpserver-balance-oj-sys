#pragma once 
#include <iostream>
#include <fstream>//ofstream, ifstream
#include <vector>
#include <cstring>
#include <atomic>

#include <sys/types.h>
#include <unistd.h>
#include <sys/resource.h>
#include <sys/signal.h>
#include <sys/time.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <boost/algorithm/string.hpp>//boost字符操作


#include "Log.hpp"

namespace ns_util{
//common
    const std::string Path = "../tmp/";//统一路径 (only for balance_server)
//common_util
    class CommonUtil{
        public: 
            static void SuffixAdd(std::string& fileName, const std::string& suffix)// "./tmp/fileName" -> "./tmp/fileName.cc"
            {
                fileName+=suffix;
            }
            static void StrCut(std::string &line, std::vector<std::string> *target, const std::string sep)
            {
                //例如：将line按" "切分，并将切分好的str push到target中(输入输出)
                boost::split((*target), line, boost::is_any_of(sep), boost::token_compress_on);//token_compress_on处理出现连续分隔符的情况
            } 
            static bool FileExists(const std::string &file)
            {
                struct stat st;
                if(stat(file.c_str(), &st)==0)
                    return true;
                return false;
            }
            static bool Write2File(const std::string &in, const std::string &Src)
            {
                //文件不存在，则一般新建一个文件
                std::ofstream ofile(Src);
                if(ofile.is_open()){
                    ofile.write(in.c_str(), in.size());
                    ofile.close();
                    return true;
                }
                return false;
                // int fd = open(Src.c_str(), O_CREAT|O_WRONLY, 0644);
                // if(fd<0){
                //     return false;
                // }
                // else{
                //     write(fd, (const char*)(code.c_str()), code.size());
                // }
            }
            static bool ReadFromFile(std::string *out, const std::string &file, bool lineBreak = false)
            {
                (*out).clear();
                std::ifstream ifile(file);
                if(ifile.is_open()){
                    std::string line;
                    //特殊情况：需要'\n'
                    while(std::getline(ifile, line)){//getline默认不读取'\n'
                        (*out) += line;
                        (*out)+=(lineBreak?"\n":"");
                    }
                    ifile.close();
                    return true;
                }
                return false;

            }
    };
//compile_util
    class CompileUtil{
    public:
        static std::string File2Src(const std::string& fileName)// "fileName" -> "./tmp/fileName.cc"
        {
            std::string src = Path + fileName;
            CommonUtil::SuffixAdd(src, ".cc");
            return src;
        }
        static std::string File2Exe(const std::string & fileName)// "fileName" -> "./tmp/fileName.exe"
        {
            std::string exe = Path + fileName;
            CommonUtil::SuffixAdd(exe, ".exe");
            return exe;
        }
        static std::string File2Error(const std::string & fileName)// "fileName" -> "./tmp/fileName.error"
        {
            std::string error = Path + fileName;
            CommonUtil::SuffixAdd(error, ".error");
            return error;
        }     
    };
//run_util
    class RunUtil{
        public:
        static std::string File2Stdin(const std::string & fileName)// "fileName" -> "./tmp/fileName.stdin"
        {
            std::string error = Path + fileName;
            CommonUtil::SuffixAdd(error, ".stdin");
            return error;
        }
        static std::string File2Stdout(const std::string & fileName)// "fileName" -> "./tmp/fileName.stdout"
        {
            std::string error = Path + fileName;
            CommonUtil::SuffixAdd(error, ".stdout");
            return error;
        }
        static std::string File2Stderr(const std::string & fileName)// "fileName" -> "./tmp/fileName.stderr"
        {
            std::string error = Path + fileName;
            CommonUtil::SuffixAdd(error, ".stderr");
            return error;
        }
        static void SetProcLimit(int cpuLimit, int memLimit)//memLimit(KB)
        {
            struct rlimit cpuR;
            cpuR.rlim_cur=cpuLimit;
            cpuR.rlim_max=RLIM_INFINITY;

            struct rlimit memR;//Virtual Memory
            memR.rlim_cur=memLimit*1024;
            memR.rlim_max=RLIM_INFINITY;

            setrlimit(RLIMIT_CPU, &cpuR);
            setrlimit(RLIMIT_AS, &memR);
        }
    };
//compile_run_util
    class CompileRunUtil{
        public:
            static std::string UniqueFileName()//毫秒时间戳+原子递增创建唯一文件名
            {
                static std::atomic_uint id(0);//静态递增
                struct timeval time;
                gettimeofday(&time, nullptr);
                std::string fileName = std::to_string(time.tv_sec*1000 + time.tv_usec/1000);//获得毫秒级时间戳 
                fileName += std::to_string(id);
                id++;
                return fileName;      
            }
            static void RemoveTmpFile(const std::string &fileName)
            {
                std::string Src = CompileUtil::File2Src(fileName);
                std::string Exe = CompileUtil::File2Exe(fileName);
                std::string Error = CompileUtil::File2Error(fileName);
                std::string Stdin = RunUtil::File2Stdin(fileName);
                std::string Stdout = RunUtil::File2Stdout(fileName);
                std::string Stderr = RunUtil::File2Stderr(fileName);
                if(CommonUtil::FileExists(Src)) unlink(Src.c_str());
                if(CommonUtil::FileExists(Exe)) unlink(Exe.c_str());
                if(CommonUtil::FileExists(Error)) unlink(Error.c_str());
                if(CommonUtil::FileExists(Stdin)) unlink(Stdin.c_str());
                if(CommonUtil::FileExists(Stdout)) unlink(Stdout.c_str());
                if(CommonUtil::FileExists(Stderr)) unlink(Stderr.c_str());
            }
            static std::string Status2Desc(int status, const std::string &fileName)//compile_run的status->描述(负数：出错，正数：信号终止，0：正常)
            {
                std::string message;
                switch(status)
                {
                    case 0:
                        message = "Compile And Run Success...";
                        break;
                    case -1:
                        message = "User Code is Empty...";
                        break;
                    case -2:
                        message = "[Server Error]: Write Code To File Failed...";
                        break;
                    case -3:
                        CommonUtil::ReadFromFile(&message, CompileUtil::File2Error(fileName), true);
                        break;
                    case -4:
                        message = "[Server Error]: Fork Child or Open File Failed...";
                        break;
                    case SIGABRT:
                        message = "6) SIGABRT: Exceeded Virtual Memory Allocation Limit...";
                        break;
                    case SIGXCPU:
                        message = "24) SIGXCPU: Exceeded CPU time Limit...";
                        break;
                    case SIGFPE:
                        message = "8) SIGFPE: Divid 0 Error...";
                        break;
                    case SIGSEGV:
                        message = "11) SIGSEGV: Segment Fault...";
                        break;
                    case SIGKILL:
                        message = "9) SIGKILL: Program Could Have Been Killed By Anyone...";
                        break;
                    default:
                        message = "Unknown Error, return code is: "+std::to_string(status);
                        break;
                }
                return message;
            }
    };
}