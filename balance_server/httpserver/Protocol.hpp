#pragma once

#include <iostream>
#include <sstream>
#include <string>
#include <vector>
#include <unordered_map>
#include <algorithm>
#include <cstdlib>
#include <algorithm>
#include <assert.h>

#include <unistd.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/socket.h>
#include <sys/sendfile.h>
#include <sys/wait.h>
#include "Log.hpp"
#include "Util.hpp"

#define SEP ": "//HTTP请求头的key value分隔符
#define WEB_ROOT "ROOT"
#define HOME_PAGE "index.html"
#define HTTP_VERSION "HTTP/1.0"
#define LINE_END "\r\n"
#define PAGE_404 "404.html"


#define OK 200
#define NOT_FOUND 404
#define BAD_REQUEST 400
#define NOT_FOUND 404
#define SERVER_ERROR 500


//=================== Http Protocol ======================
static std::string Code2Desc(int code)
{
    std::string desc;
    switch(code){
        case 200:
            desc = "OK";
            break;
        case 404:
            desc = "Not Found";
            break;
        default:
            break;
    }
    return desc;
}

static std::string Suffix2Desc(const std::string &suffix)
{
    static std::unordered_map<std::string, std::string> suffix2desc = {
        {".html", "text/html"},
        {".css", "text/css"},
        {".js", "application/javascript"},
        {".jpg", "application/x-jpg"},
        {".xml", "application/xml"},
        {".json", "application/json"}
    };
    auto iter = suffix2desc.find(suffix);
    if(iter != suffix2desc.end()){
        return iter->second;
    }
    return "text/html";
}

class HttpRequest{
    public:
        std::string requestLine;//请求行
        std::vector<std::string> requestHeader;
        std::string blank;//空行
        std::string requestBody;//请求正文
//解析后结果
        //请求行
        //解析完毕之后的结果
        std::string method;//请求方法
        std::string uri; //请求资源（可能包括路径，参数）
        std::string version;//协议即版本
        //请求头
        std::unordered_map<std::string, std::string> headerMap;
        //请求正文
        std::string body;
//继续细化拆分        
        int ContentLength;
        std::string path;//uri中的路径
        std::string suffix;//获得的后缀
        std::string query_string;//uri中的参数(?后面的就是参数语句)

        bool cgi;
        int size;
    public:
        HttpRequest():ContentLength(0), cgi(false){}
        ~HttpRequest(){}
};

class HttpResponse{
    public:
//构建需要的部分
        std::string status_line;//状态行
        std::vector<std::string> response_header;//响应头
        std::string blank;//空行
        std::string response_body;//响应正文
//细化部分
        int statusCode;
        int fd;//fd用于读取文件，EndPoint的_sock用于接收文件
        int size;//request目标文件的大小，也是返回文件大小
    public:
        HttpResponse():blank(LINE_END),statusCode(OK),fd(-1){}
        ~HttpResponse(){}
};
//=================== Http Protocol ======================

//构建对端(读取请求，分析请求，构建响应，返回响应) 读取IO
class EndPoint{
    private:
        int sock;
        HttpRequest http_request;
        HttpResponse http_response;
        bool stop;//标记读取(写入)错误
        ns_reactor::Event *event;
    private:
        bool RecvHttpRequestLine()
        {
            auto &line = http_request.requestLine;
            if(Util::ReadLine(event, line) > 0){
                line.resize(line.size()-1);
                LOG(LOG_LEVEL_INFO, http_request.requestLine);
            }
            else{
                stop = true;//读取错误
            }
            return stop;
        }
        bool RecvHttpRequestHeader()
        {
            std::string line;
            while(true){
                line.clear();
                if(Util::ReadLine(event, line) <= 0){
                    stop = true;
                    break;
                }
                if(line == "\n"){
                    http_request.blank = line;
                    break;
                }
                line.resize(line.size()-1);
                http_request.requestHeader.push_back(line);
                LOG(LOG_LEVEL_INFO, line);
            }
            return stop;
        }
        void ParseHttpRequestLine()
        {
            auto &line = http_request.requestLine;
            std::stringstream ss(line);//使用line初始化ss，按空格输出到method，uri，version
            ss >> http_request.method >> http_request.uri >> http_request.version;
            auto &method = http_request.method;
            //可能受到的method并不标准，为小写get、post需转为大写(注意这里的::upper为仿函数(g++中可能在全局))
            std::transform(method.begin(), method.end(), method.begin(), ::toupper);
            
            LOG(LOG_LEVEL_INFO, http_request.method);
            LOG(LOG_LEVEL_INFO, http_request.uri);
            LOG(LOG_LEVEL_INFO, http_request.version);
        }
        void ParseHttpRequestHeader()
        {
            std::string key;
            std::string value;
            for(auto &iter : http_request.requestHeader)
            {
                if(Util::CutString(iter, key, value, SEP)){
                    http_request.headerMap[key]=value;//插入headerMap
                }
            }
        }
        
