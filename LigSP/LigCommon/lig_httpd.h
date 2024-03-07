#ifndef _LIG_HTTPD__
#define _LIG_HTTPD__
#define _CRT_SECURE_NO_WARNINGS
#define FD_SETSIZE 512 //扩展FDSET的容量，保证监听正常完成

#ifdef _LIG_DEBUG
#ifndef DOUT
//#define DOUT if(false)cout
#define DOUT cout
#endif
#endif

#include <sdl_tcpstream.h>
#include <sdl_signal.h>
#include <sdl_memory.h>
#include <sdl_state.h>
#include <lig_parser.h>
#include <sdl_script_support.h> 
#include <cpu.h>


#include <stdio.h>
#include <stdlib.h>
#include <vector>
#include <sys/types.h>
#include <sys/stat.h>
#include <stdarg.h>
#include <typeinfo>



#include <fastcgi.h>
#if defined(_MSC_VER) && defined(_DEBUG)
#include <crtdbg.h>
#include <conio.h>
#endif

using namespace std;


#ifdef SQUNICODE 
#define scvprintf vwprintf 
#else 
#define scvprintf vprintf 
#endif 

#ifdef WIN32
#define snprintf _snprintf
#endif
#define IO_SIZE 32*1024
#define MAX_PAGE_SIZE 32*1024*1024
#define MAX_VARS 128
#define MAX_KEEP_ALIVE 256
#define EXPIRE_TIME 32  //失效时间
//----------------------
#ifdef _LIG_DEBUG
#ifndef DOUT
#define DOUT cout
#endif
#endif
//----------------------
#define ID_Connection 0
#define ID_User_Agent 1
#define ID_Content_Length 2
#define ID_Content_Type 3
#define ID_If_Modified_Since 4 

extern char* pTagHead[];
extern int TageHeadLength;
struct file_types
{
	const char *ext;
	const char *type;
};
extern file_types types[];

struct mime_data_pair  //使用mime方式上传文件使用的数据结构
{
	mime_data_pair *parent; //用于复杂MIME的指针，目前版本并不使用
	string name;
	string value;
	string filename;
	string storename;
	const char *p_data;  //指向具体文件内容的数据起始位置
	int data_length;     //具体文件的长度
};

using namespace lig;

struct http_sq;
struct http_poll;
extern void init_parser();
extern bool run_script(HSQUIRRELVM v,string &script); 
extern HSQUIRRELVM init_VirtualMachine();
extern const char* p_root_dir;
extern int gb2utf8(const char *gbstr,char *out,int buf_size);
extern bool register_func(HSQUIRRELVM v,const char* class_name,SQFUNCTION f,const char *fname,const char* para_mask,void* pObj);
extern bool register_global_func(HSQUIRRELVM v,SQFUNCTION f,const char *fname,const char* para_mask);
extern void KMP_init(const char* pat, int* next);
extern int KMP_find(const char *s,int l1,const char *t,int l2,int *next);
//extern bool macro_state(string &buf);

struct page_buf
{
	char buf_block[IO_SIZE];
	char *p_buf;   //这个指针在开始的时候指向页面缓冲区，所以，页面尺寸不大时，不需要单独申请内存
	int buf_size;  //目前缓冲区的尺寸
	int buf_curr;  //页面目前已有内容的长度
	int buf_start; //页面开始的位置
	size_t resize_times; //扩充暂存区的次数
	char *p_segment;  //当前尚未输出的内容

	page_buf()
	{
		resize_times=0;
		buf_curr=0;
		buf_start=0;
		p_buf=buf_block;
		buf_size=IO_SIZE;
	}

	void init()
	{
		resize_times=0;
		buf_curr=0;
		buf_start=0;
		p_buf=buf_block;
		buf_size=IO_SIZE;
	}

	~page_buf()
	{
		if(resize_times>0 && p_buf!=NULL) 
			delete[] p_buf;
	}

	bool read_socket(int socket_id)
	{
		int nRead=::recv(socket_id,p_buf+buf_curr,buf_size-buf_curr,0);	
		if(nRead<=0) 
			return false;
		buf_curr+=nRead;
		if(buf_curr==buf_size) 
			return resize();
		return true;
	}

	int write_socket(int socket_id)
	{
		int TransSize=(buf_curr-buf_start)>IO_SIZE?IO_SIZE:(buf_curr-buf_start);
		int nWrite=::send(socket_id,p_buf+buf_start,TransSize,0); //每次传输的尺寸固定
		//DOUT.write(p_buf+buf_start,TransSize); 
		if(nWrite<0) return -1;//出现传输错误，需要终止这个连接
		buf_start+=nWrite;
		if(buf_start<buf_curr)  return 0;  //未写完成
		return 1; //写完成

	}

