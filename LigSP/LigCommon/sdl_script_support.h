#ifndef __SQ_SCRIPT_SUPPORT_HEADER
#define __SQ_SCRIPT_SUPPORT_HEADER
#include <squirrel.h> 
#include <sqstdio.h> 
#include <sqstdaux.h> 
#include <sqstdio.h> 
#include <sqstdmath.h>
#include <sqstdsystem.h>
#include <sqstdblob.h>
#include <sqstdstring.h>
#include <set>
#include <deque>
#include <vector>

#ifdef WIN32
#include <hash_map>
#endif
#include <fstream>
#include <sdl_tcpstream.h>
#include <stdio.h>
#include <stdarg.h>

#include <lig_parser.h>
using namespace lig;
using namespace std;

extern bool GlobalExitFlag;  //对于Squirrel线程，要求终止运行的全局标识

void _push_object(HSQUIRRELVM v);
void _push_object(HSQUIRRELVM v,int val);
void _push_object(HSQUIRRELVM v,bool val);
void _push_object(HSQUIRRELVM v,short val);
void _push_object(HSQUIRRELVM v,const char* val);
void _push_object(HSQUIRRELVM v,char val[]);
void _push_object(HSQUIRRELVM v,float val);
void _push_object(HSQUIRRELVM v,double val);

//void _push_object(HSQUIRRELVM v,string& val);
void _push_object(HSQUIRRELVM v,string val);
void _push_object(HSQUIRRELVM v,time_t& val);
void _push_object(HSQUIRRELVM v,SQUserPointer pObj);
void _push_object(HSQUIRRELVM v,SQObject Obj);

struct SQInstancePointerType{}; //一个用于获取Squirreel实例的虚类型
struct SQFunctionType{};        //一个用于获取Squirreel函数指针的虚类型
typedef SQInstancePointerType* SQInstancePointer;



bool _get_object(HSQUIRRELVM v,int &val,int idx=-1);
bool _get_object(HSQUIRRELVM v,unsigned int &val,int idx=-1);
bool _get_object(HSQUIRRELVM v,unsigned long &val,int idx=-1);
bool _get_object(HSQUIRRELVM v,bool &val,int idx=-1);
bool _get_object(HSQUIRRELVM v,const char* &val,int idx=-1);
bool _get_object(HSQUIRRELVM v,float &val,int idx=-1);
bool _get_object(HSQUIRRELVM v,string &val,int idx=-1);
bool _get_object(HSQUIRRELVM v,SQUserPointer &pObj,int idx=-1);
bool _get_object(HSQUIRRELVM v,SQInstancePointer &pObj,int idx=-1);
bool _get_object(HSQUIRRELVM v,HSQOBJECT &Obj,int idx=-1);

bool register_func(HSQUIRRELVM v,const char* class_name,SQFUNCTION f,const char *fname,const char* para_mask=NULL,void* pObj=NULL);
bool register_func(HSQUIRRELVM v,SQFUNCTION f,const char *fname,const char* para_mask=NULL,void* pObj=NULL);
HSQUIRRELVM init_VirtualMachine();  //创建并初始化一个标准虚拟机
HSQUIRRELVM Create_Full_VM();       //创建一个附件shell功能的的虚拟机
bool exec_script(HSQUIRRELVM v,const char* script_filename); //编译执行一个脚本文件
void get_object_elements(HSQUIRRELVM v,const char* objname,sdl_line &outs);       //获得一个对象的成员
#ifdef WIN32

void debug_script(HSQUIRRELVM v,const char *fname,int port=1234);    // 编译及调试一个脚本文件
#endif
//------------------------------------------------------------------------------------
int  call_script(HSQUIRRELVM v,const char* function_name); //执行虚拟机内的脚本函数
bool import_script(HSQUIRRELVM v,const char* filename);   //引入一个虚拟机脚本
void inject_SqShell(HSQUIRRELVM v); //将SqShell的部分功能引入虚拟机
bool SqGetClass(HSQUIRRELVM v,const char* name); //得到一个类
bool SqCreateClass(HSQUIRRELVM v,const char* name); //创建一个类
bool SqCreateInstance(HSQUIRRELVM v,const char* classname,HSQOBJECT &hInstance); //创建一个对象的实例
void SqSetInstanceUP(HSQUIRRELVM v,HSQOBJECT &hInstance,void* p); //给一个实例设置指针
void* SqGetInstanceUP(HSQUIRRELVM v,const char* obj_name); //得到root table上一个实例的C指针
bool check_instancetype(HSQUIRRELVM v,const char* class_name,int idx=-1);
void get_public_global_time(sdl_time& st); //在C++中，获取全局时钟
#define DECLARE_SQ_INSTANCE(CLASSNAME) struct Sq_##CLASSNAME{\
CLASSNAME* pObject;\
bool flag_created;\
static SQInteger Sq2Constructor(HSQUIRRELVM v){\
		Sq_##CLASSNAME* handle=new Sq_##CLASSNAME;\
		handle->pObject=NULL;\
		handle->flag_created=false;\
		sq_setinstanceup(v,1,(SQUserPointer)(handle)); \
		sq_setreleasehook(v,1,_queryobj_releasehook); \
		return 0;}\
