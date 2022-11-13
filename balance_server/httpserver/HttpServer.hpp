#pragma once

#include <iostream>
#include <pthread.h>
#include <signal.h>

#include <assert.h>
#include "Log.hpp"
#include "Reactor.hpp"
#include "TcpServer.hpp"
//#include "Task.hpp"
#include "ThreadPool.hpp"
#include "Accepter.hpp"

#define PORT 8081

class HttpServer{
    private:
        int port;
        bool stop;
    public:
        HttpServer(int _port = PORT): port(_port),stop(false)
        {}
        void InitServer()
        {
            //忽略写入时错误(管道单向写 对端关闭)(非Reactor模式时处理写入错误)
            signal(SIGPIPE, SIG_IGN); 
        }
        void Loop()
        {
            //创建Reactor
            ns_reactor::Reactor* R = new ns_reactor::Reactor();
            R->ReactorInit();

            //创建Sock
            TcpServer *tsvr = TcpServer::GetInstance(port);
            int listen_sock_ = tsvr->Sock();
            tsvr->SetNonBlock(listen_sock_);//设为非阻塞(ET模式下所有IO均为非阻塞)
            //S->SockBind(listen_sock_, (u_int16_t)atoi(argv[1]));//端口号
            //S->SockListen(listen_sock_);

            //创建Event
            ns_reactor::Event E;
            E.sock_ = listen_sock_;
            E.r_ = R;

            //Accepter链接管理器
            E.RegisterCallback(Accepter, nullptr, nullptr);//listen_sock_只需要监听读即recv_

            //将E注册进R
            R->AddEvent(E, EPOLLIN|EPOLLET);//ET设为ET模式
#define NUM 11000
            struct epoll_event epevs[NUM];//事件数组

            //进入事件派发逻辑, 服务器启动
            //int timeout=1000;
            while(1)
            {
                int rd_num = epoll_wait(R->epfd_, epevs, NUM, -1);
                //cerr<<"epoll_wait: "<<rd_num<<endl;
                //sleep(1);
                for(int i=0;i<rd_num;i++)
                {
                    //cerr<<"========================================================="<<endl;
                    int sock = epevs[i].data.fd;
                    uint32_t events = epevs[i].events;
                    LOG(LOG_LEVEL_INFO, "Sock: "+std::to_string(sock)+", Has Data");

                    //1.事件异常（将所有的异常，全部交给读写处理，读写处理遇到的所有的异常，都会交给Errorer统一处理）
                    if(events & EPOLLERR) events |= (EPOLLIN|EPOLLOUT);
                    //2.对端关闭链接（将所有的异常，全部交给读写处理，读写处理遇到的所有的异常，都会交给Errorer统一处理）
                    if(events & EPOLLHUP ) events |= (EPOLLIN|EPOLLOUT);
                    //3.读事件就绪
                    //cerr<<"Read: "<<(R->IsExist(sock)?"exist":"not exist")<<endl;
                    if((R->IsExist(sock))&&(events & EPOLLIN))
                    {
                        if(R->events_[sock].recv_callback_)//recv_callback_不为nullptr有回调
                        {
                            R->events_[sock].recv_callback_(R->events_[sock]);//回调参数Event &
                            // if(sock != listen_sock_){//非Accept事件从线程池获取线程
                            //     Task t(sock, &(R->events_[sock]));
                            //     ThreadPool<Task>::getinstance()->PushTask(t);
                            // }
                        }
                    }
                    // cerr<<"OUTBUFFER: "<<endl<<R->events_[sock].out_buffer_<<endl;
                    // if(sock != listen_sock_){
                    //    R->EnableReadWrite(R->events_[sock].sock_, true, true);
                    // }
                    //cerr<<"Write: "<<(R->IsExist(sock)?"exist":"not exist")<<endl;
                    //4.写事件就绪
                    if((R->IsExist(sock)) && (events & EPOLLOUT))
                    {
                        //注意需要判断是否存在此sock (可能发生读错误Errorer->DelEvent->进行清楚events_[sock]找不到，出错)
                        if(R->events_[sock].send_callback_)//send_callback_不为nullptr有回调
                        {
                            R->events_[sock].send_callback_(R->events_[sock]);//回调参数Event &
                        }
                    }
                    //cerr<<"========================================================="<<endl;
                }
            }
            // TcpServer *tsvr = TcpServer::GetInstance(port);
            // LOG(LOG_LEVEL_INFO, "Loop begin");
            // while(!stop){
            //     struct sockaddr_in peer;
            //     socklen_t len = sizeof(peer);
            //     int sock = accept(tsvr->Sock(), (struct sockaddr*)&peer, &len);
            //     if(sock < 0){
            //         continue;
            //     }
            //     LOG(INFO, "Get a new link");
            //     Task task(sock);
            //     ThreadPool<>::getinstance()->PushTask(task);

            //     //未使用线程池
            //     // int* sock_ = new int(sock);
            //     // pthread_t tid;
            //     // pthread_create(&tid, NULL, Entrance::Handler, sock_);//创建多线程
            //     // pthread_detach(tid);//分离线程
            //}
        }
        ~HttpServer()
        {}
};