	inline bool resize(int min_bytes=-1)
	{
		char *p_old_buf=p_buf;
		int old_size=buf_size;

		if(min_bytes==-1)
		{
			min_bytes=(2<<resize_times)*IO_SIZE; //按照指数增加方式申请内存
			if(resize_times==0) buf_size=IO_SIZE;
			if(buf_size+min_bytes>MAX_PAGE_SIZE) return false; //越界
		}
		
		p_buf=new char[buf_size+min_bytes]; //申请到一个大于需求的空间
		if(p_buf==NULL) throw "Memory Alloc Error..page_buf::resize()";
		memcpy(p_buf,p_old_buf,old_size);
		buf_size+=min_bytes; //得到准确的新尺寸
		
		if(resize_times>0) 
			delete[] p_old_buf; //如果不是初次申请，需要删除上次申请的空间

		resize_times++;
		p_segment=p_buf; //指向新内存中的原来位置
		return true;
	};

	void printf(const char *fmt, ...)
	{
		va_list	ap;
		int nWrite;
		va_start(ap, fmt);
		nWrite=vsnprintf(p_buf+buf_curr,buf_size-buf_curr,fmt,ap);
		va_end(ap);
		while(nWrite<0 && resize())  //内存空间不够，重新分配内存，并重写
		{
			va_start(ap, fmt);
			nWrite=vsnprintf(p_buf+buf_curr,buf_size-buf_curr,fmt,ap);
			va_end(ap);
		}
		buf_curr+=nWrite;
	};

	inline void clear()
	{
		buf_curr=0;
		buf_start=0;
	};

#define directcstrwrite(x) p_buf[buf_curr++]='\\';p_buf[buf_curr++]=x;++str

	void write_cstr(const char* str)
	{

		while(*str!=0)
		{
			if(buf_size-buf_curr<4){if(!resize()) return;}
			if(*str=='\n')
			{directcstrwrite('n');continue;}
			else if(*str=='\"')
			{directcstrwrite('\"');continue;}
			else if(*str=='\\')
			{directcstrwrite('\\');continue;}
			else if(*str=='\r')
			{directcstrwrite('r');continue;}
			else if(*str=='\t')
			{directcstrwrite('t');continue;}
			else if(*str=='\v')
			{directcstrwrite('v');continue;}
			else if(*str=='\b')
			{directcstrwrite('b');continue;}
			else if(*str=='\f')
			{directcstrwrite('f');continue;}
			/*
			else if(*str=='\/')
			{directcstrwrite('/');continue;}
			*/
			else
				p_buf[buf_curr++]=*str;
			++str;
		}

	}

	void write_cstr(const char* str,int length)
	{
		int i=0;
		while(i<length)
		{
			if(buf_size-buf_curr<4){if(!resize()) return;}
			if(*str=='\n')
			{directcstrwrite('n');++i;continue;}
			else if(*str=='\"')
			{directcstrwrite('\"');++i;continue;}
			else if(*str=='\\')
			{directcstrwrite('\\');++i;continue;}
			else if(*str=='\r')
			{directcstrwrite('r');++i;continue;}
			else if(*str=='\t')
			{directcstrwrite('t');++i;continue;}
			else if(*str=='\v')
			{directcstrwrite('v');++i;continue;}
			else if(*str=='\b')
			{directcstrwrite('b');++i;continue;}
			else if(*str=='\f')
			{directcstrwrite('f');++i;continue;}
			/*
			else if(*str=='\/')
			{directcstrwrite('/');++i;continue;}
			*/
			else
				p_buf[buf_curr++]=*str;
			++str;++i;
		}

	}

	inline void direct_write(string &data) //直接写入，不进行编码
	{
		int length=data.length();
		if((buf_size-buf_curr)<length) {if(!resize()) return;}
		memcpy(p_buf+buf_curr,data.data(),length); 
		buf_curr+=length;
	}

	inline void direct_write(const char* data,int length) //直接写入，不进行编码
	{
		if((buf_size-buf_curr)<length)  {if(!resize()) return;}
		memcpy(p_buf+buf_curr,data,length); 
		buf_curr+=length;
	}

	inline void direct_write(const char* data) //直接写入，不进行编码
	{
		if(data==NULL || *data==0) return;
		while(*data!=0)
		{
			*(p_buf+buf_curr)=*data;
			++data;
			++buf_curr;
			if(buf_curr>=buf_size) {if(!resize()) return;};
		}
	}

