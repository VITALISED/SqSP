#include "sdl_script_support.h"
//#include "lig_httpd.h"
#include "sdl_db_support.h"
#include <map>
#ifdef WIN32

#ifdef _DEBUG

#pragma comment(lib, "squirrel_d.lib")
#pragma comment(lib, "sqstdlib_d.lib")
#else

#pragma comment(lib, "squirrel.lib")
#pragma comment(lib, "sqstdlib.lib")
#endif
#endif

bool GlobalExitFlag=false;  //对于Squirrel线程，要求终止运行的全局标识

bool register_func(HSQUIRRELVM v,const char* class_name,SQFUNCTION f,const char *fname,const char* para_mask,void* pObj)
{
	char rmask[256];
	int i,realPara=0;
	int atleastPara=0;
	if(para_mask!=NULL)
	{
		if(strlen(para_mask)>255) return false;
		realPara=0;i=0;
		while(*para_mask!=0)
		{
			rmask[i]=*para_mask;
			if(*para_mask=='|') {++i;++para_mask;--realPara;continue;}
			if(atleastPara==0 && *para_mask=='-') atleastPara=-realPara;
			++i;++realPara;++para_mask;
		}
		rmask[i]=0;
	}
	if(atleastPara<0) realPara=atleastPara;

	SQInteger top = sq_gettop(v); //saves the stack size before the call
	sq_pushroottable(v);
	sq_pushstring(v,class_name,-1);
	if(SQ_SUCCEEDED(sq_get(v,-2)))
	{
		sq_pushstring(v,fname,-1);
		if(pObj!=NULL) 
		{
			sq_pushuserpointer(v,pObj); //使用自由变量传递
			sq_newclosure(v,f,1); //create a new function with a free var
			if(para_mask!=NULL) sq_setparamscheck(v, realPara,rmask); 
		}
		else 
		{
			sq_newclosure(v,f,0); //create a new function without a free var
			if(para_mask!=NULL) sq_setparamscheck(v,realPara,rmask); //注意，负的参数个数表示至少满足的参数数量
		}
		sq_createslot(v,-3); 
		sq_settop(v,top); //restores the original stack size
		return true;
	}
	sq_settop(v,top); //restores the original stack size
	return false;
}
bool register_func(HSQUIRRELVM v,SQFUNCTION f,const char *fname,const char* para_mask,void* pObj)
{
	SQInteger top = sq_gettop(v); //saves the stack size before the call
	sq_pushroottable(v);

	char rmask[256];
	int i,realPara=0;
	if(para_mask!=NULL)
	{
		if(strlen(para_mask)>255) return false;
		realPara=0;i=0;
		while(*para_mask!=0)
		{
			rmask[i]=*para_mask;
			if(*para_mask=='|') {++i;++para_mask;--realPara;continue;}
			if(*para_mask=='-') realPara=-realPara;	else ++i;
			if(realPara>=0) ++realPara;
			++para_mask;
		}
		rmask[i]=0;
	}

	sq_pushstring(v,fname,-1);
	if(pObj!=NULL) 
	{
		sq_pushuserpointer(v,pObj); //使用自由变量传递
		sq_newclosure(v,f,1); //create a new function with a free var
		if(para_mask!=NULL) sq_setparamscheck(v, realPara,rmask); //注意，负的参数个数表示至少满足的参数数量
	}
	else 
	{
		sq_newclosure(v,f,0); //create a new function without a free var
		if(para_mask!=NULL) sq_setparamscheck(v,realPara,rmask); //注意，负的参数个数表示至少满足的参数数量
	}
	sq_createslot(v,-3); 
	sq_settop(v,top); //restores the original stack size
	return true;

}

bool get_function(HSQUIRRELVM v,const char* function_name,HSQOBJECT &func)
{
	sq_pushroottable(v);
	sq_pushstring(v,function_name,strlen(function_name));
	if(SQ_SUCCEEDED(sq_get(v,-2))) //找到需要页面的执行函数
	{
		SQObjectType tp;
		if((tp=sq_gettype(v,-1))==OT_CLOSURE)
		{
			sq_getstackobj(v,-1,&func);
			return true;
		}
	}
	func._type=OT_NULL;
	func._unVal.pClosure=NULL;  
	return false;
};

bool exec_function(HSQUIRRELVM v,HSQOBJECT func)
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
	sq_settop(v,oldtop);
	return true;
};

bool SqGetVar(HSQUIRRELVM v)
{
	if(sq_gettop(v)!=1) return false;
	return true;
}
bool _get_object(HSQUIRRELVM v,char &val,int idx)
{
	SQInteger ich;
	if(SQ_SUCCEEDED(sq_getinteger(v,idx,&ich))) {val=(char)ich; return true;}
	return false;
}
bool _get_object(HSQUIRRELVM v,int &val,int idx)
{
	if(SQ_SUCCEEDED(sq_getinteger(v,idx,&val))) return true;
	return false;
}
bool _get_object(HSQUIRRELVM v,unsigned int &val,int idx)
{
	if(SQ_SUCCEEDED(sq_getinteger(v,idx,(int*)&val))) return true;
	return false;
}
bool _get_object(HSQUIRRELVM v,unsigned long &val,int idx)
{
	if(SQ_SUCCEEDED(sq_getinteger(v,idx,(int*)&val))) return true;
	return false;
}
bool _get_object(HSQUIRRELVM v,bool &val,int idx)
{
	SQBool bt;
	if(SQ_SUCCEEDED(sq_getbool(v,idx,&bt))) {val=bt;return true;}
	return false;
}
bool _get_object(HSQUIRRELVM v,const char* &val,int idx)
{
	if(SQ_SUCCEEDED(sq_getstring(v,idx,&val))) return true;
	return false;
}
bool _get_object(HSQUIRRELVM v,float &val,int idx)
{
	if(SQ_SUCCEEDED(sq_getfloat(v,idx,&val))) return true;
	return false;
}
bool _get_object(HSQUIRRELVM v,string &val,int idx)
{
	const char* pstr;
	if(SQ_SUCCEEDED(sq_getstring(v,idx,&pstr))) {val=pstr;return true;}
	return false;
}
bool _get_object(HSQUIRRELVM v,SQUserPointer &pObj,int idx)
{
	if(SQ_SUCCEEDED(sq_getuserpointer(v,idx,&pObj))) return true;
	return false;
}
bool _get_object(HSQUIRRELVM v,SQInstancePointer &pObj,int idx)
{
	if(SQ_SUCCEEDED(sq_getinstanceup(v,idx,(SQUserPointer*)&pObj,0))) return true;
	return false;
}
bool _get_object(HSQUIRRELVM v,HSQOBJECT &Obj,int idx)
{
	if(SQ_FAILED(sq_getstackobj(v,idx,(HSQOBJECT*)&Obj))) return false;
	return true;
}
void _push_object(HSQUIRRELVM v){}

void _push_object(HSQUIRRELVM v,int val)
{
	sq_pushinteger(v,val);
}
void _push_object(HSQUIRRELVM v,bool val)
{
	sq_pushbool(v,(SQBool)val);
}
void _push_object(HSQUIRRELVM v,short val)
{
	sq_pushbool(v,(SQInteger)val);
}
void _push_object(HSQUIRRELVM v,const char* val)
{
	sq_pushstring(v,val,-1);
}
void _push_object(HSQUIRRELVM v,char val[])
{
	sq_pushstring(v,val,-1);
}
/*
void _push_object(HSQUIRRELVM v,string& val)
{
	sq_pushstring(v,val.data(),val.length());
}
*/
void _push_object(HSQUIRRELVM v,string val)
{
	sq_pushstring(v,val.data(),val.length());
}
void _push_object(HSQUIRRELVM v,float val)
{
	sq_pushfloat(v,(SQFloat)val);
}
void _push_object(HSQUIRRELVM v,double val)
{
	sq_pushfloat(v,(SQFloat)val);
}

void _push_object(HSQUIRRELVM v,time_t& val)
{
	sq_pushinteger(v,(SQInteger)val);
}
void _push_object(HSQUIRRELVM v,SQUserPointer pObj)
{
	sq_pushuserpointer(v,pObj);
}
void _push_object(HSQUIRRELVM v,SQObject Obj)
{
	sq_pushobject(v,Obj);
}


