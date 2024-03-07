#include "sdl_tcpstream.h"

#ifdef WIN32

#pragma comment(lib,"WSOCK32.lib")
#endif



inline bool sdl_tcp_stream::is_read_ready()
{
	FD_ZERO (&rwfds);
	FD_SET (sock_id, &rwfds);
	read_tmp_time=time_out; //used to reserver origin time_out value,select will change the argument
	int ret;
	if(time_out.tv_sec>0)
		ret=select (sock_id+1, &rwfds, NULL, NULL, &read_tmp_time);
	else
		ret=select (sock_id+1, &rwfds, NULL, NULL, NULL); //一直等待
	if (ret == -1)
	{
		if(errno=EINTR) return true;  //a unix signal cause select failed,continue 
		return false;
	}
	if (ret == 0 ) return false;
	return true;
};

bool sdl_tcp_stream::connect_ip(const char* ip,int port,int retry_times) 
{
	struct sockaddr *p_addr=(sockaddr *)&addr;

	if(conn_state!=-1) return false;  //still linked;
	addr.sin_family=AF_INET;
	addr.sin_port=htons(port);
	addr.sin_addr.s_addr=inet_addr(ip); //only support the main(first) address
	p_addr=(sockaddr *)&addr;
	create();

	while(conn_state==-1 && retry_times>0)
	{
		if(sock_id!=-1) conn_state=connect(sock_id,p_addr,sizeof(sockaddr));
		if(conn_state!=-1) return true;
#ifdef WIN32
		::Sleep(50);  //如果是Windows系统，等待50毫秒重新连接，由于Linux中的暂停比较麻烦，因此不做暂停
		closesocket(sock_id);
#else
		close(sock_id);
#endif
		retry_times--;
	}

	return false;
};


inline bool sdl_tcp_stream::is_write_ready()
{
	FD_ZERO (&rwfds);
	FD_SET (sock_id, &rwfds);
	write_tmp_time=time_out;  //used to reserver origin time_out value,select will change the argument
	int ret;
	if(time_out.tv_sec>0)
		ret= ::select (sock_id+1,NULL,&rwfds,NULL,&write_tmp_time);
	else
		ret= ::select (sock_id+1,NULL,&rwfds,NULL,NULL); //一直等待
	if (ret== -1 )
	{
#ifdef WIN32
		::Sleep(10);
#else
		if(errno==EINTR) return true;  //a unix signal cause select failed,continue
#endif
		return false;
	}
	if (ret == 0) return false;
	return true;
};


void sdl_tcp_stream::set_stream_timeout(int seconds)
{
	if(seconds<=0)	time_out.tv_sec  = 0;
	else time_out.tv_sec  = seconds;
	time_out.tv_usec = 0;
};

int sdl_tcp_stream::read_data(char *buf,int size,int time_out)
{
	fd_set rfds;
	struct timeval tv;
	int retval,readed,r_size=size;
	char *r_buf=buf;
	//time_t stime,etime,ntime;
	//time( &stime );

	/* Watch sock_id to see when it has input. */
	FD_ZERO(&rfds);
	FD_SET(sock_id, &rfds);
	//etime=stime+time_out;

	tv.tv_sec = time_out;
	tv.tv_usec = 0;
	while(r_size>0)
	{
		retval = select(sock_id+1, &rfds, NULL, NULL, &tv);
		if(retval && FD_ISSET(sock_id, &rfds))
		{
			//time( &ntime );
			//if((etime-ntime)<0) return false;
			//tv.tv_sec = etime-ntime;
			tv.tv_sec = time_out;
			tv.tv_usec = 0;
			readed=::recv(sock_id, r_buf,r_size,0);
			r_buf=r_buf+readed;
			r_size=r_size-readed;
		}
		else return false;
	}

	return true;
};
//!wait for data come in ,if data come, on_receive will be execute
//!if not set timeout ,will wait until process end
int sdl_tcp_stream::wait(int timeout_seconds)
{
	fd_set rfds;
	struct timeval tv;
	int retval;

	/* Watch sock_id to see when it has input. */
	FD_ZERO(&rfds);
	FD_SET(sock_id, &rfds);

	tv.tv_sec = timeout_seconds;
	tv.tv_usec = 0;
	if(timeout_seconds>0)
	{
		retval = select(sock_id+1, &rfds, NULL, NULL, &tv);
		//DOUT<<"Timed Socket,come in"<<endl;
		if(retval && FD_ISSET(sock_id, &rfds)) return on_receive(sock_id);
		return false;
	}
	else 
	{
		while(!flag_stop_loop)//wait for data come in
		{
			retval = select(sock_id+1, &rfds, NULL, NULL, NULL);
			//DOUT<<"None Timed Socket,come in"<<endl;
			if(retval && FD_ISSET(sock_id, &rfds)) on_receive(sock_id);
		}
	}
	return true;

};

//!constructor
sdl_tcp_stream::sdl_tcp_stream(int timeout):streambuf()//! init tcp 
{

	listen_port=7711;  //default listen port 7711
#ifdef WIN32
	if(!sdl_winsocket_start) 
	{
		WORD wVersionRequested;
		WSADATA wsaData;
		int err;

		wVersionRequested = MAKEWORD( 2, 2 );

		err = WSAStartup( wVersionRequested, &wsaData );
		if ( err != 0 )
		{
			/* Tell the user that we could not find a usable */
			/* WinSock DLL. 								 */
			throw sdl_exception("Can't find Winsocket2!! --li da gang",VERY_BIG_ERR);
			return;
		}

		/* Confirm that the WinSock DLL supports 2.2.*/
		/* Note that if the DLL supports versions greater	 */
		/* than 2.2 in addition to 2.2, it will still return */
		/* 2.2 in wVersion since that is the version we 	 */
		/* requested.										 */

		if ( LOBYTE( wsaData.wVersion ) != 2 ||
			HIBYTE( wsaData.wVersion ) != 2 ) {
				/* Tell the user that we could not find a usable */
				/* WinSock DLL. 								 */
				throw sdl_exception("Can't find Winsocket2!! --li da gang");
				WSACleanup( );
				return; 
		}
		sdl_winsocket_start=true;	//winsocket now workong;

	}
#endif
	//p_addr=(sockaddr *)&addr;
	conn_state=-1;
	streambuf::setp((char *)write_buf,(char *)write_buf+STREAM_BUF_SIZE); //set put area
	streambuf::setg(read_buf,read_buf,read_buf); //set get area
	set_stream_timeout(timeout);  //if socket no response over 'timeout' second,end it
};


sdl_tcp_stream::~sdl_tcp_stream()
{
	flag_stop_loop=false;
	if(sock_id!=-1)
#ifdef WIN32
		conn_state=closesocket(sock_id);
#else
		conn_state=close(sock_id);
#endif
};
//!stream function
int sdl_tcp_stream::overflow(int c)
{
	if(c != EOF)
	{
		*pptr() = (unsigned char)c;
		pbump(1);
	}
	if(write_socket()) return c;
	else
	{
		sdl_tcp_stream::close_connect();
		sdl_tcp_stream::clear_stream();
	}
	return EOF;
};
//!stream function
int sdl_tcp_stream::underflow()//if get area is null,get data from socket
{
	if(read_socket())	
		return (unsigned char)(*gptr());
	else
	{ 
		sdl_tcp_stream::close_connect();
		sdl_tcp_stream::clear_stream();
	}
	return EOF;
};
/*
int TCPStream::uflow(void)
{
int ret = underflow(); 
if (ret == EOF) return EOF; 
return ret; 
}*/
//!stream function
int sdl_tcp_stream::sync()
{
	overflow(EOF);
	return 0;
}
//!read data from socket
inline int sdl_tcp_stream::read_socket()
{	
	int readed;
reread:
	if(!is_read_ready()) 
	{
		//DOUT<<"Socket read timeout! --Li da gang"<<endl;
		return false;
	}
	readed = ::recv(sock_id, read_buf, STREAM_BUF_SIZE,0);
	if(readed<0)
	{
		if(errno==EINTR) goto reread;
		else
		{
			//DOUT<<"Socket closed by Client! --Li da gang"<<endl;
			return false;
		}
	}
	if (readed == 0 )	return false;
	streambuf::setg(read_buf,read_buf,read_buf+readed);
	time(&client_addr.time);
	return true;
};

inline int sdl_tcp_stream::read_nonblock(char *buf,int bufsize)
{	
	int readed;
reread:
	if(!is_read_ready()) 
	{
		//DOUT<<"Socket read timeout! --Li da gang"<<endl;
		return -1;
	}
	readed = ::recv(sock_id, buf, bufsize,0);
	if(readed<0)
	{
		if(errno==EINTR) goto reread;
		else
		{
			//DOUT<<"Socket closed by Client! --Li da gang"<<endl;
			return -1;
		}
	}

	return readed;
};
//throw the data in read_buf,if re_read ,will read socket
void sdl_tcp_stream::throw_away_data()	
{
	streambuf::setg(read_buf,read_buf,0);
};
//write data in write buf to socket
inline int sdl_tcp_stream::write_socket()
{
	int bytes_left; 
	int written_bytes; 
	char *ptr; 

	ptr=write_buf; 
	bytes_left=(char *)pptr()-write_buf; 
	while(bytes_left>0) 
	{ 
		if(!is_write_ready()) 
		{
			//throw sdl_exception("Socket write timeout! --Li da gang");
			//DOUT<<"Socket write timeout! --Li da gang"<<endl;
			return false;
		}
		written_bytes=::send(sock_id,ptr,bytes_left,0); 
		if(written_bytes<=0) 
		{		 
			if(errno==EINTR) 
				written_bytes=0; 
			else 
			{
				//throw sdl_exception("Can't write TCP Stream! --li da gang");
				//DOUT<<"Socket closed by Client! --Li da gang"<<endl;
				return false; 
			}
		} 
		bytes_left-=written_bytes; 
		ptr+=written_bytes;    
	}	 
	streambuf::setp(write_buf,write_buf+STREAM_BUF_SIZE); //set new read buf
	time(&client_addr.time);
	return true;
};

int sdl_tcp_stream::write_direct(const char *pdata,int len)
{
#ifdef WIN32		
	int written_bytes=::send(sock_id,pdata,len,0);
#else
	int written_bytes=::send(sock_id,pdata,len,MSG_NOSIGNAL);
#endif
	if(written_bytes==-1) 
	{		 
		if(errno==EINTR) 
			throw "TCP write error=EINTR";

		if(errno==EPIPE)
			throw "TCP write error=EPIPE";

		if(errno==EAGAIN)
			throw "TCP write error=EAGAIN";

		return -1; 
	}
	return written_bytes;
};
//!begin listen,auto create a socket to listen, if set port, use new port listen
int sdl_tcp_stream::begin_listen(int port)
{
	if(port>0) listen_port=port;
	if(create())
		if(bind(listen_port))
			return(listen());
	return false;
};