	inline void direct_write(int v)
	{
		if((buf_size-buf_curr)<32) {if(!resize()) return;};
		//#ifdef WIN32
		//buf_curr+=(int)(_itoa(v,p_buf+buf_curr,10)-p_buf);
		//#else
		buf_curr+=snprintf(p_buf+buf_curr,10,"%d",v);
		//#endif
	};

	inline void direct_write(char v)
	{
		if((buf_size-buf_curr)<32) {if(!resize()) return;};
		p_buf[buf_curr]=v;
		buf_curr++;
	};


	inline void direct_write(float v)
	{
		if((buf_size-buf_curr)<32) {if(!resize()) return;};
		int n=snprintf(p_buf+buf_curr,32,"%f",v);
		buf_curr+=n;
	};

	inline void direct_write(short v)
	{
		if((buf_size-buf_curr)<32) {if(!resize()) return;};
		int n=snprintf(p_buf+buf_curr,32,"%d",v);
		buf_curr+=n;
	};

	inline void direct_write(double v)
	{
		if((buf_size-buf_curr)<32) {if(!resize()) return;};
		int n=snprintf(p_buf+buf_curr,32,"%f",v);
		buf_curr+=n;
	};

	inline void replace(size_t loc,const char *data)
	{
		size_t length=strlen(data);
		if((buf_size-loc)<length) {if(!resize()) return;};
		memcpy(p_buf+loc,data,length); 
		buf_curr+=length;
	};

	inline void pop(){if(buf_curr>0) buf_curr--;}

	inline void pop(int n){if(buf_curr>=n) buf_curr-=n;}

	inline char back(){if(buf_curr>0) return p_buf[buf_curr-1];else return 0;}

	//------------------------------------------------------------

};


struct VARS
{
	char* name;
	int name_length;
	char* value;
	int value_length;
};


struct http_header //对HTTP请求头的处理对象
{

	vector<VARS> varList;   //请求的环境变量，可能来源于自己的解析，也可能来源与FCGI
	char* pUri;
	char* pSession;

	http_header(){pSession=NULL;}

	~http_header(){};

	void urldecode(char *from, char *to)
	{
		int	i, a, b;
		bool    flag_changed=false;
		if(from==NULL) return;
#define	HEXTOI(x)  (isdigit(x) ? x - '0' : x - 'W')
		for (i = 0; *from != '\0'; i++, from++) 
		{
			if (from[0] == '%' &&
				isxdigit((unsigned char) from[1]) &&
				isxdigit((unsigned char) from[2])) 
			{
				a = tolower(from[1]);
				b = tolower(from[2]);
				to[i] = (HEXTOI(a) << 4) | HEXTOI(b);
				from += 2;
				flag_changed=true;
			} 
			else if (from[0] == '+') 
			{
				to[i] = ' ';
			}
			else 
			{
				to[i] = from[0];
			}
		}
		if(flag_changed) to[i] = '\0';
	}

	void add_vars(const char *key,const char* value) // 附加的GET解码，用于url重写
	{
		VARS tmp;
		tmp.name=(char*)key;
		tmp.value=(char*)value;
		if(strcmp(tmp.name,"_csp_key")==0) //如果参数是_csp_key,则保留为session字符串
			pSession=tmp.value; 
		else
		{
			tmp.name_length=strlen(tmp.name); 
			tmp.value_length=strlen(tmp.value);
			varList.push_back(tmp); 
		}
	}

