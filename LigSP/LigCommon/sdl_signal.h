#ifndef __sdl_signal__
#define __sdl_signal__


#include <list>
#include <memory>

using namespace std;


//--------------------------------------------------------------------
//Simulator of QT's signal/slot
//--------------------------------------------------------------------
struct default_sdl_rwlock
{
	inline void rlock(){};
	inline void wlock(){};
	inline void unwlock(){};
	inline void unrlock(){};
};


//---------------这是一个最简单的回调支持，仅支持单回调-------------
template<class Arg>
struct sdl_callbase //回调函数支持
{
	typedef void (*c_func_type)(Arg arg);
	c_func_type c_callback;
	sdl_callbase(){c_callback=NULL;}
	virtual void call(Arg arg){if(c_callback!=NULL) (*c_callback)(arg);};
	virtual bool operator==(sdl_callbase &other){return false;};
};

template<class T,class Arg>
class sdl_function: public sdl_callbase<Arg>
{
	public:
		typedef void (T::*funcType)(Arg arg);
	private:
		T *obj;
		funcType func;
	public:
		sdl_function(T* obj_p, funcType f)
			: obj(obj_p),func(f){}
		inline T* get_obj(){return obj;};
		inline funcType get_func(){return func;};

		bool operator==(sdl_callbase<Arg> &tmp)
		{
			sdl_function* other=(sdl_function*) (&tmp);
			return((obj==other->get_obj()) && (func==other->get_func()));
		}
		void call(Arg arg) {sdl_callbase<Arg>::call(arg);(obj->*func)(arg);}
};
//-----------------------------------------------------------
template<class Arg,class RtType>
struct sdl_rt_callbase //能够返回参数的,回调函数支持
{
	typedef RtType (*c_rt_func_type)(Arg arg);
	c_rt_func_type c_callback;
	sdl_rt_callbase(){c_callback=NULL;}
	virtual RtType call(Arg arg){if(c_callback!=NULL) return (*c_callback)(arg);else return RtType();};
	virtual bool operator==(sdl_rt_callbase &other){return false;};
};

template<class T,class Arg,class RtType>
class sdl_rt_function: public sdl_rt_callbase<Arg,RtType>
{
	public:
		typedef RtType (T::*funcType)(Arg arg);
	private:
		T *obj;
		funcType func;
	public:
		sdl_rt_function(T* obj_p, funcType f)
			: obj(obj_p),func(f){}
		inline T* get_obj(){return obj;};

		inline funcType get_func(){return func;};

		bool operator==(sdl_rt_callbase<Arg,RtType> &tmp)
		{
			sdl_rt_function* other=(sdl_rt_function*) (&tmp);
			return((obj==other->get_obj()) && (func==other->get_func()));
		}
		inline RtType call(Arg arg) { return (obj->*func)(arg); }
};
//------------------------------------------------------------------
template<class arg1>
class sdl_callbase1 
{
public:
	virtual void call(arg1 x)=0;
	virtual bool operator==(sdl_callbase1<arg1> &other)=0;
};

template<class arg1,class _lock=default_sdl_rwlock>
class sdl_signal1 
{
public:
	typedef sdl_callbase1<arg1>* Base;
	typedef list<Base> Matrix;
private:
	Matrix lst;
	_lock lock;
public:
	sdl_signal1() {};
	
	~sdl_signal1() 
	{
		lock.wlock();
		typename Matrix::iterator i;
		for( i=lst.begin();i!=lst.end();++i)
	 		delete (*i);
		lock.unwlock();
	};

	inline void clear()
	{
		lock.wlock();
		typename Matrix::iterator i;
		for( i=lst.begin();i!=lst.end();++i)
	 		delete (*i);
		lock.unwlock();
	}
	
	inline void insert(Base item) 
	{
		lock.wlock();
		typename Matrix::iterator i=lst.begin();
		while(i!=lst.end())
			if(*(*i)==*item) return;
			else ++i;
		
		lst.push_back(item);
		lock.unwlock();
	};
	
	inline void erase(Base tmp)
	{
	
		lock.wlock();
		typename Matrix::iterator i=lst.begin();
		while(i!=lst.end() && *(*i)==*tmp) ++i;
		if(i!=lst.end()) 
		{
			Base pLink=*i;
			lst.erase(i);
			delete pLink;
			return;
		}
		lock.unwlock();
	};
	
