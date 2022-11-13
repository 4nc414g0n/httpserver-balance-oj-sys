#pragma once
#include <iostream>
#include <string>
#include <cerrno>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include "Reactor.hpp"
#include "Callback.hpp"
//#include "TcpServer.hpp"

using namespace std;
using namespace ns_reactor;

#define TPORT 8081
//回调参数为 Event & （typedef void(*callback_t)(Event &); //函数指针）

void Accepter(Event &event)
{
    LOG(LOG_LEVEL_INFO, "Calling Accepter...");
    while(1)// ET工作模式下必须循环读取 (同一时间内可能有很多链接到来）
    {
        //accept
        struct sockaddr_in peer;
        socklen_t len = sizeof(peer);
        int sock = accept(event.sock_, (struct sockaddr*)&peer, &len);
        if(sock>0)
        {
            //0. 设置fd为非阻塞
            TcpServer::GetInstance()->SetNonBlock(sock);//ET模式下所有IO必须是非阻塞的

            //1.构建新的与sock对应的Event对象
            Event ev;//多个Event对象
            ev.sock_ = sock;//多个sock
            ev.r_ = event.r_;//只有一个Reactor对象

            ev.RegisterCallback(Recver, Sender, Errorer);
            //添加事件 (设为常读（ET）)
            (ev.r_)->AddEvent(ev, EPOLLIN | EPOLLET);
        }
        
        //被信号中断(不代表没有连接了)
        else if(errno == EINTR)
        {
            continue;
        }
        //底层无连接直接break
        else if(errno == EAGAIN || errno == EWOULDBLOCK)
        {
            break;
        }
        else
        {
            LOG(LOG_LEVEL_ERROR, "Accept Error...");
            continue;
        }

    }
}