	void decode_url(char *pBuf)
	{
		if(pBuf==NULL){pUri="";return;}
		while(*pBuf=='/' || *pBuf=='\\') ++pBuf;
		pUri=pBuf;
		while(*pBuf!=0 && *pBuf!='?')
		{
			if(*pBuf=='\\') *pBuf='/';
			if(*pBuf=='.')	while(*(pBuf++)!=0 && (*pBuf=='.' || *pBuf=='/' || *pBuf=='\\')) *pBuf=' ';
			pBuf++;
		}
		char* pUriEnd=pBuf;
		if(*pBuf=='?')
		{
			*pBuf=0;
			++pBuf;
		}else return;

		while(pUri!=NULL && *pUri!=0 && (*pUri==' ' || *pUri=='/')) ++pUri; //去除左边的无效空格
		while(pUriEnd>pUri && *pUriEnd==0) --pUriEnd;
		while(pUriEnd>pUri && *pUriEnd==' '){*pUriEnd=0;--pUriEnd;} //去除右边无效空格

		urldecode(pUri,pUri); //puri没有包含get的数据选项

		VARS tmp;tmp.name=pBuf;tmp.value="";
		while(*pBuf!=0)
		{
			if(*pBuf=='=')
			{
				*pBuf=0;
				urldecode(tmp.name,tmp.name); 
				++pBuf;
				tmp.value=pBuf;
			}
			if(*pBuf=='&')
			{
				*pBuf=0;
				urldecode(tmp.value,tmp.value);
				tmp.name_length=strlen(tmp.name); 
				tmp.value_length=strlen(tmp.value);
				if(strcmp(tmp.name,"_csp_key")==0) //如果最后一个参数是_csp_key,则保留为session字符串
					pSession=tmp.value; 
				else
					varList.push_back(tmp); 
				++pBuf;
				tmp.name=pBuf;
			}
			++pBuf;
		}
		urldecode(tmp.value,tmp.value);
		if(strcmp(tmp.name,"_csp_key")==0) //如果最后一个参数是_csp_key,则保留为session字符串
			pSession=tmp.value; 
		else
		{
			tmp.name_length=strlen(tmp.name); 
			tmp.value_length=strlen(tmp.value);
			varList.push_back(tmp); 
		}
		
	}

	void decode_post(char *pBuf,int length)
	{

		char *pEnd=pBuf+length;
		VARS tmp;tmp.name=pBuf; tmp.value="";
		while(*pBuf!=0 && pBuf<pEnd)
		{
			if(*pBuf=='=')
			{
				*pBuf=0;
				urldecode(tmp.name,tmp.name); 
				++pBuf;
				tmp.value=pBuf;
			}
			if(*pBuf=='&')
			{
				*pBuf=0;
				urldecode(tmp.value,tmp.value);
				tmp.name_length=strlen(tmp.name); 
				tmp.value_length=strlen(tmp.value);
				varList.push_back(tmp); 
				++pBuf;
				tmp.name=pBuf;
			}

			++pBuf;
		}
		urldecode(tmp.value,tmp.value);
		if(strcmp(tmp.name,"_csp_key")==0)
			pSession=tmp.value; 
		else
		{
			tmp.name_length=strlen(tmp.name); 
			tmp.value_length=strlen(tmp.value);
			varList.push_back(tmp); 
		}
	}
};


class lig_httpd;

enum http_link_stat
{
	read_ready    =0,      //读准备好，允许读
	write_ready   =1,      //写准备好
	read_finished =2,      //读结束
	conn_finished =3,      //连接全部处理完毕后关闭连接
	write_finished=4      //写完毕，允许进行重新写
};

enum fcgi_link_stat
{
	fcgi_ready         =0,
	fgci_begin_request =1,    //得到fcgi请求
	fcgi_read_client   =2,    //正在读fcgi
	fcgi_read_finished =3,    //读fcgi
	fcgi_server_processing =4,//正在处理请求
	fcgi_server_processed  =5,//处理完毕
	fcgi_write_clinet      =6,//正在写出数据
	fcgi_write_finished    =7,//数据完全写出
	fcgi_close_connect     =8 //需要关闭连接
};

struct http_conn
{
	typedef bool (http_conn::*funcType)();
	//----------------------------------------------
	int socket_id;  //在WebServer中是socket,在FCGI中是FCGI的请求号
	struct sockaddr_in addr; //IP address struct
	//-----------------------------------------------
	page_buf inBuf;
	page_buf outBuf;
	//-----------------------------------------------
	time_t time_stamp;
	//-----------------------------------------------
	FILE* mFiles;   //需要写出的文件
	int FileLength; //文件的长度
	int curpos;     //文件当前的读取位置
	struct stat file_state; //文件的状态
	//-------------------------------------------------
	char* pHeader[16];
	//-------------------------------------------------
	
	http_link_stat link_stat;

	bool flag_header_ok;    //请求头分析成功
	bool flag_json_request; //表示是JsonRequest
	bool flag_file_upload;  //表示请求为文件上传
	bool flag_uri_translate; //要求脚本进行URI转换
	//---------------------------------------------------
	funcType action;        //需要在服务器端执行的操作
	http_header header;   //分析器,以及环境变量的存储位置
	
