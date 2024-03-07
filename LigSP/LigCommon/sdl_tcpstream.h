//written by Li Gang China, 2002.8.7
//this file tested in MSVC and GCC ,and can be used in Windows, Linux, Solaris ,True64UNIX
#ifndef __SDL_TCPSTREAM_HEAD__
#define __SDL_TCPSTREAM_HEAD__

#ifndef WIN32
	#define _sdl_linux
#endif

#define _CRT_SECURE_NO_WARNINGS
#ifdef WIN32

#include <windows.h>        
#include <process.h>
#include <winsock.h>  
#include <errno.h>
#else
#include <sys/time.h>
#include <sys/socket.h>
#include <netdb.h>
#include <netinet/in.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <errno.h>
#include <fcntl.h>
#include <pthread.h>
#include <string>
#include <netinet/tcp.h>
#include <net/if_arp.h> 
#include <net/if.h> 
#include <sys/ioctl.h>
#endif


#include <ctime>
#include <iostream>
#include <fstream>
#include <string>
#include <list>
#include <set>
#include <vector>
#include <algorithm>
#include <stack>
#include <stdio.h>
#include <stdarg.h>

#include "sdl_error.h"
#include "sdl_signal.h"

using namespace std;

#define LINE_BUF_SIZE 1024   //used for direct write socket
#define STREAM_BUF_SIZE 512      //tcpstream buffer size,must lagrg than 16

#define MAXLISTENS  16   //used by tcp listen 

#ifdef WIN32
void _sdl_win_thread_proc(void *pParam);
typedef unsigned long uint32_t;   //windows only
#else
void *_sdl_unix_thread_proc(void *pParam );
#endif

std::string convert_hex(char *msg_buf,int buf_length);
std::string convert_hex2(char *msg_buf,int buf_length,int timestamp);
bool convert_binary(char *msg_buf,char *binary_buf,int &buf_length,int &timestamp);
std::string get_local_ip(const char* ETH_INTERFACE_NAME="eth0"); //注意，Windows下无法指定网络端口，Linux下才可以使用
std::string get_host_ip(const char *hosname);

#define CHECK_ERROR(ret, doing_what)\
{\
	if (ret == -1) {\
	fprintf(stderr,"An error occured when %s\n", doing_what);\
	fprintf(stderr,"errno = %d\n", errno);\
	fprintf(stderr,"error_discription = %s\n", strerror(errno));\
	fprintf(stderr,"Error position: File: %s, Line %d\n", __FILE__, __LINE__);\
	exit(-1);\
	} \
}



//2004-8-15， 重新写的读写锁，与前一个版本保持兼容，这个读写锁经过了1小时高强度测试，应该没有问题
//模板第一个参数为允许的最大读线程，第二个参数为检查到锁定以后，第二次尝试的等待时间,单位为毫秒
//这两个参数只对Windows有效，对于pthread无效
template<int Max_Read_Threads=8,int Conflict_Wait_Slip=2>
class _sdl_rwlock
{
private:
#ifdef WIN32
	int read_que;
	HANDLE EventRd; //允许写信号
	HANDLE EventWr; //允许写信号
	HANDLE MuRdToken; //读队列令牌
	HANDLE MuWrToken; //写令牌

	inline bool get_read_token() //获取进入读队列的令牌
	{
		return(::WaitForSingleObject(MuRdToken,INFINITE)!=WAIT_ABANDONED);
	};

	inline void release_read_token() //释放进入读队列的令牌
	{
		::ReleaseMutex(MuRdToken);
	};

	inline bool get_write_token() //获取进入写队列的令牌
	{
		return(::WaitForSingleObject(MuWrToken,INFINITE)!=WAIT_ABANDONED);
	};

	inline void release_write_token() //释放进入写队列的令牌
	{
		::ReleaseMutex(MuWrToken);
	};

	inline void wait_read_queue() //读队列排队等候
	{
		while(read_que>=Max_Read_Threads) 
			Sleep(Conflict_Wait_Slip);     
	};

	inline void set_read_free()
	{
		::SetEvent(EventRd);
	};

	inline void set_read_busy()
	{
		::ResetEvent(EventRd);
	};

	inline void wait_read_free()
	{
		::WaitForSingleObject(EventRd,INFINITE);
	};

	inline void wait_write_free()
	{
		::WaitForSingleObject(EventWr,INFINITE);
	}

	inline void set_write_free()
	{
		::SetEvent(EventWr);
	};

	inline void set_write_busy()
	{
		::ResetEvent(EventWr);
	};
#else
	pthread_rwlock_t p_rwlock;
#endif
public:
	_sdl_rwlock()
	{
#ifdef WIN32
		read_que=0;

		EventRd=::CreateEvent(NULL,TRUE,TRUE,NULL); 
		//创建一个非自动重置写信号,此时有信号，无需等待；
		if(EventRd==NULL) throw "Create Event Failed!";

		EventWr=::CreateEvent(NULL,TRUE,TRUE,NULL); 
		//创建一个非自动重置写信号,此时有信号，无需等待；
		if(EventWr==NULL) throw "Create Event Failed!";

		MuRdToken=::CreateMutex(NULL,FALSE,NULL); 
		//创建一个互斥体,作为令牌；
		if(MuRdToken==NULL) throw "Create Mutex Failed!";

		MuWrToken=::CreateMutex(NULL,FALSE,NULL); 
		//创建一个互斥体,作为令牌；
		if(MuWrToken==NULL) throw "Create Mutex Failed!";
#else
		pthread_rwlock_init(&p_rwlock,NULL);
#endif
	};

	~_sdl_rwlock()
	{
#ifdef WIN32
		::CloseHandle(EventRd);
		::CloseHandle(EventWr);
		::CloseHandle(MuRdToken);
		::CloseHandle(MuWrToken);
#else
		pthread_rwlock_destroy(&p_rwlock);
#endif
	}

	void read_lock()
	{
#ifdef WIN32
		wait_read_queue();

		get_read_token();     //获取令牌
		read_que++;        //读队列已经增加
		release_read_token();   //释放令牌

		wait_write_free();

		set_read_busy();
		//如果没有写操作，无需等待，否则则一直等
#else
		pthread_rwlock_rdlock(&p_rwlock);
#endif
	}

	void write_lock()
	{
#ifdef WIN32
		get_write_token(); //获取令牌
		wait_read_free(); //等到读允许
		set_write_busy();
#else
		pthread_rwlock_wrlock(&p_rwlock);
#endif      
	}

	void write_unlock()
	{
#ifdef WIN32
		set_write_free(); //设置允许读信号
		release_write_token(); //释放令牌
#else
		pthread_rwlock_unlock(&p_rwlock);
#endif
	}

	void read_unlock()
	{
#ifdef WIN32
		get_read_token(); //获得令牌;
		read_que--;
		if(read_que==0) //判断是否允许写
			set_read_free();
		release_read_token(); //允许读排队
#else
		pthread_rwlock_unlock(&p_rwlock);
#endif
	}
};

typedef _sdl_rwlock<16>  sdl_rwlock; //设置读写锁最大16用户读，一个用户写

class _sdl_cond //条件变量
{
private:

#ifdef WIN32
	HANDLE EventHandle; //条件变量
#else
	pthread_cond_t EventHandle;
	pthread_mutex_t mu;
#endif

public:
	_sdl_cond() //毫秒单位的超时等待时间，不设置表示永远等待
	{
#ifdef WIN32
	
		EventHandle=::CreateEvent(NULL,FALSE,FALSE,NULL); 
		//创建一个自动重置信号,此时无信号，需等待；
#else
		if(pthread_cond_init(&EventHandle,NULL)!=0)
			throw sdl_exception("Create cond Error! Li da gang",VERY_BIG_ERR);
		if(pthread_mutex_init(&mu, NULL)!=0)
			throw sdl_exception("Create Mutex Error! Li da gang",VERY_BIG_ERR);
         
#endif
	}

	~_sdl_cond()
	{
#ifdef WIN32
		::CloseHandle(EventHandle);
#else
		pthread_mutex_destroy(&mu);
		pthread_cond_destroy(&EventHandle);

#endif
	}
	bool wait()
	{
#ifdef WIN32
		return(::WaitForSingleObject(EventHandle,INFINITE)!=WAIT_ABANDONED);
#else

		pthread_mutex_lock(&mu);
		int rc = pthread_cond_wait(&EventHandle,&mu);
        	pthread_mutex_unlock(&mu);
#endif
	}

	void active()
	{
#ifdef WIN32
		::SetEvent(EventHandle);
#else
		pthread_cond_broadcast(&EventHandle);

#endif
	}
};

class sdl_mutex
{
private:
#ifdef WIN32
	HANDLE  hRunMutex;
#else
	pthread_mutex_t mu;
#endif
	int state;
public:
	sdl_mutex()
	{
#ifdef WIN32
		hRunMutex=CreateMutex( NULL, FALSE, NULL );
		state=true;
#else
		if(pthread_mutex_init(&mu, NULL)==-1)
			throw sdl_exception("Create Mutex Error! Li da gang",VERY_BIG_ERR);
		state=true;
#endif
	};
	~sdl_mutex()
	{
#ifdef WIN32
		CloseHandle( hRunMutex );
#else
		pthread_mutex_destroy(&mu);
#endif
	};
	inline void lock()
	{
#ifdef WIN32
		WaitForSingleObject( hRunMutex, INFINITE );
		state=true;
#else
		pthread_mutex_lock(&mu);
		state=true;
#endif
	};

	inline void unlock()
	{
#ifdef WIN32
		ReleaseMutex(hRunMutex);
		state=false;
#else
		if(state) pthread_mutex_unlock(&mu);
		state=false;
#endif

	};
	inline int is_lock(){return (state);};
};


class sdl_thread
{
public:
	bool active;
	bool cancel;
	//sdl_mutex block;
#ifdef WIN32
	unsigned int  pthread;
	sdl_signal2<int,unsigned int> ThreadStartSignal;   //线程启动信号,第一参数为线程池编号，即handle_id,第二产生为Thread的编号
	sdl_signal2<int,unsigned int> ThreadStopSignal;	  //线程退出信号
#else
	pthread_t pthread;
	sdl_signal2<int,pthread_t> ThreadStartSignal;   //线程启动信号，第一参数为线程池编号，即handle_id,第二产生为Thread的编号
	sdl_signal2<int,pthread_t> ThreadStopSignal;	  //线程退出信号
#endif
	bool bstop;
	int handle_id;  //线程池中的线程编号
	
	sdl_thread()
	{
		active=false;
		bstop=false;
		cancel=false;
		handle_id=-1; //-1为无效的线程编号
	};