        bool HasBody()
        {
            auto &method = http_request.method;
            if(method == "POST"){
                auto &headerMap = http_request.headerMap;
                auto iter = headerMap.find("Content-Length");
                if(iter != headerMap.end()){
                    http_request.ContentLength = atoi(iter->second.c_str());
                    std::string logInfo="ContentLength: ";
                    logInfo+=iter->second;
                    LOG(LOG_LEVEL_INFO, logInfo);
                    return true;
                }
            }
            return false;
        }

        bool RecvHttpRequestBody()
        {
            if(HasBody()){
                int n = http_request.ContentLength;
                auto &body = http_request.requestBody;
                while(!event->in_buffer_.empty())
                {
                    body.push_back(event->in_buffer_.back());
                    event->in_buffer_.pop_back();
                }
                if(n!=body.size())
                {
                    LOG(LOG_LEVEL_ERROR, "recv body error");
                    stop=true;
                }
                LOG(LOG_LEVEL_INFO, body);
                // int ContentLength = http_request.ContentLength;
                // auto &body = http_request.requestBody;

                // char ch = 0;
                // while(ContentLength){
                //     ssize_t s = recv(sock, &ch, 1, 0);
                //     if(s > 0){
                //         body.push_back(ch);
                //         ContentLength--;
                //     }
                //     else{
                //         stop = true;
                //         break;
                //     }
                    
                // }
                // std::cerr<<"body:? stop:"<<stop<<std::endl;
                // LOG(LOG_LEVEL_INFO, http_request.requestBody);
            }
            else{
                LOG(LOG_LEVEL_INFO, "Method GET No body");
            }
            return stop;
        }
        int ProcessCgi()
        {
            LOG(LOG_LEVEL_INFO, "Process CGI...");

            int code = OK;
            //父进程数据
            auto &method = http_request.method;
            auto &query_string =  http_request.query_string; //GET
            auto &body_text = http_request.requestBody;     //POST
            auto &bin = http_request.path; //要让子进程执行的目标程序,一定存在
            int ContentLength = http_request.ContentLength;
            auto &response_body = http_response.response_body;

            std::string query_string_env;
            std::string method_env;
            std::string ContentLength_env;

            //站在父进程角度
            int input[2];
            int output[2];
            
            if(pipe(input) < 0){
                LOG(ERROR, "pipe input error");
                code = SERVER_ERROR;
                return code;
            }
            if(pipe(output) < 0){
                LOG(ERROR, "pipe output error");
                code = SERVER_ERROR;
                return code;
            }
            //std::cerr<<"*****"<<code<<std::endl;
            //新线程，但是从头到尾都只有一个进程，就是httpserver！
            pid_t pid = fork();
            if(pid == 0 ){ //child
                close(input[0]);
                close(output[1]);

                method_env = "METHOD=";
                method_env += method;

                putenv((char*)method_env.c_str());

                if(method == "GET"){
                    query_string_env = "QUERY_STRING=";
                    query_string_env += query_string;
                    putenv((char*)query_string_env.c_str());
                    LOG(LOG_LEVEL_INFO, "Get Method, Add Query_String Env");
                }
                else if(method == "POST"){
                    ContentLength_env = "CONTENT_LENGTH=";
                    ContentLength_env += std::to_string(ContentLength);
                    putenv((char*)ContentLength_env.c_str());
                    LOG(LOG_LEVEL_INFO, "Post Method, Add ContentLength Env");
                }
                else{
                    //Do Nothing
                }

                //替换成功之后，目标子进程如何得知，对应的读写文件描述符是多少呢?不需要，只要读0， 写1即可
                //站在子进程角度
                //input[1]: 写出  -> 1 -> input[1] 
                //output[0]: 读入 -> 0 -> output[0]
                
                //std::cout << "bin: " << bin << std::endl;

                dup2(output[0], 0);
                dup2(input[1], 1);

                execl(bin.c_str(), bin.c_str(), nullptr);
                exit(1);
            }
            else if(pid < 0){ //error
                LOG(ERROR, "fork error!");
                return 404;
            }
            else{ //parent
                close(input[1]);
                close(output[0]);

                if(method == "POST"){
                    //std::cerr<<"=FATHER====METHOD"<<method<<std::endl;
                    const char *start = body_text.c_str();
                    int total = 0;
                    int size = 0;
                    while(total < ContentLength && (size= write(output[1], start+total, body_text.size()-total)) > 0){
                        total += size;
                    }
                }

                char ch = 0;
                while(read(input[0], &ch, 1) > 0){
                    response_body.push_back(ch);
                }
                int status = 0;
                pid_t ret = waitpid(pid, &status, 0);
                if(ret == pid){
                    if(WIFEXITED(status)){//WIFEXITED(status) 若此值为非0 表明进程正常结束
                        if(WEXITSTATUS(status) == 0){
                            code = OK;
                        }
                        else{
                            code = BAD_REQUEST;
                        }
                    }
                    else{
                        code = SERVER_ERROR;
                    }
                }

                close(input[0]);
                close(output[1]);
            }
            
            return code;
        }
        void ErrorHandler(std::string page)
        {
            //std::cout << "debug: " << page << std::endl;
            http_request.cgi = false;
            //要给用户返回对应的404页面
            http_response.fd = open(page.c_str(), O_RDONLY);
            if(http_response.fd > 0){
                struct stat st;
                stat(page.c_str(), &st);
                http_request.size = st.st_size;

                std::string line = "Content-Type: text/html";
                line += LINE_END;
                http_response.response_header.push_back(line);

                line = "Content-Length: ";
                line += std::to_string(st.st_size);
                line += LINE_END;
                http_response.response_header.push_back(line);
            }
        }
        int ProcessNonCgi()//非cgi模式下构建response，并准备
        {
            http_response.fd = open(http_request.path.c_str(), O_RDONLY);
            if(http_response.fd >= 0){
                //LOG(INFO, http_request.path + " open success!");
                return OK;
            }
            return NOT_FOUND;//只要返回404就还没有构建response
        }
        void OkHandler()//构建200的响应（除状态行）
        {
            //构建responseHeader
            std::string line = "Content-Type: ";
            line += Suffix2Desc(http_request.suffix);
            line += LINE_END;
            http_response.response_header.push_back(line);

            line = "Content-Length: ";
            if(http_request.cgi){
                line += std::to_string(http_response.response_body.size());
            }
            else{
                line += std::to_string(http_request.size); //Get
            }
            line += LINE_END;
            http_response.response_header.push_back(line);
        }
        void BuildHttpResponseHelper()
        {
            //统一处理返回错误码的逻辑
            //构建响应状态行
            auto &code = http_response.statusCode;
            //构建状态行
            auto &status_line = http_response.status_line;
            status_line += HTTP_VERSION;
            status_line += " ";
            status_line += std::to_string(code);
            status_line += " ";
            status_line += Code2Desc(code);
            status_line += LINE_END;

            //构建响应正文,可能包括响应报头
            std::string path = WEB_ROOT;
            path += "/";
            switch(code){
                case OK:
                    OkHandler();
                    break;
                case NOT_FOUND:
                    path += PAGE_404;
                    ErrorHandler(path);
                    break;
                case BAD_REQUEST:
                    path += PAGE_404;
                    ErrorHandler(path);
                    break;
                case SERVER_ERROR:
                    path += PAGE_404;
                    ErrorHandler(path);
                    break;
//                case 500:
//                  ErrorHandler(PAGE_500);
//                    break;
                default:
                    break;
            }
        }
    public:
        EndPoint(int _sock, ns_reactor::Event *ev):sock(_sock), stop(false), event(ev)
        {}
        bool IsStop()
        {
            return stop;
        }
        void RecvHttpRequest()//如果读取数据无问题(stop==false)才执行ParseHttpRequest
        {
            std::reverse(event->in_buffer_.begin(), event->in_buffer_.end());
            // || 短路求值
            if( (!RecvHttpRequestLine()) && (!RecvHttpRequestHeader()) ){
                ParseHttpRequest();
            }
        }
        void ParseHttpRequest()
        {
            ParseHttpRequestLine();//解析(拆分)请求行
            ParseHttpRequestHeader();//解析(拆分)请求头
            RecvHttpRequestBody();//通过Content-Length获得body（GET无body，POST才有）
            event->in_buffer_.clear();
        }
        void BuildHttpResponse()
        {
            //g++报错，必须统一提前初始化（cross initialization）
            //请求已经全部读完,即可以直接构建响应了
            std::string _path;
            struct stat st;
            std::size_t found = 0;
            auto &code = http_response.statusCode;//之后的code的设置直接为httpresponse.statusCode设置
            
            //开始构建响应
            //CGI处理（GET带参 GET的path为可执行文件  POST正文）
            if(http_request.method != "GET" && http_request.method != "POST"){
                //非法请求
                //std::cout << "method: " << http_request.method << std::endl;
                LOG(LOG_LEVEL_WARNING, "method is not right");
                code = BAD_REQUEST;
                goto END;
            }
            if(http_request.method == "GET")
            {
                 //判断uri中是否带参(只有GET方法才可能uri带参)
                size_t pos = http_request.uri.find('?');
                if(pos != std::string::npos){
                    Util::CutString(http_request.uri, http_request.path, http_request.query_string, "?");
                    http_request.cgi = true;
                }
                else{//不带参
                    http_request.path = http_request.uri;
                }
            }
            else if(http_request.method == "POST"){
                //POST
                http_request.cgi = true;//proceed cgi...
                http_request.path = http_request.uri;//注意不要忘记这一步
            }
            else{
                //Do Nothing
            }

            //构建路径(ROOT为根目录)
            _path = http_request.path;
            http_request.path = WEB_ROOT;
            http_request.path += _path;
            if(http_request.path[http_request.path.size()-1] == '/'){
                //请求的是一个路径(只有路径什么都不写的时候浏览器才会自动加上'/')
                http_request.path += HOME_PAGE;//对于path结尾为'/', 加上首页index.html
            }
            if(stat(http_request.path.c_str(), &st) == 0)//stat用于判断path所表示的资源是否存在
            {
                //cout<<"资源存在"<<endl;
                if(S_ISDIR(st.st_mode))//结构体成员st_mode用于判断文件类型
                {
                    //说明请求的资源是一个目录，不被允许的,需要做一下相关处理
                    //虽然是一个目录，但是绝对不会以/结尾！
                    //为路径(加上'/index.html')
                    http_request.path += "/";
                    http_request.path += HOME_PAGE;
                    stat(http_request.path.c_str(), &st);//重新stat一次，便于获取新构建路径下文件的size
                }
                if( (st.st_mode&S_IXUSR) || (st.st_mode&S_IXGRP) || (st.st_mode&S_IXOTH) ){
                    //(S_IXUSR所有者可执行/S_IXGRP组可执行/S_IXOTH其他成员可执行)
                    http_request.cgi = true;
                    //Proccess cgi...
                    //cout<<"excuatable!"<<endl;//文件可执行
                }
                http_request.size = st.st_size;//用于传给ProceedNoneCgi（改良不用传参）
            }
            else
            {
                //资源不存在
                std::string info = http_request.path;
                info+=" Not Found...";
                LOG(LOG_LEVEL_WARNING, info);
                code=NOT_FOUND;
                goto END;
            }

            //获取资源后缀suffix：
            found = http_request.path.rfind(".");
            if(found == std::string::npos){
                http_request.suffix = ".html";
            }
            else{
                http_request.suffix = http_request.path.substr(found);
            }

/// 是否进行CGI处理  ///
            if(http_request.cgi){
                code = ProcessCgi(); //执行目标程序，拿到结果:http_response.response_body;
            }
            else{
                //1. 目标网页一定是存在的
                //2. 返回并不是单单返回网页，而是要构建HTTP响应
                code = ProcessNonCgi(); //简单的网页返回，返回静态网页,只需要打开即可
            }
END:
            BuildHttpResponseHelper(); //状态行填充了，响应报头也有了， 空行也有了，正文有了
        }
        void SendHttpResponse()
        {
            LOG(LOG_LEVEL_INFO, "Sending HttpResponse...");
            //发送响应状态行
            //send(sock, http_response.status_line.c_str(), http_response.status_line.size(), 0);
            event->out_buffer_+=http_response.status_line;
            LOG(LOG_LEVEL_INFO, http_response.status_line);

            //发送响应报头(待补充,这里只有Content-Type, Content-Length)
            for(auto iter : http_response.response_header)
            {
                //send(sock, iter.c_str(), iter.size(), 0);
                event->out_buffer_+=iter;
                LOG(LOG_LEVEL_INFO, iter);
            }
            //发送空行
            //send(sock, http_response.blank.c_str(), http_response.blank.size(), 0);
            event->out_buffer_+=http_response.blank;
            LOG(LOG_LEVEL_INFO, http_response.blank);

            //发送数据

            if(http_request.cgi){//cgi: httpresponse.responseBody.size()
                auto &response_body = http_response.response_body;
                event->out_buffer_+=response_body;
                LOG(LOG_LEVEL_INFO, response_body);
                // auto &response_body = http_response.response_body;
                // size_t size = 0;
                // size_t total = 0;
                // const char *start = response_body.c_str();
                // while( total < response_body.size() && (size = send(sock, start + total, response_body.size() - total, 0)) > 0)
                // {
                //     total += size;
                // }
            }
            else{//httpresponse.size
                LOG(LOG_LEVEL_INFO, "Read From File...");
                char buffer[1024];
                buffer[0]=0;
                ssize_t s = read(http_response.fd, buffer, sizeof(buffer)-1);
                while(s>0)
                {
                    buffer[s]=0;
                    event->out_buffer_+=buffer;
                    buffer[0]=0;
                    s = read(http_response.fd, buffer, sizeof(buffer)-1);
                    // if(event->out_buffer_.size()==http_request.size)
                    //     break;
                }
                //sendfile 0拷贝
                //sendfile(sock, http_response.fd, nullptr, http_request.size);
                close(http_response.fd);
            }
            //cerr<<"=====OUT BUFFER======"<<endl<<event->out_buffer_<<endl;
            //(event->r_)->EnableReadWrite(event->sock_, true, true);
            assert(event);
            event->send_callback_(*(event));
            
            
        }
        ~EndPoint()
        {
            close(sock);
        }
};