//!as a client to connect server,must use domain-name,no use IP address
//if sucessed return true,otherwisw return false
bool sdl_tcp_stream::connect_name(const char* host,int port,int retry_times) 
{
	struct sockaddr *p_addr=(sockaddr *)&addr;
	struct hostent *p_host;

	if(conn_state!=-1) return false;  //still linked;
	p_host=gethostbyname(host);
	if(p_host==NULL) return false;	  //can't find host
	if(p_host->h_addrtype!=AF_INET) return false; //only support IPV4
	if(p_host->h_addr_list[0]==NULL) return false; //can'nt get ip address
	string pip=inet_ntoa(*((struct in_addr *)(p_host->h_addr_list[0])));


	addr.sin_family=AF_INET;
	addr.sin_port=htons(port);
	addr.sin_addr.s_addr=inet_addr(pip.c_str()); //only support the main(first) address
	for(int i=0;i<8;i++) addr.sin_zero[i]=0;

	p_addr=(sockaddr *)&addr;
	while(conn_state==-1 && retry_times>0)
	{
		sock_id=socket(AF_INET,SOCK_STREAM,0);

		if(sock_id!=-1) conn_state=connect(sock_id,p_addr,sizeof(sockaddr));
		if(conn_state!=-1) return true;
#ifdef WIN32
		closesocket(sock_id);
		::Sleep(50);  //如果是Windows系统，等待50毫秒重新连接，由于Linux中的暂停比较麻烦，因此不做暂停
#else
		close(sock_id);
#endif
		retry_times--;
	}
	return false;
};

bool sdl_tcp_stream::connect_host(const char* host,int port,int retry_times) 
{
	struct sockaddr *p_addr=(sockaddr *)&addr;
	string pip;

	int hLen=strlen(host);
	bool flagIP=false;
	if(hLen>=7 && hLen<=15)
	{
		flagIP=true;
		for(int i=0;i<hLen;++i)
			if((host[i]>'9' || host[i]<'0') && host[i]!='.') flagIP=false;
	}

	if(conn_state!=-1) return false;  //still linked;
	if(flagIP) 
		pip=host;
	else
	{
		struct hostent *p_host=gethostbyname(host);
		if(p_host==NULL) return false;	  //can't find host
		if(p_host->h_addrtype!=AF_INET) return false; //only support IPV4
		if(p_host->h_addr_list[0]==NULL) return false; //can'nt get ip address
		pip=inet_ntoa(*((struct in_addr *)(p_host->h_addr_list[0])));
	}

	addr.sin_family=AF_INET;
	addr.sin_port=htons(port);
	addr.sin_addr.s_addr=inet_addr(pip.c_str()); //only support the main(first) address
	for(int i=0;i<8;i++) addr.sin_zero[i]=0;

	p_addr=(sockaddr *)&addr;
	while(conn_state==-1 && retry_times>0)
	{
		sock_id=socket(AF_INET,SOCK_STREAM,0);

		if(sock_id!=-1) conn_state=connect(sock_id,p_addr,sizeof(sockaddr));
		if(conn_state!=-1) return true;
#ifdef WIN32
		closesocket(sock_id);
		::Sleep(50);  //如果是Windows系统，等待50毫秒重新连接，由于Linux中的暂停比较麻烦，因此不做暂停
#else
		close(sock_id);
#endif
		retry_times--;
	}
	return false;
};

int sdl_tcp_stream::accept (sdl_tcp_stream& socket ) 
{

	int addr_length = sizeof (addr );

#ifdef WIN32
	int new_socket = ::accept(sock_id, ( sockaddr * ) &addr, (int * ) &addr_length );
#endif
#ifdef _sdl_unix
	int new_socket = ::accept(sock_id, ( sockaddr * ) &addr, &(addr_length));
#endif
#ifdef _sdl_linux
	int new_socket = ::accept(sock_id, ( sockaddr * ) &addr,((socklen_t*) &addr_length));
#endif
	if ( new_socket<= 0 )
	{
		throw sdl_exception("Socket accept failed!! --li da gang");
		return false;
	}
	else socket.set_sockid(new_socket);

	client_addr.socket_id=new_socket;  //socket
	client_addr.active=true;	   //now active;
	const char *pip=inet_ntoa(addr.sin_addr); 
	if(pip!=NULL) strcpy(client_addr.client_ip,pip);
	else throw sdl_exception("client ip address error!! --li da gang");
	client_addr.client_port=addr.sin_port;	
	time(&(client_addr.time));	   //get time
	client_addr.timeout=1200;	   //20 min time out; 
	return true;
};


int sdl_tcp_stream::close_connect()
{
	if(sock_id!=-1)
#ifdef WIN32
		conn_state=closesocket(sock_id);
#else
		conn_state=close(sock_id);
#endif
	return conn_state;
};

int sdl_tcp_stream::create()
{

	sock_id = socket ( AF_INET, SOCK_STREAM, 0 );
	if (sock_id<=0) 
	{
		throw sdl_exception("Create Socket Error!! --li da gang");
		return false;
	}
	// TIME_WAIT - argh
	int on = 1;
	if ( setsockopt (sock_id, SOL_SOCKET, SO_REUSEADDR, ( const char* ) &on, sizeof ( on ) ) == -1 )
		//依照MSDN Winsockt2 文档，成功返回 0
	{
		throw sdl_exception("Create Socket Error!! --li da gang");
		return false;
	}
	return true;
};
//bind port to a null port
int sdl_tcp_stream::bind (const int port )
{

	if (sock_id<=0) 
	{
		throw sdl_exception("Invaild Socket handle!! --li da gang");
		return false;
	}
	addr.sin_family = AF_INET;
	addr.sin_addr.s_addr = INADDR_ANY;
	addr.sin_port = htons ( port );
	int bind_return = ::bind(sock_id,( struct sockaddr * ) &addr, sizeof (addr ) );
	if ( bind_return == -1 )  
	{ 
		throw sdl_exception("Socket Can't bind the port!! --li da gang");
		return false;
	}
	return true;
};
//!listen
int sdl_tcp_stream::listen() const
{
	if (sock_id<=0) 
	{
		throw sdl_exception("Invaild Socket handle!! --li da gang");
		return false;
	}

	int listen_return = ::listen ( sock_id, MAXLISTENS);

	if ( listen_return == -1 )
	{
		throw sdl_exception("Socket Can't listen!! --li da gang");
		return false;
	}

	return true;
};

istream& operator >>(istream &in,sdl_line &line)
{
	line.readln(in); 
	return in;
};

ostream& operator <<(ostream &out,sdl_line &line)
{
	out<<line.get_line(); 
	return out;
};

#ifdef WIN32
void _sdl_win_thread_proc(void *pParam)
{
	sdl_thread* pObject = (sdl_thread*)pParam;
	if (pObject == NULL)	return;	// if pObject is not valid
	pObject->set_active(); 
	//while(true)
	//{
	//if(pObject->cancel) return;
	//(pObject->block).lock(); 
	pObject->begin();  // thread completed successfully
	pObject->set_inactive(); 
	//}
};
#endif

#ifdef _sdl_unix
void *_sdl_unix_thread_proc(void *pParam )
{
	sdl_thread* pObject = (sdl_thread*)pParam;
	if (pObject != NULL) // if pObject is not valid
		pObject->begin(); 
	pObject->set_inactive(); 
	return NULL;
};
#endif

#ifdef _sdl_linux
void *_sdl_unix_thread_proc(void *pParam )
{
	sdl_thread* pObject = (sdl_thread*)pParam;
	if (pObject != NULL) // if pObject is not valid
		pObject->begin(); 
	pObject->set_inactive(); 
	return NULL;
};
#endif


#ifndef WIN32
#define SOCKET_ERROR -1
#endif 
/*
void sdl_udp_stream::ha_service_init()
{
	memset(&ha_master, 0, sizeof(ha_master));
	ha_master.sin_family = AF_INET;

	ha_master.sin_addr.s_addr=inet_addr (DOD_HA_MASTER_IP);
	memset(&ha_slave, 0, sizeof(ha_master));
	ha_slave.sin_family = AF_INET;

	ha_slave.sin_addr.s_addr=inet_addr (DOD_HA_SLAVE_IP);

};
*/
void sdl_udp_stream::init_udp(int listen_port,int timeout)
{
	string myip=get_local_ip();
	init_udp(myip.c_str(),listen_port,timeout);
};

void sdl_udp_stream::init_udp(const char *listen_ip,int listen_port,int timeout)
{
	if(listen_ip==NULL || strlen(listen_ip)==0 || strlen(listen_ip)>32) return;
	close();

	if(timeout>0)
	{
		time_out.tv_sec=timeout;  //设置超时期限
		time_out.tv_usec=0; 
	}
#ifdef WIN32
	WSAData wsaData;
	if( WSAStartup(MAKEWORD(2,2), &wsaData) != 0 )
		throw "Error in WSAStartup";
#endif		


	server = socket(AF_INET, SOCK_DGRAM, 0); //创建一个UDP套接口
	if(server==-1)
	{
		throw "create socket error";
	}

	int ret ;
	const int on = 1; //允许程序的多个实例运行在同一台机器上
	ret = setsockopt(server, SOL_SOCKET, SO_REUSEADDR, (char *)&on, sizeof(on));
	if( ret == SOCKET_ERROR )
	{
#ifdef WIN32
		WSACleanup();
#endif
		throw "Error in setsockopt(SO_REUSEADDR): ";
	}

	memset(&local, 0, sizeof(local));
	local.sin_family = AF_INET;
	local.sin_port = htons(listen_port);
	local.sin_addr.s_addr=inet_addr (listen_ip);

	ret = bind(server, (sockaddr*)(&local), sizeof(local));
	if( ret == SOCKET_ERROR )
	{
#ifdef WIN32
		WSACleanup();
#endif
		throw "bind socket erroor!";
	}

	if(read_buf!=NULL) delete read_buf;
	if(write_buf!=NULL) delete write_buf;
	if(temp_buf!=NULL) delete temp_buf;

	read_buf=new char[UDP_BUF_SIZE];
	write_buf=new char[UDP_BUF_SIZE];
	temp_buf=new char[UDP_BUF_SIZE];

}

void sdl_udp_stream::init(const char *remote_ip,int remote_port,const char *local_ip,int local_port,int timeout)  // 初始化输出用的Socket
{
	if(remote_ip==NULL || strlen(remote_ip)==0 || strlen(remote_ip)>32) return;
	if(local_ip==NULL || strlen(local_ip)==0 || strlen(local_ip)>32) return;
#ifdef WIN32	
	::closesocket(server); 
#else
	::close(server);
#endif
	if(timeout>0)
	{
		time_out.tv_sec=timeout;  //设置超时期限
		time_out.tv_usec=0; 
	}
#ifdef WIN32
	WSAData wsaData;
	if( WSAStartup(MAKEWORD(2,2), &wsaData) != 0 )
		throw "Error in WSAStartup";
#endif		
	server = socket(AF_INET, SOCK_DGRAM, 0); //创建一个UDP套接口
	if(server==0)
		throw "create socket: error";

	int ret ;

	//UDP_BUF_SIZE=1024; //使用getsockopt找到的MAX_MSG_LEN太大，好像不对，此处设置为固定帧大小1024
	/*
	int msg_tmp=sizeof(UDP_BUF_SIZE);
	ret=::getsockopt(server,SOL_SOCKET,0x2003,(char*)(&UDP_BUF_SIZE),&msg_tmp);
	if( ret == SOCKET_ERROR )
	{
	#ifdef WIN32
	WSACleanup();
	#endif
	throw "Error in getsockopt(SO_MAX_MSG_SIZE): ";
	}
	*/
	const int on = 1; //允许程序的多个实例运行在同一台机器上
	ret = setsockopt(server, SOL_SOCKET, SO_REUSEADDR, (char *)&on, sizeof(on));
	if( ret == SOCKET_ERROR )
	{
#ifdef WIN32
		WSACleanup();
#endif
		throw "Error in setsockopt(SO_REUSEADDR): ";
	}


	memset(&local, 0, sizeof(local));
	local.sin_family = AF_INET;
	local.sin_port = htons(local_port);
	local.sin_addr.s_addr=inet_addr (local_ip);


	memset(&remote, 0, sizeof(local));
	remote.sin_family = AF_INET;
	remote.sin_port = htons(remote_port);
	remote.sin_addr.s_addr=inet_addr(remote_ip);

	ret = bind(server, (sockaddr*)(&local), sizeof(local));
	if( ret == SOCKET_ERROR )
	{
#ifdef WIN32
		WSACleanup();
#endif
		throw "bind socket erroor!";
	}

	//ha_service_init(); //add for ha switch


	if(read_buf!=NULL) delete read_buf;
	if(write_buf!=NULL) delete write_buf;
	if(temp_buf!=NULL) delete temp_buf;

	read_buf=new char[UDP_BUF_SIZE];
	write_buf=new char[UDP_BUF_SIZE];
	temp_buf=new char[UDP_BUF_SIZE];

}

