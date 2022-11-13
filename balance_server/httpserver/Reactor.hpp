#pragma once 
#include <iostream>
#include <cstring>
#include <unistd.h>
#include <unordered_map>
#include <sys/epoll.h>
#include <unistd.h>
#include <fcntl.h>
#include <assert.h>

#include "Log.hpp"

//#include "sock.hpp"

using namespace std;

namespace ns_reactor{

    class Event;
    class Reactor;

    typedef void(*callback_t)(Event &); //函数指针

    class Event{
        public:
            int sock_;//一个文件描述符对应一个事件对象
            Reactor *r_;//指向所属的Reactor对象
        
            string in_buffer_;//私有接收缓冲区
            string out_buffer_;//私有发送缓冲区

            callback_t recv_callback_;//读回调
            callback_t send_callback_;//写回调
            callback_t error_callback_;//异常回调
        public:
            Event()//初始化
                :sock_(-1),
                r_(nullptr)
            {
                recv_callback_=nullptr;
                send_callback_=nullptr;
                error_callback_=nullptr;
                in_buffer_="";
                out_buffer_="";
            }

            void RegisterCallback(callback_t recv_ ,callback_t send_, callback_t error_)//注册回调
            {
                recv_callback_=recv_;
                send_callback_=send_;
                error_callback_=error_;
            }
            ~Event()//注意这里不能直接close()，应该将close逻辑写在DelEvent内部
            {}
            //解释：因为在AddEvent函数中进行了unordered_map的insert操作,insert操作中间会产生临时变量，
            //之后会销毁，在销毁的时候调用析构函数，如果析构函数中有close(sock_),直接释放sock_,之后就会出错
    };
    class Reactor{
        public:
            int epfd_;//epoll对象的fd
            unordered_map<int, Event> events_;
        public:
            Reactor()
                :epfd_(-1)
            {}

            void ReactorInit()
            {
                epfd_ = epoll_create(128);
                if(epfd_ < 0)
                {
                    LOG(LOG_LEVEL_FATAL, "Reactor_Epoll Create Error...");
                    exit(1);
                }
            }

            //添加事件
            void AddEvent(const Event &ev, uint32_t events)
            {
                struct epoll_event epev;
                epev.data.fd = ev.sock_;
                epev.events = events;

                if(epoll_ctl(epfd_, EPOLL_CTL_ADD, ev.sock_, &epev)<0)
                {
                    LOG(LOG_LEVEL_ERROR, "epoll_ctl Error, Add Event Failed...");
                    return;
                }
                else
                    events_.insert({ev.sock_, ev});     
                LOG(LOG_LEVEL_INFO, "Event Added...");
            }

            //删除事件
            void DelEvent(int sock)
            {
                unordered_map<int, Event>::iterator iter = events_.find(sock);
                if(iter == events_.end())
                    return;//没找到
                else{
                    epoll_ctl(epfd_, EPOLL_CTL_DEL, sock, nullptr);
                    events_.erase(iter);
                    close(sock);//sock需要在这里关闭
                    LOG(LOG_LEVEL_INFO, "Sock: "+std::to_string(sock)+", Event Deleted");
                }
            }

            void EnableReadWrite(int sock, bool readable, bool writeable)
            {
                struct epoll_event ev;
                ev.events = (EPOLLET|(readable? EPOLLIN:0)|(writeable?EPOLLOUT:0));
                ev.data.fd = sock;
                if(0 == epoll_ctl(epfd_, EPOLL_CTL_MOD, sock, &ev))//成功返回0 (修改目标sock的event)
                {
                    LOG(LOG_LEVEL_INFO, "Sock: "+std::to_string(sock)+", EnableReadWrite: "+((readable)?"Read":"")+((writeable)?" Write":""));
                }
            }

            bool IsExist(int sock)
            {
                unordered_map<int, Event>::iterator iter = events_.find(sock);
                return events_.end()==iter? false:true;
            }

            //派发事件
            // void DispatchEvent(int timeout)
            // {
                

            // }
            ~Reactor()
            {
                if(epfd_ >= 0) close(epfd_);
            }
    };
}
