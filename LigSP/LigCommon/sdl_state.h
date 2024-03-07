//这是一个简单的状态变更机，用于存储状态及处理状态变化 2010.05.07


#ifndef __SDL_NFA__
#define __SDL_NFA__

#include <set>
#include "sdl_memory.h"


#define _STTermCh '.'
//定义字符串树的分割符号

struct StringTree
{
	StringTree* pChild;
	StringTree* pBrother;
	unsigned char ch;
	int handle;

	StringTree()
	{
		clear();
	}

	void clear()
	{
		ch=_STTermCh;
		pChild=NULL;
		pBrother=NULL;
		handle=-1;
	}
};

extern StringTree _STroot;

extern StringTree*  _dict_insert(const char* str,int handle,const char* strEnd=NULL,StringTree* it=&_STroot);
extern int _dict_find(const char* str,const char* strEnd=NULL,StringTree* it=&_STroot);
extern void _insert_string(const char* str,StringTree* it=&_STroot);
extern void _clear_dict();
extern const char* _get_string(int handle,StringTree* it=&_STroot);
extern int _get_handle(const char* str);
#define ST_USER_HANDLE 0x7FFFFFFF

struct sdl_mini_set
{
	unsigned int table[32];
	unsigned short pend;
	unsigned short curr_size;
	unsigned int *p_table;

	typedef	unsigned int* iterator;
	sdl_mini_set(){pend=0;p_table=table;curr_size=32;};
	sdl_mini_set(const sdl_mini_set &s){assign(s);};
	~sdl_mini_set(){if(curr_size!=32) delete[] p_table;};
	inline void resize()
	{

		if(curr_size==32)
		{
			p_table=new unsigned int[curr_size+32];
			if(p_table==NULL) throw "Memory alloc error";
			memcpy(p_table,table,32);
		}
		else
		{
			unsigned int *p_old_table=p_table;
			p_table=new unsigned int[curr_size+32];
			if(p_table==NULL) throw "Memory alloc error";
			memcpy(p_table,p_old_table,curr_size);
			delete[] p_old_table;
		}
		curr_size+=32;
		if(curr_size>65536-64) 
			throw "SDL_MINI_SET is not fit so much memeber";
	}

	inline void insert(unsigned int val)
	{
		register int i,j;
		if(pend==curr_size-1) resize();
		for(i=0;i<pend;++i)
		{
			if(val<p_table[i]) break;
			else if(val==p_table[i]) return;
		}
		for(j=pend-1;j>=i;--j) p_table[j+1]=p_table[j];
		p_table[i]=val;
		++pend;
	};

	inline void insert(int val)
	{
		register int i,j;
		if(pend==curr_size-1) resize();
		for(i=0;i<pend;++i)
		{
			if((unsigned int)val<p_table[i]) break;
			else if(val==p_table[i]) return;
		}
		for(j=pend-1;j>=i;--j) p_table[j+1]=p_table[j];
		p_table[i]=val;
		++pend;
	};

	inline iterator begin(){return	p_table;};
	inline iterator end(){return p_table+pend;};
	inline bool empty(){return	(pend==0);};
	inline void clear(){pend=0;};

	bool operator ==(const sdl_mini_set &s)
	{
		if(s.pend!=pend) return	false;
		for(int	i=0;i<pend;++i)
			if(s.p_table[i]!=p_table[i]) return	false;
		return true;
	};

	void assign(const sdl_mini_set &s)
	{
		if(s.curr_size<=curr_size)
		{
			for(int i=0;i<s.pend;++i)
				p_table[i]=s.p_table[i];
			pend=s.pend;
		}
		else
		{
			unsigned int *p_old_table=p_table;
			p_table=new unsigned int[s.curr_size];
			if(p_table==NULL) throw "Memory alloc error";
			if(curr_size>32) delete[] p_old_table;
			curr_size=s.curr_size;
			for(int i=0;i<s.pend;++i)
				p_table[i]=s.p_table[i];
			pend=s.pend;
		}
	};

	inline int get_hashvalue()
	{
		int rt=pend<<16;
		for(int	i=0;i<pend;++i)
			rt+=p_table[i];
		return rt;
	}