	inline bool is_active()
	{
		/*   //此端代码可以提高Windows下多线程程序判断线程状态的精确性，但尚未测试，故暂不使用
		#ifdef WIN32
		if(::WaitForSingleObject((HANDLE)pthread,0)==WAIT_TIMEOUT) //线程在运行，应该没有信号，所有表达式为真
		return true;
		else return false;
		#else
		return active;
		#endif
		*/
		return active;
	};
	virtual void set_active(){active=true;ThreadStartSignal(handle_id,pthread);};
	virtual void set_inactive(){active=false;ThreadStopSignal(handle_id,pthread);};
	inline int get_id() {return (int)pthread;};
//	inline int release() {block.unlock();};
#ifdef WIN32
	void kill_thread()
	{
		//::AfxTermThread((HINSTANCE)pthread); 
		::_endthread(); 
	}
#endif
	//begin the thread
	void start_thread()
	{
		sdl_thread *pself = this;
#ifdef WIN32
		pthread=::_beginthread(_sdl_win_thread_proc,0,pself);
		if(pthread==-1) 
		{
			active=false;
			throw sdl_exception("Create Thread Error! Li da gang");
		}
#else
		if(pthread_create(&pthread, NULL, _sdl_unix_thread_proc, pself)!=0) 
		{
			//Dout<<"---------Thread error!!";
			throw sdl_exception("Create thread Error! Li da gang");
		}
#endif
	};

#ifndef WIN32
	void start_fork()
	{
		pid_t rt;
		if((rt=fork())<0)
			// Dout<<"---------Can't fork new process!"<<endl;
			if(rt==0) //new process;
			{
				begin();
				//Dout<<"------------Child Process End!"<<endl;
				exit(0);
			}

	};
#endif
	virtual void begin()=0;
};

struct sdl_time //注意，如果需要获得真实时间，使用的时候需要执行init()
{
	time_t mtime; //C时间
	unsigned int m_timespan;    //毫秒数
	//---------------------
	unsigned int pre_msecs; //上一次获取时间的Time_tick计数
	
	sdl_time()
	{
		mtime=0;
		m_timespan=0;
		pre_msecs=0;
	
	}

	sdl_time(const sdl_time &st)
	{
		mtime=st.mtime;
		m_timespan=st.m_timespan;
		pre_msecs=st.pre_msecs; 
	}

	//这是一个牛人写的日期转换代码，很是仰慕，于是留在这里
	//注意，月是从1-12，年是公元年
	bool maketime (unsigned int year, unsigned int  mon, unsigned int day, unsigned int hour, unsigned int min, unsigned int sec, unsigned int msec)
    {
		if(year<1970 || mon<1 || mon>12 || day<1 || day>31 || hour>23 || min>59 || sec>59 || msec>999) return false;
		if(0>=(int)(mon -= 2)) //* 1..12 -> 11,12,1..10 */
		{   
			mon += 12;      //* Puts Feb last since it has leap day */
			year -= 1;
		}
		mtime=((((unsigned long) (year/4 - year/100 + year/400 + 367*mon/12 + day)+year*365-719499 )*24 + hour //* now have hours */
		)*60 + min  //* now have minutes */
		)*60 + sec; //* finally seconds */
		pre_msecs=0;
		m_timespan=msec;
		return true;
	}

	unsigned int msecs(){return m_timespan;} //返回从init()执行到现在的毫秒数

	bool is_invailid() //表明的无效的时标
	{
		return(mtime==0 || m_timespan==-1 || pre_msecs==-1);
	}

	string local_timestr()
	{
		char date[64];
		time_t ttime=mtime+m_timespan/1000;
		strftime(date, sizeof(date), "%a, %d %b %Y %H:%M:%S", localtime(&ttime));
		string rt=date;
		return rt;
	}

	string gm_timestr()
	{
		char date[64];
		time_t ttime=mtime+m_timespan/1000;
		strftime(date, sizeof(date), "%a, %d %b %Y %H:%M:%S GMT", gmtime(&ttime));
		string rt=date;
		return rt;
	}

	int sch_timestr(char* buf)
	{
		time_t ttime=mtime+m_timespan/1000;
		struct tm* ptm=gmtime(&ttime);
		return sprintf(buf,"%04d-%02d-%02d %02d:%02d:%02d.%03d",ptm->tm_year+1900,ptm->tm_mon+1,ptm->tm_mday,
			ptm->tm_hour,ptm->tm_min,ptm->tm_sec,m_timespan%1000);

	}

	bool chdate2time(const char *s) //从字符串获取时间
	{
		struct tm tm;
		bool flag_ok=false;
		int msecs,sec, min, hour, mday, month, year;

		(void) memset(&tm, 0, sizeof(tm));
		sec = min = hour = mday = month = year = 0;

		if (((sscanf(s, "%04d-%02d-%02d %02d:%02d:%02d.%04d",
			&year, &month, &mday, &hour, &min, &sec,&msecs) == 7)))
		{
			msecs/=10;
			flag_ok=true;
		}else
		if (((sscanf(s, "%04d-%02d-%02d %02d:%02d:%02d.%03d",
			&year, &month, &mday, &hour, &min, &sec,&msecs) == 7)))
		{
			flag_ok=true;
		}

		if(!flag_ok) return false;

		tm.tm_year=year;tm.tm_mon=month-1;tm.tm_mday=mday;
		tm.tm_hour=hour;tm.tm_min=min;tm.tm_sec=sec;      
		if (tm.tm_year > 1900)
			tm.tm_year -= 1900;
		else if (tm.tm_year < 70)
			tm.tm_year += 100;

#ifdef WIN32
		pre_msecs=0;
		m_timespan=msecs; //得到实际上的毫秒数
#else
		timeval m_stime;
		gettimeofday(&m_stime,0);
		pre_msecs=m_stime.tv_usec / 1000; 
		m_timespan=0;
#endif
#ifdef WIN32
		mtime=_mkgmtime(&tm);
#else
		mtime=mktime(&tm);
#endif
		return true;
	};

	int montoi(const char *s) //来自shttpd的函数
	{
		static const char *ar[] = {
			"Jan", "Feb", "Mar", "Apr", "May", "Jun",
			"Jul", "Aug", "Sep", "Oct", "Nov", "Dec"
		};
		size_t	i;

		for(i = 0;i < 12;i++)
			if(strcmp(s,ar[i])==0) return (i);
		return (-1);
	}

	void datetosec(const char *s) //来自shttpd的函数
	{
		struct tm tm;
		char mon[32];
		int sec, min, hour, mday, month, year;

		(void) memset(&tm, 0, sizeof(tm));
		sec = min = hour = mday = month = year = 0;

		if (((sscanf(s, "%d/%3s/%d %d:%d:%d",
			&mday, mon, &year, &hour, &min, &sec) == 6) ||
			(sscanf(s, "%d %3s %d %d:%d:%d",
			&mday, mon, &year, &hour, &min, &sec) == 6) ||
			(sscanf(s, "%*3s, %d %3s %d %d:%d:%d",
			&mday, mon, &year, &hour, &min, &sec) == 6) ||
			(sscanf(s, "%d-%3s-%d %d:%d:%d",
			&mday, mon, &year, &hour, &min, &sec) == 6)) &&
			(month = montoi(mon)) != -1) {
				tm.tm_mday	= mday;
				tm.tm_mon	= month;
				tm.tm_year	= year;
				tm.tm_hour	= hour;
				tm.tm_min	= min;
				tm.tm_sec	= sec;
		}

		if (tm.tm_year > 1900)
			tm.tm_year -= 1900;
		else if (tm.tm_year < 70)
			tm.tm_year += 100;

#ifdef WIN32
		pre_msecs=GetTickCount();
		m_timespan=0; //得到实际上的毫秒数
#else
		timeval m_stime;
		gettimeofday(&m_stime,0);
		pre_msecs=m_stime.tv_usec / 1000; 
		m_timespan=0;
#endif
		mtime=mktime(&tm);
	};

	time_t gmt_offset(int offset_sec)
	{
		struct tm m_tm;
		time_t ttime=mtime+m_timespan/1000;
		m_tm=*(gmtime(&ttime));
		m_tm.tm_hour=0;
		m_tm.tm_min=0;
		m_tm.tm_sec=0;
		time_t ofst=mktime(&m_tm);
		ofst+=offset_sec;
		mtime=ofst;
	}

	static unsigned int rand() //加入时间因素的的随机数产生器
	{
		static unsigned int counter=999983; //另外一个小随机数
		static long long RandSeed;
		long long x = 0xffffffff;x += 1;
		unsigned int time_seed=1777;
		if(counter < 4096) //平均16次更改一次种子数；
		{
#ifdef WIN32
			time_seed=GetTickCount();//得到实际上的毫秒数
#else
			timeval m_stime;
			gettimeofday(&m_stime,0);
			time_seed=m_stime.tv_usec / 1000; 
#endif
			time_seed=time_seed % 1789;
		}
		counter *= 999773;
		counter = counter % 0x10000;
		RandSeed *= ((long long)134775813);
		RandSeed += time_seed;
		RandSeed = RandSeed % x ;
		return (RandSeed % x );
	}

	void init()
	{
		time(&mtime);
#ifdef WIN32
		pre_msecs=GetTickCount();
		m_timespan=0; //得到实际上的毫秒数
#else
		timeval m_stime;
		gettimeofday(&m_stime,0);
		pre_msecs=m_stime.tv_usec / 1000; 
		m_timespan=0;
#endif
	}

	void get_time()
	{
		unsigned int gtk;
#ifdef WIN32
		gtk=GetTickCount();
		if(gtk>pre_msecs)
			m_timespan=gtk-pre_msecs;
		else init();
	
		
#else
		timeval m_stime;
		gettimeofday(&m_stime,0);
		gtk=m_stime.tv_sec * 1000 + m_stime.tv_usec /1000;
		if(gtk>pre_msecs)
			m_timespan=gtk-pre_msecs;
		else init();
		
#endif
		
	}

	sdl_time operator+(unsigned int gtk)
	{
		m_timespan+=gtk;
		return *this;
	}

	bool operator<=(sdl_time st)
	{
		if((mtime+m_timespan/1000)<(st.mtime+st.m_timespan/1000)) return true;
		if((mtime+m_timespan/1000)==(st.mtime+st.m_timespan/1000) 
			&& m_timespan<=st.m_timespan) return true;
		return false;
	}

	bool operator<(sdl_time st)
	{
		if((mtime+m_timespan/1000)<(st.mtime+st.m_timespan/1000)) return true;
		if((mtime+m_timespan/1000)==(st.mtime+st.m_timespan/1000) 
			&& m_timespan<st.m_timespan) return true;
		return false;
	}

