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
#include <ctemplate/template.h>

#include "../../comm/Log.hpp"
#include "../../comm/Util.hpp"
#include "Model.hpp"

namespace ns_view{

    using namespace std;
    using namespace ns_util;
    using namespace ns_log;
    using namespace ns_model;

    const string path = "/code_test/project/load_balance_oj/main_server/output/ROOT/";
    class View{
        public:
            View(){}
            ~View(){}
            void RenderAllQuestionHtml(const vector<Question> &allQuestion, string *html)
            {
                //形成路径
                string srcHtml = path + "question_all.html";
                //形成数字典
                ctemplate::TemplateDictionary root("question_all");
                for (const auto& q : allQuestion)
                {
                    //由于是循环渲染题目数量次，需要子数字典subroot
                    ctemplate::TemplateDictionary *subroot = root.AddSectionDictionary("question_list");
                    subroot->SetValue("num", q.num);
                    subroot->SetValue("title", q.title);
                    subroot->SetValue("difficulty", q.difficulty);
                }

                //3. 获取被渲染的html (ctemplate::DO_NOT_STRIP原样获取，不做任何分割)
                ctemplate::Template *tpl = ctemplate::Template::GetTemplate(srcHtml, ctemplate::DO_NOT_STRIP);

                //4. 开始完成渲染功能
                tpl->Expand(html, &root);//用root数字典开始渲染
            }
            void RenderOneQuestionHtml(Question q, string *html)
            {
                // 1. 形成路径
                std::string src_html = path + "question.html";

                // 2. 形成数字典
                ctemplate::TemplateDictionary root("one_question");
                root.SetValue("num", q.num);
                root.SetValue("title", q.title);
                root.SetValue("difficulty", q.difficulty);
                root.SetValue("desc", q.desc);
                root.SetValue("pre_code", q.header);

                //3. 获取被渲染的html
                ctemplate::Template *tpl = ctemplate::Template::GetTemplate(src_html, ctemplate::DO_NOT_STRIP);
            
                //4. 开始完成渲染功能
                tpl->Expand(html, &root);
            }
    };
}//namespace ns_view: 渲染网页