	lig_httpd *pServer;     //服务器
	//---------------------------------------------------
	int post_data_offset;            //内部使用的变量,用于记录POST数据的起点
	int post_length;        //客户要求的post长度
	int header_length;      //请求头的长度
	//---------------------------------------------------
	int fcgi_content_type_length; //fcgi上传的fcgi_content_type_length长度
	fcgi_link_stat flag_fcgi;//fcgi的状态
	bool flag_fcgi_close_state; //webserver要求关闭连接
	//----------------------------------------------------
	void* pUrlMap;          //URL MAP
	//----------------------------------------------------
	list<mime_data_pair> mlist; //文件上传使用的列表,注意mlist保存的数据都是在page_buf内的数据，mlist自身不会申请内存，所以不需要对mlist析构
	//----------------------------------------------------
	~http_conn()
	{
		//destructor(&mlist);
		//destructor(&header);
	}

	void init(lig_httpd* ps);
	inline void printf(char* fmt,...);
	inline bool ncasecmp(const char* str1,const char* str2);
	int montoi(const char *s); //来自shttpd的函数
	time_t datetosec(const char *s); //来自shttpd的函数
	bool create_getfile_header();
	bool action_UNKNOWN();
	bool action_GET();
	bool action_POST();
	bool action_HEAD();
	void send_http_msg(int status,const char *descr,const char *headers,const char *fmt, ...);	
	bool write_file();
	bool parse_header();
	bool read_client();
	bool write_client();
//-------------CSP support----------------------------------------
	const char*  query_val(const char* name);
	void direct_write(int i){outBuf.direct_write(i);}
	void direct_write(const char* str,int length=-1)
	{
		if(length>0)
			outBuf.direct_write(str,length);
		else
			outBuf.direct_write(str);
	}
//--------FCGI Version 1 support-----------------------------------
	void read_fcgi_server();
	int  write_fcgi_server();
	bool parse_fastcgi();
	void get_fastcgi_params(); //得到FCGI的环境变量值
	

	int para_length;

//-------------------------------------------------------------
	
	static void register_library(HSQUIRRELVM v);
	static SQInteger Sq2Write(HSQUIRRELVM v);
	static SQInteger Sq2Get(HSQUIRRELVM v);
	static SQInteger Sq2Constructor(HSQUIRRELVM v);
	static SQInteger Sq2WriteFile(HSQUIRRELVM v);
	static SQInteger Sq2WriteUpload2File(HSQUIRRELVM v);
	static bool push_obj(HSQUIRRELVM v,http_conn *pObj);
};

/*
class sq_state
{
public:

	static void register_library(HSQUIRRELVM v)
	{
		SQInteger top = sq_gettop(v); //saves the stack size before the call
		sq_pushroottable(v); //push the root table(were the globals of the script will be stored)
		sq_pushstring(v,"sq_state",-1);
		sq_newclass(v,SQFalse);
		sq_createslot(v,-3);
		sq_settop(v,top); //restores the original stack size

		//'o' null, 'i' integer, 'f' float, 'n' integer or float, 's' string, 't' table, 'a' array, 'u' userdata, 
		//'c' closure and nativeclosure, 'g' generator, 'p' userpointer, 'v' thread, 'x' instance(class instance), 'y' class, 'b' bool. and '.' any type
		register_func(v,"sq_state",Sq2Constructor,"constructor","x",NULL);//构造函数
		//register_func(v,"sq_state",Sq2Trans,"trans","xia",NULL);//激励转移函数
		register_func(v,"sq_state",SQStateStrGet,"_get","xi",NULL);
	};

	static SQInteger _queryobj_releasehook(SQUserPointer p, SQInteger size) //标准的Squirrel C++对象析构回调
	{
		sq_state *self = ((sq_state*)p);
		delete(self);
		return 1;
	}

	static SQInteger SQStateStrGet(HSQUIRRELVM v) //标准的Squirrel C++对象构造方式
	{
		sq_state *pSelf = NULL;
		sq_getinstanceup(v,1,(SQUserPointer *)&pSelf,0); //标准的获取C++对象的方式
		if(pSelf==NULL) return 0; 
		int hnd;
		sq_getinteger(v,-1,&hnd);
		const char* str=_get_string(hnd);
		if(str!=NULL) sq_pushstring(v,str,strlen(str));
		else sq_pushnull(v);
		return 1;
	};
	

	static SQInteger Sq2Constructor(HSQUIRRELVM v) //标准的Squirrel C++对象构造方式
	{
		sq_state* pobj=new sq_state; //从堆栈中获取C++对象的指针
		if(!pobj) return sq_throwerror(v,"Create instance failed");
		SQUserPointer theHandle=(SQUserPointer) pobj;
		sq_setinstanceup(v,1,theHandle); //为本实例保存C++对象指针,本实例在堆栈的底部，所以idx 使用1
		sq_setreleasehook(v,1,_queryobj_releasehook);
		return 0;
	}
	
};
*/
struct http_sq //用于Squirrel的输入输出对象
{
	http_conn* pHttp;
	//-----------------------

