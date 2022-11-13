#include <iostream>
#include <cstring>

//#include <json/json.h>

// #include "../../load_balance_oj/compile_server/Compiler.hpp"
// #include "../../load_balance_oj/compile_server/Runner.hpp"
#include "../CompileRun/Compile_Run.hpp"
#include "../../comm/Log.hpp"
#include "../../comm/Util.hpp"
#include "comm.hpp"
using std::cout;
using std::endl;
using std::cerr;

using namespace ns_compile_run;
int main()
{
    std::string inJson;
    GetQueryString(inJson);
    //cerr<<"inJson: "<<inJson<<endl;
    std::string outJson;
    CompileRun::Start(inJson, &outJson);
    std::cout << outJson<<std::endl;//重定向cout, cgi外部接收
    //cerr<<"outJson: "<<outJson<<endl;
}