	inline void operator()(arg1 x) 
	{
		lock.rlock();
		
		for(typename Matrix::iterator i=lst.begin();i!=lst.end();++i) 
			(*i)->call(x);
		lock.unrlock(); 
	}

	inline void emit(arg1 x)
	{
		lock.rlock();
		for(typename Matrix::iterator i=lst.begin();i!=lst.end();++i)
			(*i)->call(x);
		lock.unrlock();
	}
	
};

template<class T, class Arg1>
class sdl_function1 : public sdl_callbase1<Arg1> 
{
public:
	typedef void (T::*funcType)(Arg1);
private:
	T *obj;
	funcType func;
public:
	sdl_function1(T* obj_, funcType f)
		: obj(obj_),func(f){}
	inline T* get_obj(){return obj;};
	inline funcType get_func(){return func;};
		
	bool operator==(sdl_callbase1<Arg1> &tmp)
	{
		sdl_function1* other=(sdl_function1*) (&tmp);
		return((obj==other->get_obj()) && (func==other->get_func()));
	}
	inline void call(Arg1 x) { (obj->*func)(x); }
};


template<class T, class Arg1>
void connect(sdl_signal1<Arg1>& sig,T& obj, void (T::*f)(Arg1))
{
	sdl_function1<T,Arg1> *tmp=new sdl_function1<T,Arg1>(&obj,f);
	sig.insert(tmp);
}
//eg: connect(sig,*this,&CLASS_NAME::FUNCTION_NAME);
template<class T, class Arg1>
void sig_connect(sdl_signal1<Arg1>& sig,T& obj, void (T::*f)(Arg1))
{
	sdl_function1<T,Arg1> *tmp=new sdl_function1<T,Arg1>(&obj,f);
	sig.insert(tmp);
}

template<class T, class Arg1>
void sig_disconnect(sdl_signal1<Arg1>& sig,T& obj, void (T::*f)(Arg1))
{
	sdl_function1<T,Arg1> *tmp=new sdl_function1<T,Arg1>(&obj,f);
	sig.erase(tmp);
	delete tmp;
}

template<class Arg1>
inline void emit(sdl_signal1<Arg1>& sig,Arg1 x)
{
	sig(x);
}
//-------------------------------------------------------------------

template<class arg1,class arg2>
class sdl_callbase2 
{
public:
	virtual void call(arg1 x,arg2 y)=0;
	virtual bool operator==(sdl_callbase2<arg1,arg2> &other)=0;
};

template<class arg1,class arg2,class _lock=default_sdl_rwlock>
class sdl_signal2 
{
public:
	typedef sdl_callbase2<arg1,arg2>* Base;
	typedef list<Base> Matrix;
private:
	Matrix lst;
	_lock lock;
public:
	sdl_signal2() {};
	
	~sdl_signal2() 
	{
		lock.wlock();
		for(typename Matrix::iterator i=lst.begin();i!=lst.end();++i)
	 		delete (*i);
		lock.unwlock();
	 
	};
	
	inline void insert(Base item) 
	{
		lock.wlock();
		typename Matrix::iterator i=lst.begin();
		while(i!=lst.end())
			if(*(*i)==*item) return;
			else ++i;
			
		lst.push_back(item);
		lock.unwlock();
	};
	
	inline void erase(Base tmp)
	{
		lock.wlock();
		for(typename Matrix::iterator i=lst.begin();i!=lst.end();++i)
		if(*(*i)==*tmp) 
		{
			lst.erase(i);
			delete (*i);
			return;
		}
		lock.unwlock();
	};
	
	inline void operator()(arg1 x,arg2 y) 
	{
		lock.rlock();
		for(typename Matrix::iterator i=lst.begin();i!=lst.end();++i)
		(*i)->call(x,y); 
		lock.unrlock();
	}
	
};

template<class T, class Arg1,class Arg2>
class sdl_function2 : public sdl_callbase2<Arg1,Arg2> 
{
public:
	typedef void (T::*funcType)(Arg1,Arg2);
private:
	T *obj;
	funcType func;
public:
	sdl_function2(T* obj_, funcType f)
		: obj(obj_),func(f){}
	inline T* get_obj(){return obj;};
	inline funcType get_func(){return func;};
		