	bool operator>(sdl_time st)
	{
		if((mtime+m_timespan/1000)>(st.mtime+st.m_timespan/1000)) return true;
		if((mtime+m_timespan/1000)==(st.mtime+st.m_timespan/1000) 
			&& m_timespan>st.m_timespan) return true;
		return false;
	}

	bool operator>=(sdl_time st)
	{
		if((mtime+m_timespan/1000)>=(st.mtime+st.m_timespan/1000)) return true;
		if((mtime+m_timespan/1000)==(st.mtime+st.m_timespan/1000) 
			&& m_timespan>=st.m_timespan) return true;
		return false;
	}

	int operator-(const sdl_time &st) //返回两个时间之间的差值（毫秒单位）
	{
		int rt=(int)(mtime-st.mtime)*1000;
		rt+=(m_timespan-st.m_timespan); 
		return rt;
	};

	inline time_t time_sec()
	{
		return (mtime+m_timespan/1000);
	}
};

class sdl_timer:public sdl_thread
{
private:
	int m_time_slice; //时间片长度，毫秒单位,缺省值是2毫秒
	sdl_time real_time; //UTC的C时间
public:
	sdl_timer()
	{
		m_time_slice=2; //缺省2毫秒时间片，主要是考虑系统进程/线程切换的时间通常为1毫秒
		real_time.init(); //初始化实时时钟 
	}

	typedef sdl_signal1<sdl_time> TimeSignal;

	struct TimerRecord
	{
		sdl_time next_trigger;  //下一个触发时间
		int timer_delta; //定时器，毫秒单位,为0表示不触发
		TimeSignal TS;

		TimerRecord():next_trigger(),TS(),timer_delta(0){}; //初始化

		TimerRecord(const TimerRecord& tr)
		{
			next_trigger=tr.next_trigger;
			timer_delta=tr.timer_delta;
			TS=tr.TS;
		}
	};
private:
	list<TimerRecord> TRL;
public:

	typedef list<TimerRecord>::iterator TimerHandle;

	TimerHandle create_timer(int time_slice)
	{
		TimerRecord tmp;
		tmp.timer_delta=time_slice;
		tmp.next_trigger=real_time+time_slice;
		TRL.push_front(tmp);
		return TRL.begin(); 
	}

	void delete_timer(TimerHandle Th)
	{
		TRL.erase(Th); 
	}

	void reset_timer(TimerHandle Th,int time_slice)
	{
		Th->timer_delta=time_slice; 
	}

	void disable_timer(TimerHandle Th)
	{
		Th->timer_delta=0;
	}

	TimeSignal& get_signal(TimerHandle Th)
	{
		return Th->TS; 
	}

	virtual void begin()
	{
	#ifdef WIN32
		WSAData wsaData;
		if( WSAStartup(MAKEWORD(2,2), &wsaData) != 0 )
			throw "Error in WSAStartup";
	#endif      
		int server = socket(AF_INET, SOCK_DGRAM, 0); //创建一个UDP套接口
		unsigned int ptime=0; 

		real_time.init(); //初始化实时时钟 
		while(true)
		{
			fd_set rfds;
			struct timeval tv;
			FD_ZERO(&rfds);
			FD_SET (server, &rfds);
			tv.tv_sec = 0;
			tv.tv_usec =  m_time_slice*1000; //缺省定时器
			FD_SET (server, &rfds);
			int retval = select(server+1, &rfds, NULL, NULL, &tv);
	
			real_time.get_time(); //获取一次实时的时钟 
			list<TimerRecord>::iterator it=TRL.begin();
			while(it!=TRL.end())
			{
				if(it->next_trigger.is_invailid()) {++it;continue;}
				if(it->timer_delta>m_time_slice*16) //定时器的最小间隔必须超过时间片的16倍，缺省是32毫秒
				{
					if(real_time>=it->next_trigger) 
					{
						it->TS(real_time); //注意，触发器触发的进程如果执行时间大于32毫秒，可能导致定时器无效
						it->next_trigger=it->next_trigger+it->timer_delta; //重置触发器
					}
				}
				++it;
			}
		}
	}
};

static int sdl_winsocket_start=false; //only used by windows

struct sdl_link
{
	int socket_id;
	char client_ip[16];
	int client_port;
	int active;
	int timeout;
	time_t time;

	inline void read(string &value)
	{
		static char read_buf[LINE_BUF_SIZE];
		int readed = ::recv(socket_id, read_buf, LINE_BUF_SIZE,0);
		value.assign(read_buf,readed);
	};
	inline void write(string &value)
	{
		::send(socket_id,value.c_str(),value.length(),0);
	};

	string get_address()
	{
		char buf[16];
		string rt=client_ip;
		rt+=':';
		sprintf(buf,"%d",client_port);
		rt+=buf;
		return rt;
	}
};

class sdl_tcp_stream:public streambuf
{

protected:
	char read_buf[STREAM_BUF_SIZE];
	char write_buf[STREAM_BUF_SIZE]; //buffer for read and write

	struct sockaddr_in addr; //IP address struct

	int sock_id;      //-1=failed create socket handle
	int conn_state;   //connected state 0=sucess -1=failed

	int listen_port; //if it active as a server,this is the port that the server listen

	struct sdl_link client_addr; //the client of request connect

	bool flag_stop_loop;
private:

	timeval time_out;    //seconds of read or write timeout,
	//if less 1 second,will block there
	//---------Add in DOD project------
	timeval read_tmp_time;
	timeval write_tmp_time;
	//---------------------------------
	fd_set rwfds;        //used by time out;


	//add for muti-thread connect   
	//string hostname; //the hostname to link 
	//int hostport;    //the host's port

	int ThreadID;

public:
	enum type {
		sock_stream     = SOCK_STREAM,
		sock_dgram  = SOCK_DGRAM,
		sock_raw    = SOCK_RAW,
		sock_rdm    = SOCK_RDM,
		sock_seqpacket  = SOCK_SEQPACKET
	};

	enum option {
		so_debug    = SO_DEBUG,
		so_acceptconn   = SO_ACCEPTCONN,
		so_reuseaddr    = SO_REUSEADDR,
		so_keepalive    = SO_KEEPALIVE,
		so_dontroute    = SO_DONTROUTE,
		so_broadcast    = SO_BROADCAST,
#ifdef WIN32
		so_useloopback  = SO_USELOOPBACK,
#endif
		so_linger   = SO_LINGER,
		so_oobinline    = SO_OOBINLINE,
		so_sndbuf   = SO_SNDBUF,
		so_rcvbuf   = SO_RCVBUF,
		so_sndlowat     = SO_SNDLOWAT,
		so_rcvlowat     = SO_RCVLOWAT,
		so_sndtimeo     = SO_SNDTIMEO,
		so_rcvtimeo     = SO_RCVTIMEO,
		so_error    = SO_ERROR,
		so_type     = SO_TYPE,
		tcp_nodelay = TCP_NODELAY
	};  

	enum level { sol_socket = SOL_SOCKET,ipproto_tcp = IPPROTO_TCP };

	enum msgflag {
		msg_oob     = MSG_OOB,
		msg_peek    = MSG_PEEK,
#ifndef WIN32
		msg_dontwait = MSG_DONTWAIT,
#endif
		msg_dontroute   = MSG_DONTROUTE
#ifdef WIN32
		,
		msg_maxiovlen   = MSG_MAXIOVLEN
#endif
	};

	inline bool is_connected(){return conn_state==0;}

	inline bool is_read_ready();

	inline bool is_write_ready();

	void setID(int ID) {ThreadID=ID;};
	int getID(){return ThreadID;};

	void close_nagel()
	{
		int opval=1;
		setopt(tcp_nodelay,&opval,sizeof(opval),ipproto_tcp);
	};

	void open_nagel()
	{
		int opval=0;
		setopt(tcp_nodelay,&opval,sizeof(opval),ipproto_tcp);
	};

	void setopt (option op, void* buf, int len, level l) const
	{
#ifdef WIN32
		if (::setsockopt (sock_id, l, op, (char*) buf, len) == -1)
#endif

#ifdef _sdl_linux
			if (::setsockopt (sock_id, l, op, (char*) buf, ((socklen_t)len)) == -1)
#endif

#ifdef _sdl_unix
				if (::setsockopt (sock_id, l, op, (char*) buf, len) == -1)
#endif
					throw sdl_exception("set socket option error! --Li da gang");
	};

	int getopt (option op, void* buf, int len, level l) const
	{
		int rlen = len;

#ifdef WIN32
		if (::getsockopt (sock_id, l, op, (char*) buf, &rlen) == -1)
#endif

#ifdef _sdl_linux
			if (::getsockopt (sock_id, l, op, (char*) buf,  (socklen_t*)&rlen) == -1)
#endif

#ifdef _sdl_unix
				if (::getsockopt (sock_id, l, op, (char*) buf, &rlen) == -1)
#endif

					throw sdl_exception("get socket option error! --Li da gang");
		return rlen;
	};

	//!used by server socket ,get client socket infomation
	inline void set_client_addr(sdl_link &v){client_addr=v;};
	//!set socket timout seconds,n
	void set_stream_timeout(int seconds);
	//direct read data to user's buffer 
	int read_data(char *buf,int size,int time_out);
	int read_nonblock(char *buf,int bufsize);
	void stop_server(){flag_stop_loop=true;}
	//!wait for data come in ,if data come, on_receive will be execute
	//!if no set timeout ,will wait until process end
	int wait(int timeout_seconds=0);

	//!can overload
	virtual int on_receive(int socket){ return true;};

	//!constructor
	sdl_tcp_stream(int timeout=3600);//! init tcp 
	~sdl_tcp_stream();

	//!stream function
	virtual int overflow(int c);

	//!stream function
	virtual int underflow();//if get area is null,get data from socket

	//!stream function
	virtual int sync();

	//!read data from socket
	inline int read_socket();

	//write data in write buf to socket
	inline int write_socket();

	bool set_nonblock() //set non-block socket
	{

#ifdef	_WIN32
		unsigned long on = 1;
		return (ioctlsocket(sock_id,FIONBIO,&on)==0);
#else
		int	flags;
		if ((flags = fcntl(sock_id, F_GETFL, 0))!=-1 && fcntl(sock_id, F_SETFL, flags | O_NONBLOCK)==0) return true;
		return false;
#endif 
	}
	//!set the listen port
	inline void set_listen_port(int port){listen_port=port;};
	//!begin listen,auto create a socket to listen, if set port, use new port listen
	int begin_listen(int port=0);