void sdl_udp::init_broadcast(int remoteport)  // 初始化输出用的Socket
{
	remote_port=remoteport;
#ifdef WIN32
	WSAData wsaData;
	if( WSAStartup(MAKEWORD(2,2), &wsaData) != 0 )
		throw "Error in WSAStartup";
#endif		
	server = socket(AF_INET,SOCK_DGRAM,0); //创建一个UDP套接口
	if(server==0)
		throw "create socket: error";

	int ret ;

	//UDP_BUF_SIZE=1024; //使用getsockopt找到的MAX_MSG_LEN太大，好像不对，此处设置为固定帧大小1024

	const int on = 1; //允许程序的多个实例运行在同一台机器上
	ret = setsockopt(server, SOL_SOCKET, SO_REUSEADDR, (char *)&on, sizeof(on));
	if( ret == SOCKET_ERROR )
	{
#ifdef WIN32
		WSACleanup();
#endif
		throw "Error in setsockopt(SO_REUSEADDR): ";
	}


	memset(&local, 0, sizeof(local));

	//local.sin_family=AF_INET;
	//			addr.sin_port=htons(port);
	//		addr.sin_addr.s_addr=inet_addr(pip.c_str());
	local.sin_family = AF_INET;
	local.sin_port = htons(remote_port);
	local.sin_addr.s_addr = INADDR_ANY;

	memset(&remote, 0, sizeof(local));
	remote.sin_family = AF_INET;
	remote.sin_port = htons(remote_port);
	remote.sin_addr.s_addr = INADDR_BROADCAST;

	const int allow_sockopt=1;
	ret = setsockopt(server, SOL_SOCKET, SO_BROADCAST, (const char*)&allow_sockopt, sizeof (allow_sockopt));
	if( ret == SOCKET_ERROR )
	{
#ifdef WIN32
		WSACleanup();
#endif
		throw "Error in setsockopt(SO_BROADCAST): ";
	}

	ret = bind(server, (sockaddr*)(&local), sizeof(local));
	if( ret == SOCKET_ERROR )
	{
#ifdef WIN32
		WSACleanup();
#endif
		throw "bind socket erroor!";
	}


	if(read_buf!=NULL) delete read_buf;
	if(write_buf!=NULL) delete write_buf;
	if(temp_buf!=NULL) delete temp_buf;

	read_buf=new char[UDP_BUF_SIZE];
	write_buf=new char[UDP_BUF_SIZE];
	temp_buf=new char[UDP_BUF_SIZE];

};

bool sdl_udp::init(const char *local_ip,int local_port)  // 初始化输出用的Socket
{
	bool rt=true;

	if(local_ip==NULL || strlen(local_ip)==0 || strlen(local_ip)>32) return false;
#ifdef WIN32    
	::closesocket(server); 
#else
	::close(server);
#endif

#ifdef WIN32
	WSAData wsaData;
	if( WSAStartup(MAKEWORD(2,2), &wsaData) != 0 )
		throw "Error in WSAStartup";
#endif      
	server = socket(AF_INET, SOCK_DGRAM, 0); //创建一个UDP套接口
	if(server==-1)
	{
		throw "create socket: error";
	}
	int ret ;

	//UDP_BUF_SIZE=512; //使用getsockopt找到的MAX_MSG_LEN太大，好像不对，此处设置为固定帧大小1024

	const int on = 1; //允许程序的多个实例运行在同一台机器上
	ret = setsockopt(server, SOL_SOCKET, SO_REUSEADDR, (char *)&on, sizeof(on));
	if( ret == SOCKET_ERROR )
	{
#ifdef WIN32

		WSACleanup();
#endif
		throw "Error in setsockopt(SO_REUSEADDR): ";
	}

	remote_port=local_port;
	memset(&local, 0, sizeof(local));
	local.sin_family = AF_INET;
	local.sin_port = htons(local_port);
	local.sin_addr.s_addr=inet_addr (local_ip);

	ret = bind(server, (sockaddr*)(&local), sizeof(local));
	if( ret == SOCKET_ERROR )
	{
#ifdef WIN32
		int res=WSAGetLastError();
		if(res==WSAEADDRINUSE) 
		{
			rt=false;
			return rt;
		}
		WSACleanup();
#endif
		throw "bind socket erroor!";
	}


	if(read_buf!=NULL) delete read_buf;
	if(write_buf!=NULL) delete write_buf;
	if(temp_buf!=NULL) delete temp_buf;

	read_buf=new char[UDP_BUF_SIZE];
	write_buf=new char[UDP_BUF_SIZE];
	temp_buf=new char[UDP_BUF_SIZE];
	return rt;

}

void sdl_udp::init_multicast(const char *remote_ip,int remoteport,const char* local_ip)  // 初始化输出用的Socket
{
	remote_port=remoteport;
	if(remote_ip==NULL || strlen(remote_ip)==0 || strlen(remote_ip)>32) return;

#ifdef WIN32    
	::closesocket(server); 
#else
	::close(server);
#endif


#ifdef WIN32
	WSAData wsaData;
	if( WSAStartup(MAKEWORD(2,2), &wsaData) != 0 )
		throw "Error in WSAStartup";
#endif		


	server = socket(AF_INET, SOCK_DGRAM, 0); //创建一个UDP套接口
	if(server==-1)
	{
		throw "create socket error";
	}
	//DOUT<<"create socket: "<<server<<endl;

	int ret ;

	const int on = 1; //允许程序的多个实例运行在同一台机器上
	ret = setsockopt(server, SOL_SOCKET, SO_REUSEADDR, (char *)&on, sizeof(on));
	if( ret == SOCKET_ERROR )
	{
#ifdef WIN32
		WSACleanup();
#endif			
		// DOUT<<"Error in setsockopt(SO_REUSEADDR): "<<endl;
		return;
	}

	const int routenum = 10;
	ret = setsockopt(server,IPPROTO_IP,IP_MULTICAST_TTL,(char*)&routenum,sizeof(routenum));
	if( ret == SOCKET_ERROR )
	{
#ifdef WIN32
		WSACleanup();
#endif		
		//        DOUT<<"Error in setsockopt(IP_MULTICAST_TTL): "<<endl;
		return;
	}

	sockaddr_in local;
	memset(&local, 0, sizeof(local));
	local.sin_family = AF_INET;
	local.sin_port = htons(remote_port);
	local.sin_addr.s_addr = INADDR_ANY;


	ret = bind(server, (sockaddr*)(&local), sizeof(local));
	if( ret == SOCKET_ERROR )
	{
#ifdef WIN32
		WSACleanup();
#endif		
		// DOUT<<"Error in bind: "<<endl;
		return;
	}

	ip_mreq mreq;
	memset(&mreq, 0, sizeof(mreq));
	if(local_ip==NULL) local_ip=get_local_ip().c_str();
	mreq.imr_interface.s_addr = inet_addr(local_ip);
	mreq.imr_multiaddr.s_addr = inet_addr(remote_ip);


	//加入一个多播组
	ret = setsockopt(server,IPPROTO_IP,IP_ADD_MEMBERSHIP,(char*)&mreq,sizeof(mreq));
	if( ret == SOCKET_ERROR )
	{
#ifdef WIN32
		WSACleanup();
#endif		
		//        DOUT<<"Error in setsockopt(IP_ADD_MEMBERSHIP): "<<endl;
		return;
	}
	//int RcvTimeout=10;
	//ret = setsockopt(server,IPPROTO_TCP,SO_RCVTIMEO,(char *)RcvTimeout,sizeof(RcvTimeout));


	if(read_buf!=NULL) delete read_buf;
	if(write_buf!=NULL) delete write_buf;
	if(temp_buf!=NULL) delete temp_buf;

	read_buf=new char[UDP_BUF_SIZE];
	write_buf=new char[UDP_BUF_SIZE];
	temp_buf=new char[UDP_BUF_SIZE];
}

void sdl_udp_stream::init_broadcast(int remote_port)  // 初始化输出用的Socket
{

#ifdef WIN32
	WSAData wsaData;
	if( WSAStartup(MAKEWORD(2,2), &wsaData) != 0 )
		throw "Error in WSAStartup";
#endif		
	server = socket(AF_INET,SOCK_DGRAM,0); //创建一个UDP套接口
	if(server==0)
		throw "create socket: error";

	int ret ;

	//UDP_BUF_SIZE=1024; //使用getsockopt找到的MAX_MSG_LEN太大，好像不对，此处设置为固定帧大小1024

	const int on = 1; //允许程序的多个实例运行在同一台机器上
	ret = setsockopt(server, SOL_SOCKET, SO_REUSEADDR, (char *)&on, sizeof(on));
	if( ret == SOCKET_ERROR )
	{
#ifdef WIN32
		WSACleanup();
#endif
		throw "Error in setsockopt(SO_REUSEADDR): ";
	}


	memset(&local, 0, sizeof(local));


	local.sin_family = AF_INET;
	local.sin_port = htons(remote_port);
	local.sin_addr.s_addr = INADDR_ANY;

	memset(&remote, 0, sizeof(local));
	remote.sin_family = AF_INET;
	remote.sin_port = htons(remote_port);
	remote.sin_addr.s_addr = INADDR_BROADCAST;

	const int allow_sockopt=1;
	ret = setsockopt(server, SOL_SOCKET, SO_BROADCAST, (const char*)&allow_sockopt, sizeof (allow_sockopt));
	if( ret == SOCKET_ERROR )
	{
#ifdef WIN32
		WSACleanup();
#endif
		throw "Error in setsockopt(SO_BROADCAST): ";
	}

	ret = bind(server, (sockaddr*)(&local), sizeof(local));
	if( ret == SOCKET_ERROR )
	{
#ifdef WIN32
		WSACleanup();
#endif
		throw "bind socket erroor!";
	}


	if(read_buf!=NULL) delete read_buf;
	if(write_buf!=NULL) delete write_buf;
	if(temp_buf!=NULL) delete temp_buf;

	read_buf=new char[UDP_BUF_SIZE];
	write_buf=new char[UDP_BUF_SIZE];
	temp_buf=new char[UDP_BUF_SIZE];

};

