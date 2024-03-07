#ifndef _LIG_HTTPD__
#define _LIG_HTTPD__
#define _CRT_SECURE_NO_WARNINGS
#define FD_SETSIZE 512 //��չFDSET����������֤�����������

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
#define EXPIRE_TIME 32  //ʧЧʱ��
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

struct mime_data_pair  //ʹ��mime��ʽ�ϴ��ļ�ʹ�õ����ݽṹ
{
	mime_data_pair *parent; //���ڸ���MIME��ָ�룬Ŀǰ�汾����ʹ��
	string name;
	string value;
	string filename;
	string storename;
	const char *p_data;  //ָ������ļ����ݵ�������ʼλ��
	int data_length;     //�����ļ��ĳ���
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
	char *p_buf;   //���ָ���ڿ�ʼ��ʱ��ָ��ҳ�滺���������ԣ�ҳ��ߴ粻��ʱ������Ҫ���������ڴ�
	int buf_size;  //Ŀǰ�������ĳߴ�
	int buf_curr;  //ҳ��Ŀǰ�������ݵĳ���
	int buf_start; //ҳ�濪ʼ��λ��
	size_t resize_times; //�����ݴ����Ĵ���
	char *p_segment;  //��ǰ��δ���������

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
		int nWrite=::send(socket_id,p_buf+buf_start,TransSize,0); //ÿ�δ���ĳߴ�̶�
		//DOUT.write(p_buf+buf_start,TransSize); 
		if(nWrite<0) return -1;//���ִ��������Ҫ��ֹ�������
		buf_start+=nWrite;
		if(buf_start<buf_curr)  return 0;  //δд���
		return 1; //д���

	}

	inline bool resize(int min_bytes=-1)
	{
		char *p_old_buf=p_buf;
		int old_size=buf_size;

		if(min_bytes==-1)
		{
			min_bytes=(2<<resize_times)*IO_SIZE; //����ָ�����ӷ�ʽ�����ڴ�
			if(resize_times==0) buf_size=IO_SIZE;
			if(buf_size+min_bytes>MAX_PAGE_SIZE) return false; //Խ��
		}
		
		p_buf=new char[buf_size+min_bytes]; //���뵽һ����������Ŀռ�
		if(p_buf==NULL) throw "Memory Alloc Error..page_buf::resize()";
		memcpy(p_buf,p_old_buf,old_size);
		buf_size+=min_bytes; //�õ�׼ȷ���³ߴ�
		
		if(resize_times>0) 
			delete[] p_old_buf; //������ǳ������룬��Ҫɾ���ϴ�����Ŀռ�

		resize_times++;
		p_segment=p_buf; //ָ�����ڴ��е�ԭ��λ��
		return true;
	};

	void printf(const char *fmt, ...)
	{
		va_list	ap;
		int nWrite;
		va_start(ap, fmt);
		nWrite=vsnprintf(p_buf+buf_curr,buf_size-buf_curr,fmt,ap);
		va_end(ap);
		while(nWrite<0 && resize())  //�ڴ�ռ䲻�������·����ڴ棬����д
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

	inline void direct_write(string &data) //ֱ��д�룬�����б���
	{
		int length=data.length();
		if((buf_size-buf_curr)<length) {if(!resize()) return;}
		memcpy(p_buf+buf_curr,data.data(),length); 
		buf_curr+=length;
	}

	inline void direct_write(const char* data,int length) //ֱ��д�룬�����б���
	{
		if((buf_size-buf_curr)<length)  {if(!resize()) return;}
		memcpy(p_buf+buf_curr,data,length); 
		buf_curr+=length;
	}

	inline void direct_write(const char* data) //ֱ��д�룬�����б���
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


struct http_header //��HTTP����ͷ�Ĵ������
{

	vector<VARS> varList;   //����Ļ���������������Դ���Լ��Ľ�����Ҳ������Դ��FCGI
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

	void add_vars(const char *key,const char* value) // ���ӵ�GET���룬����url��д
	{
		VARS tmp;
		tmp.name=(char*)key;
		tmp.value=(char*)value;
		if(strcmp(tmp.name,"_csp_key")==0) //���������_csp_key,����Ϊsession�ַ���
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

		while(pUri!=NULL && *pUri!=0 && (*pUri==' ' || *pUri=='/')) ++pUri; //ȥ����ߵ���Ч�ո�
		while(pUriEnd>pUri && *pUriEnd==0) --pUriEnd;
		while(pUriEnd>pUri && *pUriEnd==' '){*pUriEnd=0;--pUriEnd;} //ȥ���ұ���Ч�ո�

		urldecode(pUri,pUri); //puriû�а���get������ѡ��

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
				if(strcmp(tmp.name,"_csp_key")==0) //������һ��������_csp_key,����Ϊsession�ַ���
					pSession=tmp.value; 
				else
					varList.push_back(tmp); 
				++pBuf;
				tmp.name=pBuf;
			}
			++pBuf;
		}
		urldecode(tmp.value,tmp.value);
		if(strcmp(tmp.name,"_csp_key")==0) //������һ��������_csp_key,����Ϊsession�ַ���
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
	read_ready    =0,      //��׼���ã������
	write_ready   =1,      //д׼����
	read_finished =2,      //������
	conn_finished =3,      //����ȫ��������Ϻ�ر�����
	write_finished=4      //д��ϣ������������д
};