	//!as a client to connect server,must use domain-name,no use IP address
	//if success return true,else return false
	bool connect_name(const char* host,int port,int retry_times=3); 
	bool connect_ip(const char* ip,int port,int retry_times=3);
	bool connect_host(const char* host,int port,int retry_times=3); 
	int write_direct(const char *pdata,int len);
	//----------------------------------------------------
	//--add by Li Gang 2002-8-13 ,used to muti-thread link
	/*	
	void set_host_port(const char *host_name,int host_port)
	{
		hostport=host_port;
		hostname=host_name;
	};

	//!as a client to connect server,must use domain-name,no use IP address
	//if success return true,else return false
	inline int connect_host()
	{
		return connect_host(hostname,hostport);
	};
	*/
	//----------------------------------------------------
	
	int accept (sdl_tcp_stream& socket ); 

	void throw_away_data();

	//set the socket handle
	inline void set_sockid(int sock) {sock_id=sock;};

	//close the tcp connection  
	int close_connect();

	//make the stream clean,he will clear the data buffer   
	void clear_stream()
	{
		conn_state=-1;
		write_buf[0]=0;
		read_buf[0]=0;
		streambuf::setp((char *)write_buf,(char *)write_buf+STREAM_BUF_SIZE); //set put area
		streambuf::setg(read_buf,read_buf,read_buf); //set get area
	};
private:
	//create a null socket
	int create();

	//bind port to a null port
	int bind (const int port );

	//!listen
	int listen() const;
};

// Template Argument Description
//---------------------------
// Pipe::         this is a custom class that must inherit sdl_pipe class
// MaxThreads::   the max numbers of active thread.because each client have a service thread,this parameter means the max numbers of links
// Port::         the server's listening port
// Time_out::     the max waiting seconds of each thread 
///



template<class Pipe,int MaxThreads,int Port=7310,int Time_out=15>
class sdl_tcpserver:public sdl_thread,public sdl_tcp_stream
{
protected:
	int count;
	Pipe *pools[MaxThreads]; //thread_data pool
	sdl_tcp_stream *refuse_pipe; //used closed user request
	int Total_stared_thread;
	sdl_mutex mutex;
	int current_thread; //当前获得执行的线程编号

public:

	sdl_signal1<int> ConnectedSignal; //服务器连接上信号
	sdl_signal1<int> DisconnectedSignal; //服务器中断信号

	sdl_tcpserver():sdl_tcp_stream(),sdl_thread()
	{
		refuse_pipe = NULL;
		Total_stared_thread=0;
		count=-1;
		for(int i=0;i<MaxThreads;i++)
			pools[i]=NULL;  //set pool null;    
	};

	~sdl_tcpserver()
	{
		Pipe **p_stream;
		for(int i=0;i<MaxThreads;i++)
		{
			p_stream=pools+i;
			if(*p_stream!=NULL)
			{
#ifdef WIN32
				(*p_stream)->kill_thread(); //在Windows下，中止尚在运行的线程
#endif
				delete (*p_stream);
			}
		}
#ifdef WIN32
		kill_thread(); //在Windows下，如果自己还在监听，中止自己的线程
#endif
	};
#ifdef WIN32
	void thread_set_active(int handle,unsigned int thread_id)
	{
		
		ConnectedSignal.emit(handle);  //发出建链成功的信号
	}

	void thread_set_inactive(int handle,unsigned int thread_id)
	{
		DisconnectedSignal.emit(handle);  //发出断链的信号
	}
#else
	void thread_set_active(int handle,pthread_t thread_id)
	{
		
		ConnectedSignal.emit(handle);  //发出建链成功的信号
	}

	void thread_set_inactive(int handle,pthread_t thread_id)
	{
		DisconnectedSignal.emit(handle);  //发出断链的信号
	}
#endif
	int on_receive(int socket)  
	{
		Pipe **p_stream;

		for(current_thread=0;current_thread<MaxThreads;current_thread++)
		{
			p_stream=pools+current_thread;
			//if a thread finished his operation,the thread will set himself unactive 
			if((*p_stream==NULL) || !(*p_stream)->is_active()) 
			{
				if((*p_stream)==NULL)   //alloc new pipe data area,and delete unused data
				{
					(*p_stream)=new Pipe(Time_out); //需要初始化等待时间
					connect((*p_stream)->ThreadStartSignal,*this,&sdl_tcpserver::thread_set_active); 
					connect((*p_stream)->ThreadStopSignal,*this,&sdl_tcpserver::thread_set_inactive); 
				}
				else 
				{
					delete (*p_stream); //由于删除线程执行对象自然删除了信号，所以此处不做disconnect
					//Dout<<"Delete thread:"<<i<<endl;
					(*p_stream)=new Pipe(Time_out);  //需要初始化等待时间
					connect((*p_stream)->ThreadStartSignal,*this,&sdl_tcpserver::thread_set_active); 
					connect((*p_stream)->ThreadStopSignal,*this,&sdl_tcpserver::thread_set_inactive); 
					//(*p_stream)->clear_stream();
					//(*p_stream)->release();
				}
				if((*p_stream)==NULL) return false;//if alloc memory failed ,return
				accept((*(sdl_tcp_stream *)(*p_stream)));
				(*p_stream)->set_client_addr(client_addr);  //set client data
				(*p_stream)->set_active();  //pipe is alive now
				(*p_stream)->rec_time();  //record the begin time;
				(*p_stream)->setID(Total_stared_thread);  //set Thread ID Number
				(*p_stream)->handle_id=current_thread; //记录必要的线程池编号
				(*p_stream)->start_thread(); //start client process thread
				Total_stared_thread++;
				return true;
			}
		}
		if(refuse_pipe==NULL)
			refuse_pipe = new sdl_tcp_stream(Time_out);
		accept(*refuse_pipe);  //refuse connected
		refuse_pipe->close_connect();
		return true;
	};

	//there are two method to start the server,use begin() can start the server,but process will block
	//if you hope that the process continue,use start_thread() start the server
	void begin()  //overload function
	{
		set_active();  //设置监听线程处于工作状态
		count=0;
		begin_listen(Port);
		sdl_tcp_stream::wait();  //wait for client
		set_inactive();  //设置监听线程不工作
	};

	bool is_working()
	{
		if(count>-1) return true;
		return false;
	}

};



class sdl_tcp_simple_server:public sdl_tcp_stream
{
protected:
	sdl_tcp_stream accept_stream;
	istream *_p_istream;
	ostream *_p_ostream;
	int Port;
public:
	
	sdl_signal1<int> ConnectedSignal; //服务器连接上信号
	sdl_signal1<int> DisconnectedSignal; //服务器中断信号

	sdl_tcp_simple_server(int port=7310):sdl_tcp_stream()
	{
		Port=port;
		_p_istream=NULL;
		_p_ostream=NULL;
		
	};

	~sdl_tcp_simple_server()
	{
		if(_p_istream==NULL)
		{
			delete _p_istream;
			delete _p_ostream;
		}
	};

	int on_receive(int socket)  
	{
		accept((*(sdl_tcp_stream *)(&accept_stream)));
		accept_stream.set_client_addr(client_addr);  //set client data
		_p_istream=new istream((sdl_tcp_stream*)(&accept_stream));
		_p_ostream=new ostream((sdl_tcp_stream*)(&accept_stream));
		process(); //执行网络连接
		accept_stream.close_connect(); //关闭网络连接 
		return true;
	};

	virtual void process(){};

	bool is_data_in()
	{
		if(_p_istream==NULL) return false;
		return accept_stream.is_read_ready();
	}

	bool is_data_out()
	{
		if(_p_ostream==NULL) return false;
		return accept_stream.is_write_ready();
	}
	
	void listen(int sever_time_out=-1)  //缺省-1，标识一直等待
	{
		sdl_tcp_stream::begin_listen(Port);
		sdl_tcp_stream::wait(sever_time_out);  //wait for client
	
	};

	void start_server(int port=7310,int sever_time_out=-1)  //缺省-1，标识一直等待
	{
		Port=port;
		sdl_tcp_stream::begin_listen(Port);
		//wait for client
		fd_set rfds;
		int retval;
		/* Watch sock_id to see when it has input. */
		FD_ZERO(&rfds);
		FD_SET(sock_id, &rfds);
		retval = select(sock_id+1, &rfds, NULL, NULL, NULL);
		if(retval && FD_ISSET(sock_id, &rfds))
		{
			accept((*(sdl_tcp_stream *)(&accept_stream)));
			accept_stream.set_nonblock();  //设定为非阻塞模式
			accept_stream.set_stream_timeout(sever_time_out); 
			accept_stream.set_client_addr(client_addr);  //set client data
			_p_istream=new istream((sdl_tcp_stream*)(&accept_stream));
			_p_ostream=new ostream((sdl_tcp_stream*)(&accept_stream));
		}
		else
		{
			_p_istream=NULL;
			_p_ostream=NULL;
		}
	};

	void close_server()
	{
		accept_stream.close_connect(); 
		sdl_tcp_stream::close_connect(); 
		if(_p_istream==NULL)
		{
			delete _p_istream;
			delete _p_ostream;
		}
	}

};

struct sdl_line
{
private:
	char *buf;
	int size;
	int buf_length;
public:
	sdl_line()
	{
		size=1024;
		buf_length=0;
		buf=new char[size];
	};

	sdl_line(sdl_line &line)
	{
		size=line.size;
		buf_length=line.buf_length;
		buf=new char[size];
		memcpy(buf,line.buf,size); 
	}

	~sdl_line()
	{
		delete[] buf;
	}

	bool readln(istream &in)
	{
		char c=0;

		buf_length=0;
		while(true)
		{
			in.get(c); 
			if(!in || c=='\n') break;
			buf[buf_length]=(char)c;
			++buf_length;
			if(buf_length>=size-1)  resize();
		}
		buf[buf_length]=0; //设定c str
		while(buf_length>0 && buf[buf_length-1]=='\r') buf[--buf_length]=0; //去除/r,设定c str
		if(!in && buf_length==0) return false;
		return true;
	};

	void clear(){buf_length=0;buf[0]=0;}

	bool readln(FILE *in)
	{
		int c=0;
		buf_length=0;
		while(true)
		{
			c=fgetc(in);
			if(c=='\n' || c==EOF) break;
			buf[buf_length]=(char)c;
			++buf_length;
			if(buf_length>=size-1)  resize();
		}
		buf[buf_length]=0; //设定c str
		while(buf_length>0 && buf[buf_length-1]=='\r') buf[--buf_length]=0; //去除/r,设定c str
		if(c==EOF && buf_length==0) return false;
		return true;
	};

	bool read_all(FILE *in)
	{
		int c=0;
		buf_length=0;
		while(true)
		{
			c=fgetc(in);
			if(c==EOF) break;
			buf[buf_length]=(char)c;
			++buf_length;
			if(buf_length>=size-1)  resize();
		}
		buf[buf_length]=0; //设定c str
		return true;
	};