static SQInteger _queryobj_releasehook(SQUserPointer p, SQInteger size){\
		Sq_##CLASSNAME* handle=(Sq_##CLASSNAME*)p;\
		if(handle->flag_created) delete handle->pObject;\
		delete handle;\
		return 0;}\
static SQInteger Sq2UseCppObject(HSQUIRRELVM v){\
		Sq_##CLASSNAME* handle;\
		if(SQ_SUCCEEDED(sq_getinstanceup(v,1,(SQUserPointer*)(&handle),0)) && handle!=NULL)\
			sq_getuserpointer(v,-1,(SQUserPointer*)(&(handle->pObject)));\
		return 0;};\
static SQInteger Sq2GetObject(HSQUIRRELVM v){\
		Sq_##CLASSNAME* handle;\
		if(SQ_SUCCEEDED(sq_getinstanceup(v,1,(SQUserPointer*)(&handle),0)) && handle!=NULL && handle->flag_created==true)\
		{sq_pushuserpointer(v,(SQUserPointer)handle->pObject);return 1;}\
		return 0;};\
static SQInteger Sq2CreateObject(HSQUIRRELVM v){\
		Sq_##CLASSNAME* handle;\
		if(SQ_SUCCEEDED(sq_getinstanceup(v,1,(SQUserPointer*)(&handle),0)) && handle!=NULL && handle->flag_created==false)\
		{handle->pObject= new CLASSNAME;handle->flag_created=true;}\
		return 0;};\
static SQInteger Sq2Get(HSQUIRRELVM v){\
		const char* val;\
		Sq_##CLASSNAME* handle;\
		if(SQ_SUCCEEDED(sq_getinstanceup(v,1,(SQUserPointer*)(&handle),0)) && handle!=NULL && (SQ_SUCCEEDED(sq_getstring(v,-1,&val)))){\

#define DEF_VAR(VARNAME) if(strcmp(val,#VARNAME)==0) {_push_object(v,handle->pObject->VARNAME);return 1;}
#define DEF_PTR(VARNAME) if(strcmp(val,#VARNAME)==0) {_push_object(v,(void*)(handle->pObject->VARNAME));return 1;}
#define DEF_OBJ(VARNAME) if(strcmp(val,#VARNAME)==0) {_push_object(v,(void*)(&(handle->pObject->VARNAME)));return 1;}	  
#define DECLARE_FUNC(CLASSNAME)		}return 0;};

#define DEF_FUNC(CLASSNAME,FUNCNAME) static SQInteger Sq2##FUNCNAME(HSQUIRRELVM v){\
		Sq_##CLASSNAME* handle;\
		if(SQ_SUCCEEDED(sq_getinstanceup(v,1,(SQUserPointer*)(&handle),0)) && handle!=NULL){\
		_push_object(v,handle->pObject->FUNCNAME()); return 1;}return 0;}\

#define DEF_FUNC1(CLASSNAME,FUNCNAME,VAR1) static SQInteger Sq2##FUNCNAME(HSQUIRRELVM v){\
		Sq_##CLASSNAME* handle;\
		if(SQ_SUCCEEDED(sq_getinstanceup(v,1,(SQUserPointer*)(&handle),0)) && handle!=NULL){\
		VAR1 var1;\
		if(!SqGetVar(v,var1)) return 0;\
		_push_object(v,handle->pObject->FUNCNAME(var1)); return 1;}return 0;}

#define DEF_EXE1(CLASSNAME,FUNCNAME,VAR1) static SQInteger Sq2##FUNCNAME(HSQUIRRELVM v){\
		Sq_##CLASSNAME* handle;\
		if(SQ_SUCCEEDED(sq_getinstanceup(v,1,(SQUserPointer*)(&handle),0)) && handle!=NULL){\
		VAR1 var1;\
		if(!SqGetVar(v,var1)) return 0;\
		handle->pObject->FUNCNAME(var1); return 0;}return 0;}

#define DEF_FUNC2(CLASSNAME,FUNCNAME,VAR1,VAR2) static SQInteger Sq2##FUNCNAME(HSQUIRRELVM v){\
		Sq_##CLASSNAME* handle;\
		if(SQ_SUCCEEDED(sq_getinstanceup(v,1,(SQUserPointer*)(&handle),0)) && handle!=NULL){\
		VAR1 var1;\
		VAR2 var2;\
		if(!SqGetVar(v,var1,var2)) return 0;\
		_push_object(v,handle->pObject->FUNCNAME(var1,var2)); return 1;}return 0;}

#define DEF_EXE2(CLASSNAME,FUNCNAME,VAR1,VAR2) static SQInteger Sq2##FUNCNAME(HSQUIRRELVM v){\
		Sq_##CLASSNAME* handle;\
		if(SQ_SUCCEEDED(sq_getinstanceup(v,1,(SQUserPointer*)(&handle),0)) && handle!=NULL){\
		VAR1 var1;\
		VAR2 var2;\
		if(!SqGetVar(v,var1,var2)) return 0;\
		handle->pObject->FUNCNAME(var1,var2); return 0;}return 0;}\