enum fcgi_link_stat
{
	fcgi_ready         =0,
	fgci_begin_request =1,    //�õ�fcgi����
	fcgi_read_client   =2,    //���ڶ�fcgi
	fcgi_read_finished =3,    //��fcgi
	fcgi_server_processing =4,//���ڴ�������
	fcgi_server_processed  =5,//�������
	fcgi_write_clinet      =6,//����д������
	fcgi_write_finished    =7,//������ȫд��
	fcgi_close_connect     =8 //��Ҫ�ر�����
};

struct http_conn
{
	typedef bool (http_conn::*funcType)();
	//----------------------------------------------
	int socket_id;  //��WebServer����socket,��FCGI����FCGI�������
	struct sockaddr_in addr; //IP address struct
	//-----------------------------------------------
	page_buf inBuf;
	page_buf outBuf;
	//-----------------------------------------------
	time_t time_stamp;
	//-----------------------------------------------
	FILE* mFiles;   //��Ҫд�����ļ�
	int FileLength; //�ļ��ĳ���
	int curpos;     //�ļ���ǰ�Ķ�ȡλ��
	struct stat file_state; //�ļ���״̬
	//-------------------------------------------------
	char* pHeader[16];
	//-------------------------------------------------
	
	http_link_stat link_stat;

	bool flag_header_ok;    //����ͷ�����ɹ�
	bool flag_json_request; //��ʾ��JsonRequest
	bool flag_file_upload;  //��ʾ����Ϊ�ļ��ϴ�
	bool flag_uri_translate; //Ҫ��ű�����URIת��
	//---------------------------------------------------
	funcType action;        //��Ҫ�ڷ�������ִ�еĲ���
	http_header header;   //������,�Լ����������Ĵ洢λ��
	
	lig_httpd *pServer;     //������
	//---------------------------------------------------
	int post_data_offset;            //�ڲ�ʹ�õı���,���ڼ�¼POST���ݵ����
	int post_length;        //�ͻ�Ҫ���post����
	int header_length;      //����ͷ�ĳ���
	//---------------------------------------------------
	int fcgi_content_type_length; //fcgi�ϴ���fcgi_content_type_length����
	fcgi_link_stat flag_fcgi;//fcgi��״̬
	bool flag_fcgi_close_state; //webserverҪ��ر�����
	//----------------------------------------------------
	void* pUrlMap;          //URL MAP
	//----------------------------------------------------
	list<mime_data_pair> mlist; //�ļ��ϴ�ʹ�õ��б�,ע��mlist��������ݶ�����page_buf�ڵ����ݣ�mlist�����������ڴ棬���Բ���Ҫ��mlist����
	//----------------------------------------------------
	~http_conn()
	{
		//destructor(&mlist);
		//destructor(&header);
	}

