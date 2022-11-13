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
#include <pthread.h>
#include <algorithm>
#include <json/json.h>
#include <sys/stat.h>

#include "../httpserver_cpphttplib/Pthread.hpp"
#include "../httpserver_cpphttplib/httplib.h"

#include "../../comm/Log.hpp"
#include "../../comm/Util.hpp"
#include "Model.hpp"
#include "View.hpp"//渲染网页

//Cotroller(普通)：从Model获取题目信息 -> 交给View渲染html -> 返回给CGI程序
//Cotroller(用户提交代码)：从Model获取题目信息+拼接用户代码+测试用例 -> 负载均衡选择编译主机 -> 运行结果形成outJson -> 交给View渲染html -> 返回给CGI程序

namespace ns_controller{

    using namespace std;
    using namespace ns_log;
    using namespace ns_util;
    using namespace ns_model;
    using namespace ns_view;
    using namespace httplib;

    const std::string servicePath = "../config/service_machine.conf";

    class Machine{//描述主机
        public:
            std::string ip;//选择的服务器ip
            int port;//ip的端口
            uint64_t load;//负载
            Mutex mtx;//封装的pthread_mutex互斥锁（保护负载）
        public:
            Machine():ip(""),port(0),load(0){}
            ~Machine(){}  
            void IncreaseLoad()// 提升主机负载
            {
                mtx.lock();
                ++load;
                mtx.unlock();
            }        
            void DecreaseLoad()// 减少主机负载
            {
                mtx.lock();
                --load;
                mtx.unlock();
            }
            void ResetLoad()//重置主机负载
            {
                mtx.lock();
                load = 0;
                mtx.unlock();
            }
            uint64_t Load()// 获取主机负载
            {
                uint64_t _load = 0;
                mtx.lock();
                _load = load;
                mtx.unlock();
                return _load;
            }
    };
    class LoadBalance{//进行负载均衡选择
        private:
            std::vector<Machine> _machines;
            std::vector<int> _onlineMachines;//在线主机(int id用_machines中对应下标表示)
            std::vector<int> _offlineMachines;//离线主机
            Mutex mtx;
        public:
            LoadBalance(){ 
                assert(LoadConfig(servicePath));
            }
            ~LoadBalance(){}
            bool LoadConfig(const std::string &servicePath)//从configPath加载配置文件
            {
                std::ifstream in(servicePath);
                if(!in.is_open())
                {
                    LOG(LOG_LEVEL_FATAL)<<"Load serviceConfig Error..."<<endl;
                    return false;
                }
                std::string line;
                while(getline(in, line))
                {
                    std::vector<string> tokens;
                    CommonUtil::StrCut(line, &tokens, ":");//127.0.0.1:8081
                    if(tokens.size()!=2){
                        LOG(LOG_LEVEL_ERROR)<<"LoadConfig Error: StrCut Failed... line: "<<line<<endl;
                        continue;
                    }
                    Machine machine;
                    machine.ip=tokens[0];
                    machine.port=atoi(tokens[1].c_str());
                    machine.load=0;

                    _onlineMachines.push_back(_machines.size());//第一次size为0，第一台主机的下标也为0
                    _machines.push_back(machine);
                }
                ShowMachines();
                return true;
            }
            //Round-Robin(轮询 + hash)
            bool LoadBalanceSelect(int *id, Machine **machine)//两个参数都是输出型参数，注意这里的二级指针(需要的是Machine的地址)
            {
                mtx.lock();
                int onlineNum = _onlineMachines.size();
                LOG(LOG_LEVEL_INFO)<<"Online Machines: "<<onlineNum<<" in Total"<<endl;
                if(onlineNum == 0)
                {
                    mtx.unlock();
                    LOG(LOG_LEVEL_FATAL)<<"All Machines Offine, Select Failed..."<<endl;
                    return false;
                }
                *id = _onlineMachines[0];
                *machine = &(_machines[_onlineMachines[0]]);
                uint64_t minLoad = (_machines[_onlineMachines[0]]).load;
                for(int i=0;i<onlineNum;i++){
                    if(minLoad > _machines[_onlineMachines[i]].load){
                        minLoad = _machines[_onlineMachines[i]].load;
                        *id = _onlineMachines[i];
                        *machine = &(_machines[_onlineMachines[i]]);
                    }
                }
                mtx.unlock();
                return true;
            }
            void OfflineMachine(int id)//离线主机
            {
                mtx.lock();
                for(auto iter = _onlineMachines.begin(); iter != _onlineMachines.end(); iter++)
                {
                    if(*iter == id)
                    {
                        _machines[id].ResetLoad();
                        //进行离线
                        _onlineMachines.erase(iter);//这里已经迭代器失效，但本轮就break退出了，不用考虑失效问题
                        _offlineMachines.push_back(id);
                        break;
                    }
                }
                mtx.unlock();
            }
            void OnlineMachine()//上线主机
            {
                mtx.lock();
                _onlineMachines.insert(_onlineMachines.end(), _offlineMachines.begin(), _offlineMachines.end());
                _offlineMachines.erase(_offlineMachines.begin(), _offlineMachines.end());
                //if(CommonUtil::FileExists(balancePath)) unlink(balancePath.c_str());//去除balance_machine.conf
                mtx.unlock();
                LOG(LOG_LEVEL_INFO)<<"All Machines reOnlined..."<<endl;
            }
            void ShowMachines()//日志打印在线的主机
            {
                mtx.lock();
                LOG(LOG_LEVEL_INFO)<<"Online Machines: ";
                for(auto iter:_onlineMachines) cerr<<iter<<" ";
                cerr<<endl;
                LOG(LOG_LEVEL_INFO)<<"Offline Machines: ";
                for(auto iter:_offlineMachines) cerr<<iter<<" ";
                cerr<<endl;
                mtx.unlock();
            }
            // bool Write2BalanceConfig()//更新BalanceConfig,一个没有的方法(之前想用cgi实现搞得)
            // {
            //     std::ofstream out(balancePath, ios::trunc);//没有就创建
            //     if(!out.is_open())
            //     {
            //         LOG(LOG_LEVEL_FATAL)<<"Load balanceConfig Error..."<<endl;
            //         return false;
            //     }
            //     for(auto &machine:_machines){
            //         out<<machine.ip<<":"<<std::to_string(machine.port)<<":"<<std::to_string(machine.load)<<":"<<std::to_string(machine.status)<<endl;
            //     }
            //     return true;
            // }
    };