	//-----------------------
	Scan* pScan; //指向当前顺序输出的位置，这个值在Squirrel中变化
	Scan* pSegment;
	//-------------
	Scan* pStart; //这是页面输出的起点，这个值不在Squirrel中变化
	Scan* pRoot;  //这是模板输出的起点
	const char* function_name; 
	const char* pLastStr; //指向最后输出的位置
	bool flag_switch; //控制innerWrite是否执行输出的开关

	vector<Scan*> thePosIdx; //记录各输出区的索引值，在C++初始化对象的时候填入，这个不在Squirrel中变化
	//-------------
	string javascript_code; //分析产生的js代码,用于提交数据时的页面数据提取

	void reset(){pScan=pStart;flag_switch=true;};

	static SQInteger Sq2Constructor(HSQUIRRELVM v); //标准的Squirrel C++对象构造方式
	static SQInteger Sq2GetVars(HSQUIRRELVM v);
	static SQInteger Sq2Get(HSQUIRRELVM v);
	static SQInteger Sq2Write(HSQUIRRELVM v);
	static SQInteger Sq2WriteTemplate(HSQUIRRELVM v);
	static SQInteger Sq2WriteHandle(HSQUIRRELVM v);
	static SQInteger Sq2IdxWrite(HSQUIRRELVM v);
	static SQInteger Sq2InnerClose(HSQUIRRELVM v); //打开输出
	static SQInteger Sq2InnerOpen(HSQUIRRELVM v); //关闭输出
	static SQInteger Sq2SegmentWrite(HSQUIRRELVM v);
	static SQInteger Sq2LastWrite(HSQUIRRELVM v); //写最后的字符串
	static SQInteger Sq2GetTemplate(HSQUIRRELVM v); //获取一个模板
	static SQInteger Sq2AjaxValue(HSQUIRRELVM v);
	static SQInteger Sq2AjaxAlert(HSQUIRRELVM v);
	static SQInteger Sq2AjaxURL(HSQUIRRELVM v);
	static SQInteger Sq2AjaxinnerHTML(HSQUIRRELVM v);
	static SQInteger Sq2AjaxnewList(HSQUIRRELVM v);
	static SQInteger Sq2AjaxInnerTemplate(HSQUIRRELVM v);
	static SQInteger Sq2DeleteLastComma(HSQUIRRELVM v);
	static SQInteger Sq2CreateSession(HSQUIRRELVM v);
	static SQInteger Sq2GetSession(HSQUIRRELVM v);
	static SQInteger Sq2Redirect(HSQUIRRELVM v); //页面重定向
	static SQInteger Sq2AjaxEnable(HSQUIRRELVM v); //将页面元素启用
	static SQInteger Sq2AjaxDisable(HSQUIRRELVM v); //将页面元素禁用
	static SQInteger Sq2GetHTTP(HSQUIRRELVM v); //获取上传数据
	static SQInteger Sq2HTTPWriteUpload2File(HSQUIRRELVM v);
	
	static bool push_obj(HSQUIRRELVM v,http_sq *pObj); //将本对象压入squirrel堆栈，作为对象传递给脚本解释器
	static void register_library(HSQUIRRELVM v);
	
};


typedef MemAlloctor<http_conn,1024,4> ConnList;
typedef bool(http_poll::*CallBackFuncType)(http_conn*,const char*);
typedef bool(*C_CallbackType)(http_conn*,const char*);
struct url_map
{
	char url[256];
	char squirrel_function_callback_name[256];
	bool flag_direct_sq_function; //表示为直接请求，squirrel_function_callback_name将表示squirrel虚拟机root table下，被请求的函数名称。
	CallBackFuncType pCallback;
	CallBackFuncType pJosnRequest;
	C_CallbackType pCCallback;
};

struct less_url_map
{
	inline bool operator()(const url_map& u1,const url_map& u2)const
	{
		return strcmp(u1.url,u2.url)<0;
	}
};

struct http_poll:public sdl_thread   //HTTP请求的执行体
{
	fd_set rd_set;
	fd_set wr_set;        //socket层的读写集合
	int max_fd; 
	int sock_id;
	int port;            //使用的端口号,这是执行线程与主线程通信使用的socket，对于Linux/UNIX，将使用socketpair,端口在这里无意义
	//-----------------------------------
	int milliseconds;     //轮询的间隔，毫秒单位
	//------------------------------------
	