void sdl_udp_stream::init_multicast(const char *remote_ip,int remote_port,int timeout_s)  // 初始化输出用的Socket
{
	if(remote_ip==NULL || strlen(remote_ip)==0 || strlen(remote_ip)>32) return;

	close();

	if(timeout_s>0)
	{
		time_out.tv_sec=timeout_s;  //设置超时期限
		time_out.tv_usec=0; 
	}
#ifdef WIN32
	WSAData wsaData;
	if( WSAStartup(MAKEWORD(2,2), &wsaData) != 0 )
		throw "Error in WSAStartup";
#endif		


	server = socket(AF_INET, SOCK_DGRAM, 0); //创建一个UDP套接口
	if(server==-1)
	{
		throw "create socket error";
	}
	//DOUT<<"create socket: "<<server<<endl;

	int ret ;

	const int on = 1; //允许程序的多个实例运行在同一台机器上
	ret = setsockopt(server, SOL_SOCKET, SO_REUSEADDR, (char *)&on, sizeof(on));
	if( ret == SOCKET_ERROR )
	{
#ifdef WIN32
		WSACleanup();
#endif			
		//        DOUT<<"Error in setsockopt(SO_REUSEADDR): "<<endl;
		return;
	}

	const int routenum = 10;
	ret = setsockopt(server,IPPROTO_IP,IP_MULTICAST_TTL,(char*)&routenum,sizeof(routenum));
	if( ret == SOCKET_ERROR )
	{
#ifdef WIN32
		WSACleanup();
#endif		
		//       DOUT<<"Error in setsockopt(IP_MULTICAST_TTL): "<<endl;
		return;
	}
	
	sockaddr_in local;
	memset(&local, 0, sizeof(local));
	local.sin_family = AF_INET;
	local.sin_port = htons(remote_port);
	local.sin_addr.s_addr = INADDR_ANY;


	ret = bind(server, (sockaddr*)(&local), sizeof(local));
	if( ret == SOCKET_ERROR )
	{
#ifdef WIN32
		WSACleanup();
#endif		
		//        DOUT<<"Error in bind: "<<endl;
		return;
	}

	ip_mreq mreq;
	memset(&mreq, 0, sizeof(mreq));
	mreq.imr_interface.s_addr = inet_addr(get_local_ip().c_str());
	mreq.imr_multiaddr.s_addr = inet_addr(remote_ip);


	//加入一个多播组
	ret = setsockopt(server,IPPROTO_IP,IP_ADD_MEMBERSHIP,(char*)&mreq,sizeof(mreq));
	if( ret == SOCKET_ERROR )
	{
#ifdef WIN32
		WSACleanup();
#endif		
		//       DOUT<<"Error in setsockopt(IP_ADD_MEMBERSHIP): "<<endl;
		return;
	}
	//int RcvTimeout=10;
	//ret = setsockopt(server,IPPROTO_TCP,SO_RCVTIMEO,(char *)RcvTimeout,sizeof(RcvTimeout));


	if(read_buf!=NULL) delete read_buf;
	if(write_buf!=NULL) delete write_buf;
	if(temp_buf!=NULL) delete temp_buf;

	read_buf=new char[UDP_BUF_SIZE];
	write_buf=new char[UDP_BUF_SIZE];
	temp_buf=new char[UDP_BUF_SIZE];
}


#include "sdl_state.h"

PageMemory<const char*,256> StringDB;
int _StringLastHandle=1; //指向最后一个被申请的句柄值
StringTree _STroot;
MemAlloctor<StringTree> _tmem;

void _clear_dict(){_tmem.clear();};

StringTree* _dict_insert(const char* str,int handle,const char* strEnd,StringTree* it)
{

	StringTree* itp;
	const char *p=str;
	if(it->ch!=_STTermCh) return NULL; //非法插入点
	while(p!=strEnd && *p!=0)
	{
		itp=it->pChild;
		if(itp==NULL)
		{
			it->pChild=_tmem.alloc();
			it->pChild->ch=(unsigned char)(*p);
			it->pChild->pBrother=NULL;
			it->pChild->pChild=NULL;
			it->pChild->handle=-1;  
			it=it->pChild; 
			++p;
			continue;
		}

		while(itp!=NULL)
		{
			it=itp;
			if(itp->ch==(unsigned char)(*p)) break;
			itp=itp->pBrother; 
		}

		if(itp==NULL)
		{
			it->pBrother=_tmem.alloc();
			it->pBrother->ch=(unsigned char)(*p);
			it->pBrother->pBrother=NULL;
			it->pBrother->pChild=NULL;
			it->pBrother->handle=-1;  
			it=it->pBrother; 
		}
		++p;
	}

	if(it->pChild==NULL) //没有找到一致的字符串
	{
		it->pChild=_tmem.alloc();
		it->pChild->handle=handle;
		it->pChild->pChild=NULL;
		it->pChild->pBrother=NULL;
		it->pChild->ch=_STTermCh;   //写入终结符
	}
	else //找到了一致的字符串
	{
		if(it->pChild->ch==_STTermCh) 
			return it->pChild;  //找到一致的字符串
		StringTree* pF=it->pChild; //找到平行发字符串
		it->pChild=_tmem.alloc();
		it->pChild->handle=handle;
		it->pChild->pChild=NULL;
		it->pChild->pBrother=pF;
		it->pChild->ch=_STTermCh;  
	}
	return it;
};

int _dict_find(const char* str,const char* strEnd,StringTree* it)
{
	StringTree* itp;
	const char *p=str;
	while(p!=strEnd && *p!=0)
	{
		itp=it->pChild;
		if(itp==NULL) return -1;
		while(itp!=NULL)
		{
			it=itp;
			if(itp->ch==(unsigned char)(*p)) break;
			itp=itp->pBrother; 
		}
		if(itp==NULL) return -1;
		++p;
	}
	if(it->pChild==NULL ) return -1;
	if(it->pChild->ch !=_STTermCh ) return -1;
	return it->pChild->handle;  

};



void _insert_string(const char* str,StringTree* it)
{
	StringTree* phnd=_dict_insert(str,_StringLastHandle);
	if(phnd->pChild==NULL) return;
	int hnd=phnd->pChild->handle;
	if(hnd==_StringLastHandle) 
	{
		StringDB[hnd]=new char[strlen(str)+1];
		strcpy((char*)StringDB[hnd],str); //在内存中写入字符串;
		++_StringLastHandle;  //加入字符串索引成功，为下一个索引做准备
	}
};

const char* _get_string(int handle,StringTree* it)
{
	if(handle<=_StringLastHandle)
		return StringDB[handle];
	return NULL;
};

int _get_handle(const char* str)
{
	StringTree* phnd=_dict_insert(str,_StringLastHandle);
	if(phnd->pChild==NULL) return -1;
	int hnd=phnd->pChild->handle;
	if(hnd==_StringLastHandle) 
	{
		
		StringDB[hnd]=new char[strlen(str)+1];
		strcpy((char*)StringDB[hnd],str); //在内存中写入字符串;
		++_StringLastHandle;  //加入字符串索引成功，为下一个索引做准备
	}
	return hnd;
};

void nfa_chord_set::add(int ch,int	from,int to)  //加入激励转移表
{
	nfa_chord tmp;
	std::pair<DataIter,bool> rt;	
	tmp.ch=ch;
	tmp.fromID=from;
	tmp.toID=to;

	rt=data.insert(tmp);
	if(rt.second) index.insert((nfa_chord*)&(*rt.first)); 
};


void nfa_chord_set::add_NULL(int from,int to) //加入无激励转移表
{
	nfa_chord tmp;
	std::pair<DataIter,bool> rt;	
	tmp.ch=0;
	tmp.fromID=from;
	tmp.toID=to;

	rt=data.insert(tmp);
	if(rt.second) index.insert((nfa_chord*)&(*rt.first)); 
};

void nfa_chord_set::erase(int ch,int from,int to) //删除某项激励转移--
{
	nfa_chord tmp;
	DataIter rt;
	IdxIter start,end;
	tmp.ch=ch;
	tmp.fromID=from;
	tmp.toID=to;

	rt=data.find(tmp);
	if(rt!=data.end())
	{
		nfa_chord* pt=(nfa_chord*)&(*rt);
		start=index.lower_bound(pt);
		end=index.upper_bound(pt); 
		while(start!=end)
		{
			if((*start)->toID==to)
			{
				index.erase(start);
				break;
			}
			++start;
		}
		data.erase(rt);
	} 
};

void nfa_chord_set::erase(int ch,int from) //删除某项激励转移--
{
	nfa_chord tmp;
	DataIter rt;
	IdxIter start,end,it;
	tmp.ch=ch;
	tmp.fromID=from;

	start=index.lower_bound(&tmp);
	end=index.upper_bound(&tmp); 
	while(start!=end)
	{
		it=start;
		++start;
		rt=data.find(**it);
		if(rt!=data.end())
			data.erase(rt);
		index.erase(it);
	} 
};

bool nfa_chord_set::trans(int signal,P_SET &sset) //集合自身转移--
{
	P_SET result_set;
	IdxIter start,end;
	nfa_chord tmp;

	for(P_SET::iterator it=sset.begin();it!=sset.end();++it)
	{
		tmp.ch=signal;
		tmp.fromID=*it;
		start=index.lower_bound(&tmp);
		end=index.upper_bound(&tmp); 

		if(start==index.end()) continue;
		while(start!=end)
		{
			result_set.insert((*start)->toID);
			++start;
		}
	}
	if( sset == result_set ) return false;		//信号仅仅能够激活自身，这是循环激励，因此不再进行--
	sset=result_set;
	return !(result_set.empty());
};

bool nfa_chord_set::prompt(int	from,int ch,P_SET *result_set)  //激励转移--
{
	IdxIter start,end;
	nfa_chord tmp;
	tmp.ch=ch;
	tmp.fromID=from;
	start=index.lower_bound(&tmp);
	end=index.upper_bound(&tmp); 

	if(start==index.end()) return false;
	while(start!=end)
	{
		result_set->insert((*start)->toID); 
		++start;
	}

	return true;
};

bool nfa_chord_set::prompt(int	from,int ch)  //激励转移--
{
	IdxIter start,end;
	curr.clear(); 
	nfa_chord tmp;
	tmp.ch=ch;
	tmp.fromID=from;
	start=index.lower_bound(&tmp);
	end=index.upper_bound(&tmp); 

	if(start==index.end()) return false;
	while(start!=end)
	{
		curr.insert((*start)->toID); 
		++start;
	}
	return true;
};
//---------------------------------------------------------------------------------------
int nfa_chord_set::trans_single_base(int from,int ch)  //单激励转移--
{
	IdxIter start,end;
	curr.clear(); 
	nfa_chord tmp;
	tmp.ch=ch;
	tmp.fromID=from;
	start=index.lower_bound(&tmp);
	end=index.upper_bound(&tmp); 

	if(start==index.end()) return 0; //找不到激励
	return (*start)->toID; //返回激励以后的值
}