    class Controller{
        private:
            Model _model;//获取后台数据
            View _view;//获取渲染网页
            LoadBalance _loadbalance;//负载均衡器
        public:
            Controller(){}
            ~Controller(){}
            void RecoverMachine()//回调
            {
                _loadbalance.OnlineMachine();
            }
            bool AllQuestionHtml(std::string *html)//这里的path从cgi程序传入方便定位html
            {
                vector<Question> allQuestion;
                if(_model.GetAllQuestion(&allQuestion))
                {
                    //先按题号排序
                    sort(allQuestion.begin(), allQuestion.end(), [](const Question &q1, const Question &q2){
                        return atoi(q1.num.c_str()) < atoi(q2.num.c_str());
                    });
                    //通过View模块ctemplate实时渲染网页
                    _view.RenderAllQuestionHtml(allQuestion, html);
                    return true;
                }
                else{
                    *html = "Error";//拓展一个静态页面
                    return false;
                }
            }
            bool OneQuestionHtml(std::string &num, std::string *html)
            {
                Question q;
                if(_model.GetOneQuestion(num, &q))
                {
                    _view.RenderOneQuestionHtml(q, html);
                    return true;
                }
                else{
                    *html = "Error";////拓展一个静态页面
                    return false;
                }
            }
            void Judge(std::string &num, const std::string inJson, std::string *outJson)//判题获得outJson
            {
            //Judge从cgi-bin/judge获得的inJson包含：(num, code, input)          
            //inJson进行反序列化，得到题目的id，得到用户提交源代码，input
                Json::Reader reader;//Json
                Json::Value inValue;//解析出的
                reader.parse(inJson, inValue);//解析
                //std::string num = inValue["num"].asString();//初版使用cgi_httpserver的时候获得题号只能这样写
                std::string code = inValue["code"].asString();
            //根据题目编号num，直接拿到对应的题目细节
                Question q;
                _model.GetOneQuestion(num, &q);
            //重新拼接用户代码+测试用例代码，形成新的代码
                Json::Value compileValue;
                compileValue["input"] = inValue["input"].asString();//inValue["input"]为 用户在代码块写的
                compileValue["code"] = code + "\n" + q.test;
                compileValue["cpuLimit"] = q.time;
                compileValue["memLimit"] = q.space;
                Json::FastWriter writer;
                std::string compileJson = writer.write(compileValue);//序列化新城json(compileJson)
            //选择负载最低的主机(差错处理)(规则: 一直选择，直到主机可用，否则，就是全部挂掉)
                while(1)
                {
                    int id = 0;
                    Machine *m = nullptr;
                    if(!_loadbalance.LoadBalanceSelect(&id, &m))
                    {
                        LOG(LOG_LEVEL_FATAL)<<"LoadBalanceSelect Failed..."<<endl;
                        break;
                    }
                    // 4. 使用httplib发起http请求，得到结果
                    Client cli(m->ip, m->port);
                    m->IncreaseLoad();//增加机器负载
                    LOG(LOG_LEVEL_INFO)<<"Select Machine: "<<id<<" IP:"<<m->ip<<":"<<m->port<<" Load: "<<m->load<<endl;
                    if(auto res = cli.Post("/compile_run.json", compileJson, "application/json;charset=utf-8"))
                    {
                        // 5. 将结果赋值给outJson
                        if(res->status == 200)
                        {
                            *outJson = res->body;
                            m->DecreaseLoad();//减少机器负载
                            LOG(LOG_LEVEL_INFO)<<"Balance Server Compile Success..."<<endl;
                            break;
                        }
                        m->DecreaseLoad();//减少机器负载
                    }
                    else
                    {
                        //请求失败
                        LOG(LOG_LEVEL_ERROR)<<"HttpClient Error...might Offline..."<<endl;
                        _loadbalance.OfflineMachine(id);
                        _loadbalance.ShowMachines(); //仅仅是为了用来调试
                    }
                }
                

            }   
    };
}