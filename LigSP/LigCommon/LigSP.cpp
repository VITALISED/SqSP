// LigSP.cpp : Defines the entry point for the console application.

#include "lig_httpd.h"
#include "sdl_db_support.h"
extern const char* get_init_script(const char* init_fn,bool encodeGB2312=false);


void printfunc(HSQUIRRELVM v, const SQChar *s, ...)  //�ڱ�׼�����ӡ������Ϣ
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
	
	HSQUIRRELVM vm=Create_Full_VM(); //��ʼ�������
	http_sq::register_library(vm);  //����HTTPD��֧��
	lig_httpd::register_library(vm);
	sqlite_query::register_library(vm);
	http_conn::register_library(vm);
	if(SqCreateClass(vm,"http_header")) //����һ���࣬�������෽��
	{
		register_func(vm,"http_header",Sq2_http_header_addvar,"add_var","xss");
	}
	init_parser(); //��ʼ��������
	
	if(argc==1) //û�в���
	{
		if(!import_script(vm,"init.nut")) //ִ��Ԥ���ĳ�ʼ���ű�
		{
			cout<<"init.nut file error, exit..."<<endl;
			exit(-1);
		}
	}else if(argc==2) //ָ���������ű�
	{
		if(!import_script(vm,argv[1])) //ִ���û�Ҫ��ĳ�ʼ���ű�
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