	void writeln(ostream &out)
	{
		if(buf_length>0)
		{
			out.write(buf,buf_length);
			out<<endl;
		}
	}

	void printf(const char *s, ...) //在标准输出打印调试信息
	{
		va_list arglist; 
		va_start(arglist, s); 
		buf_length+=vsnprintf(buf+buf_length,size-buf_length,s,arglist); 
		va_end(arglist); 
	}

	char operator[](int n)
	{
		return n<buf_length?buf[n]:0;
	}

	void write(const char* pstr,int length=-1)
	{
		if(length==-1) length=strlen(pstr);
		if(buf_length+length>=size)  //在缓冲区内
		{//扩展缓冲区
			char *new_buf=new char[size*2];
			memcpy(new_buf,buf,size);
			size=size*2;
			delete buf;
			buf=new_buf;
		}
		memcpy(buf+buf_length,pstr,length);
		buf_length+=length;
	}

	inline void resize()
	{
		char *new_buf=new char[size*2];
		memcpy(new_buf,buf,size);
		size=size*2;
		delete[] buf;
		buf=new_buf;
	}

	inline void put(char ch)
	{
		buf[buf_length]=ch;
		++buf_length;
		if(buf_length>=size-1)  resize();//在缓冲区内
	}

	inline void operator+=(char ch){put(ch);}

	void pop() {if(buf_length>0) *(buf+(--buf_length))=0;}

	char& back(){return buf_length>0?*(buf+buf_length-1):(*buf);}

	bool is_blank()
	{
		if(buf_length==0) return true;
		for(int i=0;i<buf_length;++i)
			if(buf[i]>='0') return false;
		return true;
	};

	string get_line(){return string(buf);};

	char* get_buf(){return buf;};

	char* data(){buf[buf_length]=0;return buf;};

	char* c_str(){buf[buf_length]=0;return buf;};

	bool empty(){return (buf_length==0);}

	int get_buffer_size(){return size;};

	int length(){return buf_length;};

	void set_length(int n){buf_length=n<size?n:size;};
};

istream& operator >>(istream &in,sdl_line &line);

ostream& operator <<(ostream &out,sdl_line &line);

//注意，重载begin()函数后，不要忘记使用sdl_tcp_stream::close_connect()中止TCP连接
class sdl_tcp_pipe:public sdl_tcp_stream,public sdl_thread
{
private:
	sdl_mutex *p_mutex;
	time_t begin_time;
protected:
	istream *_p_istream;
	ostream *_p_ostream;
public:
	sdl_tcp_pipe(int timeout=3600):
	  sdl_tcp_stream(timeout)
	  {
		  p_mutex=NULL;
		  _p_istream=NULL;
		  _p_ostream=NULL;
	  };
	  void init_mutex(sdl_mutex &mu){p_mutex=&mu;};
	  void lock(){if(p_mutex!=NULL) p_mutex->lock();}; //lock shared data
	  void unlock(){if(p_mutex!=NULL) p_mutex->unlock();};// unlock shared data
	  sdl_link& get_client(){return client_addr;}; //return remote host information
	  void rec_time(){time(&begin_time);};
	  int span_time(){time_t now_time;time(&now_time);return((int)now_time-(int)begin_time);};

	  //overload function from sdl_thread ,you can use it to write your process
	  virtual void begin()
	  {
		  _p_istream=new istream((sdl_tcp_stream*)this);
		  _p_ostream=new ostream((sdl_tcp_stream*)this);

	  };

	  ~sdl_tcp_pipe()
	  {
		  if(_p_istream==NULL) delete _p_istream;
		  if(_p_ostream==NULL) delete _p_ostream;
	  }
};

template<typename _predicate> //谓词类用于直接使用输入输出流
class sdl_client_pipe:public sdl_tcp_stream,public sdl_thread
{
private:
	sdl_mutex *p_mutex;
	time_t begin_time;
	istream tin;
	ostream tout;
	_predicate conn;
public:
	sdl_client_pipe(const char *host,int port,int timeout=30):
	  sdl_tcp_stream(timeout),tin((sdl_tcp_stream*)this),tout((sdl_tcp_stream*)this)
	  {
		  p_mutex=NULL;
		  //sdl_tcp_stream::set_host_port(host,port);
	  };
	  void init_mutex(sdl_mutex &mu){p_mutex=&mu;};
	  void lock(){if(p_mutex!=NULL) p_mutex->lock();}; //lock shared data
	  void unlock(){if(p_mutex!=NULL) p_mutex->unlock();};// unlock shared data
	  sdl_link& get_client(){return client_addr;}; //return remote host information
	  void rec_time(){time(&begin_time);};
	  int span_time(){time_t now_time;time(&now_time);return((int)now_time-(int)begin_time);};

	  //overload function from sdl_thread ,you can use it to write your process
	  virtual void begin()
	  {
		if(sdl_tcp_stream::connect_host())
		  {
			  rec_time();
			  conn(tin,tout);
			  sdl_tcp_stream::close_connect();
		  }
		  sdl_tcp_stream::close_connect();
	  };
};

template<typename _predicate>
class sdl_telnet_pipe:public sdl_tcp_stream
{
private:
	time_t begin_time;
	int span_seconds;
	istream tin;
	ostream tout;
	_predicate conn;
	bool flag_ok;
public:
	sdl_telnet_pipe(const char *address,int port,int timeout=30):
	  sdl_tcp_stream(timeout),tin((sdl_tcp_stream*)this),tout((sdl_tcp_stream*)this)
	  {
		  span_seconds=0;
		  if(sdl_tcp_stream::connect_host(address,port))
		  { 
			  rec_time();
			  conn(tin,tout);
			  sdl_tcp_stream::close_connect();
			  span_seconds=span_time();
			  flag_ok=true;
		  }
		  else 
		  {
			  flag_ok=false;
			  //Dout<<"Cann't Connect Host!"<<endl;
		  }
	  };
	  void rec_time(){time(&begin_time);};
	  int span_time(){time_t now_time;time(&now_time);return((int)now_time-(int)begin_time);};
	  int get_span(){return span_seconds;};
	  bool is_ok(){return flag_ok;}
};

#ifndef WIN32
#define SOCKET_ERROR -1
#endif 

class InPacket;
class OutPacket;

#define UDP_BUF_SIZE 1024
class sdl_udp
{
private:

#ifdef WIN32
	SOCKET server;
#else
	int server;
#endif
	sockaddr_in local,remote,from;
	sockaddr client;
	int      socket_flag;
	int      remote_port;
protected:
	char *temp_buf; //buffer for read and write
	timeval time_out;    //seconds of read or write timeout,
public:

	char *read_buf;
	int read_len;
	char *write_buf; //buffer for read and write
	int write_len;

	sdl_udp()
	{
		read_buf=NULL;
		write_buf=NULL;
		temp_buf=NULL;
		read_len=0;
		socket_flag=0;
		write_len=UDP_BUF_SIZE;
		time_out.tv_sec=0;
		time_out.tv_usec=0; 
	};

	~sdl_udp()
	{
		if(read_buf!=NULL) delete read_buf;
		if(write_buf!=NULL) delete write_buf;
		if(temp_buf!=NULL) delete temp_buf;
	}

	inline void reset()
	{
		read_len=0;
		write_len=UDP_BUF_SIZE;
	};

	inline void set_monitor(fd_set &rwfds)
	{
		FD_CLR (server, &rwfds);
		FD_SET (server, &rwfds);
	};

	inline void remove_monitor(fd_set &rwfds)
	{
		FD_CLR (server, &rwfds);
	};

	void set_remote(const char *remote_ip,int remote_port)
	{
		memset(&remote, 0, sizeof(local));
		remote.sin_family = AF_INET;
		remote.sin_port = htons(remote_port);
		remote.sin_addr.s_addr=inet_addr(remote_ip);
	};

	bool read_msg()
	{
		int size_of_addr=sizeof(sockaddr);
#ifdef WIN32
		read_len = ::recvfrom(server, read_buf, UDP_BUF_SIZE,socket_flag,(sockaddr*)&from,(int *)(&size_of_addr));
#else
		read_len = ::recvfrom(server, read_buf, UDP_BUF_SIZE,socket_flag,(sockaddr*)&from,(socklen_t *)(&size_of_addr));
#endif
		
		if(read_len<0) return false;
		return true;
	};

	const char* get_from_ip(){return inet_ntoa((in_addr)from.sin_addr);}

	int get_from_port(){return from.sin_port;}

	int get_port(){return remote_port;}

	void send_msg(char *buf,int len)
	{
		if (sendto(server,buf,len,0,(struct sockaddr*)&remote,sizeof(sockaddr))==SOCKET_ERROR)
			throw "Send UDP Failed";
	};

	void send_msg(string &msg)
	{
		if (sendto(server,(char*)(msg.c_str()),msg.length(),0,(struct sockaddr*)&remote,sizeof(sockaddr))==SOCKET_ERROR)
			throw "Send UDP Failed";
	};
	void send_msg()
	{

		if (sendto(server,write_buf,write_len,0,(struct sockaddr*)&remote,sizeof(sockaddr))==SOCKET_ERROR)
			throw "Send UDP Failed";
	};

	inline bool is_ready(fd_set &rwfds)
	{
		return (FD_ISSET(server,&rwfds)!=0);
	}

	bool set_nonblock()
	{

	#ifdef	_WIN32
		unsigned long on = 1;
		return (ioctlsocket(server,FIONBIO,&on)==0);
	#else
		int	flags;
		socket_flag=MSG_DONTWAIT;
		if ((flags = fcntl(server, F_GETFL, 0))!=-1 && fcntl(server, F_SETFL, flags | O_NONBLOCK)==0) return true;
		return false;
	#endif 
	}

	bool init(const char *local_ip,int local_port);  // 初始化输出用的Socket

	void init_broadcast(int remote_port);  

	void init_multicast(const char *remote_ip,int remote_port,const char* local_ip=NULL);

};

#define MAX_SERVER_NUM 44001
#define DEAMON_PORT_BEGIN 4000
#define HEART_BEAT_TIME  3 //心跳时间为3秒一次

struct sdl_udp_bt_head
{
	unsigned short ID;
	unsigned short Version;
	unsigned short Cmd;
	unsigned short Seq;
};

#define LIG_CMD_KEEP_ALIVE 0x0011;
#define LIG_CMD_MESSAGE 0x0000;
#define LIG_CMD_STREAM 0x0001;
class sdl_udp_deamon
{
private:
	fd_set rwfds;
	timeval time_out;
	int m_Port_count;
	string local_ip;
	int system_heartbeat; //系统心跳
	
public:
	sdl_time sys_time;
	vector<sdl_udp*> udp_pool;
	sdl_signal1<int> sig_heart;  //心跳信号

