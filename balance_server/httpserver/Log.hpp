
#pragma once
#include <iostream>
#include <ctime>
using std::cout;
using std::endl;
using std::cerr;

#define NONE "\033[m"
#define RED "\033[0;32;31m"
#define LIGHT_RED "\033[1;31m"
#define GREEN "\033[0;32;32m"
#define LIGHT_GREEN "\033[1;32m"
#define BLUE "\033[0;32;34m"
#define LIGHT_BLUE "\033[1;34m"
#define DARY_GRAY "\033[1;30m"
#define CYAN "\033[0;36m"
#define LIGHT_CYAN "\033[1;36m"
#define PURPLE "\033[0;35m"
#define LIGHT_PURPLE "\033[1;35m"
#define BROWN "\033[0;33m"
#define YELLOW "\033[1;33m"
#define LIGHT_GRAY "\033[0;37m"
#define WHITE "\033[1;37m"
#define SHINE "\033[5m"      //闪烁
#define DASH "\033[9m"       // 中间一道横线
#define QUICKSHINE "\033[6m" //快闪
#define FANXIAN "\033[7m"    //反显
#define XIAOYIN "\033[8m"    // 消隐，消失隐藏

//======== 简易日志 ===========

enum LOGLEVEL
{
	LOG_LEVEL_ERROR=1,     // error
	LOG_LEVEL_WARNING,   // warning
	LOG_LEVEL_FATAL,     // fatal
	LOG_LEVEL_INFO,      // info	
};

std::string TimeStampToTime()
{
	std::string ret;
	char myStr[25] = { 0 };
	time_t cur_t = time(nullptr);
	struct tm *t = gmtime(&cur_t);
	t->tm_hour += 8;//转为北京时间记的要加8
	std::string myFormat = "%Y-%m-%d:%H:%M:%S";
	strftime(myStr, sizeof(myStr), myFormat.c_str(), t);
	for (int i = 0; myStr[i]; ++i) {
		ret += myStr[i];
	}
	return ret;
}

//Log(level)<<"message"<<'\n'
std::string GetColor(const std::string &level)
{
	if(level=="LOG_LEVEL_ERROR")
		return "\033[1;35m";
	else if(level == "LOG_LEVEL_WARNING")
		return "\033[1;33m";
	else if(level == "LOG_LEVEL_FATAL")
		return "\033[1;31m";
	else if(level == "LOG_LEVEL_INFO")
		return "\033[1;32m";
	else
		return "\033[m";
}

#define LOG(level, message) Log(#level, message, __FILE__, __LINE__);
void Log(std::string level, std::string message, std::string file_name, int line)
{
	std::cout <<GetColor(level)<< "[" << level << "] " 
	<<LIGHT_CYAN<< " ["<<TimeStampToTime()<<"] " 
	<<GetColor(level)<< " [" << message << "] " 
	<<LIGHT_CYAN<< " [In file: " << file_name << "] " 
	<< " [Line: " << line << "]" <<NONE<< std::endl;
}


// #pragma once
// #include <iostream>
// #include <ctime>
// using std::cout;
// using std::endl;

// // int main(){
// // 	struct tm *mytm;
// // 	time_t  t = time(NULL);
// // 	mytm =gmtime(&t);

// // 	return 0;
// // }

// //======== 简易日志 ===========
// enum LOGLEVEL
// {
// 	LOG_LEVEL_ERROR=1,     // error
// 	LOG_LEVEL_WARNING,   // warning
// 	LOG_LEVEL_FATAL,     // fatal
// 	LOG_LEVEL_INFO,      // info	
// };
// void TimeStampToTime()
// {
//     char myStr[25] = { 0 };
// 	time_t cur_t = time(nullptr);
// 	struct tm *t = gmtime(&cur_t);
// 	t->tm_hour += 8;//转为北京时间记的要加8
// 	std::string myFormat = "%Y-%m-%d:%H:%M:%S";
// 	strftime(myStr, sizeof(myStr), myFormat.c_str(), t);
// 	for (int i = 0; myStr[i]; ++i) {
// 		cout << myStr[i];
// 	}
// }