bool nfa_chord_set::trans_single(int signal,P_SET &sset)
{
	P_SET result_set;
	IdxIter start,end;
	nfa_chord tmp;

	for(P_SET::iterator it=sset.begin();it!=sset.end();++it)
	{
		tmp.ch=signal;
		tmp.fromID=*it;
		start=index.lower_bound(&tmp);
		end=index.upper_bound(&tmp); 

		if(start==index.end()) continue;
		while(start!=end)
		{
			result_set.insert((*start)->toID); 
			++start;
		}
	}
	sset=result_set;
	return !(result_set.empty());

};

//--------------------------------------------------------------------------------------------
bool nfa_chord_set::find(int from,int ch)  //激励转移--
{
	IdxIter start,end;
	nfa_chord tmp;
	tmp.ch=ch;
	tmp.fromID=from;
	start=index.lower_bound(&tmp);
	end=index.upper_bound(&tmp); 
	if(start==index.end()) return false;
	if(start!=end) return true;
	return false;
};

void nfa_chord_set::clear()
{
	curr.clear();
}

nfa_chord_set _lig_set; //缺省的状态管理机

std::string get_local_ip(const char* ETH_INTERFACE_NAME)
{
	char name[128];
	string ip="";
	hostent* hostinfo;
#ifdef WIN32
	WORD wVersionRequested;
	WSADATA wsaData;

	wVersionRequested = MAKEWORD( 2, 0 );

	if ( WSAStartup( wVersionRequested, &wsaData ) == 0 )
	{
		if( gethostname ( name, sizeof(name)) == 0)
		{
			//			DOUT<<"Local Host Name is "<<name<<endl;
			if((hostinfo = gethostbyname(name)) != NULL)
			{
				ip = inet_ntoa (*(struct in_addr *)*hostinfo->h_addr_list);
				//				DOUT<<"Host ip is "<<ip<<endl;
			}
		}
	}
#else
	int sock_fd;
	struct sockaddr_in my_addr;
	struct ifreq ifr;

	if ((sock_fd = socket(PF_INET, SOCK_DGRAM, 0)) == -1)
	{
		perror("socket");
		exit(1);
	}

	strncpy(ifr.ifr_name, ETH_INTERFACE_NAME, strlen(ETH_INTERFACE_NAME));
	ifr.ifr_name[strlen(ETH_INTERFACE_NAME)]='\0';
	if (ioctl(sock_fd, SIOCGIFADDR, &ifr) < 0)
	{
		perror("ioctl");
		exit(1);
	}
	memcpy(&my_addr, &ifr.ifr_addr, sizeof(my_addr));
	ip=inet_ntoa(my_addr.sin_addr);

#endif
	return ip;
};

std::string get_host_ip(const char *hostname)
{
	string ip;
	hostent* hostinfo;
#ifdef WIN32
	WORD wVersionRequested;
	WSADATA wsaData;

	wVersionRequested = MAKEWORD( 2, 0 );

	if ( WSAStartup( wVersionRequested, &wsaData ) == 0 )
	{

#endif
		if((hostinfo = gethostbyname(hostname)) != NULL)
		{
			ip = inet_ntoa (*(struct in_addr *)*hostinfo->h_addr_list);
		}

#ifdef WIN32
	}
#endif

	return ip;

};

std::string convert_hex(char *msg_buf,int buf_length)
{
	char ch;
	int tt=0;

	string message_hex="";
	string message_val="";

	char dbuf[32];
	sprintf(dbuf,"\nMsg Len=%d\n",buf_length);
	message_hex+=dbuf;

	for(int i=0;i<buf_length;++i)
	{
		ch=msg_buf[i];
		int v=(unsigned char)ch;
		char hc=(char)(v>>4);
		char lc=(char)(v-(hc<<4));
		if(hc<10) hc=hc+'0';
		else hc=hc+'A'-10;
		if(lc<10) lc=lc+'0';
		else lc=lc+'A'-10;

		message_hex+=hc; 
		message_hex+=lc; 
		message_hex+=' ';

		if(ch==0 || ch<0 || ch<32) 
			message_val+='.';
		else message_val+=ch;

		if(i>0 && ((i+1) % 18)==0) //换行操作
		{
			message_hex+=": ";
			message_hex=message_hex+message_val;
			message_hex+="\n";
			message_val="";
		}
		tt++;
	}

	for(int j=0;j<18-(buf_length%18);++j) //处理后续的字符
	{
		message_hex+="-- ";
	}

	message_hex+=": ";
	message_hex+=message_val;
	message_hex+="\n";



	return message_hex;
}

std::string convert_hex2(char *msg_buf,int buf_length,int timestamp)
{
	char ch;
	int tt=0;

	string message_hex="";

	char dbuf[32];
	sprintf(dbuf,"%11d,",timestamp);
	message_hex+=dbuf;

	for(int i=0;i<buf_length;++i)
	{
		ch=msg_buf[i];
		int v=(unsigned char)ch;
		char hc=(char)(v>>4);
		char lc=(char)(v-(hc<<4));
		if(hc<10) hc=hc+'0';
		else hc=hc+'A'-10;
		if(lc<10) lc=lc+'0';
		else lc=lc+'A'-10;
		message_hex+=hc; 
		message_hex+=lc; 
	}
	return message_hex;
}

bool convert_binary(char *msg_buf,char *binary_buf,int &buf_length,int &timestamp)
{
	char tbuf[16];
	memcpy(tbuf,msg_buf,12);
	if(tbuf[11]!=',') return false;
	tbuf[11]=0;
	timestamp=atoi(tbuf);

	char *pmsg=msg_buf+12;
	int old_buf_length=buf_length;
	unsigned char *pres=(unsigned char*)binary_buf;
	buf_length=0;
	while(*pmsg!=0)
	{	
		char chh=*pmsg;
		if(*(++pmsg)==0) return false;
		char chl=*pmsg;
		++pmsg;
		if(chh>='0' && chh<='9') pres[buf_length]=(chh-'0')*16;
		if(chh>='A' && chh<='F') pres[buf_length]=(chh-'A'+10)*16;
		if(chl>='0' && chl<='9') pres[buf_length]+=(chl-'0');
		if(chl>='A' && chl<='F') pres[buf_length]+=(chl-'A'+10);
		if(++buf_length>old_buf_length) return false;
	}
	return true;
}

//-----------------------------------------------
//----------------以下是串口支持代码-------------
#include <fstream>
#include <string>
#include <cmath>
#include <cstdlib>
#include <climits>

char *setString(char *dest, size_t size, const char *src)
{
	size_t len = strlen(src);

	if(size == 1)
		*dest = 0;

	if(size < 2)
		return dest;

	if(len >= size)
		len = size - 1;

	if(!len)
	{
		dest[0] = 0;
		return dest;
	}

	memcpy(dest, src, len);
	dest[len] = 0;
	return dest;
}

#ifdef	WIN32

#define	B256000		CBR_256000
#define	B128000		CBR_128000
#define	B115200		CBR_115200
#define B57600		CBR_57600
#define	B56000		CBR_56000
#define	B38400		CBR_38400
#define	B19200		CBR_19200
#define	B14400		CBR_14400
#define	B9600		CBR_9600
#define	B4800		CBR_4800
#define	B2400		CBR_2400
#define	B1200		CBR_1200
#define	B600		CBR_600
#define	B300		CBR_300
#define	B110		CBR_110

#include <io.h>
#include <fcntl.h>
#else
#include <sys/ioctl.h>
#include <termios.h>
#endif

#include <cerrno>
#include <iostream>


using std::streambuf;
using std::iostream;
using std::ios;


#ifndef	MAX_INPUT
#define	MAX_INPUT 255
#endif

#ifndef MAX_CANON
#define MAX_CANON MAX_INPUT
#endif

#ifdef	__FreeBSD__
#undef	_PC_MAX_INPUT
#undef	_PC_MAX_CANON
#endif

#if defined(__QNX__)
#define CRTSCTS (IHFLOW | OHFLOW)
#endif

#if defined(_THR_UNIXWARE) || defined(__hpux) || defined(_AIX)  
#include <sys/termiox.h>
#define	CRTSCTS	(CTSXON | RTSXOFF)
#endif


// IRIX

#ifndef	CRTSCTS
#ifdef	CNEW_RTSCTS
#define	CRTSCTS (CNEW_RTSCTS)
#endif
#endif

#if defined(CTSXON) && defined(RTSXOFF) && !defined(CRTSCTS) 
#define	CRTSCTS (CTSXON | RTSXOFF)
#endif

#ifndef	CRTSCTS
#define	CRTSCTS	0
#endif

Serial::Serial(const char *fname)
{
	initSerial();

	open(fname);

#ifdef	WIN32
	if(dev == INVALID_HANDLE_VALUE)
#else
	if(dev < 0)
#endif
	{
		error(errOpenFailed);
		return;
	}

#ifdef	WIN32
	COMMTIMEOUTS  CommTimeOuts ;

	GetCommTimeouts(dev, &CommTimeOuts);

	//    CommTimeOuts.ReadIntervalTimeout = MAXDWORD;
	CommTimeOuts.ReadIntervalTimeout = 0;

	CommTimeOuts.ReadTotalTimeoutMultiplier = 0 ;
	CommTimeOuts.ReadTotalTimeoutConstant = 0;
	CommTimeOuts.WriteTotalTimeoutMultiplier = 0 ;
	CommTimeOuts.WriteTotalTimeoutConstant = 1000;

	SetCommTimeouts(dev, &CommTimeOuts) ;

#else

	if(!isatty(dev))
	{
		Serial::close();
		error(errOpenNoTty);
		return;
	}
#endif
}

Serial::~Serial()
{
	endSerial();
}

void Serial::initConfig(void)
{
#ifdef	WIN32

#define ASCII_XON       0x11
#define ASCII_XOFF      0x13

	DCB * attr = (DCB *)current;
	DCB * orig = (DCB *)original;

	attr->DCBlength = sizeof(DCB);
	orig->DCBlength = sizeof(DCB);

	GetCommState(dev, orig);
	GetCommState(dev, attr);

	attr->DCBlength = sizeof(DCB);
	attr->BaudRate = 1200;
	attr->Parity = NOPARITY;
	attr->ByteSize = 8;

	attr->XonChar = ASCII_XON;
	attr->XoffChar = ASCII_XOFF;
	attr->XonLim = 100;
	attr->XoffLim = 100;
	attr->fOutxDsrFlow = 0;
	attr->fDtrControl = DTR_CONTROL_ENABLE;
	attr->fOutxCtsFlow = 1;
	attr->fRtsControl = RTS_CONTROL_ENABLE;
	attr->fInX = attr->fOutX = 0;

	attr->fBinary = true;
	attr->fParity = true;

	SetCommState(dev, attr);

#else
	struct termios *attr = (struct termios *)current;
	struct termios *orig = (struct termios *)original;
	long ioflags = fcntl(dev, F_GETFL);

	tcgetattr(dev, (struct termios *)original);
	tcgetattr(dev, (struct termios *)current);

	attr->c_oflag = attr->c_lflag = 0;
	attr->c_cflag = CLOCAL | CREAD | HUPCL;
	attr->c_iflag = IGNBRK;

	memset(attr->c_cc, 0, sizeof(attr->c_cc));
	attr->c_cc[VMIN] = 1;

	// inherit original settings, maybe we should keep more??
	cfsetispeed(attr, cfgetispeed(orig));
	cfsetospeed(attr, cfgetospeed(orig));
	attr->c_cflag |= orig->c_cflag & (CRTSCTS | CSIZE | PARENB | PARODD | CSTOPB);
	attr->c_iflag |= orig->c_iflag & (IXON | IXANY | IXOFF);

	tcsetattr(dev, TCSANOW, attr);
	fcntl(dev, F_SETFL, ioflags & ~O_NDELAY);

#if defined(TIOCM_RTS) && defined(TIOCMODG)
	int mcs = 0;
	ioctl(dev, TIOCMODG, &mcs);
	mcs |= TIOCM_RTS;
	ioctl(dev, TIOCMODS, &mcs);
#endif	

#ifdef	_COHERENT
	ioctl(dev, TIOCSRTS, 0);
#endif

#endif	// WIN32
}