	sdl_callbase<sdl_udp*>* p_sig_msg_in;  //有数据进入,回调对象指针，回调函数支持
					
	template<typename Ts>
	void connect(Ts *pobj, void (Ts::*f)(sdl_udp* arg)) //回调支持函数
	{
		if(p_sig_msg_in!=NULL) delete p_sig_msg_in; //删除不使用的回调
			p_sig_msg_in = new sdl_function<Ts,sdl_udp*>(pobj,f); //每次只能连接一个回调
	};
			
	void connect(void (*f)(sdl_udp*)) //回调支持函数
	{
		if(p_sig_msg_in==NULL) 
			p_sig_msg_in = new sdl_callbase<sdl_udp*>; //每次只能连接一个回调
		p_sig_msg_in->c_callback=f;
	};

private:
	char wr_buf[UDP_BUF_SIZE*2];
	unsigned int m_seq_counter;
public:
	void clear_fdset()
	{
		FD_ZERO (&rwfds);
	};

	sdl_udp_deamon():udp_pool()
	{
		local_ip=get_local_ip();
		time_out.tv_sec=1;
		time_out.tv_usec=0; 
		m_Port_count=DEAMON_PORT_BEGIN;
		FD_ZERO (&rwfds);
		system_heartbeat=0;
		m_seq_counter=0;
		p_sig_msg_in=NULL;
		udp_pool.clear(); 
		sys_time.init(); 
	}

	
	virtual void heart_beat()
	{
		++system_heartbeat;
		sig_heart(system_heartbeat);
		//   Dout<<"SYS_HEART_BEAT "<<system_heartbeat<<endl;
	};

	inline bool waiting_for()
	{
		bool rt=false;
		timeval read_tmp_time=time_out; //used to reserver origin time_out value,select will change the argument
		int ret=select (MAX_SERVER_NUM, &rwfds, 0, 0, &read_tmp_time);
		if (ret != SOCKET_ERROR) 
			for(vector<sdl_udp*>::iterator i=udp_pool.begin(); i!=udp_pool.end();++i)
			{
				//Dout<<"TEST:: Time Out or Data coming......"<<endl; //心跳显示包就不要了
				if((*i)->is_ready(rwfds))
				{
					if((*i)->read_msg()) 
					{
						p_sig_msg_in->call(*i);  //Send Packet in Signal
						rt=true;
					}
				}
				(*i)->set_monitor(rwfds); 
			}

			sys_time.get_time(); 
			if(((long)(sys_time.time_sec()) % HEART_BEAT_TIME)==0) heart_beat();

			return rt;
	};

	inline bool waiting_for(int msecs)
	{
		bool rt=false;

		struct timeval read_tmp_time;
		read_tmp_time.tv_sec=msecs/1000;
		read_tmp_time.tv_usec=msecs%1000;
		int ret=select (MAX_SERVER_NUM, &rwfds, 0, 0, &read_tmp_time);

		if (ret != SOCKET_ERROR) 
			for(vector<sdl_udp*>::iterator i=udp_pool.begin(); i!=udp_pool.end();++i)
			{
				//Dout<<"TEST:: Time Out or Data coming......"<<endl; //心跳显示包就不要了
				if((*i)->is_ready(rwfds))
				{
					if((*i)->read_msg())
					{
						p_sig_msg_in->call(*i);  //Send Packet in Signal
						rt=true;
					}
				}
				(*i)->set_monitor(rwfds); 
			}
		sys_time.get_time(); 
		if(((long)(sys_time.time_sec()) % HEART_BEAT_TIME)==0) heart_beat();
		return rt;
	};

	void broadcast_send(char *buf,int buf_length)
	{
		for(vector<sdl_udp*>::iterator i=udp_pool.begin(); i!=udp_pool.end();++i)
			(*i)->send_msg(buf,buf_length);
	};

	void write_to(char *buf,int buf_length)
	{
		sdl_udp_bt_head *p_head=(sdl_udp_bt_head *)wr_buf;
		p_head->ID=0x0002;
		p_head->Version=0x0001;
		p_head->Cmd=LIG_CMD_STREAM;
		p_head->Seq=m_seq_counter % 65536;
		++m_seq_counter;
		memcpy(wr_buf+sizeof(sdl_udp_bt_head),buf,buf_length);
		for(vector<sdl_udp*>::iterator i=udp_pool.begin(); i!=udp_pool.end();++i)
			(*i)->send_msg(wr_buf,buf_length+sizeof(sdl_udp_bt_head));
	}


	sdl_udp* new_multicast(const char* remote_ip,int remote_port)
	{
		sdl_udp *p=new sdl_udp;
		if(p==NULL) return NULL;
		p->init_multicast(remote_ip,remote_port);
		p->set_nonblock(); //要求无阻塞调用 
		p->set_monitor(rwfds);
		udp_pool.push_back(p);
		return p;
	}

	sdl_udp* new_broadcast(int remote_port)
	{
		sdl_udp *p=new sdl_udp;
		if(p==NULL) return NULL;
		p->init_broadcast(remote_port);
		p->set_nonblock(); //要求无阻塞调用 
		p->set_monitor(rwfds);
		udp_pool.push_back(p);
		return p;
	}

	sdl_udp* new_udp(int port=0)
	{
		bool rt;
		sdl_udp *p=new sdl_udp;
		if(p==NULL) return NULL;
		if(port==0)
			rt=p->init(local_ip.c_str() ,m_Port_count);
		else
			rt=p->init(local_ip.c_str() ,port);

		//try  first time
		if(!rt)
		{
			if(port==0)
				rt=p->init(local_ip.c_str() ,m_Port_count);
			else
				rt=p->init(local_ip.c_str() ,port);
		}
		//try second time

		if(!rt)
		{
			if(port==0)
				rt=p->init(local_ip.c_str() ,m_Port_count);
			else
				rt=p->init(local_ip.c_str() ,port);
		}
		//try third time
		p->set_nonblock(); //要求无阻塞调用 
		p->set_monitor(rwfds);
		udp_pool.push_back(p);

		if(port==0) ++m_Port_count;
		return p;

	}

};

class sdl_udp_stream:public streambuf
{
private:
#ifdef WIN32
	SOCKET server;
#else
	int server;
#endif
	sockaddr_in local,remote;
	sockaddr client;
	sockaddr_in ha_master,ha_slave;

	char *read_buf;
	char *write_buf; //buffer for read and write
	char *temp_buf; //buffer for read and write

	fd_set rwfds;        //used by time out;

	timeval time_out;    //seconds of read or write timeout,

	
	unsigned long pre_msg_seq_num; //used for debug dod multicast stream

public:
	int msg_header_length;  //收到的每个UDP报文，需要去掉的头长度，缺省为0

	const char* get_remote_ip()
	{
		return (const char*)(inet_ntoa(remote.sin_addr));
	};

	sdl_udp_stream(int timeout=300)
	{
		read_buf=NULL;
		write_buf=NULL;
		temp_buf=NULL;
		msg_header_length=0;
		pre_msg_seq_num=0;
		if(timeout>0)
		{
			time_out.tv_sec=timeout;  //设置超时期限
			time_out.tv_usec=0; 
		}
	};

	~sdl_udp_stream()
	{
		close();
		if(read_buf!=NULL) delete read_buf;
		if(write_buf!=NULL) delete write_buf;
		if(temp_buf!=NULL) delete temp_buf;

		read_buf=NULL;
		write_buf=NULL;
		temp_buf=NULL;
	}

	void set_timeout(int timeout)
	{
		if(timeout>0)
		{
			time_out.tv_sec=timeout;  //设置超时期限
			time_out.tv_usec=0; 
		}
	}

	void set_msg_header(int len){msg_header_length=len;};

	int sync()
	{
		overflow(EOF);
		return 0;
	};

	void throw_away_data()  
	{
		streambuf::setg(read_buf,read_buf,0);
	};

	void init(const char *remote_ip,int remote_port,const char *local_ip,int local_port,int timeout=0);

	void init_broadcast(int remote_port);  

	void init_multicast(const char *remote_ip,int remote_port,int timeout=0);

	void init_udp(const char *listen_ip,int listen_port,int timeout=0);

	void init_udp(int listen_port,int timeout=0);

	inline int write_socket()  //this functions is not suitable for udp stream, don't use it
	{
		int bytes_left; 
		int written_bytes; 
		char *ptr; 

		ptr=write_buf; 
		bytes_left=(int)((char *)pptr()-write_buf); 

		// Dout<<"Socket is ready to write! --Li da gang"<<endl;
		while(bytes_left>0) 
		{ 
			if(!is_write_ready()) 
			{
				//throw sdl_exception("Socket write timeout! --Li da gang");
				//                Dout<<"Socket write timeout! --Li da gang"<<endl;
				return false;
			}

			written_bytes=::sendto(server,ptr,bytes_left,0,(const sockaddr*)(&remote),sizeof(remote));
			if(written_bytes<=0) 
			{  
#ifdef WIN32
				return false;
#else
				if(errno==EINTR) 
					written_bytes=0; 
				else 
				{
					//throw sdl_exception("Can't write TCP Stream! --li da gang");
					//                    Dout<<"Socket closed by Client! --Li da gang"<<endl;
					return false; 
				}
#endif
			} 
			bytes_left-=written_bytes; 
			ptr+=written_bytes;    
		}    
		streambuf::setp(write_buf,write_buf+UDP_BUF_SIZE); //set new read buf

		return true;
	};

	inline void close()
	{
#ifdef WIN32
		::closesocket(server);
#else
#endif
	};

	int overflow(int c)
	{
		if(c != EOF)
		{
			*pptr() = (unsigned char)c;
			pbump(1);
		}
		if(write_socket()) 
			return c;
		else
			close(); 

		//  Dout<<"overflow failed,stream failed!"<<endl;
		return EOF;
	};
	//!stream function
	int underflow()//if get area is null,get data from socket
	{
		if(read_socket())   
			return (unsigned char)(*gptr());
		else
			close();

		// Dout<<"underflow failed,stream failed!"<<endl;
		return EOF;
	};

	inline bool read_socket()
	{   
		int readed;
		int size_of_addr=sizeof(sockaddr);


		if(!is_read_ready()) 
		{
			return false;
		}
#ifdef WIN32
		readed = ::recvfrom(server, temp_buf, UDP_BUF_SIZE,0,(sockaddr*)&remote,(int *)(&size_of_addr));
#else
		readed = ::recvfrom(server, temp_buf, UDP_BUF_SIZE,0,(sockaddr*)&remote,(socklen_t *)(&size_of_addr));
#endif

		if(readed<0)
		{

			//Dout<<"Socket closed! --Li da gang"<<endl;
			return false;
		}

		if (readed-msg_header_length <= 0 )
		{
			//Dout<<"Message is not suitable for decoding! --Li da gang"<<endl;
			return false;
		}

		debug_dod_muticast_stream(); //for debug only

		memcpy(read_buf,temp_buf+msg_header_length,readed-msg_header_length);
		streambuf::setg(read_buf,read_buf,read_buf+readed-msg_header_length);

		return true;
	};