	bool operator==(sdl_callbase2<Arg1,Arg2> &tmp)
	{
		sdl_function2* other=(sdl_function2*) (&tmp);
		return((obj==other->get_obj()) && (func==other->get_func()));
	}
	inline void call(Arg1 x,Arg2 y) { (obj->*func)(x,y); }
};
//--------------------------------------------------------------------

template<class T, class Arg1, class Arg2>
inline void connect(sdl_signal2<Arg1,Arg2>& sig,T& obj, void (T::*f)(Arg1,Arg2))
{
	sdl_function2<T,Arg1,Arg2> *tmp=new sdl_function2<T,Arg1,Arg2>(&obj,f);
	sig.insert(tmp);
}

template<class T, class Arg1, class Arg2>
inline void disconnect(sdl_signal2<Arg1,Arg2>& sig,T& obj, void (T::*f)(Arg1,Arg2))
{
	sdl_function2<T,Arg1,Arg2> *tmp=new sdl_function2<T,Arg1,Arg2>(&obj,f);
	sig.erase(tmp);
	delete tmp;
}

template<class Arg1,class Arg2>
inline void emit(sdl_signal2<Arg1,Arg2>& sig,Arg1 x,Arg2 y)
{
	sig(x,y);
};
//-------------------------------------------------------------------
template<class arg1,class arg2,class arg3>
class sdl_callbase3 
{
public:
	virtual void call(arg1 x,arg2 y,arg3 z)=0;
	virtual bool operator==(sdl_callbase3<arg1,arg2,arg3> &other)=0;
};

template<class arg1,class arg2,class arg3>
class sdl_signal3 
{
public:
	typedef sdl_callbase3<arg1,arg2,arg3>* Base;
	typedef list<Base> Matrix;
private:
	Matrix lst;
public:
	sdl_signal3() {};
	
	~sdl_signal3() 
	{
	for(typename Matrix::iterator i=lst.begin();i!=lst.end();++i)
	 delete (*i);
	};
	
	inline void insert(Base item) 
	{
		typename Matrix::iterator i=lst.begin();
		while(i!=lst.end())
			if(*(*i)==*item) return;
			else ++i;
			
		lst.push_back(item);
	};
	
	inline void erase(Base tmp)
	{
		for(typename Matrix::iterator i=lst.begin();i!=lst.end();++i)
		if(*(*i)==*tmp) 
		{
			lst.erase(i);
			delete (*i);
			
			return;
		}
	};
	
	inline void operator()(arg1 x,arg2 y,arg3 z) 
	{
		for(typename Matrix::iterator i=lst.begin();i!=lst.end();++i)
		(*i)->call(x,y,z); 
	}
	
};

template<class T, class Arg1,class Arg2,class Arg3>
class sdl_function3 : public sdl_callbase3<Arg1,Arg2,Arg3> 
{
public:
	typedef void (T::*funcType)(Arg1,Arg2,Arg3);
private:
	T *obj;
	funcType func;
public:
	sdl_function3(T* obj_, funcType f)
		: obj(obj_),func(f){}
	inline T* get_obj(){return obj;};
	inline funcType get_func(){return func;};
		
	bool operator==(sdl_callbase3<Arg1,Arg2,Arg3> &tmp)
	{
		sdl_function3* other=(sdl_function3*) (&tmp);
		return((obj==other->get_obj()) && (func==other->get_func()));
	}
	inline void call(Arg1 x,Arg2 y,Arg3 z) { (obj->*func)(x,y,z); }
};
//--------------------------------------------------------------------

template<class T, class Arg1, class Arg2, class Arg3>
inline void connect(sdl_signal3<Arg1,Arg2,Arg3>& sig,T& obj, void(T::*f)(Arg1,Arg2,Arg3))
{
	sdl_function3<T,Arg1,Arg2,Arg3> *tmp=new sdl_function3<T,Arg1,Arg2,Arg3>(&obj,f);
	sig.insert(tmp);
}

