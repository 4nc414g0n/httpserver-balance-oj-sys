#pragma once

#include <iostream>
#include <cstring>
#include <vector>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/types.h>

#include "../../comm/Log.hpp"

using namespace std;

#define LINE_END "\r\n"
#define HTTP_VERSION "HTTP/1.0"
#define NOT_FOUND 404
#define OK 200


/*********************************************
*  这里的httpclient仅用于负载均衡选择主机发送POST请求
*  ！！！仅进行简单处理获取response
*  获得的body一定是一个json串
*********************************************/

static int ReadLine(int sock, std::string &out)
{
    //cout<<"inbuffer: "<<endl<<event->in_buffer_<<endl;
    char ch = 'X';
    while(ch != '\n'){
        ssize_t s = recv(sock, &ch, 1, 0);
        if(s > 0){
            if(ch == '\r'){
                recv(sock, &ch, 1, MSG_PEEK);
                if(ch == '\n'){
                    //把\r\n->\n
                    //窥探成功，这个字符一定存在
                    recv(sock, &ch, 1, 0);
                }
                else{
                    ch = '\n';
                }
            }
            //1. 普通字符
            //2. \n
            out.push_back(ch);
        }
        else if(s == 0) return 0;
        else return -1;
    }
    return out.size();
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
namespace ns_httpclient{
	
    using namespace ns_log;


    class HttpResponse{
    public:
    //构建需要的部分
        std::string status_line;//状态行
        std::vector<std::string> response_header;//响应头
        std::string blank;//空行
        std::string response_body;//响应正文
    //细化部分
        int ContentLength;
        int statusCode;
    public:
        HttpResponse():blank(LINE_END),statusCode(OK),ContentLength(0){}
        ~HttpResponse(){}
    };


	class HttpClient{
		private:
			string dest_ip;//对端服务器ip
			uint16_t dest_port;//对端服务器port
			int sock;//fd
            HttpResponse http_response;//获得的响应
		public:
		    HttpClient(string _ip, uint16_t _port)
				:dest_ip(_ip)
				 ,dest_port(_port)
				 ,sock(-1)
			{}
			void InitTCPclient()
			{
				//创建socket
				sock = socket(AF_INET, SOCK_STREAM, 0);
				if(sock<0){
					cerr<<"socket error"<<endl;
					exit(1);
				}
				//注意：tcp_client不用绑定bind，监听listen，接收accept
			}
			//tcp面向连接，client要通信前要先连接
			bool GetoutJson(std::string &path, std::string &inJson, std::string& ContentType, std::string *outJson)
			{
                InitTCPclient();//初始化

				//填充对端服务器socket信息
				struct sockaddr_in svr;
				bzero(&svr, sizeof(svr));
				svr.sin_family=AF_INET;
				svr.sin_port = htons(dest_port);
				svr.sin_addr.s_addr = inet_addr(dest_ip.c_str());//注意用.c_str()将string转c字符
				//发起链接请求
				if(connect(sock, (struct sockaddr*)&svr, sizeof(svr))==0)//connect成功返回0
					LOG(LOG_LEVEL_INFO)<<"HttpClient "<<dest_ip<<":"<<dest_port<<" Connection Succeed !"<<endl;
				else{
					LOG(LOG_LEVEL_FATAL)<<"HttpClient "<<dest_ip<<":"<<dest_port<<" Connection Failed..."<<endl;
					return false;
				}
            //构建请求：
                std::string HttpRequest;
                HttpRequest+="POST ";
                HttpRequest+=path;
                HttpRequest+=" ";
                HttpRequest+=HTTP_VERSION;
                HttpRequest+=LINE_END;
            //请求头
                HttpRequest+="Content-Type: ";
                HttpRequest+=ContentType;
                HttpRequest+=LINE_END;

                HttpRequest+="Content-Length: ";
                HttpRequest+=std::to_string(inJson.size());
                HttpRequest+=LINE_END;
            //空行
                HttpRequest+=LINE_END;
            //正文
                HttpRequest+=inJson;

                //cout<<HttpRequest<<endl;

            //写入：
                if(send(sock, (const void *)HttpRequest.c_str(), HttpRequest.size(), 0) < 0)
                {
                    LOG(LOG_LEVEL_ERROR)<<"HttpClient Send HttpRequest Error: "<<std::to_string(errno)<<endl;
                    return false;
                }
                auto &line  = http_response.status_line;//状态行
                if(ReadLine(sock, line)>0)
                {
                    line.resize(line.size()-1);
                    std::string header;
                    while(1){
                        header.clear();
                        if(ReadLine(sock, header) <= 0){
                            LOG(LOG_LEVEL_ERROR)<<"HttpClient Recv Error: "<<std::to_string(errno)<<endl;
                            return false;
                        }
                        if(header == "\n"){//读到空行了
                            http_response.blank = header;
                            break;
                        }
                        else{//正常的一行报头
                            header.resize(header.size()-1);
                            std::string key;
                            std::string value;
                            CutString(header,key,value,": ");//获取body大小
                            if(key == "Content-Length") http_response.ContentLength=atoi(value.c_str());
                        }
                        http_response.response_header.push_back(header);
                        char ch = 0;
                        while(http_response.ContentLength){
                            ssize_t s = recv(sock, &ch, 1, 0);
                            if(s > 0){
                                http_response.response_body.push_back(ch);
                                http_response.ContentLength--;
                            }
                            else if(s<0){
                                LOG(LOG_LEVEL_ERROR)<<"HttpClient Recv Error: "<<std::to_string(errno)<<endl;
                                return false;
                            }
                            else break;
                        }
                    }
                    *(outJson)=http_response.response_body;
                    return true;
                }
                LOG(LOG_LEVEL_ERROR)<<"HttpClient Recv Error: "<<std::to_string(errno)<<endl;
                return false;
                
			}
			~HttpClient()
			{
				if(sock >= 0) close(sock);
			}
	};
};