	void init(lig_httpd* ps);
	inline void printf(char* fmt,...);
	inline bool ncasecmp(const char* str1,const char* str2);
	int montoi(const char *s); //����shttpd�ĺ���
	time_t datetosec(const char *s); //����shttpd�ĺ���
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
	void get_fastcgi_params(); //�õ�FCGI�Ļ�������ֵ
	

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
		register_func(v,"sq_state",Sq2Constructor,"constructor","x",NULL);//���캯��
		//register_func(v,"sq_state",Sq2Trans,"trans","xia",NULL);//����ת�ƺ���
		register_func(v,"sq_state",SQStateStrGet,"_get","xi",NULL);
	};

	static SQInteger _queryobj_releasehook(SQUserPointer p, SQInteger size) //��׼��Squirrel C++���������ص�
	{
		sq_state *self = ((sq_state*)p);
		delete(self);
		return 1;
	}

	static SQInteger SQStateStrGet(HSQUIRRELVM v) //��׼��Squirrel C++�����췽ʽ
	{
		sq_state *pSelf = NULL;
		sq_getinstanceup(v,1,(SQUserPointer *)&pSelf,0); //��׼�Ļ�ȡC++����ķ�ʽ
		if(pSelf==NULL) return 0; 
		int hnd;
		sq_getinteger(v,-1,&hnd);
		const char* str=_get_string(hnd);
		if(str!=NULL) sq_pushstring(v,str,strlen(str));
		else sq_pushnull(v);
		return 1;
	};
	

	static SQInteger Sq2Constructor(HSQUIRRELVM v) //��׼��Squirrel C++�����췽ʽ
	{
		sq_state* pobj=new sq_state; //�Ӷ�ջ�л�ȡC++�����ָ��
		if(!pobj) return sq_throwerror(v,"Create instance failed");
		SQUserPointer theHandle=(SQUserPointer) pobj;
		sq_setinstanceup(v,1,theHandle); //Ϊ��ʵ������C++����ָ��,��ʵ���ڶ�ջ�ĵײ�������idx ʹ��1
		sq_setreleasehook(v,1,_queryobj_releasehook);
		return 0;
	}
	
};
*/
struct http_sq //����Squirrel�������������
{
	http_conn* pHttp;
	//-----------------------

	//-----------------------
	Scan* pScan; //ָ��ǰ˳�������λ�ã����ֵ��Squirrel�б仯
	Scan* pSegment;
	//-------------
	Scan* pStart; //����ҳ���������㣬���ֵ����Squirrel�б仯
	Scan* pRoot;  //����ģ����������
	const char* function_name; 
	const char* pLastStr; //ָ����������λ��
	bool flag_switch; //����innerWrite�Ƿ�ִ������Ŀ���

	vector<Scan*> thePosIdx; //��¼�������������ֵ����C++��ʼ�������ʱ�����룬�������Squirrel�б仯
	//-------------
	string javascript_code; //����������js����,�����ύ����ʱ��ҳ��������ȡ

	void reset(){pScan=pStart;flag_switch=true;};