template<class T, class Arg1, class Arg2, class Arg3>
inline void disconnect(sdl_signal3<Arg1,Arg2,Arg3>& sig,T& obj, void(T::*f)(Arg1,Arg2,Arg3))
{
	sdl_function3<T,Arg1,Arg2,Arg3> *tmp=new sdl_function3<T,Arg1,Arg2,Arg3>(&obj,f);
	sig.erase(tmp);
	delete tmp;
}

template<class Arg1,class Arg2,class Arg3>
inline void emit(sdl_signal3<Arg1,Arg2,Arg3>& sig,Arg1 x,Arg2 y,Arg3 z)
{
	sig(x,y,z);
};

//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
template<class arg1,class arg2,class arg3,class arg4>
class sdl_callbase4 
{
public:
	virtual void call(arg1 x,arg2 y,arg3 z,arg4 w)=0;
	virtual bool operator==(sdl_callbase4<arg1,arg2,arg3,arg4> &other)=0;
};

template<class arg1,class arg2,class arg3,class arg4>
class sdl_signal4 
{
public:
	typedef sdl_callbase4<arg1,arg2,arg3,arg4>* Base;
	typedef list<Base> Matrix;
private:
	Matrix lst;
public:
	sdl_signal4() {};
	
	~sdl_signal4() 
	{
	for(typename Matrix::iterator i=lst.begin();i!=lst.end();++i)
	 delete (*i);
	};
	
	inline void insert(Base item) 
	{
		typename Matrix::iterator i=lst.begin();
		while(i!=lst.end())
			if(*(*i)==*item) return;
			else ++i;
			
		lst.push_back(item);
	};
	
	inline void erase(Base tmp)
	{
		for(typename Matrix::iterator i=lst.begin();i!=lst.end();++i)
		if(*(*i)==*tmp) 
		{
			lst.erase(i);
			delete (*i);
			
			return;
		}
	};
	
	inline void operator()(arg1 x,arg2 y,arg3 z,arg4 w) 
	{
		for(typename Matrix::iterator i=lst.begin();i!=lst.end();++i)
		(*i)->call(x,y,z,w); 
	}
	
};

template<class T, class Arg1,class Arg2,class Arg3,class Arg4>
class sdl_function4 : public sdl_callbase4<Arg1,Arg2,Arg3,Arg4> 
{
public:
	typedef void (T::*funcType)(Arg1,Arg2,Arg3,Arg4);
private:
	T *obj;
	funcType func;
public:
	sdl_function4(T* obj_, funcType f)
		: obj(obj_),func(f){}
	inline T* get_obj(){return obj;};
	inline funcType get_func(){return func;};
		
	bool operator==(sdl_callbase4<Arg1,Arg2,Arg3,Arg4> &tmp)
	{
		sdl_function4* other=(sdl_function4*) (&tmp);
		return((obj==other->get_obj()) && (func==other->get_func()));
	}
	inline void call(Arg1 x,Arg2 y,Arg3 z,Arg4 w) { (obj->*func)(x,y,z,w); }
};
//--------------------------------------------------------------------

template<class T, class Arg1, class Arg2, class Arg3,class Arg4>
inline void connect(sdl_signal4<Arg1,Arg2,Arg3,Arg4>& sig,T& obj, void(T::*f)(Arg1,Arg2,Arg3,Arg4))
{
	sdl_function4<T,Arg1,Arg2,Arg3,Arg4> *tmp=new sdl_function4<T,Arg1,Arg2,Arg3,Arg4>(&obj,f);
	sig.insert(tmp);
}

template<class T, class Arg1, class Arg2, class Arg3,class Arg4>
inline void disconnect(sdl_signal4<Arg1,Arg2,Arg3,Arg4>& sig,T& obj, void(T::*f)(Arg1,Arg2,Arg3,Arg4))
{
	sdl_function4<T,Arg1,Arg2,Arg3,Arg4> *tmp=new sdl_function4<T,Arg1,Arg2,Arg3,Arg4>(&obj,f);
	sig.erase(tmp);
	delete tmp;
}

template<class Arg1,class Arg2,class Arg3,class Arg4>
inline void emit(sdl_signal4<Arg1,Arg2,Arg3,Arg4>& sig,Arg1 x,Arg2 y,Arg3 z,Arg4 w)
{
	sig(x,y,z,w);
};

#endif