void Serial::restore(void)
{
#ifdef	WIN32
	memcpy(current, original, sizeof(DCB));
	SetCommState(dev, (DCB *)current);
#else
	memcpy(current, original, sizeof(struct termios));
	tcsetattr(dev, TCSANOW, (struct termios *)current);
#endif
}

void Serial::initSerial(void)
{
	flags.thrown = false;
	flags.linebuf = false;
	errid = errSuccess;
	errstr = NULL;

	dev = INVALID_HANDLE_VALUE;
#ifdef	WIN32
	current = new DCB;
	original = new DCB;
#else
	current = new struct termios;
	original = new struct termios;
#endif
}

void Serial::endSerial(void)
{
#ifdef	WIN32
	if(dev == INVALID_HANDLE_VALUE && original)
		SetCommState(dev, (DCB *)original);

	if(current)
		delete (DCB *)current;

	if(original)
		delete (DCB *)original;
#else
	if(dev < 0 && original)
		tcsetattr(dev, TCSANOW, (struct termios *)original);

	if(current)
		delete (struct termios *)current;

	if(original)
		delete (struct termios *)original;
#endif
	Serial::close();

	current = NULL;
	original = NULL;

}

Serial::Error Serial::error(Error err, char *errs)
{
	errid = err;
	errstr = errs;
	if(!err)
		return err;

	if(flags.thrown)
		return err;

	// prevents recursive throws

	flags.thrown = true;
#ifdef	CCXX_EXCEPTIONS
	if(Thread::getException() == Thread::throwObject)
		throw((Serial *)this);
#ifdef	COMMON_STD_EXCEPTION
	else if(Thread::getException() == Thread::throwException)
	{
		if(!errs)
			errs = "";
		throw SerException(String(errs));
	}
#endif
#endif
	return err;                                                             
}

int Serial::setPacketInput(int size, unsigned char btimer)
{
#ifdef	WIN32
	//	Still to be done......
	return 0;
#else

#ifdef	_PC_MAX_INPUT
	int max = fpathconf(dev, _PC_MAX_INPUT);
#else
	int max = MAX_INPUT;
#endif
	struct termios *attr = (struct termios *)current;

	if(size > max)
		size = max;

	attr->c_cc[VEOL] = attr->c_cc[VEOL2] = 0;
	attr->c_cc[VMIN] = (unsigned char)size;
	attr->c_cc[VTIME] = btimer;
	attr->c_lflag &= ~ICANON;
	tcsetattr(dev, TCSANOW, attr);
	bufsize = size;
	return size;
#endif
}

int Serial::setLineInput(char newline, char nl1)
{
#ifdef	WIN32
	//	Still to be done......
	return 0;
#else

	struct termios *attr = (struct termios *)current;
	attr->c_cc[VMIN] = attr->c_cc[VTIME] = 0;
	attr->c_cc[VEOL] = newline;
	attr->c_cc[VEOL2] = nl1;
	attr->c_lflag |= ICANON;
	tcsetattr(dev, TCSANOW, attr);
#ifdef _PC_MAX_CANON
	bufsize = fpathconf(dev, _PC_MAX_CANON);
#else
	bufsize = MAX_CANON;
#endif
	return bufsize;
#endif

}

void Serial::flushInput(void)
{
#ifdef	WIN32
	PurgeComm(dev, PURGE_RXABORT | PURGE_RXCLEAR);
#else
	tcflush(dev, TCIFLUSH);
#endif
}

void Serial::flushOutput(void)
{
#ifdef	WIN32
	PurgeComm(dev, PURGE_TXABORT | PURGE_TXCLEAR);
#else
	tcflush(dev, TCOFLUSH);
#endif
}

void Serial::waitOutput(void)
{
#ifdef	WIN32

#else
	tcdrain(dev);
#endif
}

Serial &Serial::operator=(const Serial &ser)
{
	Serial::close();

	if(ser.dev < 0)
		return *this;

#ifdef	WIN32
	HANDLE process = GetCurrentProcess();

	int result = DuplicateHandle(process, ser.dev, process, &dev, DUPLICATE_SAME_ACCESS, 0, 0);

	if (0 == result)
	{
		memcpy(current, ser.current, sizeof(DCB));
		memcpy(original, ser.original, sizeof(DCB));
	}
#else
	dev = dup(ser.dev);

	memcpy(current, ser.current, sizeof(struct termios));
	memcpy(original, ser.original, sizeof(struct termios));
#endif
	return *this;
}


void Serial::open(const char * fname)
{

#ifndef	WIN32
	int cflags = O_RDWR | O_NDELAY;
	dev = ::open(fname, cflags);
	if(dev > -1)
		initConfig();
#else
	// open COMM device
	dev = CreateFile(fname,
		GENERIC_READ | GENERIC_WRITE,
		0,                    // exclusive access
		NULL,                 // no security attrs
		OPEN_EXISTING,
		FILE_FLAG_OVERLAPPED | FILE_ATTRIBUTE_NORMAL | FILE_FLAG_WRITE_THROUGH | FILE_FLAG_NO_BUFFERING,
		NULL);
	if(dev != INVALID_HANDLE_VALUE)
		initConfig();
#endif
}

#ifdef	WIN32
int Serial::aRead(char * Data, const int Length)
{

	unsigned long	dwLength = 0, dwError, dwReadLength;
	COMSTAT	cs;
	OVERLAPPED ol;

	// Return zero if handle is invalid
	if(dev == INVALID_HANDLE_VALUE)
		return 0;

	// Read max length or only what is available
	ClearCommError(dev, &dwError, &cs);

	// If not requiring an exact byte count, get whatever is available
	if(dwLength > (int)cs.cbInQue)
		dwReadLength = cs.cbInQue;
	else
		dwReadLength = Length;

	memset(&ol, 0, sizeof(OVERLAPPED));
	ol.hEvent = CreateEvent(NULL, TRUE, FALSE, NULL);

	if(dwReadLength > 0)
	{
		if(ReadFile(dev, Data, dwReadLength, &dwLength, &ol) == FALSE) 
		{
			if(GetLastError() == ERROR_IO_PENDING)
			{
				WaitForSingleObject(ol.hEvent, INFINITE);
				GetOverlappedResult(dev, &ol, &dwLength, TRUE);
			}
			else
				ClearCommError(dev, &dwError, &cs);
		}
	}

	if(ol.hEvent != INVALID_HANDLE_VALUE)
		CloseHandle(ol.hEvent);

	return dwLength;
}

int Serial::aWrite(const char * Data, const int Length)
{
	COMSTAT	cs;
	unsigned long dwError = 0;
	OVERLAPPED ol;

	// Clear the com port of any error condition prior to read
	ClearCommError(dev, &dwError, &cs);

	unsigned long retSize = 0;

	memset(&ol, 0, sizeof(OVERLAPPED));
	ol.hEvent = CreateEvent(NULL, TRUE, FALSE, NULL);

	if(WriteFile(dev, Data, Length, &retSize, &ol) == FALSE)
	{
		if(GetLastError() == ERROR_IO_PENDING)
		{
			WaitForSingleObject(ol.hEvent, INFINITE);
			GetOverlappedResult(dev, &ol, &retSize, TRUE);
		}
		else
			ClearCommError(dev, &dwError, &cs);
	}

	if(ol.hEvent != INVALID_HANDLE_VALUE)
		CloseHandle(ol.hEvent);

	return retSize;
}
#else
int Serial::aRead(char *Data, const int Length)
{
	return ::read(dev, Data, Length);
}

int Serial::aWrite(const char *Data, const int Length)
{
	return ::write(dev, Data, Length);
}
#endif

void Serial::close()
{
#ifdef	WIN32
	CloseHandle(dev);
#else
	::close(dev);
#endif

	dev = INVALID_HANDLE_VALUE;	
}


Serial::Error Serial::setSpeed(unsigned long speed)
{
	unsigned long rate;

	switch(speed)
	{
#ifdef B115200
case 115200: 
	rate = B115200;
	break; 
#endif
#ifdef B57600
case 57600: 
	rate = B57600;
	break; 
#endif
#ifdef B38400
case 38400: 
	rate = B38400;
	break; 
#endif
case 19200:
	rate = B19200;
	break;
case 9600:
	rate = B9600;
	break;
case 4800:
	rate = B4800;
	break;                                                          
case 2400:
	rate = B2400;
	break;
case 1200:
	rate = B1200;
	break;
case 600:
	rate = B600;
	break;
case 300:
	rate = B300;
	break;
case 110:
	rate = B110;
	break;
#ifdef	B0
case 0:
	rate = B0;
	break;
#endif
default:
	return error(errSpeedInvalid);
	}

#ifdef	WIN32

	DCB		* dcb = (DCB *)current;
	dcb->DCBlength = sizeof(DCB);
	GetCommState(dev, dcb);

	dcb->BaudRate = rate;
	SetCommState(dev, dcb) ;

#else
	struct termios *attr = (struct termios *)current;
	cfsetispeed(attr, rate);
	cfsetospeed(attr, rate);
	tcsetattr(dev, TCSANOW, attr);
#endif
	return errSuccess;
}

Serial::Error Serial::setFlowControl(Flow flow)
{
#ifdef	WIN32

	DCB * attr = (DCB *)current;
	attr->XonChar = ASCII_XON;
	attr->XoffChar = ASCII_XOFF;
	attr->XonLim = 100;
	attr->XoffLim = 100;

	switch(flow)
	{
	case flowSoft:
		attr->fInX = attr->fOutX = 1;
		break;
	case flowBoth:
		attr->fInX = attr->fOutX = 1;
	case flowHard:
		attr->fOutxCtsFlow = 1;
		attr->fRtsControl = RTS_CONTROL_HANDSHAKE;
		break;
	case flowNone:
		break;
	default:
		return error(errFlowInvalid);
	}

	SetCommState(dev, attr);
#else

	struct termios *attr = (struct termios *)current;

	attr->c_cflag &= ~CRTSCTS;
	attr->c_iflag &= ~(IXON | IXANY | IXOFF);

	switch(flow)
	{
	case flowSoft:
		attr->c_iflag |= (IXON | IXANY | IXOFF);
		break;
	case flowBoth:
		attr->c_iflag |= (IXON | IXANY | IXOFF);
	case flowHard:
		attr->c_cflag |= CRTSCTS;
		break;
	case flowNone:
		break;
	default:
		return error(errFlowInvalid);
	}                                                                       

	tcsetattr(dev, TCSANOW, attr);

#endif
	return errSuccess;
}