	static SQInteger Sq2Constructor(HSQUIRRELVM v); //��׼��Squirrel C++�����췽ʽ
	static SQInteger Sq2GetVars(HSQUIRRELVM v);
	static SQInteger Sq2Get(HSQUIRRELVM v);
	static SQInteger Sq2Write(HSQUIRRELVM v);
	static SQInteger Sq2WriteTemplate(HSQUIRRELVM v);
	static SQInteger Sq2WriteHandle(HSQUIRRELVM v);
	static SQInteger Sq2IdxWrite(HSQUIRRELVM v);
	static SQInteger Sq2InnerClose(HSQUIRRELVM v); //�����
	static SQInteger Sq2InnerOpen(HSQUIRRELVM v); //�ر����
	static SQInteger Sq2SegmentWrite(HSQUIRRELVM v);
	static SQInteger Sq2LastWrite(HSQUIRRELVM v); //д�����ַ���
	static SQInteger Sq2GetTemplate(HSQUIRRELVM v); //��ȡһ��ģ��
	static SQInteger Sq2AjaxValue(HSQUIRRELVM v);
	static SQInteger Sq2AjaxAlert(HSQUIRRELVM v);
	static SQInteger Sq2AjaxURL(HSQUIRRELVM v);
	static SQInteger Sq2AjaxinnerHTML(HSQUIRRELVM v);
	static SQInteger Sq2AjaxnewList(HSQUIRRELVM v);
	static SQInteger Sq2AjaxInnerTemplate(HSQUIRRELVM v);
	static SQInteger Sq2DeleteLastComma(HSQUIRRELVM v);
	static SQInteger Sq2CreateSession(HSQUIRRELVM v);
	static SQInteger Sq2GetSession(HSQUIRRELVM v);
	static SQInteger Sq2Redirect(HSQUIRRELVM v); //ҳ���ض���
	static SQInteger Sq2AjaxEnable(HSQUIRRELVM v); //��ҳ��Ԫ������
	static SQInteger Sq2AjaxDisable(HSQUIRRELVM v); //��ҳ��Ԫ�ؽ���
	static SQInteger Sq2GetHTTP(HSQUIRRELVM v); //��ȡ�ϴ�����
	static SQInteger Sq2HTTPWriteUpload2File(HSQUIRRELVM v);
	
	static bool push_obj(HSQUIRRELVM v,http_sq *pObj); //��������ѹ��squirrel��ջ����Ϊ���󴫵ݸ��ű�������
	static void register_library(HSQUIRRELVM v);
	
};


typedef MemAlloctor<http_conn,1024,4> ConnList;
typedef bool(http_poll::*CallBackFuncType)(http_conn*,const char*);
typedef bool(*C_CallbackType)(http_conn*,const char*);
struct url_map
{
	char url[256];
	char squirrel_function_callback_name[256];
	bool flag_direct_sq_function; //��ʾΪֱ������squirrel_function_callback_name����ʾsquirrel�����root table�£�������ĺ������ơ�
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

struct http_poll:public sdl_thread   //HTTP�����ִ����
{
	fd_set rd_set;
	fd_set wr_set;        //socket��Ķ�д����
	int max_fd; 
	int sock_id;
	int port;            //ʹ�õĶ˿ں�,����ִ���߳������߳�ͨ��ʹ�õ�socket������Linux/UNIX����ʹ��socketpair,�˿�������������
	//-----------------------------------
	int milliseconds;     //��ѯ�ļ�������뵥λ
	//------------------------------------
	
	list<http_conn*> m_que;
	_sdl_cond excute_lock;     //һ���������������߳�ͬ��

	void push_excuting(http_conn* pConn) //д����Ҫ�߳�ִ�е�script
	{
		m_que.push_back(pConn);
		excute_lock.active(); 
	}
	//------------------------------------
	HSQUIRRELVM v;             //Squirrel�����
	HSQOBJECT tObjectAST;       //Ԥ�����AST���󣬷�ֹÿ�����²�������
	HSQOBJECT tObjectHttpSq;    //Ԥ�����http_sq���󣬷�ֹÿ�����²�������
	HSQOBJECT tObjectHttpConn;    //Ԥ�����http_conn���󣬷�ֹÿ�����²�������
	HSQOBJECT tObjecthttp_header;    //Ԥ�����http_conn���󣬷�ֹÿ�����²�������
	HSQOBJECT sqUriTranslate; //ִ�нű��е�URL���뺯��
	string    AUTO_LOGIN_URI; //�ض���ʹ�õ�URI;
	lig::JsonParser tjson;     //����AST��������json������
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
	bool squirrel_callback(http_conn* pConn,const char* requested_page); //ͨ�ûص�����
	bool squirrel_functioncall(http_conn* pConn,const char* requested_page); //ͨ�ûص�����,ֱ��ִ��
	bool squirrel_excute_function(http_conn* pConn,const char* requested_function);
	bool squirrel_excute_session_function(http_conn* pConn,const char* requested_function);
	bool squirrel_session_callback(http_conn* pConn,const char* requested_page); 
	bool create_script(const char* function_name,string &running_code);  //���������ע�ắ�� 
	bool register_global_func(SQFUNCTION f,const char *fname,const char* para_mask=NULL); //���������ע��ȫ�ֺ���
	//--------------------------------------------------------------------------
	void vm_exec(http_conn* pConn); //���߳���ִ������Ĳ���
	//------------------------------------------------
	void begin();
};

extern set<url_map,less_url_map> RegDB;		//�ص������¼��,��lig_httpd.cpp�ж���
extern list<http_conn*> m_connList;			//���Ӽ�¼�⣬,��lig_httpd.cpp�ж���

class lig_httpd:public http_poll
{
private:
	http_poll FILEthreadA;  //�ļ����ִ���߳�1
	http_poll FILEthreadB;  //�ļ����ִ���߳�2
	http_poll VMthread; //�����ִ���߳�
public:	
	int time_out;  //���ʱ��ֵ�����������ȴ����ӵ�ʱ�䣬�������ֵ�����ӻᱻ��ֹ
	bool flag_local_listen; //�趨�����Ƿ���������ڱ�������
	string defaulf_page;    //ȱʡִ��ҳ������

	void insert_request(int my_sock);
	friend class http_conn;

	lig_httpd():http_poll(),time_out(30)
	{
		flag_local_listen=false;
		defaulf_page="index.html";
	};//ȱʡֵΪ30��,ȱʡ���������ⲿ��������

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
		tmp.pCCallback=NULL;  //C�ص���ȱʡΪNULL 
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
		tmp.pCCallback=NULL;  //C�ص���ȱʡΪNULL 
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
		tmp.pCCallback=NULL;  //C�ص���ȱʡΪNULL 
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
		tmp.pCCallback=NULL;  //C�ص���ȱʡΪNULL 
		tmp.flag_direct_sq_function=true; 
		RegDB.insert(tmp); 
		return true;
	}

	bool set_listen();  //�趨����
	
	void begin();
	void wait_connect();
	void wait_fcgi();

	bool check_register(const char* pUri,url_map* &pRt) //����ע����в�����Ҫִ�еĶ���
	{
		url_map tmp;
	 	strcpy(tmp.url,pUri);
		set<url_map,less_url_map>::iterator rt=RegDB.find(tmp); 
		if(rt!=RegDB.end()) //��ע������ҵ�����Ҫִ�еĽű�
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
	static SQInteger Sq2Constructor(HSQUIRRELVM v); //��׼��Squirrel C++�����췽ʽ
	static SQInteger _queryobj_releasehook(SQUserPointer p, SQInteger size); //��������

	static SQInteger Sq2RunWebServer(HSQUIRRELVM v); 
	static SQInteger Sq2RunFCGIServer(HSQUIRRELVM v);
	static SQInteger Sq2LocalService(HSQUIRRELVM v);
	static SQInteger Sq2JoinPage(HSQUIRRELVM v);
	static SQInteger Sq2JoinSessionPage(HSQUIRRELVM v);
	
	static SQInteger Sq2MapScript(HSQUIRRELVM v); //��һ��URLӳ�䵽һ������ֱ��ִ�е�squirrel�ļ���
	static SQInteger Sq2MapSessionScript(HSQUIRRELVM v); //��һ��URLӳ�䵽һ������ֱ��ִ�е�squirrel�ļ��ϣ�ӳ���ʱ���Զ����session�Ƿ���Ч
	static SQInteger Sq2MapFunction(HSQUIRRELVM v); //��һ��URLӳ�䵽һ������ֱ��ִ�е�squirrel������
	static SQInteger Sq2MapSessionFunction(HSQUIRRELVM v); //��һ��URLӳ�䵽һ������ֱ��ִ�е�squirrel�����ϣ�ӳ���ʱ���Զ����session�Ƿ���Ч
	static SQInteger Sq2SetDefaultPage(HSQUIRRELVM v); //���ñ�����������ȱʡҳ
	static SQInteger Sq2ReturnRootPath(HSQUIRRELVM v) //���ط�������Ŀ¼
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