	void send_msg(char *msg_buf,int msg_buf_size)
	{
		if (sendto(server,msg_buf,msg_buf_size,0,(struct sockaddr*)&remote,sizeof(sockaddr))==SOCKET_ERROR)
			throw "Send UDP Failed";
	};

	inline bool read_msg(char *msg_buf,int &msg_buf_size)
	{   
		int readed;
		int size_of_addr=sizeof(sockaddr);

		if(!is_read_ready()) 
		{
			// Dout<<"Socket read timeout! --Li da gang"<<endl;
			return false;
		}
#ifdef WIN32
		readed = ::recvfrom(server, msg_buf, msg_buf_size,0,(sockaddr*)&remote,(int *)(&size_of_addr));
#else
		readed = ::recvfrom(server, msg_buf, msg_buf_size,0,(sockaddr*)&remote,(socklen_t *)(&size_of_addr));
#endif

		if(readed<0)
		{

			//Dout<<"Socket closed! --Li da gang"<<endl;
			return false;
		}

		if (readed-msg_header_length <= 0 )
		{
			//Dout<<"Message is not suitable for decoding! --Li da gang"<<endl;
			return false;
		}

		msg_buf_size=readed;
		//debug_dod_muticast_stream(); //for debug only
		return true;
	};

	virtual void channel_switch(){};

	inline void debug_dod_muticast_stream()
	{
		//-----for dod multicast udp stream  lig 2005-8-16---
		//for dod multicast udp stream,the header of msg is 8 bytes
		uint32_t *plen=(uint32_t*)temp_buf;
		uint32_t *pseq=(uint32_t*)(temp_buf+sizeof(uint32_t));
		unsigned long pkglen=ntohl(*plen);
		unsigned long pkgseq=ntohl(*pseq);

		if(pkglen==0 && pkgseq==0)  //tinet process is normal down,it's the last frame
		{
			channel_switch();   //channel will switch
		}

		if(pre_msg_seq_num!=0 && (pre_msg_seq_num+1)!=pkgseq)
		{
			//Dout<<"Multicast UDP messages have been missed!"<<endl;
		}
		pre_msg_seq_num=pkgseq;
/*
		if(!is_ha_master())
		{
			if(is_ha_slave()) throw dod_ha_exception(); 
		}
*/
		//Dout<<'.';
		//Dout<<"Message is Received :: "<<pkgseq<<endl;
	};

	inline bool is_read_ready()
	{
		if(time_out.tv_sec<1) return true;
		FD_ZERO (&rwfds);
		FD_SET (server, &rwfds);
		timeval read_tmp_time=time_out; //used to reserver origin time_out value,select will change the argument
		int ret=select (server+1, &rwfds, 0, 0, &read_tmp_time);
		if (ret == -1) 
		{
#ifndef WIN32
			if(errno==EINTR) return true; //select break by unix signal.continue
#endif

			return false;
		}
		if (ret == 0 ) return false;
		return true;
	};

	inline bool is_write_ready()
	{
		return true;
	};

	void close_connect(){close();};  //do nothing for udp streams


};

class sdl_udp_pipe:public sdl_udp_stream,public sdl_thread
{
private:
	sdl_mutex *p_mutex;
	time_t begin_time;
	sdl_link null_link;
protected:
	istream *_p_istream;
	ostream *_p_ostream;
public:
	sdl_udp_pipe(int timeout=3600):
	  sdl_udp_stream(timeout)
	  {
		  p_mutex=NULL;
		  _p_istream=NULL;
		  _p_ostream=NULL;
	  };
	  void init_mutex(sdl_mutex &mu){p_mutex=&mu;};
	  void lock(){if(p_mutex!=NULL) p_mutex->lock();}; //lock shared data
	  void unlock(){if(p_mutex!=NULL) p_mutex->unlock();};// unlock shared data
	  sdl_link& get_client(){return null_link;}; //return remote host information
	  void rec_time(){time(&begin_time);};
	  int span_time(){time_t now_time;time(&now_time);return((int)now_time-(int)begin_time);};

	  //overload function from sdl_thread ,you can use it to write your process
	  virtual void begin()
	  {
		  _p_istream=new istream((sdl_udp_stream*)this);
		  _p_ostream=new ostream((sdl_udp_stream*)this);
	  };


	  ~sdl_udp_pipe()
	  {
		  if(_p_istream==NULL) delete _p_istream;
		  if(_p_ostream==NULL) delete _p_ostream;
	  }
};
typedef unsigned int timeout_t;
#ifdef WIN32
//typedef unsigned int ssize_t;
#endif
#define TIMEOUT_INF 6000
#ifndef INVALID_HANDLE_VALUE
#define INVALID_HANDLE_VALUE -1
#endif
class  Serial
{
public:
	enum Error
	{
		errSuccess = 0,
		errOpenNoTty,
		errOpenFailed,
		errSpeedInvalid,
		errFlowInvalid,
		errParityInvalid,
		errCharsizeInvalid,
		errStopbitsInvalid,
		errOptionInvalid,
		errResourceFailure,
		errOutput,
		errInput,
		errTimeout,
		errExtended
	};
	typedef enum Error Error;

	enum Flow
	{
		flowNone,
		flowSoft,
		flowHard,
		flowBoth
	};
	typedef enum Flow Flow;

	enum Parity
	{
		parityNone,
		parityOdd,
		parityEven
	};
	typedef enum Parity Parity;

	enum Pending
	{
		pendingInput,
		pendingOutput,
		pendingError
	};
	typedef enum Pending Pending;

private:
	Error errid;
	char *errstr;

	struct
	{
		bool thrown: 1;
		bool linebuf: 1;
	} flags;

	void	*	original;
	void	*	current;

	void initSerial(void);

protected:
#ifdef WIN32
	HANDLE	dev;
#else
	int dev;
#endif
	int bufsize;

	/**
	 * Opens the serial device.
	 *
	 * @param fname Pathname of device to open
	 */
	void		open(const char *fname);

	/**
	 * Closes the serial device.
	 *
	 */
	void		close(void);

	/**
	 * Reads from serial device.
	 *
	 * @param Data  Point to character buffer to receive data.  Buffers MUST
	 *				be at least Length + 1 bytes in size.
	 * @param Length Number of bytes to read.
	 */
	virtual int	aRead(char * Data, const int Length);

	/**
	 * Writes to serial device.
	 *
	 * @param Data  Point to character buffer containing data to write.  Buffers MUST
	 * @param Length Number of bytes to write.
	 */
	virtual int	aWrite(const char * Data, const int Length);

	/**
	 * This service is used to throw all serial errors which usually
	 * occur during the serial constructor.
	 *
	 * @param error defined serial error id.
	 * @param errstr string or message to optionally pass.
	 */
	Error error(Error error, char *errstr = NULL);

	/**
	 * This service is used to thow application defined serial
	 * errors where the application specific error code is a string.
	 *
	 * @param errstr string or message to pass.
	 */
	inline void error(char *err)
		{error(errExtended, err);};


	/**
	 * This method is used to turn the error handler on or off for
	 * "throwing" execptions by manipulating the thrown flag.
	 *
	 * @param enable true to enable handler.
	 */
	inline void setError(bool enable)
		{flags.thrown = !enable;};

	/**
	 * Set packet read mode and "size" of packet read buffer.
	 * This sets VMIN to x.  VTIM is normally set to "0" so that
	 * "isPending()" can wait for an entire packet rather than just
	 * the first byte.
	 *
	 * @return actual buffer size set.
	 * @param size of packet read request.
	 * @param btimer optional inter-byte data packet timeout.
	 */
	int setPacketInput(int size, unsigned char btimer = 0);

	/**
	 * Set "line buffering" read mode and specifies the newline
	 * character to be used in seperating line records.  isPending
	 * can then be used to wait for an entire line of input.
	 *
	 * @param newline newline character.
	 * @param nl1 EOL2 control character.
 	 * @return size of conical input buffer.
	 */
	int setLineInput(char newline = 13, char nl1 = 0);

	/**
	 * Restore serial device to the original settings at time of open.
	 */
	void restore(void);

	/**
	 * Used to flush the input waiting queue.
	 */
	void flushInput(void);

	/**
	 * Used to flush any pending output data.
	 */
	void flushOutput(void);

	/**
	 * Used to wait until all output has been sent.
	 */
	void waitOutput(void);

	/**
	 * Used as the default destructor for ending serial I/O
	 * services.  It will restore the port to it's original state.
	 */
	void endSerial(void);

	/**
	 * Used to initialize a newly opened serial file handle.  You
	 * should set serial properties and DTR manually before first
	 * use.
	 */
	void initConfig(void);

	/**
	 * This allows later ttystream class to open and close a serial
	 * device.
	 */
	Serial()
		{initSerial();};

	/**
	 * A serial object may be constructed from a named file on the
	 * file system.  This named device must be "isatty()".
	 *
	 * @param name of file.
	 */
	Serial(const char *name);


public:

	/**
	 * The serial base class may be "thrown" as a result on an error,
	 * and the "catcher" may then choose to destory the object.  By
	 * assuring the socket base class is a virtual destructor, we
	 * can assure the full object is properly terminated.
	 */
	virtual ~Serial();

	/**
	 * Serial ports may also be duplecated by the assignment
	 * operator.
	 */
	Serial &operator=(const Serial &from);

	/**
	 * Set serial port speed for both input and output.
	 *
 	 * @return 0 on success.
	 * @param speed to select. 0 signifies modem "hang up".
	 */
	Error setSpeed(unsigned long speed);

	/**
	 * Set character size.
	 *
	 * @return 0 on success.
	 * @param bits character size to use (usually 7 or 8).
	 */
	Error setCharBits(int bits);

	/**
	 * Set parity mode.
	 *
	 * @return 0 on success.
	 * @param parity mode.
	 */
	Error setParity(Parity parity);

	/**
	 * Set number of stop bits.
	 *
	 * @return 0 on success.
	 * @param bits stop bits.
	 */
	Error setStopBits(int bits);

	/**
	 * Set flow control.
	 *
	 * @return 0 on success.
	 * @param flow control mode.
	 */
	Error setFlowControl(Flow flow);

	/**
	 * Set the DTR mode off momentarily.
	 *
	 * @param millisec number of milliseconds.
	 */
	void toggleDTR(timeout_t millisec);