Serial::Error Serial::setStopBits(int bits)
{
#ifdef	WIN32

	DCB * attr = (DCB *)current;
	switch(bits)
	{
	case 1:
		attr->StopBits = ONESTOPBIT;
		break;
	case 2:
		attr->StopBits = TWOSTOPBITS;
		break;
	default:
		return error(errStopbitsInvalid);
	}

	SetCommState(dev, attr);
#else
	struct termios *attr = (struct termios *)current;
	attr->c_cflag &= ~CSTOPB;

	switch(bits)
	{
	case 1:
		break;
	case 2:
		attr->c_cflag |= CSTOPB;
		break;
	default:
		return error(errStopbitsInvalid);
	}
	tcsetattr(dev, TCSANOW, attr);
#endif
	return errSuccess;
}

Serial::Error Serial::setCharBits(int bits)
{
#ifdef	WIN32

	DCB * attr = (DCB *)current;
	switch(bits)
	{
	case 5:
	case 6:
	case 7:
	case 8:
		attr->ByteSize = bits;
		break;
	default:
		return error(errCharsizeInvalid);
	}
	SetCommState(dev, attr);
#else
	struct termios *attr = (struct termios *)current;
	attr->c_cflag &= ~CSIZE;

	switch(bits)
	{
	case 5:
		attr->c_cflag |= CS5;
		break;
	case 6:
		attr->c_cflag |= CS6;
		break;
	case 7:
		attr->c_cflag |= CS7;
		break;
	case 8:
		attr->c_cflag |= CS8;
		break;
	default:
		return error(errCharsizeInvalid);
	}
	tcsetattr(dev, TCSANOW, attr);
#endif
	return errSuccess;
}

Serial::Error Serial::setParity(Parity parity)
{
#ifdef	WIN32

	DCB * attr = (DCB *)current;
	switch(parity)
	{
	case parityEven:
		attr->Parity = EVENPARITY;
		break;
	case parityOdd:
		attr->Parity = ODDPARITY;
		break;
	case parityNone:
		attr->Parity = NOPARITY;
		break;
	default:
		return error(errParityInvalid);
	}
	SetCommState(dev, attr);
#else
	struct termios *attr = (struct termios *)current;
	attr->c_cflag &= ~(PARENB | PARODD);

	switch(parity)
	{
	case parityEven:
		attr->c_cflag |= PARENB;
		break;
	case parityOdd:
		attr->c_cflag |= (PARENB | PARODD);
		break;
	case parityNone:
		break;
	default:
		return error(errParityInvalid);
	}
	tcsetattr(dev, TCSANOW, attr);
#endif
	return errSuccess;
}

void Serial::sendBreak(void)
{
#ifdef	WIN32
	SetCommBreak(dev);
	//Thread::sleep(100L);
	::Sleep(100);
	ClearCommBreak(dev);
#else
	tcsendbreak(dev, 0);
#endif
}

void Serial::toggleDTR(timeout_t millisec)
{
#ifdef	WIN32
	EscapeCommFunction(dev, CLRDTR);
	if(millisec)
	{
		::Sleep(millisec);
		//Thread::sleep(millisec);
		EscapeCommFunction(dev, SETDTR);
	}

#else
	struct termios tty, old;
	tcgetattr(dev, &tty);
	tcgetattr(dev, &old);
	cfsetospeed(&tty, B0);
	cfsetispeed(&tty, B0);
	tcsetattr(dev, TCSANOW, &tty);

	if(millisec)
	{
		sleep(millisec);
		tcsetattr(dev, TCSANOW, &old);
	}
#endif
}

bool Serial::isPending(Pending pending, timeout_t timeout)
{
#ifdef	WIN32
	unsigned long	dwError;
	COMSTAT	cs;

	ClearCommError(dev, &dwError, &cs);

	if(timeout == 0 || ((pending == pendingInput) && (0 != cs.cbInQue)) ||
		((pending == pendingOutput) && (0 != cs.cbOutQue)) || (pending == pendingError)) 
	{
		switch(pending)
		{
		case pendingInput:
			return (0 != cs.cbInQue);
		case pendingOutput:
			return (0 != cs.cbOutQue);
		case pendingError:
			return false;
		}
	}
	else
	{
		// Thread::Cancel save = Thread::enterCancel();
		OVERLAPPED ol;
		DWORD dwMask;
		BOOL suc;

		memset(&ol, 0, sizeof(OVERLAPPED));
		ol.hEvent = CreateEvent(NULL, TRUE, FALSE, NULL);

		if(pending == pendingInput)
			dwMask = EV_RXCHAR;
		else if(pending == pendingOutput)
			dwMask = EV_TXEMPTY;
		else   // on error
			dwMask = EV_ERR;

		SetCommMask(dev, dwMask);
		// let's wait for event or timeout
		if((suc = WaitCommEvent(dev, &dwMask, &ol)) == FALSE)
		{
			if(GetLastError() == ERROR_IO_PENDING)
			{
				dwError = WaitForSingleObject(ol.hEvent, timeout);
				suc = (dwError == WAIT_OBJECT_0);
				SetCommMask(dev, 0);
			}
			else
				ClearCommError(dev, &dwError, &cs);
		}

		if(ol.hEvent != INVALID_HANDLE_VALUE)
			CloseHandle(ol.hEvent);

		//Thread::exitCancel(save);
		if(suc == FALSE)
			return false;
		return true;
	}
#else


	int status;
#ifdef HAVE_POLL
	struct pollfd pfd;	

	pfd.fd = dev;
	pfd.revents = 0;
	switch(pending)
	{
	case pendingInput:
		pfd.events = POLLIN;
		break;
	case pendingOutput:
		pfd.events = POLLOUT;
		break;
	case pendingError:
		pfd.events = POLLERR | POLLHUP;
		break;
	}

	status = 0;
	while(status < 1)
	{
		if(timeout == TIMEOUT_INF)
			status = poll(&pfd, 1, -1);
		else
			status = poll(&pfd, 1, timeout);

		if(status < 1)
		{
			if(status == -1 && errno == EINTR)
				continue;
			return false;
		}
	}

	if(pfd.revents & pfd.events)
		return true;

#else
	struct timeval tv;
	fd_set grp;
	struct timeval *tvp = &tv;

	if(timeout == TIMEOUT_INF)
		tvp = NULL;
	else
	{
		tv.tv_usec = (timeout % 1000) * 1000;
		tv.tv_sec = timeout / 1000;
	}

	FD_ZERO(&grp);
	FD_SET(dev, &grp);
	switch(pending)
	{
	case pendingInput:
		status = select(dev + 1, &grp, NULL, NULL, tvp);
		break;
	case pendingOutput:
		status = select(dev + 1, NULL, &grp, NULL, tvp);
		break;
	case pendingError:
		status = select(dev + 1, NULL, NULL, &grp, tvp);
		break;
	}
	if(status < 1)
		return false;

	if(FD_ISSET(dev, &grp))
		return true;

#endif

#endif	//	WIN32

	return false;
}





TTYStream::TTYStream(const char *filename, timeout_t to)
:	streambuf(),
Serial(filename),
#ifdef	HAVE_OLD_IOSTREAM
iostream()
#else 
iostream((streambuf *)this) 
#endif
{
#ifdef	HAVE_OLD_IOSTREAM
	init((streambuf *)this);
#endif
	gbuf = pbuf = NULL;
	timeout = to;

	if(INVALID_HANDLE_VALUE != dev)
		allocate();
}

TTYStream::TTYStream() 
:	streambuf(), 
Serial(),
#ifdef	HAVE_OLD_IOSTREAM
iostream()
#else
iostream((streambuf *)this)
#endif
{
#ifdef	HAVE_OLD_IOSTREAM
	init((streambuf *)this);
#endif
	timeout = 0;
	gbuf = pbuf = NULL;
}

TTYStream::~TTYStream()
{
	endStream();
	endSerial();
}

void TTYStream::endStream(void)
{
	if(bufsize)
		sync();

	if(gbuf)
	{
		delete[] gbuf;
		gbuf = NULL;
	}
	if(pbuf)
	{
		delete[] pbuf;
		pbuf = NULL;
	}
	bufsize = 0;
	clear();
}

void TTYStream::allocate(void)
{
	if(INVALID_HANDLE_VALUE == dev)
		return;

#ifdef _PC_MAX_INPUT
	bufsize = fpathconf(dev, _PC_MAX_INPUT);
#else
	bufsize = MAX_INPUT;
#endif

	gbuf = new char[bufsize];
	pbuf = new char[bufsize];

	if(!pbuf || !gbuf)
	{
		error(errResourceFailure);
		return;
	}

	clear();

#if !(defined(STLPORT) || defined(__KCC))
	setg(gbuf, gbuf + bufsize, 0);
#endif

	setg(gbuf, gbuf + bufsize, gbuf + bufsize);
	setp(pbuf, pbuf + bufsize);
}

int TTYStream::doallocate()
{
	if(bufsize)
		return 0;

	allocate();
	return 1;
}

void TTYStream::interactive(bool iflag)
{
#ifdef	WIN32
	if(dev == INVALID_HANDLE_VALUE)
#else
	if(dev < 0)
#endif
		return;

	if(bufsize >= 1)
		endStream();

	if(iflag)
	{
		// setting to unbuffered mode

		bufsize = 1;
		gbuf = new char[bufsize];

#if !(defined(STLPORT) || defined(__KCC))
#if defined(__GNUC__) && (__GNUC__ < 3)
		setb(0,0);
#endif 
#endif
		setg(gbuf, gbuf+bufsize, gbuf+bufsize);
		setp(pbuf, pbuf);
		return;
	}

	if(bufsize < 2)
		allocate();
}

int TTYStream::uflow(void)
{
	int rlen;
	unsigned char ch;

	if(bufsize < 2)
	{
		if(timeout)
		{
			if(Serial::isPending(pendingInput, timeout))
				rlen = aRead((char *)&ch, 1);
			else
				rlen = -1;
		}
		else
			rlen = aRead((char *)&ch, 1);
		if(rlen < 1)
		{
			if(rlen < 0)
				clear(ios::failbit | rdstate());
			return EOF;
		}
		return ch;
	}
	else
	{
		ch = underflow();
		gbump(1);
		return ch;
	}
}

int TTYStream::underflow(void)
{
	unsigned int rlen = 1;

	if(!gptr())
		return EOF;

	if(gptr() < egptr())
		return (unsigned char)*gptr();

	rlen = (unsigned int)((gbuf + bufsize) - eback());
	if(timeout && !Serial::isPending(pendingInput, timeout))
		rlen = -1;
	else
		rlen = aRead((char *)eback(), rlen);

	if(rlen < 1)
	{
		if(rlen < 0)
		{
			clear(ios::failbit | rdstate());
			error(errInput);
		}
		return EOF;
	}

	setg(eback(), eback(), eback() + rlen);
	return (unsigned char) *gptr();
}

int TTYStream::sync(void)
{
	if(bufsize > 1 && pbase() && ((pptr() - pbase()) > 0))
	{
		overflow(0);
		waitOutput();
		setp(pbuf, pbuf + bufsize);
	}
	setg(gbuf, gbuf + bufsize, gbuf + bufsize);
	return 0;
}

