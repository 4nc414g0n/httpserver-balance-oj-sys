#pragma once
#include <iostream>
using std::cout;
using std::endl;
using std::cerr;

#include <unordered_map>
#include <fstream>
#include <vector>
#include <cassert>
#include <cstring>
#include <cstdlib>

#include "../../comm/Log.hpp"
#include "../../comm/Util.hpp"

namespace ns_model{

    using namespace ns_util;
    using namespace ns_log;

    struct Question
    {
    //5个变量描述题目
        std::string num;//题号
        std::string title;//题目标题
        std::string difficulty;//题目难度
        std::string desc;//题目描述
        int time;//事件复杂度要求(s)
        int space;//空间复杂度要求(kb)
    //拼接下面两部分
        std::string header;//题目自带代码部分
        std::string test;//题目测试用例部分
    };

    const std::string configfileName = "../questions/questions.list";
    const std::string questionPath = "../questions/";
    class Model{
        private:
            std::unordered_map<std::string, Question> _num2Qu;//题号到题目的映射
        public:
            Model(){
                assert(LoadQuestionGE(configfileName));
            }
            ~Model(){}
            bool LoadQuestionGE(const std::string &configfileName)//从question.list获取题目配置文件 (GE: confiGfilE)
            {
                std::ifstream in(configfileName);
                if(!in.is_open()){
                    LOG(LOG_LEVEL_FATAL)<<"Open File Failed... Check ur configfile"<<endl;
                    return false;
                }
                std::string line;//配置文件中题目都是按行的
                while(getline(in, line))
                {
                    std::vector<std::string> tokens;
                    CommonUtil::StrCut(line, &tokens, " ");
                    if(tokens.size()!=5) continue;//当前题目的配置文件有问题

                    Question q;

                    q.num = tokens[0];//获取题号
                    q.title = tokens[1];//获取标题
                    q.difficulty = tokens[2];//获取难度
                    q.time = atoi(tokens[3].c_str());//获取时间限制
                    q.space = atoi(tokens[4].c_str());//获取空间限制

                    std::string path = questionPath;
                    path+=q.num;
                    path+="/";

                    CommonUtil::ReadFromFile(&(q.desc), path+"desc.txt", true);//获取题目描述
                    CommonUtil::ReadFromFile(&(q.header), path+"header.cc", true);//获取自带代码部分
                    CommonUtil::ReadFromFile(&(q.test), path+"test.cc", true);//获取测试用例
                    
                    _num2Qu[q.num] = q;
                }
                LOG(LOG_LEVEL_INFO)<<"Load Question Bank Success, "+ std::to_string(_num2Qu.size())+ " Questions in Total"<<endl;
                in.close();
                return true;
            }
            bool GetOneQuestion(const std::string &num, Question *qu)
            {
                const auto& iter = _num2Qu.find(num);
                if(iter==_num2Qu.end()){
                    LOG(LOG_LEVEL_INFO)<<"No."+num+" Question Not Found, GetOneQuestion Failed..."<<endl;
                    return false;
                }    
                *qu = iter->second;
                return true;
            }
            bool GetAllQuestion(std::vector<Question> *quVec)//输入输出参数
            {
                if(0 == _num2Qu.size()){
                    LOG(LOG_LEVEL_INFO)<<"Question Bank is Empty, GetAllQuestion Failed..."<<endl;
                    return false;
                } 
                for(const auto& iter : _num2Qu)
                    quVec->push_back(iter.second);
                return true;
            }
    };
}