	/**
	 * Send the "break" signal.
	 */
	void sendBreak(void);

	/**
	 * Often used by a "catch" to fetch the last error of a thrown
	 * serial.
	 *
	 * @return error numbr of last Error.
	 */
	inline Error getErrorNumber(void)
		{return errid;};

	/**
	 * Often used by a "catch" to fetch the user set error string
	 * of a thrown serial.
	 *
	 * @return string for error message.
	 */
	inline char *getErrorString(void)
		{return errstr;};

	/**
	 * Get the "buffer" size for buffered operations.  This can
	 * be used when setting packet or line read modes to determine
	 * how many bytes to wait for in a given read call.
	 *
	 * @return number of bytes used for buffering.
	 */
	inline int getBufferSize(void)
		{return bufsize;};

	/**
	 * Get the status of pending operations.  This can be used to
	 * examine if input or output is waiting, or if an error has
	 * occured on the serial device.
	 *
	 * @return true if ready, false if timeout.
	 * @param pend ready check to perform.
	 * @param timeout in milliseconds.
	 */
	virtual bool isPending(Pending pend, timeout_t timeout = TIMEOUT_INF);

	inline void flushLine()
	{
#ifdef WIN32
		flushOutput();
#endif
	}
};

/**
 * TTY streams are used to represent serial connections that are fully
 * "streamable" objects using C++ stream classes and friends.
 * 
 * The first application relevant serial I/O class is the TTYStream class.
 * TTYStream offers a linearly buffered "streaming" I/O session with the
 * serial device.  Furthermore, traditional C++ "stream" operators (<< and
 * >>) may be used with the serial device.  A more "true" to ANSI C++ library
 * format "ttystream" is also available, and this supports an "open" method
 * in which one can pass initial serial device parameters immediately
 * following the device name in a single string, as in
 * "/dev/tty3a:9600,7,e,1", as an example.
 * 
 * The TTYSession aggragates a TTYStream and a Common C++ Thread which is
 * assumed to be the execution context that will be used to perform actual
 * I/O operations.  This class is very anagolous to TCPSession.
 * 
 *
 * @author David Sugar <dyfet@ostel.com>
 * @short streamable tty serial I/O class.
 */
class  TTYStream : protected std::streambuf, public Serial, public std::iostream
{
private:
	int doallocate();

	friend TTYStream& crlf(TTYStream&);
	friend TTYStream& lfcr(TTYStream&);

protected:
	char *gbuf, *pbuf;
	timeout_t timeout;

	/**
	 * This constructor is used to derive "ttystream", a more
	 * C++ style version of the TTYStream class.
	 */
	TTYStream();

	/**
	 * Used to allocate the buffer space needed for iostream
	 * operations.  This is based on MAX_INPUT.
	 */
	void allocate(void);

	/**
	 * Used to terminate the buffer space and clean up the tty
	 * connection.  This function is called by the destructor.
	 */
	void endStream(void);

	/**
	 * This streambuf method is used to load the input buffer
	 * through the established tty serial port.
	 *
	 * @return char from get buffer, EOF also possible.
	 */
	int underflow(void);

	/**
	 * This streambuf method is used for doing unbuffered reads
	 * through the establish tty serial port when in interactive mode.
	 * Also this method will handle proper use of buffers if not in
	 * interative mode.
	 *
	 * @return char from tty serial port, EOF also possible.
	 */
	int uflow(void);

	/**
	 * This streambuf method is used to write the output
	 * buffer through the established tty port.
	 *
	 * @param ch char to push through.
	 * @return char pushed through.
	 */
	int overflow(int ch);

public:
	/**
	 * Create and open a tty serial port.
	 *
	 * @param filename char name of device to open.
	 * @param to default timeout.
	 */
	TTYStream(const char *filename, timeout_t to = 0);

	/**
	 * End the tty stream and cleanup.
	 */
	virtual ~TTYStream();

	/**
	 * Set the timeout control.
	 *
	 * @param to timeout to use.
	 */
	inline void setTimeout(timeout_t to)
		{timeout = to;};

	/**
	 * Set tty mode to buffered or "interactive".  When interactive,
	 * all streamed I/O is directly sent to the serial port
	 * immediately.
	 *
	 * @param flag bool set to true to make interactive.
	 */
	void interactive(bool flag);
	
	/**
	 * Flushes the stream input and out buffers, writes
	 * pending output.
	 *
	 * @return 0 on success.
	 */
	int sync(void);	

	/**
	 * Get the status of pending operations.  This can be used to
	 * examine if input or output is waiting, or if an error has
	 * occured on the serial device.  If read buffer contains data
	 * then input is ready and if write buffer contains data it is
	 * first flushed then checked.
	 *
	 * @return true if ready, false if timeout.
	 * @param pend ready check to perform.
	 * @param timeout in milliseconds.
	 */
	bool isPending(Pending pend, timeout_t timeout = TIMEOUT_INF);
};	

/**
 * A more natural C++ "ttystream" class for use by non-threaded
 * applications.  This class behaves a lot more like fstream and
 * similar classes.
 *
 * @author David Sugar <dyfet@ostel.com>
 * @short C++ "fstream" style ttystream class.
 */

class  ttystream : public TTYStream
{
public:
	/**
	 * Construct an unopened "ttystream" object.
	 */
	ttystream();

	/**
	 * Construct and "open" a tty stream object.  A filename in
	 * the form "device:options[,options]" may be used to pass
	 * device options as part of the open.
	 *
	 * @param name of file and serial options.
	 */
	ttystream(const char *name);

	/**
	 * Open method for a tty stream.
	 *
	 * @param name filename to open.
	 */
	void open(const char *name);

	/**
	 * Close method for a tty stream.
	 */
	void close(void);

	/**
	 * Test to see if stream is opened.
	 */
	inline bool operator!()
	{
#ifdef WIN32
		return (INVALID_HANDLE_VALUE == dev);
#else
		return (dev < 0);
#endif
	};
};



//------------MemCache支持----------------

class MemCacheClient:public sdl_tcp_stream
{
private:
	bool flag_link_ok;
	int  m_time_out;
	ostream* pOut;
	istream* pIn;
	char lineBuf[1024];
	unsigned short m_serial;
	const char* m_para_list[8];
	const char* m_key;
public:
	MemCacheClient(const char* ip="127.0.0.1",int port=11211,int timeout=0):sdl_tcp_stream()
	{
		flag_link_ok=connect_ip(ip,port);
		m_time_out=timeout;  //数据永不过期
		m_serial=0;
		if(flag_link_ok)
		{
			pOut=new ostream(this);
			pIn=new istream(this);
		}
		else
		{
			pOut=NULL;
			pIn=NULL;
		}
	}

	~MemCacheClient()
	{
		if(pOut!=NULL) delete pOut;
		if(pIn!=NULL) delete pIn;
	}

	MemCacheClient& operator[](const char* pKey)
	{
		m_key=pKey;
		return *this;
	}

	void operator=(const char* pValue)
	{
		set(m_key,pValue);
	}

	bool set(string &key,string &value)
	{
		if(!flag_link_ok) return false;
		ostream &out=*pOut;
		istream &in=*pIn;

		out<<"set ";
		out.write(key.data(),key.length());
		out<<' '<<m_serial++<<' '<<m_time_out<<' '<<value.length()<<"\r\n";
		out.write(value.data(),value.length())<<"\r\n"; 
		out.flush(); 

		in.getline(lineBuf,1024);
		if(in.gcount()>0)
		{
			lineBuf[6]=0; //去除不必要的\r
			if(strcmp(lineBuf,"STORED")==0) return true;
		}
		return false;
	}

	bool set(const char* key,const char* value)
	{
		if(!flag_link_ok) return false;
		ostream &out=*pOut;
		istream &in=*pIn;

		out<<"set "<<key<<' '<<m_serial++<<' '<<m_time_out<<' '<<strlen(value)<<"\r\n";
		out<<value<<"\r\n"; 
		out.flush(); 

		in.getline(lineBuf,1024);
		if(in.gcount()>0)
		{
			lineBuf[6]=0; //去除不必要的\r
			if(strcmp(lineBuf,"STORED")==0) return true;
		}
		return false;
	}

	bool get(string &key,string &value)
	{
		if(!flag_link_ok) return false;
		ostream &out=*pOut;
		istream &in=*pIn;

		out<<"get ";
		out.write(key.data(),key.length());
		out<<"\r\n";
		out.flush(); 

		in.getline(lineBuf,1024);
		int value_length=0;
		int key_length=key.length();
		if((value_length=in.gcount())>6+key_length)
		{
			int field=0;
			m_para_list[field]=lineBuf;
			for(int i=0;i<value_length;++i)
			{
				if(lineBuf[i]==' ' || lineBuf[i]=='\r')
				{
					m_para_list[++field]=lineBuf+i+1;
					lineBuf[i]=0;
				}
			}
			if(field==4 && strcmp(m_para_list[0],"VALUE")==0) //有返回值
			{
				value_length=atoi(m_para_list[3]); 
				value.clear(); 
				do
				{
					in.read(lineBuf,value_length>1024?1024:value_length);
					value.append(lineBuf,value_length>1024?1024:value_length); 
					value_length-=1024;
				}while(value_length>1024);
				in.read(lineBuf,7); //读入\r\nEND\r\n
				if(lineBuf[2]=='E' && lineBuf[3]=='N' && lineBuf[4]=='D')
					return true;
			}
		}
		return false;
	}

	bool get(const char* key,string &value)
	{
		if(!flag_link_ok) return false;
		ostream &out=*pOut;
		istream &in=*pIn;

		out<<"get "<<key;
		out<<"\r\n";
		out.flush(); 

		in.getline(lineBuf,1024);
		int value_length=0;
		int key_length=strlen(key);
		if((value_length=in.gcount())>6+key_length)
		{
			int field=0;
			m_para_list[field]=lineBuf;
			for(int i=0;i<value_length;++i)
			{
				if(lineBuf[i]==' ' || lineBuf[i]=='\r')
				{
					m_para_list[++field]=lineBuf+i+1;
					lineBuf[i]=0;
				}
			}
			if(field==4 && strcmp(m_para_list[0],"VALUE")==0) //有返回值
			{
				value_length=atoi(m_para_list[3]); 
				value.clear(); 
				do
				{
					in.read(lineBuf,value_length>1024?1024:value_length);
					value.append(lineBuf,value_length>1024?1024:value_length); 
					value_length-=1024;
				}while(value_length>1024);
				in.read(lineBuf,7); //读入\r\nEND\r\n
				if(lineBuf[2]=='E' && lineBuf[3]=='N' && lineBuf[4]=='D')
					return true;
			}
		}
		return false;
	}
};
#endif