	list<http_conn*> m_que;
	_sdl_cond excute_lock;     //一个互斥锁，用于线程同步

	void push_excuting(http_conn* pConn) //写入需要线程执行的script
	{
		m_que.push_back(pConn);
		excute_lock.active(); 
	}
	//------------------------------------
	HSQUIRRELVM v;             //Squirrel虚拟机
	HSQOBJECT tObjectAST;       //预定义的AST对象，防止每次重新产生对象
	HSQOBJECT tObjectHttpSq;    //预定义的http_sq对象，防止每次重新产生对象
	HSQOBJECT tObjectHttpConn;    //预定义的http_conn对象，防止每次重新产生对象
	HSQOBJECT tObjecthttp_header;    //预定义的http_conn对象，防止每次重新产生对象
	HSQOBJECT sqUriTranslate; //执行脚本中的URL翻译函数
	string    AUTO_LOGIN_URI; //重定向使用的URI;
	lig::JsonParser tjson;     //包含AST生成树的json分析器
	//-----------------------------------
	http_poll():sdl_thread(){}
	
	void init(int _port)
	{
		port=_port;
		milliseconds=50;
		v=NULL;
		sock_id=-1;
	};

	~http_poll(){}

	bool set_nonblock(int sock_id);
	//----------------------------------------------------------------
	bool write_callback_head(http_conn* pConn);
	bool write_json_head(http_conn* pConn);
	bool json_callback(http_conn* pConn,const char* requested_page);
	bool squirrel_callback(http_conn* pConn,const char* requested_page); //通用回调函数
	bool squirrel_functioncall(http_conn* pConn,const char* requested_page); //通用回调函数,直接执行
	bool squirrel_excute_function(http_conn* pConn,const char* requested_function);
	bool squirrel_excute_session_function(http_conn* pConn,const char* requested_function);
	bool squirrel_session_callback(http_conn* pConn,const char* requested_page); 
	bool create_script(const char* function_name,string &running_code);  //在虚拟机内注册函数 
	bool register_global_func(SQFUNCTION f,const char *fname,const char* para_mask=NULL); //在虚拟机内注册全局函数
	//--------------------------------------------------------------------------
	void vm_exec(http_conn* pConn); //在线程内执行请求的操作
	//------------------------------------------------
	void begin();
};

extern set<url_map,less_url_map> RegDB;		//回调请求记录库,在lig_httpd.cpp中定义
extern list<http_conn*> m_connList;			//连接记录库，,在lig_httpd.cpp中定义

class lig_httpd:public http_poll
{
private:
	http_poll FILEthreadA;  //文件输出执行线程1
	http_poll FILEthreadB;  //文件输出执行线程2
	http_poll VMthread; //虚拟机执行线程
public:	
	int time_out;  //这个时间值决定服务器等待连接的时间，超过这个值，连接会被终止
	bool flag_local_listen; //设定监听是否仅仅局限于本地连接
	string defaulf_page;    //缺省执行页面名称

	void insert_request(int my_sock);
	friend class http_conn;

	lig_httpd():http_poll(),time_out(30)
	{
		flag_local_listen=false;
		defaulf_page="index.html";
	};//缺省值为30秒,缺省仅仅监听外部传入数据

	void init(int PORT,int timeout=30);

	void connect(const char* url,C_CallbackType f)
	{
		url_map tmp;
		strcpy(tmp.url,url);
		tmp.squirrel_function_callback_name[0]=0; 
		tmp.pJosnRequest=&http_poll::json_callback;
		tmp.pCallback=&http_poll::squirrel_callback;
		tmp.pCCallback=f; 
		RegDB.insert(tmp); 
	}

	bool register_page(const char* url,const char* function_name,string &running_code) 
	{
		url_map tmp;
		while(url!=NULL && (*url=='/' || *url=='\\') && *url!=0) ++url;
		strcpy(tmp.url,url);
		strcpy(tmp.squirrel_function_callback_name ,function_name);
		tmp.pJosnRequest=&http_poll::json_callback;
		tmp.pCallback=&http_poll::squirrel_callback;
		tmp.pCCallback=NULL;  //C回调，缺省为NULL 
		tmp.flag_direct_sq_function=false; 
		create_script(function_name,running_code); 
		RegDB.insert(tmp); 
		return true;
	}

