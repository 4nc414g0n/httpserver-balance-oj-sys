#pragma once

//#include <algorithm>

#include "Reactor.hpp"
#include "Util.hpp"

using namespace ns_reactor;

static int RecvHelper(int sock, string *out)//(注意循环读取)
{
    while(true)
    {
        char buffer[1024];
        ssize_t s = recv(sock, &buffer, sizeof(buffer)-1, 0);//0设置非阻塞读取
        if(s>0)
        {
            buffer[s]=0;
            (*out)+=buffer;
        }
        else if(s<0){
            if(errno == EINTR)//被中断
                continue;
            else if(errno == EAGAIN || errno == EWOULDBLOCK){
                LOG(LOG_LEVEL_INFO, "RecvHelper, This Turn Over...");
                return 0;//本轮读完返回0
            }
                
            else 
                return -1;//读取出错返回-1
        }
        else
            return -1;//出错返回-1
    }
}

void Recver(Event &event)
{
    //1.如何知道本轮读取完毕（return 0）
    //
    if(-1 == RecvHelper(event.sock_, &(event.in_buffer_)))//负责读取 (读取出错返回-1)
    {
        if(event.error_callback_) event.error_callback_(event);
        return;
    }
    //未出错继续向下
    //2. 协议（协议分析，自定X为报文间分隔符，）

    // 往后我们所写的内容，已经和Reactor无关了！全部都是数据分析与处理
    // 协议：你怎么知道你拿到了一个完整的报文？我们不知道，因为我们没有定制协议!
    // 但是，所有的数据全部已经在inbuffer中(无论是现在的，还是未来的)
    // 协议：如果读取数据在协议层面，没有读完完整报文，应该如何？
    // 1+1X2*3X3*5X7*9X9*10X9*   //\3
    // X: 叫做报文和报文之间的分割符
    // 类似1+1:一个完整报文，协议解析, 解决粘包问题
    //cout<<"====inbuffer: "<<event.in_buffer_<<endl;
    
    Task t(event.sock_, &event);
    ThreadPool<Task>::getinstance()->PushTask(t);
    
}

// recv_buffer: 输入输出型参数
// ret >  0 : 缓冲区数据全部发完
// ret == 0 : 本轮发送完, 但是缓冲区还有数据
// ret < 0  : 发送失败
static int SenderHelper(int sock, string &recv_buffer)
{
    //这次能发了，是不是代表能发所有的数据呢？？
    int total = 0; //目前我们已经累计发送了所少
    const char *start = recv_buffer.c_str();
    int size = recv_buffer.size();
    while(true)
    {
        ssize_t s = send(sock, start+total, size-total, 0); //size-total需要发送的字节数（不一定一次发送完，缓冲区填满），flag(0)
        // 3. 本轮缓冲区足够大，引起我们把所有的数据已经全部发送完毕
        if(s>0)
        {
            total+=s;
            if(total==size)
            {
                recv_buffer.clear();//清空缓冲区
                return 1;
            }
        }
        else{
            if(errno == EINTR)
                continue;
            else if(errno==EAGAIN || errno == EWOULDBLOCK)//EAGAIN || EWOULDBLOCK
            {
                //缓冲区打满代表本轮发送结束
                //先清空这部分的缓冲区
                recv_buffer.erase(0, total);
                return 0;
            }
            else{
                //出错
                return -1;
            }
        }
    }
}
void Sender(Event &event)
{
    LOG(LOG_LEVEL_INFO, "Start Sending...");
    //assert((event.r_->events_[event.sock_] & EPOLLOUT)==true);
    int ret = SenderHelper(event.sock_, event.out_buffer_);
    
    if(-1 == ret)//出错
    {
        LOG(LOG_LEVEL_ERROR, "SenderHelper Error, Send Failed...");
        if(event.error_callback_) event.error_callback_(event);
    }
    else if(0 == ret)//缓冲区满，可以继续下一轮
    {
        (event.r_)->EnableReadWrite(event.sock_ ,true ,true);
    }
    else if(1 == ret)//发送完毕，写事件关闭
    {
        (event.r_)->EnableReadWrite(event.sock_ ,true ,false);
    }
}
void Errorer(Event &event)
{
    LOG(LOG_LEVEL_INFO, "Calling Errorer");
    (event.r_)->DelEvent(event.sock_);
}