#define END_FUNC(CLASSNAME) static void inject_class(HSQUIRRELVM v){\
		int top=sq_gettop(v);\
		sq_pushroottable(v); \
		sq_pushstring(v,#CLASSNAME,-1);\
		sq_newclass(v,SQFalse);\
		sq_createslot(v,-3);\
		sq_settop(v,top); \
		register_func(v,#CLASSNAME,Sq2Constructor,"constructor");\
		register_func(v,#CLASSNAME,Sq2UseCppObject,"use","xp");\
		register_func(v,#CLASSNAME,Sq2CreateObject,"create","x");\
		register_func(v,#CLASSNAME,Sq2GetObject,"this_ptr","x");\
		register_func(v,#CLASSNAME,Sq2Get,"_get","xs");

#define REG_FUNC(CLASSNAME,FUNCNAME,PARAMETER) register_func(v,#CLASSNAME,Sq2##FUNCNAME,#FUNCNAME,#PARAMETER);

#define END_DECLARE };};

#define END_SQ(CLASSNAME)		}return 0;};\
static void inject_class(HSQUIRRELVM v){\
		int top=sq_gettop(v);\
		sq_pushroottable(v); \
		sq_pushstring(v,#CLASSNAME,-1);\
		sq_newclass(v,SQFalse);\
		sq_createslot(v,-3);\
		sq_settop(v,top); \
		register_func(v,#CLASSNAME,Sq2Constructor,"constructor");\
		register_func(v,#CLASSNAME,Sq2UseCppObject,"use","xp");\
		register_func(v,#CLASSNAME,Sq2CreateObject,"create","x");\
		register_func(v,#CLASSNAME,Sq2Get,"_get","xs");}\
};

#define END_SQ_FUNC_Void(CLASSNAME,FUNCNAME)		}return 0;};\
static SQInteger Sq2##FUNCNAME(HSQUIRRELVM v){\
		Sq_##CLASSNAME* handle;\
		if(SQ_SUCCEEDED(sq_getinstanceup(v,1,(SQUserPointer*)(&handle),0)) && handle!=NULL){\
		_push_object(v,handle->pObject->FUNCNAME()); return 1;}}\
static void inject_class(HSQUIRRELVM v){\
		int top=sq_gettop(v);\
		sq_pushroottable(v); \
		sq_pushstring(v,#CLASSNAME,-1);\
		sq_newclass(v,SQFalse);\
		sq_createslot(v,-3);\
		sq_settop(v,top); \
		register_func(v,#CLASSNAME,Sq2##FUNCNAME,#FUNCNAME,"x");\
		register_func(v,#CLASSNAME,Sq2Constructor,"constructor");\
		register_func(v,#CLASSNAME,Sq2UseCppObject,"use","xp");\
		register_func(v,#CLASSNAME,Sq2CreateObject,"create","x");\
		register_func(v,#CLASSNAME,Sq2Get,"_get","xs");}\
};

#define END_SQ_FUNC_Int(CLASSNAME,FUNCNAME)		}return 0;};\
static SQInteger Sq2##FUNCNAME(HSQUIRRELVM v){\
		Sq_##CLASSNAME* handle;int para;\
		if(SQ_SUCCEEDED(sq_getinstanceup(v,1,(SQUserPointer*)(&handle),0)) && handle!=NULL){\
		if(SQ_SUCCEEDED(sq_getinteger(v,-1,&para))){\
		_push_object(v,handle->pObject->FUNCNAME(para)); return 1;}}return 0;}\
static void inject_class(HSQUIRRELVM v){\
		int top=sq_gettop(v);\
		sq_pushroottable(v); \
		sq_pushstring(v,#CLASSNAME,-1);\
		sq_newclass(v,SQFalse);\
		sq_createslot(v,-3);\
		sq_settop(v,top); \
		register_func(v,#CLASSNAME,Sq2##FUNCNAME,#FUNCNAME,"xi");\
		register_func(v,#CLASSNAME,Sq2Constructor,"constructor");\
		register_func(v,#CLASSNAME,Sq2UseCppObject,"use","xp");\
		register_func(v,#CLASSNAME,Sq2CreateObject,"create","x");\
		register_func(v,#CLASSNAME,Sq2Get,"_get","xs");}\
};

#define Inject_Class(VM,X) Sq_##X::inject_class(VM)



template<typename VAR1>
int call_script(HSQUIRRELVM v,const char* function_name,VAR1 var1)
{
	SQInteger oldtop=sq_gettop(v);
	sq_pushroottable(v);
	sq_pushstring(v,function_name,strlen(function_name));
	if(SQ_SUCCEEDED(sq_get(v,-2))) //找到需要页面的执行函数
	{
		SQObjectType tp;
		if((tp=sq_gettype(v,-1))==OT_CLOSURE)
		{
			sq_push(v,-2); //push the 'this' (in this case is the global table)
			_push_object(v,var1);
			if(SQ_FAILED(sq_call(v,2,SQTrue,SQTrue))) return 0;		 //false
			//执行squirrel函数
			SQInteger rt;
			if(SQ_SUCCEEDED(sq_getinteger(v,-1,&rt)))
			{
				sq_settop(v,oldtop);
				return rt;	
			}
		}
	}
	sq_settop(v,oldtop);
	return 0;
}

template<typename VAR1,typename VAR2>
int call_script(HSQUIRRELVM v,const char* function_name,VAR1 var1,VAR2 var2)
{
	SQInteger oldtop=sq_gettop(v);
	sq_pushroottable(v);
	sq_pushstring(v,function_name,strlen(function_name));
	if(SQ_SUCCEEDED(sq_get(v,-2))) //找到需要页面的执行函数
	{
		SQObjectType tp;
		if((tp=sq_gettype(v,-1))==OT_CLOSURE)
		{
			sq_push(v,-2); //push the 'this' (in this case is the global table)
			_push_object(v,var1);
			_push_object(v,var2);
			if(SQ_FAILED(sq_call(v,3,SQTrue,SQTrue)))
			{
				sq_settop(v,oldtop);
				return 0;
			}//执行squirrel函数
			SQInteger rt;
			if(SQ_SUCCEEDED(sq_getinteger(v,-1,&rt)))
			{
				sq_settop(v,oldtop);
				return rt;	
			}
		}
	}
	sq_settop(v,oldtop);
	return 0;
}

template<typename VAR1,typename VAR2,typename VAR3>
int call_script(HSQUIRRELVM v,const char* function_name,VAR1 var1,VAR2 var2,VAR3 var3)
{
	SQInteger oldtop=sq_gettop(v);
	sq_pushroottable(v);
	sq_pushstring(v,function_name,strlen(function_name));
	if(SQ_SUCCEEDED(sq_get(v,-2))) //找到需要页面的执行函数
	{
		SQObjectType tp;
		if((tp=sq_gettype(v,-1))==OT_CLOSURE)
		{
			sq_push(v,-2); //push the 'this' (in this case is the global table)
			_push_object(v,var1);
			_push_object(v,var2);
			_push_object(v,var3);
			if(SQ_FAILED(sq_call(v,4,SQTrue,SQTrue)))
			{
				sq_settop(v,oldtop);
				return 0;
			}//执行squirrel函数
			SQInteger rt;
			if(SQ_SUCCEEDED(sq_getinteger(v,-1,&rt)))
			{
				sq_settop(v,oldtop);
				return rt;	
			}
		}
	}
	sq_settop(v,oldtop);
	return 0;
}
template<typename VAR1,typename VAR2,typename VAR3,typename VAR4>
int call_script(HSQUIRRELVM v,const char* function_name,VAR1 var1,VAR2 var2,VAR3 var3,VAR4 var4)
{
	SQInteger oldtop=sq_gettop(v);
	sq_pushroottable(v);
	sq_pushstring(v,function_name,strlen(function_name));
	if(SQ_SUCCEEDED(sq_get(v,-2))) //找到需要页面的执行函数
	{
		SQObjectType tp;
		if((tp=sq_gettype(v,-1))==OT_CLOSURE)
		{
			sq_push(v,-2); //push the 'this' (in this case is the root table)
			_push_object(v,var1);
			_push_object(v,var2);
			_push_object(v,var3);
			_push_object(v,var4);
			if(SQ_FAILED(sq_call(v,5,SQTrue,SQTrue)))
			{
				sq_settop(v,oldtop);
				return 0;
			}//执行squirrel函数
			SQInteger rt;
			if(SQ_SUCCEEDED(sq_getinteger(v,-1,&rt)))
			{
				sq_settop(v,oldtop);
				return rt;	
			}
		}
	}
	sq_settop(v,oldtop);
	return 0;
}

struct SqFile
{
	char* linePointer;
	char* lineEnd;
	sdl_line linestr; //读入的行字符串
	sdl_line data;    //用于工作的数据缓冲区
	FILE*  file;

	bool readLn();
	int  convertHex(int start,int length);
	static SQInteger Sq2Constructor(HSQUIRRELVM v); //标准的Squirrel C++对象构造方式
	static SQInteger _queryobj_releasehook(SQUserPointer p, SQInteger size); //标准的Squirrel C++对象析构回调
	static SQInteger Sq2Open(HSQUIRRELVM v);
	static SQInteger Sq2WriteLn(HSQUIRRELVM v);
	static SQInteger Sq2WriteEndl(HSQUIRRELVM v);
	static SQInteger Sq2WriteHex(HSQUIRRELVM v);
	static SQInteger Sq2ReadHex2Buf(HSQUIRRELVM v);
	static SQInteger Sq2ReadHexLine(HSQUIRRELVM v);
	static SQInteger Sq2Write(HSQUIRRELVM v); 
	static SQInteger Sq2WritePtr(HSQUIRRELVM v); 
	static SQInteger Sq2GetDataPtr(HSQUIRRELVM v); 
	static SQInteger Sq2GetLinePtr(HSQUIRRELVM v); 
	static SQInteger Sq2GetLineString(HSQUIRRELVM v); 
	static SQInteger Sq2ClearPtr(HSQUIRRELVM v); 
	static SQInteger Sq2Eof(HSQUIRRELVM v);
	static SQInteger Sq2Good(HSQUIRRELVM v);
	static SQInteger Sq2Close(HSQUIRRELVM v);
	static SQInteger Sq2GetLn(HSQUIRRELVM v);
	static SQInteger Sq2ReadLn(HSQUIRRELVM v);  
	static SQInteger Sq2ReadAll(HSQUIRRELVM v);
	static SQInteger Sq2LineReadInteger(HSQUIRRELVM v); 
	static SQInteger Sq2LineReadFloat(HSQUIRRELVM v); 
	static SQInteger Sq2LineReadString(HSQUIRRELVM v); 
	static void inject_class(HSQUIRRELVM v);

	static bool check_type(HSQUIRRELVM v,bool initFlag=false) //检查实例是否由指定的类派生
	{
		static SQObject myclassObj;
		bool rt=false;
		if(initFlag) 
		{
			sq_getstackobj(v,-1,&myclassObj);
			return true;
		}
		sq_pushobject(v,myclassObj);
		sq_push(v,-2); //压入原来的instance
		if(SQ_SUCCEEDED(sq_instanceof(v))) rt=true;
		sq_pop(v,2);
		return rt;
	}
};

bool get_function(HSQUIRRELVM v,const char* function_name,HSQOBJECT &func);


template<typename RetType>	
bool call_function(HSQUIRRELVM v,RetType& ret,HSQOBJECT func)
{
	if(func._type!=OT_CLOSURE) return false; 
	int oldtop=sq_gettop(v);
	sq_pushobject(v,func);
	sq_pushroottable(v);
	if(SQ_FAILED(sq_call(v,1,SQTrue,SQTrue)))//执行squirrel函数
	{
		sq_settop(v,oldtop);
		sq_throwerror(v,"Call function error!");
		return false;
	}
	_get_object(v,ret);
	sq_settop(v,oldtop);
	return true;
};
template<typename RetType,typename VAR1>
bool call_function(HSQUIRRELVM v,RetType& ret,HSQOBJECT func,VAR1 var1)
{
	if(func._type!=OT_CLOSURE) return false; 
	int oldtop=sq_gettop(v);
	sq_pushobject(v,func);
	sq_pushroottable(v);
	_push_object(v,var1);
	if(SQ_FAILED(sq_call(v,2,SQTrue,SQTrue)))//执行squirrel函数
	{
		sq_settop(v,oldtop);
		sq_throwerror(v,"Call function error!");
		return false;
	}
	_get_object(v,ret);
	sq_settop(v,oldtop);
	return true;
};
template<typename RetType,typename VAR1,typename VAR2>
bool call_function(HSQUIRRELVM v,RetType& ret,HSQOBJECT func,VAR1 var1,VAR2 var2)
{
	if(func._type!=OT_CLOSURE) return false; 
	int oldtop=sq_gettop(v);
	sq_pushobject(v,func);
	sq_pushroottable(v);
	_push_object(v,var1);
	_push_object(v,var2);
	if(SQ_FAILED(sq_call(v,3,SQTrue,SQTrue)))//执行squirrel函数
	{
		sq_settop(v,oldtop);
		sq_throwerror(v,"Call function error!");
		return false;
	}
	_get_object(v,ret);
	sq_settop(v,oldtop);
	return true;
};
template<typename RetType,typename VAR1,typename VAR2,typename VAR3>
bool call_function(HSQUIRRELVM v,RetType& ret,HSQOBJECT func,VAR1 var1,VAR2 var2,VAR3 var3)
{
	if(func._type!=OT_CLOSURE) return false; 
	int oldtop=sq_gettop(v);
	sq_pushobject(v,func);
	sq_pushroottable(v);
	_push_object(v,var1);
	_push_object(v,var2);
	_push_object(v,var3);
	if(SQ_FAILED(sq_call(v,4,SQTrue,SQTrue)))//执行squirrel函数
	{
		sq_settop(v,oldtop);
		sq_throwerror(v,"Call function error!");
		return false;
	}
	_get_object(v,ret);
	sq_settop(v,oldtop);
	return true;
};


bool exec_function(HSQUIRRELVM v,HSQOBJECT func);

template<typename VAR1>
bool exec_function(HSQUIRRELVM v,HSQOBJECT func,VAR1 var1)
{
	if(func._type!=OT_CLOSURE) return false; 
	int oldtop=sq_gettop(v);
	sq_pushobject(v,func);
	sq_pushroottable(v);
	_push_object(v,var1);
	if(SQ_FAILED(sq_call(v,2,SQTrue,SQTrue)))//执行squirrel函数
	{
		sq_settop(v,oldtop);
		sq_throwerror(v,"Call function error!");
		return false;
	}
	sq_settop(v,oldtop);
	return true;
};
template<typename VAR1,typename VAR2>
bool exec_function(HSQUIRRELVM v,HSQOBJECT func,VAR1 var1,VAR2 var2)
{
	if(func._type!=OT_CLOSURE) return false; 
	int oldtop=sq_gettop(v);
	sq_pushobject(v,func);
	sq_pushroottable(v);
	_push_object(v,var1);
	_push_object(v,var2);
	if(SQ_FAILED(sq_call(v,3,SQTrue,SQTrue)))//执行squirrel函数
	{
		sq_settop(v,oldtop);
		sq_throwerror(v,"Call function error!");
		return false;
	}
	sq_settop(v,oldtop);
	return true;
};
template<typename VAR1,typename VAR2,typename VAR3>
bool exec_function(HSQUIRRELVM v,HSQOBJECT func,VAR1 var1,VAR2 var2,VAR3 var3)
{
	if(func._type!=OT_CLOSURE) return false; 
	int oldtop=sq_gettop(v);
	sq_pushobject(v,func);
	sq_pushroottable(v);
	_push_object(v,var1);
	_push_object(v,var2);
	_push_object(v,var3);
	if(SQ_FAILED(sq_call(v,4,SQTrue,SQTrue)))//执行squirrel函数
	{
		sq_settop(v,oldtop);
		sq_throwerror(v,"Call function error!");
		return false;
	}
	sq_settop(v,oldtop);
	return true;
};
template<typename VAR1,typename VAR2,typename VAR3,typename VAR4>
bool exec_function(HSQUIRRELVM v,HSQOBJECT func,VAR1 var1,VAR2 var2,VAR3 var3,VAR4 var4)
{
	if(func._type!=OT_CLOSURE) return false; 
	int oldtop=sq_gettop(v);
	sq_pushobject(v,func);
	sq_pushroottable(v);
	_push_object(v,var1);
	_push_object(v,var2);
	_push_object(v,var3);
	_push_object(v,var4);
	if(SQ_FAILED(sq_call(v,5,SQTrue,SQTrue)))//执行squirrel函数
	{
		sq_settop(v,oldtop);
		sq_throwerror(v,"Call function error!");
		return false;
	}
	sq_settop(v,oldtop);
	return true;
};
template<typename VAR1,typename VAR2,typename VAR3,typename VAR4,typename VAR5>
bool exec_function(HSQUIRRELVM v,HSQOBJECT func,VAR1 var1,VAR2 var2,VAR3 var3,VAR4 var4,VAR5 var5)
{
	if(func._type!=OT_CLOSURE) return false; 
	int oldtop=sq_gettop(v);
	sq_pushobject(v,func);
	sq_pushroottable(v);
	_push_object(v,var1);
	_push_object(v,var2);
	_push_object(v,var3);
	_push_object(v,var4);
	_push_object(v,var5);
	if(SQ_FAILED(sq_call(v,6,SQTrue,SQTrue)))//执行squirrel函数
	{
		sq_settop(v,oldtop);
		sq_throwerror(v,"Call function error!");
		return false;
	}
	sq_settop(v,oldtop);
	return true;
};
template<typename VAR1,typename VAR2,typename VAR3,typename VAR4,typename VAR5,typename VAR6>
bool exec_function(HSQUIRRELVM v,HSQOBJECT func,VAR1 var1,VAR2 var2,VAR3 var3,VAR4 var4,VAR5 var5,VAR6 var6)
{
	if(func._type!=OT_CLOSURE) return false; 
	int oldtop=sq_gettop(v);
	sq_pushobject(v,func);
	sq_pushroottable(v);
	_push_object(v,var1);
	_push_object(v,var2);
	_push_object(v,var3);
	_push_object(v,var4);
	_push_object(v,var5);
	_push_object(v,var6);
	if(SQ_FAILED(sq_call(v,7,SQTrue,SQTrue)))//执行squirrel函数
	{
		sq_settop(v,oldtop);
		sq_throwerror(v,"Call function error!");
		return false;
	}
	sq_settop(v,oldtop);
	return true;
};

bool SqGetVar(HSQUIRRELVM v);

template<class VAR1>
bool SqGetVar(HSQUIRRELVM v,VAR1 &var1)
{
	if(sq_gettop(v)!=2) return false;
	return _get_object(v,var1,2);
}

template<class VAR1,class VAR2>
bool SqGetVar(HSQUIRRELVM v,VAR1 &var1,VAR2 &var2)
{
	if(sq_gettop(v)!=3) return false;
	if(!_get_object(v,var2,3)) return false;
	if(_get_object(v,var1,2)) return true;
	return false;
}

template<class VAR1,class VAR2,class VAR3>
bool SqGetVar(HSQUIRRELVM v,VAR1 &var1,VAR2 &var2,VAR3 &var3)
{
	if(sq_gettop(v)!=4) return false;
	if(!_get_object(v,var3,4)) return false;
	if(!_get_object(v,var2,3)) return false;
	if(_get_object(v,var1,2)) return true;
	return false;
}
template<class VAR1,class VAR2,class VAR3,class VAR4>
bool SqGetVar(HSQUIRRELVM v,VAR1 &var1,VAR2 &var2,VAR3 &var3,VAR4 &var4)
{
	if(sq_gettop(v)!=5) return false;
	if(!_get_object(v,var4,5)) return false;
	if(!_get_object(v,var3,4)) return false;
	if(!_get_object(v,var2,3)) return false;
	if(_get_object(v,var1,2)) return true;
	return false;
}

template<class VAR1,class VAR2,class VAR3,class VAR4,class VAR5>
bool SqGetVar(HSQUIRRELVM v,VAR1 &var1,VAR2 &var2,VAR3 &var3,VAR4 &var4,VAR5 &var5)
{
	if(sq_gettop(v)!=6) return false;
	if(!_get_object(v,var5,6)) return false;
	if(!_get_object(v,var4,5)) return false;
	if(!_get_object(v,var3,4)) return false;
	if(!_get_object(v,var2,3)) return false;
	if(_get_object(v,var1,2)) return true;
	return false;
}
template<class VAR1,class VAR2,class VAR3,class VAR4,class VAR5,class VAR6>
bool SqGetVar(HSQUIRRELVM v,VAR1 &var1,VAR2 &var2,VAR3 &var3,VAR4 &var4,VAR5 &var5,VAR6 &var6)
{
	if(sq_gettop(v)!=7) return false;
	if(!_get_object(v,var6,7)) return false;
	if(!_get_object(v,var5,6)) return false;
	if(!_get_object(v,var4,5)) return false;
	if(!_get_object(v,var3,4)) return false;
	if(!_get_object(v,var2,3)) return false;
	if(_get_object(v,var1,2)) return true;
	return false;
}
template<class VAR1,class VAR2,class VAR3,class VAR4,class VAR5,class VAR6,class VAR7>
bool SqGetVar(HSQUIRRELVM v,VAR1 &var1,VAR2 &var2,VAR3 &var3,VAR4 &var4,VAR5 &var5,VAR6 &var6,VAR7 &var7)
{
	if(sq_gettop(v)!=8) return false;
	if(!_get_object(v,var7,8)) return false;
	if(!_get_object(v,var6,7)) return false;
	if(!_get_object(v,var5,6)) return false;
	if(!_get_object(v,var4,5)) return false;
	if(!_get_object(v,var3,4)) return false;
	if(!_get_object(v,var2,3)) return false;
	if(_get_object(v,var1,2)) return true;
	return false;
}

//extern void sq_setnativedebughook(HSQUIRRELVM v,SQDEBUGHOOK hook);
#include <vector>
#include <set>



struct NullInitFunc{void operator()(HSQUIRRELVM v){}};


extern void printfunc(HSQUIRRELVM v, const SQChar *s, ...);  //在标准输出打印调试信息
template<class InitFunc=NullInitFunc>
struct SqThread:public sdl_thread,sdl_tcp_simple_server // 在另一个线程中运行的虚拟机
{
	HSQUIRRELVM tvm;
	string script_name;
		
	SqThread()
	{
		script_name="";
		
	}

	SqThread(const char* scriptName)
	{
		script_name=scriptName;
		
	}

	void GetSqStack(HSQUIRRELVM v,const char* varname,ostream &out);

	void init(const char* scriptName,int port=1234,bool flagDebug=false)
	{
		script_name=scriptName;
		
	}

	static SqThread* getHandle(HSQUIRRELVM v,SqThread* wrap=NULL)
	{
		static vector<SqThread*> db;
		typename vector<SqThread*>::iterator it;
		for(it=db.begin();it!=db.end();++it)
			if((*it)->tvm==v) return *it;
		if(wrap!=NULL && wrap->tvm==v) db.push_back(wrap);
		return NULL;
	}

	static bool mystrcmp(sdl_line &line,const char* str)
	{
		int i=0;
		if(str==NULL) return false;
		while(*str!=0)
		{
			if(line[i]!=*str) return false;
			++i;++str;
		}
		return true;

	}
	
	virtual void process(){};
	
	virtual void begin()
	{
		try
		{
			InitFunc func; //准备注入不同的初始化程序
			tvm=Create_Full_VM(); //初始化虚拟机
	//#if (SQUIRREL_VERSION == _SC("Squirrel 2.2.3 stable"))
			//sq_setprintfunc(tvm,printfunc);
	//#endif
			sq_setprintfunc(tvm,printfunc,printfunc); 
			func(tvm); //执行初始化程序
			if(script_name=="") return;
			getHandle(tvm,this); //记录当前对象与虚拟机的对应关系
			exec_script(tvm,script_name.c_str());
		}
		catch(const char* error)
		{
			printfunc(tvm,"%s",error);
		}
	}
};

struct SqTime
{
	sdl_time* pTime;
	bool flag_native_time;
	

	static void inject_class(HSQUIRRELVM v);
	static SQInteger _queryobj_releasehook(SQUserPointer p, SQInteger size); //标准的Squirrel C++对象析构回调
	static SQInteger Sq2Constructor(HSQUIRRELVM v); //标准的Squirrel C++对象构造方式
	static SQInteger Sq2Init(HSQUIRRELVM v);
	static SQInteger Sq2GetTime(HSQUIRRELVM v);
	static SQInteger Sq2GetTimeSpan(HSQUIRRELVM v);
	static SQInteger Sq2CompareTime(HSQUIRRELVM v);
	static SQInteger Sq2AddTimeSpan(HSQUIRRELVM v);
	static SQInteger Sq2CopyTime(HSQUIRRELVM v);
	static SQInteger Sq2SetTime(HSQUIRRELVM v);
	static SQInteger Sq2MakeTime(HSQUIRRELVM v);
	static SQInteger Sq2Ctime(HSQUIRRELVM v);
	static SQInteger Sq2Msecs(HSQUIRRELVM v);
	static SQInteger Sq2TimeStr(HSQUIRRELVM v);
	static SQInteger Sq2Strftime(HSQUIRRELVM v);
};


extern void fwrite_json_object(sdl_line& io,HSQUIRRELVM mv);
extern void fwrite_json_array(sdl_line& io,HSQUIRRELVM mv);

struct SqDBM
{
	string dbName;
	char*  pData;	
	int    mDataSize;
	int    mBufferSize;
	void*  pGDBM;
	sdl_line mLine;
	

	static void inject_class(HSQUIRRELVM v)
	{
		int top=sq_gettop(v);
		const char* classname="GDBM"; 
		sq_pushroottable(v); //push the root table(were the globals of the script will be stored)
		sq_pushstring(v,classname,-1);
		sq_newclass(v,SQFalse);
		sq_createslot(v,-3);
		sq_settop(v,top); //restores the original stack size
				
		//'o' null, 'i' integer, 'f' float, 'n' integer or float, 's' string, 't' table, 'a' array, 'u' userdata, 
		//'c' closure and nativeclosure, 'g' generator, 'p' userpointer, 'v' thread, 'x' instance(class instance), 'y' class, 'b' bool. and '.' any type
		register_func(v,classname,Sq2Constructor,"constructor","x-s");//构造函数
		register_func(v,classname,Sq2JsonParse,"get_json","xs"); 
		register_func(v,classname,Sq2JsonParse,"json","xs"); 
		register_func(v,classname,Sq2Get,"_get","xs"); //
		register_func(v,classname,Sq2Set,"_set","xss"); //
		register_func(v,classname,Sq2Set,"_newslot","xss"); //
	
	}

	static SQInteger _queryobj_releasehook(SQUserPointer p, SQInteger size); //标准的Squirrel C++对象析构回调
	static SQInteger Sq2Constructor(HSQUIRRELVM v); //标准的Squirrel C++对象构造方式
	static SQInteger Sq2ReadJson(HSQUIRRELVM v);
	static SQInteger Sq2WriteJson(HSQUIRRELVM v);
	static SQInteger Sq2Get(HSQUIRRELVM v);
	static SQInteger Sq2Set(HSQUIRRELVM v);
	static SQInteger Sq2JsonParse(HSQUIRRELVM v);

	void cstr_write(const char* str);
	
};

struct SqScanWrap
{
	typedef lig::Scan TreeItem;
	
	static void inject_class(HSQUIRRELVM v)
	{
		int top=sq_gettop(v);
		const char* classname="AST"; 
		sq_pushroottable(v); //push the root table(were the globals of the script will be stored)
		sq_pushstring(v,classname,-1);
		sq_newclass(v,SQFalse);
		sq_createslot(v,-3);
		sq_settop(v,top); //restores the original stack size
				
		//'o' null, 'i' integer, 'f' float, 'n' integer or float, 's' string, 't' table, 'a' array, 'u' userdata, 
		//'c' closure and nativeclosure, 'g' generator, 'p' userpointer, 'v' thread, 'x' instance(class instance), 'y' class, 'b' bool. and '.' any type
		register_func(v,classname,Sq2Constructor,"constructor");//构造函数
		register_func(v,classname,Sq2Nexti,"_nexti","x."); //
		register_func(v,classname,Sq2Get,"_get","xs|i|p"); //
		register_func(v,classname,Sq2Cmp,"_cmp","xs|i|p"); //
		register_func(v,classname,Sq2Length,"len","x"); //
		register_func(v,classname,Sq2Convert2String,"tostring","x"); //
		
	}

	static SQInteger Sq2Length(HSQUIRRELVM v) ;
	static SQInteger Sq2Convert2String(HSQUIRRELVM v);
	static SQInteger _queryobj_releasehook(SQUserPointer p, SQInteger size); //标准的Squirrel C++对象析构回调
	static SQInteger Sq2Constructor(HSQUIRRELVM v); //标准的Squirrel C++对象构造方式
	static SQInteger Sq2Get(HSQUIRRELVM v);
	static SQInteger Sq2Cmp(HSQUIRRELVM v);
  	static SQInteger Sq2Nexti(HSQUIRRELVM v);
};

struct SqStdVectorWarp
{
	sdl_mutex   lock; 
	bool flag_public_vector; //表明是否是全局对象
	bool flag_native_vector; //表明是否是自身创建vector
	string name;             //公用表的名字

	typedef vector<SQObject> ObjectLst;
	ObjectLst* pVector;
	ObjectLst::iterator Iter;
	deque<ObjectLst*>* pHistoryDB; //历史数据
	int versionSize;  //历史数据的最大保留长度

	static SQInteger _queryobj_releasehook(SQUserPointer p, SQInteger size); //标准的Squirrel C++对象析构回调
	static void inject_class(HSQUIRRELVM v);
	static SQInteger Sq2Length(HSQUIRRELVM v);
	static SQInteger Sq2GetHistoryRec(HSQUIRRELVM v);
	static SQInteger Sq2GetHistorySize(HSQUIRRELVM v);
	static SQInteger Sq2Constructor(HSQUIRRELVM v); //标准的Squirrel C++对象构造方式
	static SQInteger Sq2Get(HSQUIRRELVM v); 
	static SQInteger Sq2Set(HSQUIRRELVM v);
	static SQInteger Sq2Nexti(HSQUIRRELVM v);
	static SQInteger Sq2CreateHistoryRec(HSQUIRRELVM v);
	static SQInteger Sq2PushHistoryRec(HSQUIRRELVM v);
	
};
#endif