template<int StateNumber>
class StateSet
{
private:
	bitset<StateNumber> msets;
public:
	StateSet()
	{
		msets.reset();
	}
	StateSet(StateSet &st)
	{
		msets=st.msets;
	}
	void insert(int st)
	{
		msets.set(st); 
	}
	void erase(int st)
	{
		msets.set(st,false);
	}
	inline bool operator[](int st)
	{
		return msets[st];
	}
};
/*
template<int StateNumber>
class StateMachine
{
private:
	struct ruledef
	{
		unsigned short from;
		unsigned short to;
		int   prompt;
	};

	struct less_ruledef
	{
		bool operator()(const ruledef& r1,const ruledef& r2)const
		{
			if(r1.prompt==r2.prompt) return(r1.from<r2.from);
			return (r1.prompt<r2.prompt);  
		}
	};
	multiset<ruledef,less_ruledef> RuleSet;
	typedef StateSet<StateNumber> State;
public:
	void add_rule(unsigned short from,unsigned short to,int prompt)
	{
		ruledef rd;
		rd.from=from;
		rd.to=to;
		rd.prompt=prompt;
		RuleSet.insert(rd);
	};

	void prompt(State &st,int pt)
	{
		StSet rt(st);
		ruledef rd;
		for(int i=0;i<StateNumber;++i)
		{
			if(!st[i]) continue;
			rd.from=i;
			rd.prompt=pt;
			rt.erase(pt);
			pair<multiset<ruledef,less_ruledef>::iterator,multiset<ruledef,less_ruledef>::iterator> rt=RuleSet.equal_range(rd)
				while(rt.first!=rt.second)
				{

					rt.insert((rt.first)->to);
					++(rt.first);
				}
		}
		st=rt;
	}
};



*/
int call_script(HSQUIRRELVM v,const char* function_name)
{
	SQInteger oldtop=sq_gettop(v);
	sq_pushroottable(v);
	sq_pushstring(v,function_name,-1);
	if(SQ_SUCCEEDED(sq_get(v,-2))) //找到需要页面的执行函数
	{
		SQObjectType tp;
		if((tp=sq_gettype(v,-1))==OT_CLOSURE)
		{
			sq_push(v,-2); //push the 'this' (in this case is the global table)
			if(SQ_FAILED(sq_call(v,1,SQTrue,SQTrue)))
			{
				sq_settop(v,oldtop);
				return 0;//执行squirrel函数
			}
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

void compile_error_process(HSQUIRRELVM v,const SQChar * desc,const SQChar *source,SQInteger line,SQInteger column)
{
	cout<<"Error at Line "<<line<<" Column "<<column<<endl;
	cout<<desc<<endl;
	cout<<source<<endl;
}

bool import_script(HSQUIRRELVM v,const char* filename) 
{
	bool rt=false;
	SQInteger oldtop=sq_gettop(v);
	sq_pushroottable(v);
	if(SQ_SUCCEEDED(sqstd_dofile(v,filename,SQTrue,SQTrue))) rt=true;
	sq_settop(v,oldtop);
	return rt;
};

SQInteger SqStdVectorWarp::Sq2CreateHistoryRec(HSQUIRRELVM v) //创建一个数据日志记录
{
	SqStdVectorWarp* handle;
	if(SQ_SUCCEEDED(sq_getinstanceup(v,1,(SQUserPointer*)(&handle),0)) && SqGetVar(v,handle->versionSize))
	{
		if(handle->pHistoryDB==NULL)
			handle->pHistoryDB=new deque<ObjectLst*>;
	}
	return 0;
};

SQInteger SqStdVectorWarp::Sq2GetHistoryRec(HSQUIRRELVM v) //获取数据日志记录的数据
{
	SqStdVectorWarp* handle;
	int idx,loc;
	if(SQ_FAILED(sq_getinstanceup(v,1,(SQUserPointer*)(&handle),0))) return 0;
	if(SqGetVar(v,idx,loc))
	{
		if(idx>=0 && handle->pHistoryDB!=NULL && idx<handle->pHistoryDB->size())
		{
			if(idx>=handle->pHistoryDB->size()) return 0;
			ObjectLst &Lst=*((*handle->pHistoryDB)[handle->pHistoryDB->size()-idx-1]); 
			if(loc>=0 && loc<Lst.size())
			{
				sq_pushobject(v,Lst[loc]);
				return 1;
			}
		}
	}
	int idx2;
	if(SqGetVar(v,idx,idx2,loc))
	{
		if(idx>=0 && handle->pHistoryDB!=NULL && idx<handle->pHistoryDB->size())
		{
			if(idx>=handle->pHistoryDB->size() || idx2>=handle->pHistoryDB->size()) return 0;
			ObjectLst &Lst=*((*handle->pHistoryDB)[idx]); 
			ObjectLst &Lst2=*((*handle->pHistoryDB)[idx2]); 
			if(loc>=0 && loc<Lst.size() && Lst[loc]._type==OT_INTEGER)
			{
				int ret=Lst[loc]._unVal.nInteger ;
				ret-=Lst2[loc]._unVal.nInteger;
				sq_pushinteger(v,ret);
				return 1;
			}
		}
	}
	sq_pushinteger(v,0);
	return 1;
};

SQInteger SqStdVectorWarp::Sq2GetHistorySize(HSQUIRRELVM v) //得到一个数据日志记录长度
{
	SqStdVectorWarp* handle;
	if(SQ_SUCCEEDED(sq_getinstanceup(v,1,(SQUserPointer*)(&handle),0)))
	{
		if(handle->pHistoryDB==NULL) sq_pushinteger(v,0);
		else sq_pushinteger(v,handle->pHistoryDB->size());  
		return 1;
	}
	return 0;
}
SQInteger SqStdVectorWarp::_queryobj_releasehook(SQUserPointer p, SQInteger size) //标准的Squirrel C++对象析构回调
{
	SqStdVectorWarp* handle=(SqStdVectorWarp*)p;
	if(handle->flag_native_vector) delete (handle->pVector); 
	if(handle->pHistoryDB!=NULL) 
	{
		for(deque<ObjectLst*>::iterator it=handle->pHistoryDB->begin();it!=handle->pHistoryDB->end();++it)
			delete (*it);
		delete handle->pHistoryDB;
	}
	delete handle;
	return 0;
}
SQInteger SqStdVectorWarp::Sq2PushHistoryRec(HSQUIRRELVM v) //加入一个数据日志记录
{
	SqStdVectorWarp* handle;
	if(SQ_SUCCEEDED(sq_getinstanceup(v,1,(SQUserPointer*)(&handle),0)))
	{
		if(handle->pHistoryDB!=NULL)
		{
			if(handle->pHistoryDB->size()>handle->versionSize)
			{
				ObjectLst* pOld=handle->pHistoryDB->back();
				handle->pHistoryDB->pop_back(); 
				*pOld=*(handle->pVector); 
				handle->pHistoryDB->push_front(pOld);
				return 0;
			}
			handle->pHistoryDB->push_front(new ObjectLst(*(handle->pVector)));
		}
	}	
	return 0;
};

SQInteger SqStdVectorWarp::Sq2Constructor(HSQUIRRELVM v) //标准的Squirrel C++对象构造方式
{
	static std::map<string,vector<SQObject> > pubVects;
	SqStdVectorWarp* handle=new SqStdVectorWarp;
	handle->flag_native_vector=false;
	handle->pHistoryDB=NULL; 
	sq_setinstanceup(v,1,(SQUserPointer)(handle)); //为本实例保存C++对象指针,本实例在堆栈的底部，所以idx 使用1
	sq_setreleasehook(v,1,_queryobj_releasehook);  //由于在虚拟机上申请对象，此处要求删除对象

	SQInstancePointer pVCT;
	SQInteger size;
	SQInteger versionSize;
	const char* name;
	HSQOBJECT  defaultValue;
	if(SqGetVar(v,pVCT))
	{
		if(check_instancetype(v,"StdVector"))
		{
			handle->pVector=((SqStdVectorWarp*)pVCT)->pVector;
			handle->flag_native_vector=false;
			handle->flag_public_vector=false;
			handle->Iter=handle->pVector->begin();  
		}
		else sq_throwerror(v,"Parameter Require Type StdVector"); 
		return 0;
	}
	if(SqGetVar(v,size))
	{
		HSQOBJECT nobj;
		nobj._type=OT_NULL;
		nobj._unVal.nInteger=0;  
		handle->pVector=new vector<SQObject>;
		handle->flag_native_vector=true;
		handle->flag_public_vector=false;
		handle->pVector->resize(size,nobj);
		handle->Iter=handle->pVector->begin();
		return 0;
	}
	if(SqGetVar(v,name))
	{
		HSQOBJECT nobj;
		nobj._type=OT_NULL;
		nobj._unVal.nInteger=0;  
		handle->name=name;
		handle->lock.lock(); 
		std::map<string,vector<SQObject> >::iterator rt=pubVects.find(handle->name);
		if(rt==pubVects.end())	throw "public vector isn't defined";
		handle->pVector=&(pubVects[handle->name]);
		handle->flag_native_vector=false;
		handle->flag_public_vector=true;
		handle->Iter=handle->pVector->begin();
		handle->lock.unlock(); 
		return 0;
	}
	if(SqGetVar(v,name,size))
	{
		HSQOBJECT nobj;
		nobj._type=OT_NULL;
		nobj._unVal.nInteger=0;  
		handle->name=name;
		handle->lock.lock(); 
		std::map<string,vector<SQObject> >::iterator rt=pubVects.find(handle->name);
		if(rt==pubVects.end())	pubVects[handle->name].resize(size,nobj); //第一次建立时，才能决定尺寸
		handle->pVector=&(pubVects[handle->name]);
		handle->flag_native_vector=false;
		handle->flag_public_vector=true;
		handle->Iter=handle->pVector->begin();
		handle->lock.unlock(); 
		return 0;
	}

	if(SqGetVar(v,name,size,defaultValue))
	{
		handle->name=name;
		handle->lock.lock(); 
		std::map<string,vector<SQObject> >::iterator rt=pubVects.find(handle->name);
		if(rt==pubVects.end())	pubVects[handle->name].resize(size,defaultValue); //第一次建立时，才能决定尺寸
		handle->pVector=&(pubVects[handle->name]);
		handle->flag_native_vector=false;
		handle->flag_public_vector=true;
		handle->Iter=handle->pVector->begin();
		handle->lock.unlock(); 
		return 0;
	}
	
	handle->pVector=new vector<SQObject>;
	handle->flag_native_vector=true;
	handle->flag_public_vector=false;
	handle->Iter=handle->pVector->begin();
	return 0;
}

SQInteger SqStdVectorWarp::Sq2Get(HSQUIRRELVM v) 
{
	int val;

	SqStdVectorWarp* handle;
	if(SQ_SUCCEEDED(sq_getinstanceup(v,1,(SQUserPointer*)(&handle),0)) &&
		(SQ_SUCCEEDED(sq_getinteger(v,-1,&val))))
	{
		if(handle!=NULL && handle->pVector!=NULL && val<handle->pVector->size()) 
		{
			if(handle->flag_public_vector) handle->lock.lock(); 
			HSQOBJECT obj=(*(handle->pVector))[val];
			if(handle->flag_public_vector) handle->lock.unlock(); 
			if(obj._type!=OT_NULL) 
			{
				sq_pushobject(v,obj); 
				return 1;
			}
		}
	}
	
	return 0;
};

SQInteger SqStdVectorWarp::Sq2Set(HSQUIRRELVM v) 
{
	int idx;
	HSQOBJECT val;
	SqStdVectorWarp* handle;
	if(SQ_SUCCEEDED(sq_getinstanceup(v,1,(SQUserPointer*)(&handle),0)) &&
		(SQ_SUCCEEDED(sq_getinteger(v,-2,&idx))))
	{
		if(handle!=NULL && handle->pVector!=NULL && idx<handle->pVector->size())
		{
			sq_getstackobj(v,-1,&val);
			if(val._type==OT_INTEGER || val._type==OT_FLOAT || val._type==OT_USERPOINTER)
			{
				if(handle->flag_public_vector) handle->lock.lock(); 
				(*(handle->pVector))[idx]=val;
				if(handle->flag_public_vector) handle->lock.unlock(); 
				return 1;
			}
		}
	}
	return 0;
};

SQInteger SqStdVectorWarp::Sq2Nexti(HSQUIRRELVM v) 
{
	const char* val;
	SqStdVectorWarp* handle;
	int idx=0;
	if(SQ_SUCCEEDED(sq_getinstanceup(v,1,(SQUserPointer*)(&handle),0)) 
		&& handle->pVector!=NULL)
	{
		if(sq_gettype(v,-1)==OT_NULL)
		{
			handle->Iter=handle->pVector->begin();  
			if(handle->Iter==handle->pVector->end())   return 0;
			sq_pushinteger(v,idx);
			return 1;
		}
		else 
		{
			if(handle->Iter==handle->pVector->end()) return 0;
			sq_getinteger(v,-1,&idx);
			(handle->Iter)++;
			idx++;
		}
		sq_pushinteger(v,idx);
		if(handle->Iter==handle->pVector->end())  return 0; 
		return 1;

	}
	return 0;
};

void SqStdVectorWarp::inject_class(HSQUIRRELVM v)
{
	int top=sq_gettop(v);
	const char* classname="StdVector"; 
	sq_pushroottable(v); //push the root table(were the globals of the script will be stored)
	sq_pushstring(v,classname,-1);
	sq_newclass(v,SQFalse);
	sq_createslot(v,-3);
	sq_settop(v,top); //restores the original stack size

	//'o' null, 'i' integer, 'f' float, 'n' integer or float, 's' string, 't' table, 'a' array, 'u' userdata, 
	//'c' closure and nativeclosure, 'g' generator, 'p' userpointer, 'v' thread, 'x' instance(class instance), 'y' class, 'b' bool. and '.' any type
	register_func(v,classname,Sq2Constructor,"constructor",".-i|sin");//构造函数
	register_func(v,classname,Sq2Nexti,"_nexti","x."); //
	register_func(v,classname,Sq2Get,"_get","xi"); //
	register_func(v,classname,Sq2Set,"_set","xis|n|p"); //
	register_func(v,classname,Sq2Length,"len","x"); //
	register_func(v,classname,Sq2GetHistoryRec,"history_get","xii-i"); //history id, loc;
	register_func(v,classname,Sq2CreateHistoryRec,"create_history","xi");
	register_func(v,classname,Sq2PushHistoryRec,"push_history","x");
	register_func(v,classname,Sq2GetHistorySize,"history_size","x");

}

SQInteger SqStdVectorWarp::Sq2Length(HSQUIRRELVM v) 
{
	SqStdVectorWarp* handle;
	if(SQ_SUCCEEDED(sq_getinstanceup(v,1,(SQUserPointer*)(&handle),0)))
	{
		sq_pushinteger(v,handle->pVector->size());
		return 1;
	}
	return 0;
};

SQInteger SqScanWrap::Sq2Length(HSQUIRRELVM v) 
{
	TreeItem* handle;
	if(SQ_SUCCEEDED(sq_getinstanceup(v,1,(SQUserPointer*)(&handle),0)))
	{
		sq_pushinteger(v,handle->size());
		return 1;
	}
	return 0;
}

SQInteger SqScanWrap::Sq2Convert2String(HSQUIRRELVM v) 
{
	TreeItem* handle;
	if(SQ_SUCCEEDED(sq_getinstanceup(v,1,(SQUserPointer*)(&handle),0)))
	{
		sq_pushinteger(v,handle->size());
		return 1;
	}
	return 0;
}

SQInteger SqScanWrap::_queryobj_releasehook(SQUserPointer p, SQInteger size) //标准的Squirrel C++对象析构回调
{
	return 0;
}

SQInteger SqScanWrap::Sq2Constructor(HSQUIRRELVM v) //标准的Squirrel C++对象构造方式
{
	TreeItem* handle;
	SQInstancePointer pVCT;
	if(SqGetVar(v,pVCT))
	{
		if(check_instancetype(v,"AST"))
		{
			handle=(TreeItem*)pVCT;
			sq_setinstanceup(v,1,(SQUserPointer)(handle)); //为本实例保存C++对象指针,本实例在堆栈的底部，所以idx 使用1
			sq_setreleasehook(v,1,_queryobj_releasehook);  //由于在虚拟机上申请对象，此处要求删除对象
			return 0;
		}
	}
	sq_throwerror(v,"Parameter Require Type AST");
	return 0;
}

SQInteger SqScanWrap::Sq2Get(HSQUIRRELVM v) 
{
	const char* valstr;
	int validx;
	SQUserPointer valpt;
	TreeItem* handle;

	if(SQ_SUCCEEDED(sq_getinstanceup(v,1,(SQUserPointer*)(&handle),0)) )
	{
		if(handle!=NULL && handle->size()>0) 
		{
			TreeItem* tt;
			if(SqGetVar(v,valstr))
				tt=handle->get(valstr);
			else if(SqGetVar(v,valpt))
				tt=(TreeItem*)valpt;
			else if(SqGetVar(v,validx))
				tt=handle->get(validx);

			if(tt==NULL) return 0;
			switch(tt->matched_handle)
			{
			case JSON_STRING :
				sq_pushstring(v,tt->start,tt->controled_size); 
				break;
			case JSON_INTEGER :
				sq_pushinteger(v,tt->to_int());
				break;
			case JSON_FLOAT :
				sq_pushfloat(v,(SQFloat)tt->to_double());
				break;
			default:
				sq_clone(v,1);
				sq_setinstanceup(v,-1,(SQUserPointer)(tt));
			}
			return 1;
		}
	}
	return 0;
};

SQInteger SqScanWrap::Sq2Cmp(HSQUIRRELVM v) 
{
	const char* valstr;
	int validx;
	SQFloat valfloat,tfloat;
	TreeItem* tt;
	int rt=0;
	if(SQ_SUCCEEDED(sq_getinstanceup(v,1,(SQUserPointer*)(&tt),0)) )
	{
		if(tt==NULL || tt->size()==0) 
			goto type_error;
		else
		{

			switch(tt->matched_handle)
			{
			case JSON_STRING :
				if(SqGetVar(v,valstr))
					rt=tt->cstrcmp(valstr);
				else goto type_error;
				break;
			case JSON_INTEGER :
				if(SqGetVar(v,validx))
					rt=tt->to_int()-validx;
				else goto type_error;
				break;
			case JSON_FLOAT :
				if(SqGetVar(v,valfloat))
				{
					tfloat=(SQFloat)tt->to_double();
					rt=(tfloat==valfloat)?0:(tfloat<valfloat?-1:1);
				}
				else goto type_error;
				break;
			default:
				goto type_error;
			}
		}
		sq_pushinteger(v,rt);
		return 1;
	}
type_error:
	sq_throwerror(v,"ScanWrap Object compare type error");
	return 0;
};

SQInteger SqScanWrap::Sq2Nexti(HSQUIRRELVM v) 
{
	TreeItem* handle;
	if(SQ_SUCCEEDED(sq_getinstanceup(v,1,(SQUserPointer*)(&handle),0)))
	{
		if(sq_gettype(v,-1)==OT_NULL)
		{
			sq_pushuserpointer(v,(SQUserPointer)(handle->pChild));
			return 1;
		}
		else 
		{
			TreeItem* prehd;
			sq_getuserpointer(v,-1,(SQUserPointer*)&prehd);
			sq_pushuserpointer(v,(SQUserPointer)(prehd->pChild));
			return 1;
		}
	}
	return 0;
};

struct SqUDPServer
{
	static SQInteger _queryobj_releasehook(SQUserPointer p, SQInteger size) //标准的Squirrel C++对象析构回调
	{
		sdl_udp_deamon* handle=(sdl_udp_deamon*)p;
		delete handle;
		return 0;
	}

	static SQInteger Sq2Constructor(HSQUIRRELVM v) //标准的Squirrel C++对象构造方式
	{
		sdl_udp_deamon* handle=new sdl_udp_deamon;
		static list<SqUDPServer> serverDB;
		serverDB.push_back(SqUDPServer());
		SqUDPServer* server=(SqUDPServer*)&(serverDB.back());
		server->mv=v;

		if(SqGetVar(v,server->ScriptCallFunction )) 
			handle->connect(server,&SqUDPServer::udp_in_callback); 

		sq_setinstanceup(v,1,(SQUserPointer)(handle)); //为本实例保存C++对象指针,本实例在堆栈的底部，所以idx 使用1
		sq_setreleasehook(v,1,_queryobj_releasehook);  //由于在虚拟机上申请对象，此处要求删除对象
		return 0;
	}


	static SQInteger Sq2WriteUDP(HSQUIRRELVM v)
	{
		static sdl_udp *pUDP=NULL;
		SQUserPointer pBuf;
		int Length;
		const char *rip;
		const char *lip;
		int rport,lport;

		if(SqGetVar(v,lip,lport,rip,rport))
		{
			if(pUDP!=NULL) delete pUDP;
			pUDP=new sdl_udp();
			pUDP->init(lip,lport); 
			pUDP->set_remote(rip,rport); 
			return 0;
		}
		if(SqGetVar(v,rip,rport))
			pUDP->set_remote(rip,rport); 
		else if(SqGetVar(v,pBuf,Length))
			pUDP->send_msg((char*)pBuf,Length);
		return 0;

	}

	static void inject_class(HSQUIRRELVM v,const char* classname)
	{
		int top=sq_gettop(v);
		sq_pushroottable(v); //push the root table(were the globals of the script will be stored)
		sq_pushstring(v,classname,-1);
		sq_newclass(v,SQFalse);
		sq_createslot(v,-3);
		sq_settop(v,top); //restores the original stack size

		//'o' null, 'i' integer, 'f' float, 'n' integer or float, 's' string, 't' table, 'a' array, 'u' userdata, 
		//'c' closure and nativeclosure, 'g' generator, 'p' userpointer, 'v' thread, 'x' instance(class instance), 'y' class, 'b' bool. and '.' any type
		register_func(v,classname,Sq2Constructor,"constructor","xc");//构造函数,必须指明回调函数的名称
		register_func(v,classname,Sq2AddUDPPort,"listenUDP","xi");//要求监听一个端口
		register_func(v,classname,Sq2AddBroadCast,"listenBroadCast","xi");//要求监听一个广播端口
		register_func(v,classname,Sq2AddMultiCast,"listenMultiCast","xsi");//要求监听一个多播数据
		register_func(v,classname,Sq2WaitUDP,"wait","x-ii");//要求开始监听UDP，第一参数为等待间隔，第二参数为指定时间间隔（均为毫秒单位）
		register_func(v,classname,Sq2WriteUDP,"udpSend");
		register_func(v,Sq2WriteUDP,"udpSend");
	}

	HSQUIRRELVM mv;
	HSQOBJECT ScriptCallFunction;

	void udp_in_callback(sdl_udp* pUDP)
	{
		SQUserPointer p=pUDP;
		exec_function(mv,ScriptCallFunction,pUDP->get_port(),pUDP->get_from_ip(),pUDP->get_from_port(),(SQUserPointer)(pUDP->read_buf),pUDP->read_len);
	}
	

	static SQInteger Sq2WaitUDP(HSQUIRRELVM v)
	{
		sdl_udp_deamon* handle;
		int msecs;
		int kmsecs;
		if(SQ_FAILED(sq_getinstanceup(v,1,(SQUserPointer*)(&handle),0))) return 0;
		int n=sq_gettop(v);
		if(n==1) handle->waiting_for();
		if(SqGetVar(v,msecs)) 
			handle->waiting_for(msecs);

		if(SqGetVar(v,msecs,kmsecs))
		{
			handle->waiting_for(msecs);
			int curr_msecs=handle->sys_time.msecs();
			while(curr_msecs<kmsecs)  //等待的时间不够
			{
				handle->waiting_for(msecs); //继续等待
				curr_msecs=handle->sys_time.msecs();
			}

		}
		sq_pushinteger(v,(SQInteger)(handle->sys_time.msecs()));
		return 1;
	}

	static SQInteger Sq2AddUDPPort(HSQUIRRELVM v)
	{
		sdl_udp_deamon* handle;
		if(SQ_FAILED(sq_getinstanceup(v,1,(SQUserPointer*)(&handle),0))) return 0;
		int port;SqGetVar(v,port);
		sdl_udp* pudp=handle->new_udp(port); 
		sq_pushuserpointer(v,(SQUserPointer)pudp);
		return 1;
	}

	static SQInteger Sq2AddMultiCast(HSQUIRRELVM v)
	{
		sdl_udp_deamon* handle;
		if(SQ_FAILED(sq_getinstanceup(v,1,(SQUserPointer*)(&handle),0))) return 0;
		const char* ip;int port;SqGetVar(v,ip,port);
		sdl_udp* pudp=handle->new_multicast(ip,port); 
		sq_pushuserpointer(v,(SQUserPointer)pudp);
		return 1;
	}

	static SQInteger Sq2AddBroadCast(HSQUIRRELVM v)
	{
		sdl_udp_deamon* handle;
		if(SQ_FAILED(sq_getinstanceup(v,1,(SQUserPointer*)(&handle),0))) return 0;
		int port;SqGetVar(v,port);
		sdl_udp* pudp=handle->new_broadcast(port); 
		sq_pushuserpointer(v,(SQUserPointer)pudp);
		return 1;
	}
};

bool SqGetClass(HSQUIRRELVM v,const char* name)
{

	int top=sq_gettop(v);
	sq_pushroottable(v);
	sq_pushstring(v,name,-1);
	if(SQ_SUCCEEDED(sq_get(v,-2)))
	{
		SQObjectType tp=sq_gettype(v,-1);
		if(tp==OT_CLASS) return true;
	}
	sq_settop(v,top);
	return false;
}
void SqSetInstanceUP(HSQUIRRELVM v,HSQOBJECT &hInstance,void* p) //给一个实例设置指针
{
	sq_pushobject(v,hInstance);
	sq_setinstanceup(v,-1,p);
	sq_pop(v,1);
}
bool SqCreateClass(HSQUIRRELVM v,const char* name)
{
	int top=sq_gettop(v);
	sq_pushroottable(v);
	sq_pushstring(v,name,-1);
	if(SQ_SUCCEEDED(sq_get(v,-2))) return false;
	sq_pushstring(v,name,-1);
	sq_newclass(v,SQFalse);
	return SQ_SUCCEEDED(sq_createslot(v,-3));
}
// 向一个数组内添加一个对象的实例，对象并不执行脚本中的构造函数
bool SqPushInstance(HSQUIRRELVM v,const char* name,const char* arrayname,HSQOBJECT &hInstance)
{
	int top=sq_gettop(v);
	sq_pushroottable(v);
	sq_pushstring(v,arrayname,-1);
	if(SQ_SUCCEEDED(sq_get(v,-2)))
	{
		SQObjectType tp=sq_gettype(v,-1);
		if(tp!=OT_ARRAY) 
		{
			sq_settop(v,top);
			return false;
		}
	}
	int array_idx=sq_gettop(v);
	sq_pushroottable(v);
	sq_pushstring(v,name,-1);
	if(SQ_SUCCEEDED(sq_get(v,-2)))
	{
		SQObjectType tp=sq_gettype(v,-1);
		if(tp==OT_CLASS) 
		{
			sq_createinstance(v,-1);
			sq_getstackobj(v,-1,&hInstance);
			sq_arrayappend(v,array_idx);
			sq_settop(v,top);
			return true;
		}
	}
	sq_settop(v,top);
	return false;
}
//创建一个对象的实例，并执行脚本中的构造函数
bool SqCreateInstance(HSQUIRRELVM v,const char* classname,HSQOBJECT &hInstance)
{
	int top=sq_gettop(v);
	sq_pushroottable(v);
	sq_pushstring(v,classname,-1);
	if(SQ_SUCCEEDED(sq_get(v,-2)))
	{
		SQObjectType tp=sq_gettype(v,-1);
		if(tp==OT_CLASS) 
		{
			sq_createinstance(v,-1);
			sq_getstackobj(v,-1,&hInstance);
			sq_pushstring(v,"constructor",-1);
			if(SQ_SUCCEEDED(sq_get(v,-2))) //得到构造函数
			{
				sq_push(v,-2); //压入instance
				if(SQ_SUCCEEDED(sq_call(v,1,SQFalse,SQTrue)))//执行构造函数
				{
					sq_push(v,top);
					return true;
				}
			}
			return true; //没有构造函数，也应该认为创建实例成功
		}
	}
	sq_settop(v,top);
	return false;
}
//创建一个对象的实例，并执行脚本中的带一个参数的构造函数
template<class VAR1>
bool SqCreateInstance(HSQUIRRELVM v,const char* classname,HSQOBJECT &hInstance,VAR1 var1)
{
	int top=sq_gettop(v);
	sq_pushroottable(v);
	sq_pushstring(v,classname,-1);
	if(SQ_SUCCEEDED(sq_get(v,-2)))
	{
		SQObjectType tp=sq_gettype(v,-1);
		if(tp==OT_CLASS) 
		{
			sq_createinstance(v,-1);
			top=sq_gettop(v);
			sq_getstackobj(v,-1,&hInstance);
			sq_pushstring(v,"constructor",-1);
			if(SQ_SUCCEEDED(sq_get(v,-2))) //得到带参数的构造函数
			{
				sq_push(v,-2); //压入instance
				_push_object(v,var1); //压入参数
				if(SQ_SUCCEEDED(sq_call(v,2,SQFalse,SQTrue)))//执行构造函数
				{
					sq_push(v,top);
					return true;
				}
			}
		}
	}
	sq_settop(v,top);
	return false;
}

bool exec_script(HSQUIRRELVM v,const char* script_filename)
{
	if(SQ_SUCCEEDED(sqstd_dofile(v,script_filename,SQTrue,SQTrue))) return true;
	return false;
}

static SQInteger Sq2GlobalRunning(HSQUIRRELVM v) 
{
	if(GlobalExitFlag) sq_pushbool(v,SQFalse);
	else sq_pushbool(v,SQTrue);
	return 1;
}
static SQInteger Sq2GetFileviaHttp(HSQUIRRELVM v) 
{
	const char* host;
	const char* filename;
	const char* dirPrefix="";
	char buf[2048]; //定义文件接收缓冲区大小，这里是2K
	char fnbuf[256]; //定义文件名缓冲区
	int port=80;

	string memDoc;
	string server;
	sdl_tcp_stream ts(5); //5秒的超时等待
	sdl_line line;

	int n=sq_gettop(v);
	string shost,suri;
	if(n==3)
	{
		if(SQ_FAILED(sq_getstring(v,-2,&host))) return 0;
		if(SQ_FAILED(sq_getstring(v,-1,&dirPrefix))) return 0;
		string url(host);
		if(url.substr(0,7)!="http://") return 0;
		url=url.substr(7,url.length()-7);
		int sp=url.find('/');
		if(sp==string::npos) filename="/";  
		else 
		{
			shost=url.substr(0,sp-1);
			suri=url.substr(sp+1,url.length()-sp-1);  
			port=80;
			if((sp=shost.find(':'))!=string::npos)
			{
				port=atoi(shost.substr(sp+1,shost.length()-sp-1).c_str());  
				shost=shost.substr(0,sp-1);
			}
			filename=suri.c_str();
			host=shost.c_str(); 
		}
	}

	if(n==4)
	{
		if(SQ_FAILED(sq_getstring(v,-3,&host))) return 0;
		if(SQ_FAILED(sq_getinteger(v,-2,&port))) return 0;
		if(SQ_FAILED(sq_getstring(v,-1,&filename))) return 0;
	}
	if(n==5)
	{
		if(SQ_FAILED(sq_getstring(v,-4,&host))) return 0;
		if(SQ_FAILED(sq_getinteger(v,-3,&port))) return 0;
		if(SQ_FAILED(sq_getstring(v,-2,&filename))) return 0;
		if(SQ_FAILED(sq_getstring(v,-1,&dirPrefix))) return 0;

	}

	if(ts.connect_host(host,port))
	{
		istream* _p_istream=new istream((sdl_tcp_stream*)(&ts));
		ostream* _p_ostream=new ostream((sdl_tcp_stream*)(&ts));
		istream &ios=*_p_istream;
		ostream &oos=*_p_ostream;
		bool flag_break=false;

		if(filename==NULL || *filename=='/')
			oos<<"GET /"<<" HTTP/1.1\r\nHost: "<<host<<"\r\n\r\n";
		else
			oos<<"GET /"<<filename<<" HTTP/1.1\r\nHost: "<<host<<"\r\n\r\n";
		oos.flush(); 

		string headLine;
		int fileLength=-1;
		line.readln(ios);headLine=line.get_line(); 
		if(headLine.substr(0,12)=="HTTP/1.1 200") 
		{
			FILE* file=NULL;
			while(line.readln(ios))
			{
				headLine=line.get_line();
				if(headLine.substr(0,15)=="Content-Length:")
					fileLength=atoi(headLine.substr(15,headLine.length()-15).c_str());
				if(headLine.substr(0,15)=="Server:")
					server=headLine.substr(7,headLine.length()-7);
				if(line.is_blank())
				{
					if(filename!=NULL && *filename!='/' && file==NULL && *dirPrefix!=0 && *dirPrefix!='=')
					{
						n=strlen(dirPrefix);
						if(dirPrefix[n-1]=='/' || dirPrefix[n-1]=='\\')
							sprintf(fnbuf,"%s%s",dirPrefix,filename);
						else
							sprintf(fnbuf,"%s",dirPrefix);
						file=fopen(fnbuf,"wb");
					}

					if(fileLength<=0) //没有从网络上获取文件大小
					{
						char ch;
						fileLength=0;
						while(true)
						{
							ios.get(ch);
							if(!ios) break; //服务器中断认为数据传输完毕

							if(file!=NULL) fputc(ch,file);
							else memDoc+=ch; 
							fileLength++;
						}
					}
					else
					{
						ios.read(buf,fileLength % 2048);
						if(ios.good())
						{
							if(file!=NULL) fwrite(buf,1,fileLength % 2048,file);
							else memDoc.append(buf, fileLength % 2048);
							for(int i=0;i<fileLength/2048;++i)
							{
								ios.read(buf,2048);
								if(!ios)
								{
									flag_break=true;
									break;
								}
								if(file!=NULL) fwrite(buf,1,2048,file);
								else memDoc.append(buf,2048);
							}
						}
					}
					if(flag_break) 
					{
						if(file!=NULL) fclose(file);
						delete _p_istream;
						delete _p_ostream;
						ts.close_connect();
						return 0; //breaked in transfering,return null
					}
					else
					{
						if(file!=NULL)	
						{
							fclose(file);
							sq_pushbool(v,SQTrue);
						}
						else sq_pushstring(v,memDoc.data(),memDoc.length()); 
						delete _p_istream;
						delete _p_ostream;
						ts.close_connect();
						return 1; //OK
					}
					break;
				}
			}

		}
		sq_pushbool(v,SQFalse); //cann't find file in server
		delete _p_istream;
		delete _p_ostream;
		ts.close_connect();
		return 1;
	}
	return 0; //cann't linked server,return null
};




SQInteger SqFile::_queryobj_releasehook(SQUserPointer p, SQInteger size) //标准的Squirrel C++对象析构回调
{
	SqFile* handle=(SqFile*)p;
	if(handle!=NULL) 
	{
		if(handle->file!=NULL) 
			fclose(handle->file); 
		delete handle;
	}
	return 0;
}

SQInteger SqFile::Sq2Constructor(HSQUIRRELVM v) //标准的Squirrel C++对象构造方式
{
	int para=sq_gettop(v);
	const char* fn;
	const char* md;
	SqFile* handle=new SqFile;
	if(handle==NULL) return 0;
	handle->file=NULL;
	handle->linePointer=NULL;
	handle->lineEnd=NULL; 
	switch(para)
	{
	case 1:
		handle->file=NULL;
		sq_setinstanceup(v,1,(SQUserPointer)(handle)); //为本实例保存C++对象指针,本实例在堆栈的底部，所以idx 使用1
		sq_setreleasehook(v,1,_queryobj_releasehook);  //由于在虚拟机上申请对象，此处要求删除对象
		break;
	case 2:
		if(SQ_FAILED(sq_getstring(v,-1,&fn))) break;
		handle->file=fopen(fn,"wb");
		sq_setinstanceup(v,1,(SQUserPointer)(handle)); //为本实例保存C++对象指针,本实例在堆栈的底部，所以idx 使用1
		sq_setreleasehook(v,1,_queryobj_releasehook);  //由于在虚拟机上申请对象，此处要求删除对象
		break;
	case 3:
		if(SQ_FAILED(sq_getstring(v,-1,&md))) break;
		if(SQ_FAILED(sq_getstring(v,-2,&fn))) break;
		if(strcmp(md,"r")==0) handle->file=fopen(fn,"r");
		if(strcmp(md,"w")==0) handle->file=fopen(fn,"w");
		if(strcmp(md,"rb")==0) handle->file=fopen(fn,"rb");
		if(strcmp(md,"wb")==0) handle->file=fopen(fn,"wb");
		if(strcmp(md,"rw")==0) handle->file=fopen(fn,"rw");
		sq_setinstanceup(v,1,(SQUserPointer)(handle)); //为本实例保存C++对象指针,本实例在堆栈的底部，所以idx 使用1
		sq_setreleasehook(v,1,_queryobj_releasehook);  //由于在虚拟机上申请对象，此处要求删除对象
		break;
	default:
		handle=NULL;
		sq_setinstanceup(v,1,(SQUserPointer)(handle)); //为本实例保存C++对象指针,本实例在堆栈的底部，所以idx 使用1
		sq_setreleasehook(v,1,_queryobj_releasehook);  //由于在虚拟机上申请对象，此处要求删除对象
		break;
	}
	return 0;
}

SQInteger SqFile::Sq2Open(HSQUIRRELVM v) 
{
	int para=sq_gettop(v);
	const char* fn;
	const char* md;
	SqFile* handle=NULL;
	if(SQ_SUCCEEDED(sq_getinstanceup(v,1,(SQUserPointer*)(&handle),0)))
	{
		if(handle!=NULL && handle->file!=NULL) fclose(handle->file);
		handle->file=NULL;
	}
	switch(para)
	{
	case 1:
		handle->file=NULL;
		break;
	case 2:
		if(SQ_FAILED(sq_getstring(v,-1,&fn))) break;
		handle->file=fopen(fn,"wb");
		break;
	case 3:
		if(SQ_FAILED(sq_getstring(v,-1,&md))) break;
		if(SQ_FAILED(sq_getstring(v,-2,&fn))) break;
		if(strcmp(md,"r")==0) handle->file=fopen(fn,"r");
		if(strcmp(md,"w")==0) handle->file=fopen(fn,"w");
		if(strcmp(md,"rb")==0) handle->file=fopen(fn,"rb");
		if(strcmp(md,"wb")==0) handle->file=fopen(fn,"wb");
		if(strcmp(md,"rw")==0) handle->file=fopen(fn,"rw");
		break;
	default:
		handle->file=NULL;
		break;
	}
	if(handle->file!=NULL) sq_pushbool(v,SQTrue);
	else sq_pushbool(v,SQFalse);
	return 1;
}

SQInteger SqFile::Sq2WriteEndl(HSQUIRRELVM v) 
{
	const char* line;
	SqFile* handle;
	if(SQ_SUCCEEDED(sq_getinstanceup(v,1,(SQUserPointer*)(&handle),0)))
	{
		if(handle==NULL || handle->file==NULL) return 0;
		fprintf(handle->file,"\n");
		fflush(handle->file);
	}
	return 0;
}
SQInteger SqFile::Sq2WriteLn(HSQUIRRELVM v) 
{

	SqFile* handle;
	if(SQ_SUCCEEDED(sq_getinstanceup(v,1,(SQUserPointer*)(&handle),0)))
	{
		if(handle==NULL || handle->file==NULL) {sq_pushbool(v,SQFalse);return 1;}
		const char* line;
		if(SqGetVar(v,line))
		{
			fprintf(handle->file,"%s\n",line);
			fflush(handle->file);
			sq_pushbool(v,SQTrue);
			return 1;
		}
		int ival;
		if(SqGetVar(v,ival))
		{
			fprintf(handle->file,"%d\n",ival);
			fflush(handle->file);
			sq_pushbool(v,SQTrue);
			return 1;
		}
		float fval;
		if(SqGetVar(v,fval))
		{
			fprintf(handle->file,"%f\n",fval);
			fflush(handle->file);
			sq_pushbool(v,SQTrue);
			return 1;
		}
	}else sq_pushbool(v,SQFalse);
	return 1;
}

SQInteger SqFile::Sq2WriteHex(HSQUIRRELVM v) 
{
	SqFile* handle;
	if(SQ_SUCCEEDED(sq_getinstanceup(v,1,(SQUserPointer*)(&handle),0)))
	{
		SQUserPointer pData;int Len;
		if(handle==NULL || handle->file==NULL) {sq_pushbool(v,SQFalse);return 1;}
		if(SqGetVar(v,pData,Len))
		{
			for(int i=0;i<Len;++i)
			{
				int cc=((unsigned char*)pData)[i];
				fputc((cc/16<10)?(cc/16+48):(cc/16+55),handle->file);
				fputc((cc%16<10)?(cc%16+48):(cc%16+55),handle->file);
			}
			sq_pushbool(v,SQTrue);
		}else sq_pushbool(v,SQFalse);
	}else sq_pushbool(v,SQFalse);
	return 1;
}

SQInteger SqFile::Sq2ReadHex2Buf(HSQUIRRELVM v) 
{

	SqFile* handle;
	int s,len;
	int counter=0;
	if(SQ_SUCCEEDED(sq_getinstanceup(v,1,(SQUserPointer*)(&handle),0)))
	{
		if(SqGetVar(v,s,len))
		{
			counter=handle->convertHex(s,len); 
		}

	}
	sq_pushinteger(v,counter);
	return 1;
}

SQInteger SqFile::Sq2ReadHexLine(HSQUIRRELVM v) 
{
	SqFile* handle;
	char* pbuf;
	if(SQ_SUCCEEDED(sq_getinstanceup(v,1,(SQUserPointer*)(&handle),0)))
	{
		if(handle==NULL || handle->file==NULL) return 0;
		handle->readLn();
		handle->data.clear();
		int len=handle->convertHex(0,handle->linestr.length());   
		sq_pushinteger(v,len); 
		return 1;
	}
	return 0;
}

SQInteger SqFile::Sq2Write(HSQUIRRELVM v) 
{
	const char* line;
	int len;
	SqFile* handle;

	if(SQ_SUCCEEDED(sq_getinstanceup(v,1,(SQUserPointer*)(&handle),0)))
	{
		if(handle==NULL || handle->file==NULL) {sq_pushbool(v,SQFalse);return 1;}
		if(SqGetVar(v,line))
		{
			fprintf(handle->file,"%s",line);
			sq_pushbool(v,SQTrue);
			return 1;
		}

		if(SqGetVar(v,line,len))
		{
			fwrite(line,1,len,handle->file);
			sq_pushbool(v,SQTrue);
			return 1;
		}

	}
	sq_pushbool(v,SQFalse);
	return 1;
}

SQInteger SqFile::Sq2WritePtr(HSQUIRRELVM v) 
{
	SqFile* handle;
	if(SQ_SUCCEEDED(sq_getinstanceup(v,1,(SQUserPointer*)(&handle),0)))
	{
		if(handle==NULL || handle->file==NULL) {sq_pushbool(v,SQFalse);return 1;}
		SQUserPointer pData;
		int len;
		if(SqGetVar(v,pData,len))
		{
			fwrite(pData,1,len,handle->file);
			sq_pushbool(v,SQTrue);
			return 1;
		}

	}
	sq_pushbool(v,SQFalse);
	return 1;
}
SQInteger SqFile::Sq2GetDataPtr(HSQUIRRELVM v) 
{
	SqFile* handle;
	if(SQ_SUCCEEDED(sq_getinstanceup(v,1,(SQUserPointer*)(&handle),0)))
	{
		if(handle==NULL || handle->file==NULL) return 0;
		sq_pushuserpointer(v,(SQUserPointer)(handle->data.data()));
		return 1;
	}
	return 0;
}
SQInteger SqFile::Sq2GetLinePtr(HSQUIRRELVM v) 
{
	SqFile* handle;
	if(SQ_SUCCEEDED(sq_getinstanceup(v,1,(SQUserPointer*)(&handle),0)))
	{
		if(handle==NULL || handle->file==NULL) return 0;
		sq_pushuserpointer(v,(SQUserPointer)(handle->linestr.data()));
		return 1;
	}
	return 0;
}
SQInteger SqFile::Sq2LineReadInteger(HSQUIRRELVM v) 
{
	SqFile* handle;
	char sp=',';
	if(SQ_SUCCEEDED(sq_getinstanceup(v,1,(SQUserPointer*)(&handle),0)))
	{
		SqGetVar(v,sp);
		char *pLine=handle->linePointer;
		if(pLine==NULL || pLine>=handle->lineEnd) return 0;
		while(pLine<handle->lineEnd )
		{
			if(*pLine==sp || (pLine+1)==handle->lineEnd) 
			{
				if((pLine+1)<handle->lineEnd) *pLine=0;
				int ret=atoi(handle->linePointer);
				sq_pushinteger(v,ret);
				handle->linePointer=pLine+1;
				return 1;
			}
			pLine++;
		}
	}
	return 0;
}
SQInteger SqFile::Sq2LineReadFloat(HSQUIRRELVM v) 
{
	SqFile* handle;
	char sp=',';
	if(SQ_SUCCEEDED(sq_getinstanceup(v,1,(SQUserPointer*)(&handle),0)))
	{
		SqGetVar(v,sp);
		char *pLine=handle->linePointer;
		if(pLine==NULL) return 0;
		while(pLine<handle->lineEnd )
		{
			if(*pLine==sp || (pLine+1)==handle->lineEnd) 
			{
				if((pLine+1)<handle->lineEnd) *pLine=0;
				SQFloat ret=atof(handle->linePointer);
				sq_pushfloat(v,ret);
				handle->linePointer=pLine+1;
				return 1;
			}
			pLine++;
		}
	}
	return 0;
}
SQInteger SqFile::Sq2LineReadString(HSQUIRRELVM v) 
{
	SqFile* handle;
	char sp=',';
	if(SQ_SUCCEEDED(sq_getinstanceup(v,1,(SQUserPointer*)(&handle),0)))
	{
		SqGetVar(v,sp);
		char *pLine=handle->linePointer;
		if(pLine==NULL) return 0;
		while(pLine<handle->lineEnd )
		{
			if(*pLine==sp || (pLine+1)==handle->lineEnd) 
			{
				if((pLine+1)<handle->lineEnd) *pLine=0;
				sq_pushstring(v,handle->linePointer,-1);
				handle->linePointer=pLine+1;
				return 1;
			}
			pLine++;
		}
	}
	return 0;
}
SQInteger SqFile::Sq2GetLineString(HSQUIRRELVM v) 
{
	SqFile* handle;
	if(SQ_SUCCEEDED(sq_getinstanceup(v,1,(SQUserPointer*)(&handle),0)))
	{
		if(handle==NULL || handle->file==NULL) return 0;
		sq_pushstring(v,handle->linestr.c_str(),(handle->linestr.length()));
		return 1;
	}
	return 0;
}
SQInteger SqFile::Sq2ClearPtr(HSQUIRRELVM v) 
{
	SqFile* handle;
	if(SQ_SUCCEEDED(sq_getinstanceup(v,1,(SQUserPointer*)(&handle),0)))
	{
		if(handle==NULL || handle->file==NULL) return 0;
		handle->data.clear();
	}
	return 0;
}
SQInteger SqFile::Sq2Eof(HSQUIRRELVM v) 
{
	SqFile* handle;
	if(SQ_SUCCEEDED(sq_getinstanceup(v,1,(SQUserPointer*)(&handle),0)))
	{
		if(handle==NULL || handle->file==NULL) {sq_pushbool(v,SQTrue);return 1;}
		if(feof(handle->file)) sq_pushbool(v,SQTrue);
		else sq_pushbool(v,SQFalse);
		return 1;
	}
	sq_pushbool(v,SQTrue);
	return 1;
}

SQInteger SqFile::Sq2Good(HSQUIRRELVM v)
{
	SqFile* handle;
	if(SQ_SUCCEEDED(sq_getinstanceup(v,1,(SQUserPointer*)(&handle),0)))
	{
		if(handle==NULL || handle->file==NULL) {sq_pushbool(v,SQFalse);return 1;}
		if(feof(handle->file)) sq_pushbool(v,SQFalse);
		else sq_pushbool(v,SQTrue);
		return 1;
	}
	sq_pushbool(v,SQFalse);
	return 1;
}

SQInteger SqFile::Sq2Close(HSQUIRRELVM v)
{
	SqFile* handle;
	if(SQ_SUCCEEDED(sq_getinstanceup(v,1,(SQUserPointer*)(&handle),0)))
	{
		if(handle!=NULL && handle->file!=NULL) fclose(handle->file); 
	}
	return 0;
}


SQInteger SqFile::Sq2GetLn(HSQUIRRELVM v) //如果文件结束，返回null，否则返回一个string
{
	SqFile* handle;
	if(SQ_SUCCEEDED(sq_getinstanceup(v,1,(SQUserPointer*)(&handle),0)))
	{
		if(handle->readLn())
		{
			sq_pushstring(v,handle->linestr.data(),handle->linestr.length());
			handle->linePointer=handle->linestr.get_buf(); 
			handle->lineEnd=handle->linePointer+handle->linestr.length();
			return 1;
		}
	}
	return 0;
}

SQInteger SqFile::Sq2ReadLn(HSQUIRRELVM v) //如果文件结束，返回false，否则返回true
{
	SqFile* handle;
	if(SQ_SUCCEEDED(sq_getinstanceup(v,1,(SQUserPointer*)(&handle),0)))
	{
		if(handle->readLn()) sq_pushinteger(v,handle->linestr.length());
		else return 0;
		handle->linePointer=handle->linestr.get_buf(); 
		handle->lineEnd=handle->linePointer+handle->linestr.length();
		return 1;
	}
	return 0;
}

int SqFile::convertHex(int start,int length)
{
	int linelen=linestr.length();
	char* pData=linestr.data()+start; 
	if(start>=linelen || (start+length)>linelen) return 0;
	char* pEnd=pData+length;
	int ch;
	unsigned char cc;
	int counter=0;
	while(true)
	{
		while((*pData==' '|| *pData=='\t') && pData<pEnd) ++pData;
		ch=*pData;
		ch=(ch>='0' && ch<='9')?(ch-'0')*16:((ch>='A' && ch<='F')?(ch-'A'+10)*16:((ch>='a' && ch<='f')?(ch-'a'+10)*16:-1));
		if(ch<0 || ++pData>=pEnd) break;
		cc=ch;
		ch=*pData;
		ch=(ch>='0' && ch<='9')?(ch-'0'):((ch>='A' && ch<='F')?(ch-'A'+10):((ch>='a' && ch<='f')?(ch-'a'+10):0));
		cc+=ch;
		data+=cc;

		counter++;
		if(ch<0 || ++pData>=pEnd) break;
	}
	return counter;
}

bool SqFile::readLn() 
{
	linestr.clear();
	if(file==NULL) return false;
	return linestr.readln(file);
}

SQInteger SqFile::Sq2ReadAll(HSQUIRRELVM v) 
{
	SqFile* handle;
	if(SQ_SUCCEEDED(sq_getinstanceup(v,1,(SQUserPointer*)(&handle),0)))
	{
		if(handle==NULL || handle->file==NULL) {sq_pushbool(v,SQFalse);return 1;}
		char ch;handle->linestr.clear();  
		while(true)
		{
			ch=fgetc(handle->file);
			if(feof(handle->file)) break;
			handle->linestr+=ch;
		}
		if(handle->linestr.length()>0) 
			sq_pushstring(v,handle->linestr.data(),handle->linestr.length());
		else return 0;
	}else return 0;
	return 1;
}

SQInteger Sq2Vsscanf(HSQUIRRELVM v);

void SqFile::inject_class(HSQUIRRELVM v)
{
	int top=sq_gettop(v);
	const char* classname="FileStream";
	sq_pushroottable(v); //push the root table(were the globals of the script will be stored)
	sq_pushstring(v,classname,-1);
	sq_newclass(v,SQFalse);
	check_type(v,true);   //记录类信息，用于快速类检查
	sq_createslot(v,-3);
	sq_settop(v,top); //restores the original stack size

	//'o' null, 'i' integer, 'f' float, 'n' integer or float, 's' string, 't' table, 'a' array, 'u' userdata, 
	//'c' closure and nativeclosure, 'g' generator, 'p' userpointer, 'v' thread, 'x' instance(class instance), 'y' class, 'b' bool. and '.' any type
	register_func(v,classname,Sq2Constructor,"constructor","x-ss");//构造函数
	register_func(v,classname,Sq2Open,"open","xss");//打开一个文件
	register_func(v,classname,Sq2WriteLn,"writeln","x."); //写出一行
	register_func(v,classname,Sq2WriteEndl,"endl","x"); //写出一个换行
	register_func(v,classname,Sq2Write,"write","xs"); //写出字符串
	register_func(v,classname,Sq2WritePtr,"write_ptr","xpi"); //写出一个内存区域
	register_func(v,classname,Sq2GetDataPtr,"data_ptr","x"); //给出读入并转换以后的内存区域指针
	register_func(v,classname,Sq2GetLinePtr,"c_str","x");    //给出readln读入的,cstr类的指针字符串
	register_func(v,classname,Sq2GetLineString,"str","x");    //给出readln读入的字符串
	register_func(v,classname,Sq2ClearPtr,"clear_data","x"); //清除数据缓冲区内的数据
	register_func(v,classname,Sq2WriteHex,"write_hex","xpi"); //写出一个内存区域16进制字符串
	register_func(v,classname,Sq2ReadHex2Buf,"convert_hex","xii"); //将当前读入的行，从指定位置开始，到指定的长度，将16进制字符串转换为二进制，返回转换的长度
	register_func(v,classname,Sq2ReadHexLine,"read_hexln","x"); //读入一个内存区域16进制字符串,并转换成二进制数据，返回二进制数据长度
	register_func(v,classname,Sq2GetLn,"getln","x"); //读入一行，并返回一个字符串
	register_func(v,classname,Sq2ReadLn,"readln","x"); //读入一行，并返回字符串长度
	register_func(v,classname,Sq2Vsscanf,"scanln","xsx"); //读入一行，并返回分割以后的对象列表
	register_func(v,classname,Sq2ReadAll,"read","x"); //读入全部文件，并返回一个字符串
	register_func(v,classname,Sq2Eof,"eof","x"); //判定文件流是否中止
	register_func(v,classname,Sq2Eof,"fail","x"); //判定文件流是否失效
	register_func(v,classname,Sq2Good,"good","x"); //判定文件流是否正常
	register_func(v,classname,Sq2Close,"close","x"); //关闭文件流

	register_func(v,classname,Sq2LineReadInteger,"read_integer","x-i"); //行读入整数
	register_func(v,classname,Sq2LineReadFloat,"read_float","x-i"); //行读入浮点数
	register_func(v,classname,Sq2LineReadString,"read_string","x-i"); //行读入字符串
}

static SQInteger Sq2Sleep(HSQUIRRELVM v) 
{
	int n=sq_gettop(v);
	int timeout=1000;
	if(n==2)
	{
		if(SQ_FAILED(sq_getinteger(v,-1,&timeout))) return 0;
#ifdef WIN32
		::Sleep(timeout);
#else
		sleep(timeout/1000);
#endif

	}
	return 0;
}


void SqTime::inject_class(HSQUIRRELVM v)
{
	int top=sq_gettop(v);
	const char* classname="HTime";
	sq_pushroottable(v); //push the root table(were the globals of the script will be stored)
	sq_pushstring(v,classname,-1);
	sq_newclass(v,SQFalse);
	sq_createslot(v,-3);
	sq_settop(v,top); //restores the original stack size

	//'o' null, 'i' integer, 'f' float, 'n' integer or float, 's' string, 't' table, 'a' array, 'u' userdata, 
	//'c' closure and nativeclosure, 'g' generator, 'p' userpointer, 'v' thread, 'x' instance(class instance), 'y' class, 'b' bool. and '.' any type
	register_func(v,classname,Sq2Constructor,"constructor");//构造函数
	register_func(v,classname,Sq2Init,"init","x");
	register_func(v,classname,Sq2TimeStr,"time_str","x");
	register_func(v,classname,Sq2GetTime,"get_time","x");
	register_func(v,classname,Sq2SetTime,"set_time","xs");
	register_func(v,classname,Sq2MakeTime,"make_time","xiiiiiii");
	register_func(v,classname,Sq2CopyTime,"copy","xx");
	register_func(v,classname,Sq2Ctime,"ctime","x");
	register_func(v,classname,Sq2Msecs,"msecs","x");
	register_func(v,classname,Sq2GetTimeSpan,"time_span","xx");
	register_func(v,classname,Sq2Strftime,"strftime","xs");
	register_func(v,classname,Sq2GetTimeSpan,"_sub","xx");
	register_func(v,classname,Sq2CompareTime,"_cmp","xx");
	register_func(v,classname,Sq2AddTimeSpan,"_add","xi");

}

SQInteger SqTime::_queryobj_releasehook(SQUserPointer p, SQInteger size) //标准的Squirrel C++对象析构回调
{
	SqTime* handle=(SqTime*)p;
	if(handle!=NULL && handle->flag_native_time && handle->pTime!=NULL) delete handle->pTime;
	delete handle;
	return 0;
}

SQInteger SqTime::Sq2Constructor(HSQUIRRELVM v) //标准的Squirrel C++对象构造方式
{
	int para=sq_gettop(v);
	SqTime* handle=new SqTime;
	sq_setinstanceup(v,1,(SQUserPointer)(handle)); //为本实例保存C++对象指针,本实例在堆栈的底部，所以idx 使用1
	sq_setreleasehook(v,1,_queryobj_releasehook);  //由于在虚拟机上申请对象，此处要求删除对象

	SQUserPointer pt;
	int ct;
	if(SqGetVar(v,ct)) //参数构造时间
	{
		handle->pTime=new sdl_time();
		handle->flag_native_time=true; 
		handle->pTime->mtime=(time_t)ct;
		handle->pTime->m_timespan=0; 
		return 0;
	}
	if(SqGetVar(v,pt)) //C++指针构造时间
	{
		handle->flag_native_time=false;
		handle->pTime=(sdl_time*)pt; 
		return 0;
	}
	handle->pTime=new sdl_time();
	handle->flag_native_time=true; 
	handle->pTime->init();  
	return 0;

}

SQInteger SqTime::Sq2Init(HSQUIRRELVM v) 
{
	SqTime* handle;
	if(SQ_SUCCEEDED(sq_getinstanceup(v,1,(SQUserPointer*)(&handle),0)))
	{
		handle->pTime->init();
	}
	return 0;
};

SQInteger SqTime::Sq2TimeStr(HSQUIRRELVM v) 
{
	SqTime* handle;
	if(SQ_SUCCEEDED(sq_getinstanceup(v,1,(SQUserPointer*)(&handle),0)))
	{
		char buf[64];
		int len=handle->pTime->sch_timestr(buf);
		sq_pushstring(v,buf,len);
		return 1;
	}
	return 0;
};

SQInteger SqTime::Sq2GetTime(HSQUIRRELVM v) 
{
	SqTime* handle;
	if(SQ_SUCCEEDED(sq_getinstanceup(v,1,(SQUserPointer*)(&handle),0)))
	{
		handle->pTime->get_time();
	}
	return 0;
};

SQInteger SqTime::Sq2Strftime(HSQUIRRELVM v) 
{
	SqTime* handle;
	const char* fmt;
	if(SQ_SUCCEEDED(sq_getinstanceup(v,1,(SQUserPointer*)(&handle),0)))
	{
		if(SqGetVar(v,fmt))
		{
			char date[64];
			time_t ttime=handle->pTime->mtime+handle->pTime->m_timespan/1000;
			int len=strftime(date,sizeof(date),fmt,gmtime(&ttime));
			sq_pushstring(v,date,len);
			return 1;
		}
	}
	return 0;
};

SQInteger SqTime::Sq2GetTimeSpan(HSQUIRRELVM v) 
{
	SqTime* handle;
	SQInstancePointer pTS;
	if(SQ_SUCCEEDED(sq_getinstanceup(v,1,(SQUserPointer*)(&handle),0)))
	{
		HSQOBJECT me,pa;
		SQUserPointer tag1,tag2;
		sq_getstackobj(v,1,&me);
		sq_getstackobj(v,2,&pa);
		sq_getobjtypetag(&me,&tag1);
		sq_getobjtypetag(&pa,&tag2);
		if(tag1!=tag2) {sq_throwerror(v,"HTime type Required!"); return 0;}
		if(SqGetVar(v,pTS))
		{
			sdl_time* pt=((SqTime*)pTS)->pTime;
			int sq= (int)handle->pTime->mtime - (int)pt->mtime;
			sq=sq*1000+(handle->pTime->m_timespan - pt->m_timespan);
			sq_pushinteger(v,sq);
			return 1;
		}
	}
	return 0;
};

SQInteger SqTime::Sq2CompareTime(HSQUIRRELVM v) 
{
	SqTime* handle;
	SQInstancePointer pTS;
	if(SQ_SUCCEEDED(sq_getinstanceup(v,1,(SQUserPointer*)(&handle),0)))
	{
		HSQOBJECT me,pa;
		SQUserPointer tag1,tag2;
		sq_getstackobj(v,1,&me);
		sq_getstackobj(v,2,&pa);
		sq_getobjtypetag(&me,&tag1);
		sq_getobjtypetag(&pa,&tag2);
		if(tag1!=tag2) {sq_throwerror(v,"HTime type Required!"); return 0;}
		if(SqGetVar(v,pTS))
		{
			sdl_time* pt=((SqTime*)pTS)->pTime;
			int sq= (int)(handle->pTime->mtime) - (int)(pt->mtime);
			sq=sq*1000+(handle->pTime->m_timespan - pt->m_timespan);
			sq_pushinteger(v,sq);
			return 1;
		}
	}
	return 0;
};

SQInteger SqTime::Sq2AddTimeSpan(HSQUIRRELVM v) 
{
	SqTime* handle;
	int tspan;

	if(SQ_SUCCEEDED(sq_getinstanceup(v,1,(SQUserPointer*)(&handle),0)))
	{
		if(SqGetVar(v,tspan))
		{
			handle->pTime->mtime+=(handle->pTime->m_timespan+tspan)/1000;
			handle->pTime->m_timespan=(handle->pTime->m_timespan+tspan)%1000;
			sq_pop(v,1); //去掉参数，返回对象
			return 1;
		}
	}
	return 0;
};

SQInteger SqTime::Sq2CopyTime(HSQUIRRELVM v) 
{
	SqTime* handle;
	SQInstancePointer pTS;
	if(SQ_SUCCEEDED(sq_getinstanceup(v,1,(SQUserPointer*)(&handle),0)))
	{
		HSQOBJECT me,pa;
		SQUserPointer tag1,tag2;
		sq_getstackobj(v,1,&me);
		sq_getstackobj(v,2,&pa);
		sq_getobjtypetag(&me,&tag1);
		sq_getobjtypetag(&pa,&tag2);
		if(tag1!=tag2) {sq_throwerror(v,"HTime type Required!"); return 0;}

		if(SqGetVar(v,pTS))
		{
			SqTime* pt=(SqTime*)pTS;
			handle->pTime->mtime=pt->pTime->mtime;
			handle->pTime->pre_msecs=pt->pTime->pre_msecs;
			handle->pTime->m_timespan=pt->pTime->m_timespan;  
		}
	}
	return 0;
};

SQInteger SqTime::Sq2SetTime(HSQUIRRELVM v) 
{
	SqTime* handle;
	const char* time_str;
	if(SQ_SUCCEEDED(sq_getinstanceup(v,1,(SQUserPointer*)(&handle),0)))
	{
		bool rt=false;
		if(SqGetVar(v,time_str)) 
			rt=handle->pTime->chdate2time(time_str);
		if(rt) sq_pushbool(v,SQTrue); 
		else sq_pushbool(v,SQFalse); 
		return 1;
	}
	return 0;
};

SQInteger SqTime::Sq2MakeTime(HSQUIRRELVM v) 
{
	SqTime* handle;
	const char* time_str;
	if(SQ_SUCCEEDED(sq_getinstanceup(v,1,(SQUserPointer*)(&handle),0)))
	{
		bool rt=false;
		int year,mon,day,hour,min,sec,msec;
		if(SqGetVar(v,year,mon,day,hour,min,sec,msec))
			rt=handle->pTime->maketime(year,mon,day,hour,min,sec,msec); 
		if(rt) sq_pushbool(v,SQTrue); 
		else sq_pushbool(v,SQFalse); 
		return 1;
	}
	return 0;
};

SQInteger SqTime::Sq2Ctime(HSQUIRRELVM v) 
{
	SqTime* handle;
	if(SQ_SUCCEEDED(sq_getinstanceup(v,1,(SQUserPointer*)(&handle),0)))
	{
		int ct=(int)(handle->pTime->time_sec());
		sq_pushinteger(v,ct);
		return 1;
	}
	return 0;
};

SQInteger SqTime::Sq2Msecs(HSQUIRRELVM v) 
{
	SqTime* handle;
	if(SQ_SUCCEEDED(sq_getinstanceup(v,1,(SQUserPointer*)(&handle),0)))
	{
		int mcs=handle->pTime->msecs();
		sq_pushinteger(v,mcs);
		return 1;
	}
	return 0;
};



extern void Init_Base (HSQUIRRELVM sqvm);
extern void Init_Util (HSQUIRRELVM sqvm);

void inject_SqShell(HSQUIRRELVM v)
{
	Init_Base(v);
	Init_Util(v);
}

static SQInteger Sq2ImportFile(HSQUIRRELVM v)
{
	const char* filename;
	SqGetVar(v,filename);
	import_script(v,filename);
	return 0;
}


#ifdef _UNICODE
#define scfprintf fwprintf
#define scvprintf vwprintf
#else
#define scfprintf fprintf
#define scvprintf vprintf
#endif



void PrintError(HSQUIRRELVM v)
{
	const SQChar *err;
	sq_getlasterror(v);
	if(SQ_SUCCEEDED(sq_getstring(v,-1,&err))) {
		scprintf(_SC("SQDBG error : %s"),err);
	}else {
		scprintf(_SC("SQDBG error"),err);
	}
	sq_pop(v,1);
}


HSQUIRRELVM init_VirtualMachine()
{
	HSQUIRRELVM v = sq_open(1024); // creates a VM with initial stack size 1024 
	sq_pushroottable(v); //push the root table were to register the lib function
	//#if (SQUIRREL_VERSION == _SC("Squirrel 2.2.3 stable"))
	//sq_setprintfunc(v,printfunc);
	//#endif
	sq_setprintfunc(v,printfunc,printfunc);
	sqstd_register_bloblib(v);
	sqstd_register_iolib(v);
	sqstd_register_systemlib(v);
	sqstd_register_mathlib(v);
	sqstd_register_stringlib(v);

	sqstd_seterrorhandlers(v); //registers the default error handlers

	return v;
};



//------ PublicAgency-------------
//---------------------------------
sdl_time _the_global_time_here; //全局公共时间,一个应用程序中只应该出现一个
map<string,HSQOBJECT> GlobalTable; //STL具有有限的线程安全性，因此需要读写锁保持同步
sdl_rwlock RwLck; //读写锁
sdl_rwlock GTLck;  //全局时间锁
_sdl_cond  GCcond; //全局的条件锁
//HSQUIRRELVM publicVM; //全局使用的虚拟机
void get_public_global_time(sdl_time& st) //在C++中，获取全局时钟
{
	GTLck.read_lock();
	st.mtime=_the_global_time_here.mtime; 	
	st.m_timespan=_the_global_time_here.m_timespan; 
	st.pre_msecs=_the_global_time_here.pre_msecs;
	GTLck.read_unlock(); 
}
struct PublicAgency
{
	static SQInteger Sq2PublicTabNewSlot(HSQUIRRELVM v) 
	{
		const char* key;
		bool flag_exist_string=false;
		int type=sq_gettype(v,-1);
		if(SQ_FAILED(sq_getstring(v,-2,&key))) return 0;
		RwLck.write_lock(); 
		string skey(key);
		std::map<string,HSQOBJECT>::iterator rt=GlobalTable.find(skey);
		HSQOBJECT* pObj;
		if(rt!= GlobalTable.end())
		{
			pObj=(HSQOBJECT*)&(rt->second);
			if(pObj->_type==OT_STRING) flag_exist_string=true;//如果对象已经存在，而且是STRING
		}
		else
			pObj=(HSQOBJECT*)&(GlobalTable[skey]);


		if(type==OT_INTEGER || type==OT_BOOL || type==OT_FLOAT || type==OT_USERPOINTER)
		{
			if(flag_exist_string) //如果原来的类型是string,类型改变，需要删除原来的string
			{
				string* pStr=(string*)(pObj->_unVal.pString);
				delete pStr;  //删除不需要的字符串
				pObj->_unVal.pString=NULL;
			}
			sq_getstackobj(v,-1,pObj);
			RwLck.write_unlock();
			return 0;
		}
		if(type==OT_STRING)
		{
			const char* val;
			sq_getstring(v,-1,&val);
			if(flag_exist_string)
				*((string*)pObj->_unVal.pString)=val;
			else
			{
				pObj->_type=OT_STRING;
				pObj->_unVal.pString=(SQString*)(new string(val));
			}
			RwLck.write_unlock();
			return 0;
		}
		RwLck.write_unlock();
		return 0;
	}

	static SQInteger Sq2PublicTabGet(HSQUIRRELVM v) 
	{
		const char* key;
		
		if(SQ_FAILED(sq_getstring(v,-1,&key))) return 0;
		RwLck.read_lock(); 
		std::map<string,HSQOBJECT>::iterator rt=GlobalTable.find(string(key));
		if(rt == GlobalTable.end()) 
		{
			RwLck.read_unlock();
			return 0;
		}
		else
		{
			HSQOBJECT &obj=rt->second;
			int type=obj._type; 
			if(type==OT_INTEGER || type==OT_BOOL || type==OT_FLOAT || type==OT_USERPOINTER)
			{
				sq_pushobject(v,obj);
				RwLck.read_unlock();
				return 1;
			}
			if(type==OT_STRING)
			{
				string* pStr=(string*)(obj._unVal.pString);
				sq_pushstring(v,pStr->data(),pStr->length()); 
				RwLck.read_unlock();
				return 1;
			}
		}
		RwLck.read_unlock();
		return 0;
	}

	static SQInteger Sq2PublicTabCompare(HSQUIRRELVM v) 
	{
		const char* key;

		if(SQ_FAILED(sq_getstring(v,-2,&key))) return 0;
		int type=sq_gettype(v,-1);
		RwLck.read_lock(); 
		std::map<string,HSQOBJECT>::iterator rt=GlobalTable.find(string(key));
		if(rt == GlobalTable.end()) 
		{
			RwLck.read_unlock();
			return 0;
		}
		else
		{
			if(type!=rt->second._type)  
			{
				RwLck.read_unlock();
				return 0;
			}
			if(type==OT_STRING)
			{
				const char* value;
				sq_getstring(v,-1,&value);
				const char* cstr=((string*)(rt->second._unVal.pString))->c_str(); 
				sq_pushinteger(v,strcmp(cstr,value)); 
				RwLck.read_unlock();
				return 1;
			}
			if(type==OT_INTEGER)
			{
				SQInteger value;
				sq_getinteger(v,-1,&value);
				if(value==rt->second._unVal.nInteger) value=0;
				else {if(rt->second._unVal.nInteger<value) value=-1;else value=1;}
				sq_pushinteger(v,value); 
				RwLck.read_unlock();
				return 1;
			}
			if(type==OT_BOOL)
			{
				SQBool value;
				sq_getbool(v,-1,&value);
				if(value==rt->second._unVal.nInteger) value=0;
				else {if(rt->second._unVal.nInteger<value) value=-1;else value=1;}
				sq_pushinteger(v,value); 
				RwLck.read_unlock();
				return 1;
			}
			if(type==OT_FLOAT)
			{
				SQInteger ret;
				SQFloat value;
				sq_getfloat(v,-1,&value);
				if(value==rt->second._unVal.fFloat) ret=0;
				else {if(rt->second._unVal.fFloat<value) ret=-1;else ret=1;}
				sq_pushinteger(v,ret); 
				RwLck.read_unlock();
				return 1;
			}
			if(type==OT_USERPOINTER)
			{
				SQInteger ret;
				SQUserPointer value;
				sq_getuserpointer(v,-1,&value);
				if(value==rt->second._unVal.pUserPointer) ret=0;
				else {if(rt->second._unVal.pUserPointer<value) ret=-1;else ret=1;}
				sq_pushinteger(v,ret); 
				RwLck.read_unlock();
				return 1;
			}
		}
		RwLck.read_unlock();
		return 0;
	}

	static SQInteger Sq2PublicTabIn(HSQUIRRELVM v) 
	{
		const char* key;
		if(SQ_FAILED(sq_getstring(v,-1,&key))) return 0;
		RwLck.read_lock(); 
		std::map<string,HSQOBJECT>::iterator rt=GlobalTable.find(string(key));
		if(rt == GlobalTable.end()) 
			sq_pushbool(v,SQFalse);
		else
			sq_pushbool(v,SQTrue);  
		RwLck.read_unlock();
		return 1;
	}

	static SQInteger Sq2PublicGetGlobalTime(HSQUIRRELVM v) 
	{
		SQInstancePointer t;
		if(!check_instancetype(v,"HTime")){sq_throwerror(v,"HTime instance is Required!");return 0;}
		if(SqGetVar(v,t))
		{
			SqTime* pT=(SqTime*)t;
			GTLck.read_lock();
			pT->pTime->mtime=_the_global_time_here.mtime; 	
			pT->pTime->m_timespan=_the_global_time_here.m_timespan; 
			pT->pTime->pre_msecs=_the_global_time_here.pre_msecs;
			GTLck.read_unlock(); 
		}
		return 0;
	}

	static SQInteger Sq2PublicSetGlobalTime(HSQUIRRELVM v) 
	{

		SQInstancePointer t;
		if(!check_instancetype(v,"HTime")){sq_throwerror(v,"HTime instance is Required!");return 0;}
		if(SqGetVar(v,t))
		{
			SqTime* pT=(SqTime*)t;
			GTLck.write_lock();
			_the_global_time_here.mtime=pT->pTime->mtime; 	
			_the_global_time_here.m_timespan=pT->pTime->m_timespan; 
			_the_global_time_here.pre_msecs=pT->pTime->pre_msecs;
			GTLck.write_unlock();  
		}
		return 0;
	}

	static SQInteger Sq2WaitingForSignal(HSQUIRRELVM v) //阻塞并等待被激活
	{
		GCcond.wait();
		return 0;
	}

	static SQInteger Sq2ActivtSignal(HSQUIRRELVM v) //激活被阻塞线程
	{
		GCcond.active();  
		return 0;
	}

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

	static void inject_class(HSQUIRRELVM v)
	{
		const char *classname="Public";
		int top=sq_gettop(v);
		sq_pushroottable(v); //push the root table(were the globals of the script will be stored)
		sq_pushstring(v,classname,-1);
		sq_newclass(v,SQFalse);
		check_type(v,true); //初始化类定义
		sq_createslot(v,-3);
		sq_settop(v,top); //restores the original stack size

		//'o' null, 'i' integer, 'f' float, 'n' integer or float, 's' string, 't' table, 'a' array, 'u' userdata, 
		//'c' closure and nativeclosure, 'g' generator, 'p' userpointer, 'v' thread, 'x' instance(class instance), 'y' class, 'b' bool. and '.' any type

		register_func(v,classname,Sq2PublicTabNewSlot,"_set",".ss|n|p|b"); 
		register_func(v,classname,Sq2PublicTabGet,"_get",".s");
		register_func(v,classname,Sq2PublicTabNewSlot,"_newslot",".ss|n|p|b"); 
		register_func(v,classname,Sq2PublicTabCompare,"_cmp",".ss|n|p|b");
		register_func(v,classname,Sq2PublicTabIn,"is_in",".s"); 
		register_func(v,classname,Sq2PublicGetGlobalTime,"get_global_time",".x"); 
		register_func(v,classname,Sq2PublicSetGlobalTime,"set_global_time",".x"); 
		register_func(v,classname,Sq2WaitingForSignal,"waitting_active",".");  //等待被激活
		register_func(v,classname,Sq2ActivtSignal,"active_signal",".");  //激活等待的对象

	}
};
//-------------------------------------------------------------------------

bool check_instancetype(HSQUIRRELVM v,const char* class_name,int idx) //检查实例是否由指定的类派生
{
	bool rt=false;
	SQInteger oldtop=sq_gettop(v);
	sq_pushroottable(v);
	sq_pushstring(v,class_name,-1);
	if(SQ_SUCCEEDED(sq_get(v,-2)) && sq_gettype(v,-1)==OT_CLASS) //找到需要类
	{
		if(idx<0) idx+=oldtop;
		sq_push(v,idx);
		if(SQ_SUCCEEDED(sq_instanceof(v))) rt=true;
	}
	sq_settop(v,oldtop);
	return rt;
}

bool check_instancetype(HSQUIRRELVM v,HSQOBJECT classobj) //检查实例是否由指定的类派生
{
	bool rt=false;
	sq_pushobject(v,classobj);
	sq_push(v,-2);
	if(SQ_SUCCEEDED(sq_instanceof(v))) rt=true;
	sq_pop(v,2);
	return rt;
}

void* SqGetInstanceUP(HSQUIRRELVM v,const char* obj_name) //得到root table上一个实例的C指针
{
	SQUserPointer rt;
	int oldtop=sq_gettop(v);
	sq_pushroottable(v);
	sq_pushstring(v,obj_name,-1);
	if(SQ_SUCCEEDED(sq_get(v,-2)) && sq_gettype(v,-1)==OT_INSTANCE) //找到需要实例
	{
		if(SQ_SUCCEEDED(sq_getinstanceup(v,-1,&rt,0)))
		{
			sq_settop(v,oldtop);
			return rt;
		}
	}
	sq_settop(v,oldtop);
	return NULL;
}
static SQInteger Sq2IncludeFile(HSQUIRRELVM v)
{
	const char* filename=NULL;
	if(!SqGetVar(v,filename)) return 0;
	sq_pushconsttable(v);
	int tp=sq_gettop(v);
	sq_pushstring(v,"_include_table_",-1);
	if(SQ_SUCCEEDED(sq_get(v,-2)) && sq_gettype(v,-1)==OT_TABLE)
	{
		sq_pushstring(v,filename,-1);
		if(SQ_SUCCEEDED(sq_get(v,-2)) && sq_gettype(v,-1)==OT_INTEGER) 
			return 0;
		sq_pushinteger(v,1);
		sq_createslot(v,-2);
	}
	else
	{
		sq_newtable(v);
		sq_createslot(v,tp);
		sq_pushstring(v,filename,-1);
		sq_pushinteger(v,1);
		sq_createslot(v,-2);
	}
	import_script(v,filename);
	return 0;
}
//----------------------------------------------------------


void cstr_write(sdl_line& mLine,const char* str) //写入一个带引号的cstring
{
	mLine.put('\"'); 
	while(*str!=0)
	{
		if(*str=='\n')
		{mLine.write("\\n",2);++str;continue;}
		else if(*str=='\"')
		{mLine.write("\\\"",2);++str;continue;}
		else if(*str=='\\')
		{mLine.write("\\\\",2);++str;continue;}
		else if(*str=='\r')
		{mLine.write("\\r",2);++str;continue;}
		else if(*str=='\t')
		{mLine.write("\\t",2);++str;continue;}
		else if(*str=='\v')
		{mLine.write("\\v",2);++str;continue;}
		else if(*str=='\b')
		{mLine.write("\\b",2);++str;continue;}
		else if(*str=='\f')
		{mLine.write("\\f",2);++str;continue;}
		else mLine.put(*str); 
		++str;
	}
	mLine.put('\"'); 
}
void fwrite_json_object(sdl_line &io,HSQUIRRELVM mv); //写入json对象
void fwrite_json_array(sdl_line &io,HSQUIRRELVM mv)
{
	if(mv==NULL) return;
	if(sq_gettype(mv,-1)!=OT_ARRAY) return;
	int arrayIndex=sq_gettop(mv);
	sq_pushnull(mv); //准备迭代Table

	io.write("[ ");  
	bool flag_output_comma=false; //需要写逗号
	while(SQ_SUCCEEDED(sq_next(mv,arrayIndex))) //注意，已经压入null,所以index是-2
	{
		if(flag_output_comma) io.put(','); 
		flag_output_comma=true;
		SQObjectType tp=sq_gettype(mv,-1);
		switch(tp)
		{
		case OT_TABLE:
			fwrite_json_object(io,mv);
			break;
		case OT_ARRAY:
			fwrite_json_array(io,mv);
			break;
		case OT_STRING:
			const char* valuestr;
			sq_getstring(mv,-1,&valuestr);
			cstr_write(io,valuestr);
			break;
		case OT_INTEGER:
			int valueint;
			sq_getinteger(mv,-1,&valueint);
			io.printf("%d",valueint);
			break;
		case OT_FLOAT:
			SQFloat valuefloat;
			sq_getfloat(mv,-1,&valuefloat);
			io.printf("%f",valuefloat);
			break;
		case OT_BOOL:
			SQBool valuebool;
			sq_getbool(mv,-1,&valuebool);
			if(valuebool)
				io.write("true");
			else
				io.write("false");
			break;
		}
		sq_pop(mv,2); //弹出得到的key和value;
	}
	io.write("]\n");  
	sq_poptop(mv); //弹出迭代需要的null
};

void fwrite_json_object(sdl_line &io,HSQUIRRELVM mv) //写入json对象
{
	if(mv==NULL) return;
	if(sq_gettype(mv,-1)!=OT_TABLE) return;
	int write_bytes=0;
	static int tabidx=0;

	int tableIndex=sq_gettop(mv);
	sq_pushnull(mv); //准备迭代Table

	for(int i=0;i<tabidx;++i) io.put(' '); 
	io.write("{\n");tabidx+=2;
	bool flag_output_comma=false; //需要写逗号
	while(SQ_SUCCEEDED(sq_next(mv,tableIndex))) 
	{
		const char* keystr;
		if(flag_output_comma) io.write(" , \n");
		flag_output_comma=true;
		for(int i=0;i<tabidx*2;++i) io.put(' '); 

		sq_getstring(mv,-2,&keystr);
		cstr_write(io,keystr);
		io.write(" : ");
		switch(sq_gettype(mv,-1))
		{
		case OT_TABLE:
			fwrite_json_object(io,mv);
			break;
		case OT_ARRAY:
			fwrite_json_array(io,mv);
			break;
		case OT_STRING:
			const char* valuestr;
			sq_getstring(mv,-1,&valuestr);
			cstr_write(io,valuestr);
			break;
		case OT_INTEGER:
			int valueint;
			sq_getinteger(mv,-1,&valueint);
			io.printf("%d",valueint); 
			break;
		case OT_FLOAT:
			SQFloat valuefloat;
			sq_getfloat(mv,-1,&valuefloat);
			io.printf("%f",valuefloat);  
			break;
		case OT_BOOL:
			SQBool valuebool;
			sq_getbool(mv,-1,&valuebool);
			if(valuebool)
				io.write("true");
			else
				io.write("false");
			break;
		}
		sq_pop(mv,2); //弹出得到的key和value;
	}
	tabidx-=2;
	for(int i=0;i<tabidx;++i) io.put(' '); 
	io.write("}\n");  
	sq_poptop(mv); //弹出迭代需要的null
}

SQInteger ReadJson2Table(HSQUIRRELVM v,lig::Scan* pItem,int idx)
{
	while(pItem!=NULL)
	{
		if(pItem->matched_handle==JSON_INTEGER) 
		{
			if(pItem->name!=NULL) 
				sq_pushstring(v,pItem->name,-1);
			sq_pushinteger(v,pItem->to_int());
			if(pItem->name!=NULL) 
				sq_createslot(v,idx);
			else 
				sq_arrayappend(v,idx);
		}
		if(pItem->matched_handle==JSON_FLOAT) 
		{
			if(pItem->name!=NULL) 
				sq_pushstring(v,pItem->name,-1);
			sq_pushfloat(v,(SQFloat)(pItem->to_double()));
			if(pItem->name!=NULL) 
				sq_createslot(v,idx);
			else 
				sq_arrayappend(v,idx);
		}
		if(pItem->matched_handle==JSON_STRING) 
		{
			if(pItem->name!=NULL) 
				sq_pushstring(v,pItem->name,-1);
			sq_pushstring(v,pItem->start,-1);
			if(pItem->name!=NULL) 
				sq_createslot(v,idx);
			else 
				sq_arrayappend(v,idx);
		}
		if(pItem->matched_handle==JSON_NULL) 
		{
			if(pItem->name!=NULL) 
				sq_pushstring(v,pItem->name,pItem->controled_size);
			sq_pushnull(v);
			if(pItem->name!=NULL) 
				sq_createslot(v,idx);
			else 
				sq_arrayappend(v,idx);
		}
		if(pItem->matched_handle==JSON_TRUE) 
		{
			if(pItem->name!=NULL) 
				sq_pushstring(v,pItem->name,-1);
			sq_pushbool(v,SQTrue);
			if(pItem->name!=NULL) 
				sq_createslot(v,idx);
			else 
				sq_arrayappend(v,idx);
		}
		if(pItem->matched_handle==JSON_FALSE) 
		{
			if(pItem->name!=NULL) 
				sq_pushstring(v,pItem->name,-1);
			sq_pushbool(v,SQFalse);
			if(pItem->name!=NULL) 
				sq_createslot(v,idx);
			else 
				sq_arrayappend(v,idx);
		}
		if(pItem->matched_handle==JSON_HEX) 
		{
			if(pItem->name!=NULL) 
				sq_pushstring(v,pItem->name,-1);
			sq_pushinteger(v,pItem->to_hex());
			if(pItem->name!=NULL) 
				sq_createslot(v,idx);
			else 
				sq_arrayappend(v,idx);
		}
		if(pItem->matched_handle== JSON_OBJECT) 
		{
			if(pItem->name!=NULL) 
				sq_pushstring(v,pItem->name,-1);
			sq_newtable(v);
			int tbidx=sq_gettop(v);
			ReadJson2Table(v,pItem->pChild,tbidx);
			if(pItem->pFather==NULL) return 0; 
			if(pItem->name!=NULL) 
				sq_createslot(v,idx);
			else 
				sq_arrayappend(v,idx);
		}
		if(pItem->matched_handle== JSON_ARRAY) 
		{
			if(pItem->name!=NULL) 
				sq_pushstring(v,pItem->name,-1);
			sq_newarray(v,0);
			int tbidx=sq_gettop(v);
			ReadJson2Table(v,pItem->pChild,tbidx);
			if(pItem->name!=NULL) 
				sq_createslot(v,idx);
			else 
				sq_arrayappend(v,idx);
		}
		pItem=pItem->pBrother;
	}
	return 0;
}

static SQInteger Sq2ReadJson(HSQUIRRELVM v)
{
	lig::JsonParser jps;
	const char* filename=NULL;
	const char* key=NULL;
	static sdl_mutex lck;
	if(!SqGetVar(v,filename) && !SqGetVar(v,filename,key)) return 0;
	FILE* file=fopen(filename,"rb");
	if(file==NULL) return 0;
	fseek(file,0,SEEK_END); //定位到文件末 
	int nFileLen = ftell(file); //文件长度
	char* buf=new char[nFileLen+16];

	fseek(file,0,SEEK_SET); //定位到文件头 
	fread(buf,1,nFileLen,file);
	for(int i=0;i<16;++i) buf[nFileLen+i]=0;    //cstr及缓存
	fclose(file);
	jps.init();
	const char* jTxt=buf;
	if(jps.json_parse(jTxt))
	{
		int idx=sq_gettop(v);
		ReadJson2Table(v,jps.pRoot,idx); 
		sq_settop(v,idx+1);
		delete[] buf;
		return 1;
	}
	delete[] buf;
	return 0;
}

SQInteger Sq2WriteJson(HSQUIRRELVM v)
{
	const char* filename;
	sdl_line mLine;
	HSQOBJECT obj;
	if(SqGetVar(v,filename,obj))
	{
		mLine.clear();  
		fwrite_json_object(mLine,v); 
		if(mLine.length()>0)
		{
			FILE* file=fopen(filename,"w");
			if(file==NULL)  return 0;
			fwrite(mLine.get_buf(),1,mLine.length(),file);
			fclose(file);
		}
	}
	return 0;
}


HSQUIRRELVM Create_Full_VM()
{
	HSQUIRRELVM v=init_VirtualMachine(); //初始化虚拟机，加载标准库

	Init_Base(v);
	Init_Util(v); //加载SQShell函数

	//------------------------------------------------------------
	register_func(v,Sq2ImportFile,"import",".s");
	register_func(v,Sq2IncludeFile,"include",".s");
	register_func(v,Sq2GetFileviaHttp,"http_get","ts-isss");
	register_func(v,Sq2ReadJson,"read_json",".s-s");
	register_func(v,Sq2WriteJson,"write_json",".st");
	//获取HTTP文件，host,port,URI,filename,prefix
	//或者 URL,prefix,如果prefix='=',表示输出字符串，prefix是文件名，表示输出到给定的文件，如果prefix是目录
	//（目录需要以'/'或者'\'结尾），表示输出到指定的目录，并以URL规定的文件名保存
	register_func(v,Sq2Vsscanf,"scanf","tssx");
	register_func(v,Sq2GlobalRunning,"is_proc_running",".");
	//------------------------------------------------------------
	SqStdVectorWarp::inject_class(v);   //注入Vector对象 
	SqScanWrap::inject_class(v);       //注入生成树对象

	SqFile::inject_class(v);  //加载文件输入输出类
	SqTime::inject_class(v);  //高精度时间类 
	SqDBM::inject_class(v);  //GDBM数据库支持 
	SqUDPServer::inject_class(v,"UDPServer"); //加载UDP处理类
	PublicAgency::inject_class(v);            //注入公共表，用于不同线程间的数据交换

	return v;
}

#ifdef WIN32

#include "sqrdbg.h"
void debug_script(HSQUIRRELVM v,const char *fname,int port)
{
	//!! INITIALIZES THE DEBUGGER ON THE TCP PORT 1234
	//!! ENABLES AUTOUPDATE
	HSQREMOTEDBG rdbg = sq_rdbg_init(v,port,SQTrue); //Autoupdate =true
	if(rdbg) 
	{
		//!! ENABLES DEBUG INFO GENERATION(for the compiler)
		sq_enabledebuginfo(v,SQTrue);
		//!! SUSPENDS THE APP UNTIL THE DEBUGGER CLIENT CONNECTS
		if(SQ_SUCCEEDED(sq_rdbg_waitforconnections(rdbg))) 
		{
			scprintf(_SC("connected\n"));

			if(SQ_FAILED(sqstd_dofile(v,fname,SQFalse,SQTrue))) 
			{
				PrintError(v);
			}
		}
		//!! CLEANUP
		sq_rdbg_shutdown(rdbg);
	}
	else 
	{
		PrintError(v);
	}

}



#else

void debug_script(HSQUIRRELVM v,const char *fname,int port)
{
};
#endif