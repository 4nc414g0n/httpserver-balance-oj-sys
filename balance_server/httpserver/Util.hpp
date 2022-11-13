#pragma once

#include <iostream>
#include <string>
#include <sys/types.h>
#include <sys/socket.h>

//工具类
class Util{
    public:
        static int ReadLine(ns_reactor::Event* event, std::string &out)
        {
            //cout<<"inbuffer: "<<endl<<event->in_buffer_<<endl;
            if(event->in_buffer_.empty())
                return -1;
            char ch = 0;
            while(!event->in_buffer_.empty() && ch!='\n')
            {
                ch = event->in_buffer_.back();
                event->in_buffer_.pop_back();
                if('\r'==ch){
                    ch = event->in_buffer_.back();//数据窥探
                    if('\n'==ch)// \r\n->\n
                        event->in_buffer_.pop_back();
                    else{
                        ch='\n';
                    }      
                }
                //else: ch=='\n'
                out.push_back(ch);
            }
            if(out.back()=='\n')//没有读取到结束符一定有错
                return out.size();
            else
                return -1;
            // char ch = 'X';
            // while(ch != '\n'){
            //     ssize_t s = recv(sock, &ch, 1, 0);
            //     if(s > 0){
            //         if(ch == '\r'){
            //             recv(sock, &ch, 1, MSG_PEEK);
            //             if(ch == '\n'){
            //                 //把\r\n->\n
            //                 //窥探成功，这个字符一定存在
            //                 recv(sock, &ch, 1, 0);
            //             }
            //             else{
            //                 ch = '\n';
            //             }
            //         }
            //         //1. 普通字符
            //         //2. \n
            //         out.push_back(ch);
            //     }
            //     else if(s == 0){
            //         return 0;
            //     }
            //     else{
            //         return -1;
            //     }
            // }
            // return out.size();
        }
        static bool CutString(const std::string &target, std::string &sub1_out, std::string &sub2_out, std::string sep)
        {
            size_t pos = target.find(sep);
            if(pos != std::string::npos){
                sub1_out = target.substr(0, pos);
                sub2_out = target.substr(pos+sep.size());
                return true;
            }
            return false;
        }
};
