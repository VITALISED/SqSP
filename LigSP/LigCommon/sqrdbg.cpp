/*
	see copyright notice in sqrdbg.h
*/
#include <squirrel.h>
#ifdef WIN32
#include <winsock.h>
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
#define SOCKET int
#endif
#include <squirrel.h>
#include <squirrel.h>
#include "sqrdbg.h"
#include "sqdbgserver.h"
int debug_hook(HSQUIRRELVM v);
int error_handler(HSQUIRRELVM v);

#include "serialize_state.inl"
#ifndef SOCKET_ERROR
#define SOCKET_ERROR -1
#endif
#ifndef INVALID_SOCKET
#define INVALID_SOCKET 0
#endif
HSQREMOTEDBG sq_rdbg_init(HSQUIRRELVM v,unsigned short port,SQBool autoupdate)
{
#ifdef _WIN32
	sockaddr_in bindaddr;
	WSADATA wsadata;
	if (WSAStartup (MAKEWORD(1,1), &wsadata) != 0){
		return NULL;
	}	
#else
	sockaddr_in bindaddr;
#endif 
	SQDbgServer *rdbg = new SQDbgServer(v);
	rdbg->_autoupdate = autoupdate?true:false;
	rdbg->_accept = socket(AF_INET,SOCK_STREAM,0);
	bindaddr.sin_family = AF_INET;
	bindaddr.sin_port = htons(port);
	bindaddr.sin_addr.s_addr = htonl (INADDR_ANY);
	if(bind(rdbg->_accept,(sockaddr*)&bindaddr,sizeof(bindaddr))==SOCKET_ERROR){
		delete rdbg;
		sq_throwerror(v,_SC("failed to bind the socket"));
		return NULL;
	}
	if(!rdbg->Init()) {
		delete rdbg;
		sq_throwerror(v,_SC("failed to initialize the debugger"));
		return NULL;
	}
	
    return rdbg;
}

SQRESULT sq_rdbg_waitforconnections(HSQREMOTEDBG rdbg)
{
	if(SQ_FAILED(sq_compilebuffer(rdbg->_v,serialize_state_nut,(SQInteger)scstrlen(serialize_state_nut),_SC("SERIALIZE_STATE"),SQFalse))) {
		sq_throwerror(rdbg->_v,_SC("error compiling the serialization function"));
	}
	sq_getstackobj(rdbg->_v,-1,&rdbg->_serializefunc);
	sq_addref(rdbg->_v,&rdbg->_serializefunc);
	sq_pop(rdbg->_v,1);

	sockaddr_in cliaddr;
#ifdef WIN32
	int addrlen=sizeof(cliaddr);
#else
	socklen_t addrlen=sizeof(cliaddr);
#endif
	if(listen(rdbg->_accept,0)==SOCKET_ERROR)
		return sq_throwerror(rdbg->_v,_SC("error on listen(socket)"));
	rdbg->_endpoint = accept(rdbg->_accept,(sockaddr*)&cliaddr,&addrlen);
	//do not accept any other connection
	sqdbg_closesocket(rdbg->_accept);
	rdbg->_accept = INVALID_SOCKET;
	if(rdbg->_endpoint==INVALID_SOCKET){
		return sq_throwerror(rdbg->_v,_SC("error accept(socket)"));
	}
	while(!rdbg->_ready){
		sq_rdbg_update(rdbg);
	}
	return SQ_OK;
}

SQRESULT sq_rdbg_update(HSQREMOTEDBG rdbg)
{
	struct timeval time;
	time.tv_sec=10;
	time.tv_usec=0;
	fd_set read_flags;
#ifdef WIN32
reselect:
#endif
    FD_ZERO(&read_flags);
	FD_SET(rdbg->_endpoint, &read_flags);

	if(select(rdbg->_endpoint+1,&read_flags,NULL,NULL,NULL)==-1)
	{
#ifdef WIN32
		::Sleep(50); 
		goto reselect;
#endif
	}
	if(FD_ISSET(rdbg->_endpoint,&read_flags)){
		char temp[1024];
		int size=0;
		char c,prev=NULL;
		memset(&temp,0,sizeof(temp));
		int res;
		FD_CLR(rdbg->_endpoint, &read_flags);
		while((res = recv(rdbg->_endpoint,&c,1,0))>0){
			
			if(c=='\n')break;
			if(c!='\r'){
				temp[size]=c;
				prev=c;
				size++;
			}
		}
		switch(res){
		case 0:
			return sq_throwerror(rdbg->_v,_SC("disconnected"));
		case SOCKET_ERROR:
			return sq_throwerror(rdbg->_v,_SC("socket error"));
        }
		
		temp[size]=NULL;
		temp[size+1]=NULL;
		rdbg->ParseMsg(temp);
	}
	return SQ_OK;
}

int debug_hook(HSQUIRRELVM v)
{
	SQUserPointer up;
	int event_type,line;
	const SQChar *src,*func;
	sq_getinteger(v,2,&event_type);
	sq_getstring(v,3,&src);
	sq_getinteger(v,4,&line);
	sq_getstring(v,5,&func);
	sq_getuserpointer(v,-1,&up);
	HSQREMOTEDBG rdbg = (HSQREMOTEDBG)up;
	rdbg->Hook(event_type,line,src,func);
	if(rdbg->_autoupdate) {
		if(SQ_FAILED(sq_rdbg_update(rdbg)))
			return sq_throwerror(v,_SC("socket failed"));
	}
	return 0;
}

int error_handler(HSQUIRRELVM v)
{
	SQUserPointer up;
	const SQChar *sErr=NULL;
	const SQChar *fn=_SC("unknown");
	const SQChar *src=_SC("unknown");
	int line=-1;
	SQStackInfos si;
	sq_getuserpointer(v,-1,&up);
	HSQREMOTEDBG rdbg=(HSQREMOTEDBG)up;
	if(SQ_SUCCEEDED(sq_stackinfos(v,1,&si)))
	{
		if(si.funcname)fn=si.funcname;
		if(si.source)src=si.source;
		line=si.line;
		scprintf(_SC("*FUNCTION [%s] %s line [%d]\n"),fn,src,si.line);
	}
	if(sq_gettop(v)>=1){
		if(SQ_SUCCEEDED(sq_getstring(v,2,&sErr)))	{
			scprintf(_SC("\nAN ERROR HAS OCCURED [%s]\n"),sErr);
			rdbg->Break(si.line,src,_SC("error"),sErr);
		}
		else{
			scprintf(_SC("\nAN ERROR HAS OCCURED [unknown]\n"));
			rdbg->Break(si.line,src,_SC("error"),_SC("unknown"));
		}
	}
	rdbg->BreakExecution();
	return 0;
}


SQRESULT sq_rdbg_shutdown(HSQREMOTEDBG rdbg)
{
	delete rdbg;
#ifdef _WIN32
	WSACleanup();
#endif
	return SQ_OK;
}