	int operator[](int loc)
	{
		unsigned short sloc=(unsigned short)loc;
		if(sloc<pend)
			return p_table[loc];
		else return 0;
	}
};
struct nfa_chord
{
	int	ch;
	int	fromID;
	int	toID;
};

struct less_nfa_chord_ch_from
{
	bool operator()(const nfa_chord* p1,const nfa_chord *p2) const
	{
		if(p1->ch==p2->ch)	return p1->fromID<p2->fromID;	 
		return p1->ch<p2->ch; 
	};
};

struct less_nfa_chord
{
	bool operator()(const nfa_chord &o1,const nfa_chord &o2) const
	{
		if(o1.ch==o2.ch)
		{
			if(o1.fromID==o2.fromID) return	o1.toID<o2.toID;
			return o1.fromID<o2.fromID;	 
		}
		return o1.ch<o2.ch;	
	};
};

#define P_SET sdl_mini_set

#ifdef _NFA_DEBUG
static int perference_counter=0;
#endif

typedef std::set<nfa_chord,less_nfa_chord> DataSet;
typedef std::multiset<nfa_chord*,less_nfa_chord_ch_from> IdxSet;
typedef std::set<nfa_chord,less_nfa_chord>::iterator  DataIter;
typedef std::multiset<nfa_chord*,less_nfa_chord_ch_from>::iterator  IdxIter;

class nfa_chord_set     
{
private:
	DataSet data;
	IdxSet index;
	int id;
public:

	P_SET curr;

	nfa_chord_set(){id=1;};

	int get_id(){id++;return id;};
	
	void clear();

	bool find(int from,int ch);

	bool trans_single(int signal,P_SET &sset);

	int trans_single_base(int from,int ch);

	bool prompt(int	from,int ch);

	bool prompt(int	from,int ch,P_SET *result_set);

	bool trans(int signal,P_SET &sset);

	void erase(int ch,int from);

	void erase(int ch,int from,int to);

	void add_NULL(int from,int to);

	void add(int ch,int	from,int to);
	
};


extern nfa_chord_set _lig_set; //申明一个状态管理机--

//定义一条规则，第一参数是源状态，第二参数是目的状态，第三参数是触发信号--
#define RULE(F,T,S) _lig_set.add(_get_handle(#S),_get_handle(#F),_get_handle(#T));

//执行一个单一状态转移，第一参数是源状态，第二参数是触发机制--
#define TRANS(F,S) _lig_set.prompt(_get_handle(#F),_get_handle(#S))

#define Trans_First(F,S) _lig_set.trans_first(F,S)

/*
struct StringDir 
{
	int st;
	int Path[32];
	int pPath;

	StringDir()
	{
		st=0;
	};

	StringDir(int s)
	{
		st=s;
	};

	StringDir(const char* name)
	{
		const char *p=name;
		while(*p!=0)
		{
			if((*p)<'0' || (*p)>'9' )
			{
				st=_dict_find(name);
				return;
			}
			++p;
		}
		st=atoi(name);
	};


	inline StringDir operator=(const char* name)
	{
		const char *p=name;
		while(*p!=0)
		{
			if((*p)<'0' || (*p)>'9' )
			{
				st=_dict_find(name);
				return *this;
			}
			++p;
		}
		st=atoi(name);
		return *this;
	};

	inline StringDir operator=(int s)
	{
		st=s;
		return *this;
	};

	inline StringDir operator=(StringDir s)
	{
		st=s.st; 
		return *this;
	};

	inline bool operator!()
	{
		return (st<=0);
	};

	inline bool operator==(StringDir s)
	{
		return s.st==st;
	};

	inline bool operator!=(StringDir s)
	{
		return s.st!=st;
	};

	inline bool operator==(int s)
	{
		return s==st;
	};

	inline bool operator==(const char *name)
	{
		return (strcmp(_get_string(st),name)==0);
	};

	inline bool operator!=(const char *name)
	{
		return (strcmp(_get_string(st),name)!=0);
	};


	inline bool operator<(StringDir s)
	{
		return st<s.st;
	};

	inline bool operator>(StringDir s)
	{
		return st>s.st;
	};

	inline const char* c_str()
	{
		return _get_string(st);
	};

	inline const char* name()
	{
		return _get_string(st);
	};
};

*/

#endif