	bool register_sqfunction(const char* url,const char* function_name) 
	{
		url_map tmp;
		while(url!=NULL && (*url=='/' || *url=='\\') && *url!=0) ++url;
		strcpy(tmp.url,url);
		strcpy(tmp.squirrel_function_callback_name ,function_name);
		tmp.pJosnRequest=&http_poll::json_callback;
		tmp.pCallback=&http_poll::squirrel_excute_function; 
		tmp.pCCallback=NULL;  //C回调，缺省为NULL 
		tmp.flag_direct_sq_function=true; 
		RegDB.insert(tmp); 
		return true;
	}

	bool register_session_page(const char* url,const char* function_name,string &running_code) 
	{
		url_map tmp;
		while(url!=NULL && (*url=='/' || *url=='\\' || *url==' ') && *url!=0) ++url;
		strcpy(tmp.url,url);
		strcpy(tmp.squirrel_function_callback_name ,function_name);
		tmp.pJosnRequest=&http_poll::json_callback;
		tmp.pCallback=&http_poll::squirrel_session_callback;
		tmp.pCCallback=NULL;  //C回调，缺省为NULL 
		tmp.flag_direct_sq_function=false; 
		create_script(function_name,running_code); 
		RegDB.insert(tmp); 
		return true;
	}

	bool register_session_sqfunction(const char* url,const char* function_name) 
	{
		url_map tmp;
		while(url!=NULL && (*url=='/' || *url=='\\' || *url==' ') && *url!=0) ++url;
		strcpy(tmp.url,url);
		strcpy(tmp.squirrel_function_callback_name ,function_name);
		tmp.pJosnRequest=&http_poll::json_callback;
		tmp.pCallback=&http_poll::squirrel_excute_session_function; 
		tmp.pCCallback=NULL;  //C回调，缺省为NULL 
		tmp.flag_direct_sq_function=true; 
		RegDB.insert(tmp); 
		return true;
	}

	bool set_listen();  //设定监听
	
	void begin();
	void wait_connect();
	void wait_fcgi();

	bool check_register(const char* pUri,url_map* &pRt) //在已注册库中查找需要执行的对象
	{
		url_map tmp;
	 	strcpy(tmp.url,pUri);
		set<url_map,less_url_map>::iterator rt=RegDB.find(tmp); 
		if(rt!=RegDB.end()) //在注册库中找到了需要执行的脚本
		{
#ifdef _LIG_DEBUG
			DOUT<<"It's a excuted file,url="<<pUri<<endl;
#endif
			pRt=(url_map*)&(*rt);
			return true;
		}
		return false;
		
	}

	static void register_library(HSQUIRRELVM v);
	static SQInteger Sq2Constructor(HSQUIRRELVM v); //标准的Squirrel C++对象构造方式
	static SQInteger _queryobj_releasehook(SQUserPointer p, SQInteger size); //析构函数

	static SQInteger Sq2RunWebServer(HSQUIRRELVM v); 
	static SQInteger Sq2RunFCGIServer(HSQUIRRELVM v);
	static SQInteger Sq2LocalService(HSQUIRRELVM v);
	static SQInteger Sq2JoinPage(HSQUIRRELVM v);
	static SQInteger Sq2JoinSessionPage(HSQUIRRELVM v);
	
	static SQInteger Sq2MapScript(HSQUIRRELVM v); //将一个URL映射到一个可以直接执行的squirrel文件上
	static SQInteger Sq2MapSessionScript(HSQUIRRELVM v); //将一个URL映射到一个可以直接执行的squirrel文件上，映射的时候，自动检测session是否有效
	static SQInteger Sq2MapFunction(HSQUIRRELVM v); //将一个URL映射到一个可以直接执行的squirrel函数上
	static SQInteger Sq2MapSessionFunction(HSQUIRRELVM v); //将一个URL映射到一个可以直接执行的squirrel函数上，映射的时候，自动检测session是否有效
	static SQInteger Sq2SetDefaultPage(HSQUIRRELVM v); //设置本机服务器的缺省页
	static SQInteger Sq2ReturnRootPath(HSQUIRRELVM v) //返回服务器根目录
	{
		sq_pushstring(v,p_root_dir,-1);
		return 1;
	}
};
extern lig_httpd *pHttpd;
typedef lig_httpd  httpd;
extern const char* create_page(httpd& server,const char* url,const char* filename,bool encodeGB2312=false);
extern const char* create_session_page(httpd& server,const char* url,const char* filename,bool encodeGB2312=false);
extern SQInteger Sq2_http_header_addvar(HSQUIRRELVM v);
#endif