int TTYStream::overflow(int c)
{
	unsigned char ch;
	unsigned int rlen, req;

	if(bufsize < 2)
	{
		if(c == EOF)
			return 0;

		ch = (unsigned char)(c);
		rlen = aWrite((char *)&ch, 1);
		if(rlen < 1)
		{
			if(rlen < 0)
				clear(ios::failbit | rdstate());
			return EOF;
		}
		else
			return c;
	}

	if(!pbase())
		return EOF;

	req = (unsigned int)(pptr() - pbase());
	if(req)
	{
		rlen = aWrite((char *)pbase(), req);
		if(rlen < 1)
		{
			if(rlen < 0)
				clear(ios::failbit | rdstate());
			return EOF;
		}
		req -= rlen;
	}

	if(req)
		//		memmove(pptr(), pptr() + rlen, req);
		memmove(pbuf, pbuf + rlen, req);
	setp(pbuf + req, pbuf + bufsize);

	if(c != EOF)
	{
		*pptr() = (unsigned char)c;
		pbump(1);
	}
	return c;
}

bool TTYStream::isPending(Pending pending, timeout_t timer)
{
	//	if(pending == pendingInput && in_avail())
	//		return true;
	//	else if(pending == pendingOutput)
	//		flush();

	return Serial::isPending(pending, timer);
}






ttystream::ttystream() :
TTYStream()
{
	setError(false);
}

ttystream::ttystream(const char *name) :
TTYStream()
{
	setError(false);
	open(name);
}

void ttystream::close(void)
{
#ifdef	WIN32
	if (INVALID_HANDLE_VALUE == dev)
#else
	if(dev < 0)
#endif
		return;

	endStream();
	restore();
	TTYStream::close();
}

void ttystream::open(const char *name)
{
	const char *cpp;
	char *cp;
	char pathname[256];
	size_t namelen;
	long opt;

	if (INVALID_HANDLE_VALUE != dev)
	{
		restore();
		close();
	}

	cpp = strrchr(name, ':');
	if(cpp)
		namelen = cpp - name;
	else
		namelen = strlen(name);
#ifdef WIN32
	cp = pathname+4;
	pathname[0]='\\';
	pathname[1]='\\';
	pathname[2]='.';
	pathname[3]='\\';
	//eg:	"\\\\\.\\com13"

#else
	cp = pathname;
	if(*name != '/')
	{
		strcpy(pathname, "/dev/");
		cp += 5;
	}

	if((cp - pathname) + namelen > 255)
	{
		error(errResourceFailure);
		return;
	}
#endif
	setString(cp, pathname - cp + sizeof(pathname), name);
	cp += namelen;
	*cp = 0;

	Serial::open(pathname);

	if(INVALID_HANDLE_VALUE == dev)
	{
		error(errOpenFailed);
		return;
	}

	allocate();

	setString(pathname, sizeof(pathname), name + namelen);
	cp = pathname + 1;

	if(*pathname == ':')
		cp = strtok(cp, ",");
	else
		cp = NULL;

	while(cp)
	{
		switch(*cp)
		{
		case 'h':
		case 'H':
			setFlowControl(flowHard);
			break;
		case 's':
		case 'S':
			setFlowControl(flowSoft);
			break;
		case 'b':
		case 'B':
			setFlowControl(flowBoth);
			break;
		case 'n':
		case 'N':
			setParity(parityNone);
			break;
		case 'O':
		case 'o':
			setParity(parityOdd);
			break;
		case 'e':
		case 'E':
			setParity(parityEven);
			break;
		case '0':
		case '1':
		case '2':
		case '3':
		case '4':
		case '5':
		case '6':
		case '7':
		case '8':
		case '9':
			opt = atol(cp);
			if(opt == 1 || opt == 2)
			{
				setStopBits((int)opt);
				break;
			}
			if(opt > 4 && opt < 9)
			{
				setCharBits((int)opt);
				break;
			}
			setSpeed(opt);
			break;
		default:
			error(errOptionInvalid);
		}
		cp = strtok(NULL, ","); 
	}			
}



#ifdef NOT_SUPPORT_FEATURE
//	Not supporting this right now........
//

SerialPort::SerialPort(SerialService *svc, const char *name) :
Serial(name),
detect_pending(true),
detect_output(false),
detect_disconnect(true)
{
	next = prev = NULL;
	service = NULL;

#ifdef	WIN32
	if(INVALID_HANDLE_VALUE != dev)
#else
	if(dev > -1)
#endif
	{
		setError(false);
		service = svc;
		svc->attach(this);
	}
}

SerialPort::~SerialPort()
{
	if(service)
		service->detach(this);

	endSerial();
}

void SerialPort::expired(void)
{
}

void SerialPort::pending(void)
{
}

void SerialPort::disconnect(void)
{
}

void SerialPort::output(void)
{
}

void SerialPort::setTimer(timeout_t ptimer)
{
	TimerPort::setTimer(ptimer);
	service->update();
}

void SerialPort::incTimer(timeout_t ptimer)
{
	TimerPort::incTimer(ptimer);
	service->update();
}


void SerialPort::setDetectPending( bool val )
{
	if ( detect_pending != val ) {
		detect_pending = val;
#ifdef USE_POLL
		if ( ufd ) {
			if ( val ) {
				ufd->events |= POLLIN;
			} else {
				ufd->events &= ~POLLIN;
			}
		}
#endif
		service->update();
	}
}

void SerialPort::setDetectOutput( bool val )
{
	if ( detect_output != val ) {
		detect_output = val;
#ifdef  USE_POLL
		if ( ufd ) {
			if ( val ) {
				ufd->events |= POLLOUT;
			} else {
				ufd->events &= ~POLLOUT;
			}
		}
#endif
		service->update();
	}
}


SerialService::SerialService(int pri, size_t stack, const char *id) :
Thread(pri, stack), Mutex(id)
{
	long opt;

	first = last = NULL;
	count = 0;
	FD_ZERO(&connect);
	::pipe(iosync);
	hiwater = iosync[0] + 1;
	FD_SET(iosync[0], &connect);

	opt = fcntl(iosync[0], F_GETFL);
	fcntl(iosync[0], F_SETFL, opt | O_NDELAY);	
}

SerialService::~SerialService()
{
	update(0);
	terminate();

	while(first)
		delete first;
}

void SerialService::onUpdate(unsigned char flag)
{
}

void SerialService::onEvent(void)
{
}

void SerialService::onCallback(SerialPort *port)
{
}

void SerialService::attach(SerialPort *port)
{
	enterMutex();
#ifdef	USE_POLL
	port->ufd = 0;
#endif
	if(last)
		last->next = port;

	port->prev = last;
	last = port;
	FD_SET(port->dev, &connect);
	if(port->dev >= hiwater)
		hiwater = port->dev + 1;

	if(!first)
	{
		first = port;
		leaveMutex();
		++count;
		start();
	}
	else
	{
		leaveMutex();
		update();
		++count;
	}
}

void SerialService::detach(SerialPort *port)
{
	enterMutex();

#ifndef USE_POLL
	FD_CLR(port->dev, &connect);
#endif

	if(port->prev)
		port->prev->next = port->next;
	else
		first = port->next;

	if(port->next)
		port->next->prev = port->prev;
	else
		last = port->prev;

	--count;
	leaveMutex();
	update();
}

void SerialService::update(unsigned char flag)
{
	::write(iosync[1], (char *)&flag, 1);
}


void SerialService::run(void)
{
	timeout_t timer, expires;
	SerialPort *port;
	unsigned char buf;

#ifdef	USE_POLL

	Poller	mfd;
	pollfd *p_ufd;
	int lastcount = 0;

	// initialize ufd in all attached ports :
	// probably don't need this but it can't hurt.
	enterMutex();
	port = first;
	while(port)
	{
		port->ufd = 0;
		port = port->next;
	}
	leaveMutex();

#else
	struct timeval timeout, *tvp;
	fd_set inp, out, err;
	int dev;
	FD_ZERO(&inp);
	FD_ZERO(&out);
	FD_ZERO(&err);
#endif

	setCancel(cancelDeferred);
	for(;;)
	{
		timer = TIMEOUT_INF;
		while(1 == ::read(iosync[0], (char *)&buf, 1))
		{
			if(buf)
			{
				onUpdate(buf);
				continue;
			}

			setCancel(cancelImmediate);
			sleep(TIMEOUT_INF);
			exit();
		}

#ifdef	USE_POLL

		bool	reallocate = false;

		enterMutex();
		onEvent();
		port = first;
		while(port)
		{
			onCallback(port);
			if ( ( p_ufd = port->ufd ) ) {

				if ( ( POLLHUP | POLLNVAL ) & p_ufd->revents ) {
					// Avoid infinite loop from disconnected sockets
					port->detect_disconnect = false;
					p_ufd->events &= ~POLLHUP;
					port->disconnect();
				}

				if ( ( POLLIN | POLLPRI ) & p_ufd->revents )
					port->pending();

				if ( POLLOUT & p_ufd->revents )
					port->output();

			} else {
				reallocate = true;
			}

retry:
			expires = port->getTimer();
			if(expires > 0)
				if(expires < timer)
					timer = expires;

			if(!expires)
			{
				port->endTimer();
				port->expired();
				goto retry;
			}

			port = port->next;
		}

		//
		// reallocate things if we saw a ServerPort without
		// ufd set !
		if ( reallocate || ( ( count + 1 ) != lastcount ) ) {
			lastcount = count + 1;
			p_ufd = mfd.getList( count + 1 );

			// Set up iosync polling
			p_ufd->fd = iosync[0];
			p_ufd->events = POLLIN | POLLHUP;
			p_ufd ++;

			port = first;
			while(port)
			{
				p_ufd->fd = port->dev;
				p_ufd->events =
					( port->detect_disconnect ? POLLHUP : 0 )
					| ( port->detect_output ? POLLOUT : 0 )
					| ( port->detect_pending ? POLLIN : 0 )
					;
				port->ufd = p_ufd;
				p_ufd ++;
				port = port->next;
			}
		}
		leaveMutex();

		poll( mfd.getList(), count + 1, timer );

#else
		enterMutex();
		onEvent();
		port = first;

		while(port)
		{
			onCallback(port);
			dev = port->dev;
			if(FD_ISSET(dev, &err)) {
				port->detect_disconnect = false;
				port->disconnect();
			}

			if(FD_ISSET(dev, &inp))
				port->pending();

			if(FD_ISSET(dev, &out))
				port->output();

retry:
			expires = port->getTimer();
			if(expires > 0)
				if(expires < timer)
					timer = expires;

			if(!expires)
			{
				port->endTimer();
				port->expired();
				goto retry;
			}

			port = port->next;
		}

		FD_ZERO(&inp);
		FD_ZERO(&out);
		FD_ZERO(&err);
		int so;
		port = first;
		while(port)
		{
			so = port->dev;

			if(port->detect_pending)
				FD_SET(so, &inp);

			if(port->detect_output)
				FD_SET(so, &out);

			if(port->detect_disconnect)
				FD_SET(so, &err);

			port = port->next;
		}

		leaveMutex();
		if(timer == TIMEOUT_INF)
			tvp = NULL;
		else
		{
			tvp = &timeout;
			timeout.tv_sec = timer / 1000;
			timeout.tv_usec = (timer % 1000) * 1000;
		}
		select(hiwater, &inp, &out, &err, tvp);		
#endif
	}
}						


//---------------------------------------------------



#endif