class CallBack{
    public:
        CallBack()
        {}
        void operator()(int sock, ns_reactor::Event *ev)
        {
            HandlerRequest(sock, ev);
        }
        void HandlerRequest(int sock, ns_reactor::Event *ev)
        {
            cout<<endl;
            LOG(LOG_LEVEL_INFO, "Entrance: Handler begin ...");
            cout<<endl;
#ifdef DEBUG
            //For Test
            char buffer[4096];
            recv(sock, buffer, sizeof(buffer), 0);
            std::cout << "-------------begin----------------" << std::endl;
            std::cout << buffer << std::endl;
            std::cout << "-------------end----------------" << std::endl;
#else 
            EndPoint *ep = new EndPoint(sock, ev);
            ep->RecvHttpRequest();
            if(!ep->IsStop()){ //如果stop为true了后面的逻辑就没必要执行了
                LOG(LOG_LEVEL_INFO, "Start Building&&Sending");
                ep->BuildHttpResponse();
                ep->SendHttpResponse();
            }
            else{
                LOG(LOG_LEVEL_WARNING, "Recv Line or Recv Header Error Stop Building&&Sending ...");
            }
            if(ep)
                delete ep;
#endif
            LOG(LOG_LEVEL_INFO, "Entrance: Handler end ...");
        }
        ~CallBack()
        {}
};

