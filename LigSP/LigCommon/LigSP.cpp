// LigSP.cpp : Defines the entry point for the console application.

#include "lig_httpd.h"
#include "sdl_db_support.h"
extern const char* get_init_script(const char* init_fn,bool encodeGB2312=false);


void printfunc(HSQUIRRELVM v, const SQChar *s, ...)  //在标准输出打印调试信息
{ 
	va_list arglist; 
	va_start(arglist, s); 
#ifdef WIN32
	vprintf_s(s,arglist); 
#else
	vprintf(s,arglist); 
#endif
	va_end(arglist); 
};



int main(int argc, char* argv[])
{
	
	HSQUIRRELVM vm=Create_Full_VM(); //初始化虚拟机
	http_sq::register_library(vm);  //加入HTTPD的支持
	lig_httpd::register_library(vm);
	sqlite_query::register_library(vm);
	http_conn::register_library(vm);
	if(SqCreateClass(vm,"http_header")) //创建一个类，并定义类方法
	{
		register_func(vm,"http_header",Sq2_http_header_addvar,"add_var","xss");
	}
	init_parser(); //初始化分析器
	
	if(argc==1) //没有参数
	{
		if(!import_script(vm,"init.nut")) //执行预订的初始化脚本
		{
			cout<<"init.nut file error, exit..."<<endl;
			exit(-1);
		}
	}else if(argc==2) //指定了启动脚本
	{
		if(!import_script(vm,argv[1])) //执行用户要求的初始化脚本
		{
			cout<<argv[1]<<" file error, exit..."<<endl;
			exit(-1);
		}
	}else
	{
		cout<<"usage: server [initfilename]"<<endl;
	}

	return 0;
}

