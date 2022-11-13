#include <iostream>
#include <signal.h>

#include "httplib.h"
#include "../MVC/Controller.hpp"

using namespace httplib;
using namespace ns_controller;

static Controller *ctrl_ptr = nullptr;

void Recovery(int signo)
{
    ctrl_ptr->RecoverMachine();
}

int main()
{
    signal(SIGQUIT, Recovery);

    //用户请求的服务路由功能
    Server svr;

    Controller ctrl;
    ctrl_ptr = &ctrl;


//不需要View渲染
    

//需要View渲染
    // 获取所有的题目列表
    svr.Get("/question_all", [&ctrl](const Request &req, Response &resp){
        //返回一张包含有所有题目的html网页
        std::string html;
        ctrl.AllQuestionHtml(&html);
        //用户看到的是什么呢？？网页数据 + 拼上了题目相关的数据
        resp.set_content(html, "text/html; charset=utf-8");
    });

    // 用户要根据题目编号，获取题目的内容
    // /question/100 -> 正则匹配
    // R"()", 原始字符串raw string,保持字符串内容的原貌，不用做相关的转义
    svr.Get(R"(/question/(\d+))", [&ctrl](const Request &req, Response &resp){ //restful风格
        std::string number = req.matches[1];
        std::string html;
        ctrl.OneQuestionHtml(number, &html);
        resp.set_content(html, "text/html; charset=utf-8");
    });

    // 用户提交代码，使用我们的判题功能(1. 每道题的测试用例 2. compile_and_run)
    svr.Post(R"(/judge/(\d+))", [&ctrl](const Request &req, Response &resp){ //restful风格
        std::string number = req.matches[1];
        std::string resultJson;
        ctrl.Judge(number, req.body, &resultJson);
        resp.set_content(resultJson, "application/json;charset=utf-8");
        // resp.set_content("指定题目的判题: " + number, "text/plain; charset=utf-8");
    });
    
    svr.set_base_dir("./ROOT");//根目录
    svr.listen("0.0.0.0", 8081);//ip端口
    return 0;
}