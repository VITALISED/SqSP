#ifndef __SDL_MEMEORY_MNG__
#define __SDL_MEMEORY_MNG__
//----------------------------------------------
//------------提供手工构造和解构的模板函数-----
template <class T1, class T2>
inline void constructor(T1* p, T2& val) 
{
	new ((void *) p) T1(val);
}

template <class T1, class T2, class T3>
inline void constructor(T1* p, T2 val1, T3 val2) 
{
	new ((void *) p) T1(val1,val2);
}

template <class T1>
inline void constructor(T1* p) 
{
	new ((void *) p) T1;
}

template <class T1>
inline void destructor(T1* p)
{
	p->~T1();
}
//页面式内存管理器，PageNmuber为最大页面数量，PageBits为页面大小缺省为（2^10）
template<class T,int PageNumber,int PageBits=10> 
class PageMemory
{
private:
	T** p_table;  //缓冲区指针,指向可以使用的若干页面区
	int PageSize; //内存页面尺寸
	int PageMask; //内存页面掩码
	int TopMem; //最大可用内存

public:
	PageMemory()
	{
		typedef T* TT;
		PageSize=1;
		PageSize=PageSize<<PageBits; //得到内存页面的大小
		TopMem=PageSize*PageNumber;
		PageMask=PageSize-1; //得到掩码的值，用于快速计算偏移量
		p_table = new TT[PageNumber];
		for(int i=0;i<PageNumber;++i) p_table[i]=NULL;
	}

	~PageMemory()
	{
		for(int i=0;i<PageNumber;++i) 
			if(p_table[i]!=NULL) delete[] (char*)(p_table[i]);
		delete[] p_table;
	}

	inline T& operator[](int loc)
	{
		if(loc>=TopMem) throw "Defined memory too small";
		register T* p_base=p_table[loc>>PageBits];
		if(p_base==NULL)
		{
			p_base=(T*)(new char[sizeof(T)*PageSize]); //申请新内存空间
			p_table[loc>>PageBits]=p_base; //将新申请的空间记录到页面管理器
		}
		return p_base[loc & PageMask];
	}

	inline T* operator()(int loc)
	{
		if(loc>=TopMem) throw "Defined memory too small";
		register T* p_base=p_table[loc>>PageBits];
		if(p_base==NULL)
		{
			p_base=(T*)(new char[sizeof(T)*PageSize]); //申请新内存空间
			p_table[loc>>PageBits]=p_base; //将新申请的空间记录到页面管理器
		}
		return p_base+(loc & PageMask);
	}

	inline T* offset(T* pnt,int off) //由于是分页式内存管理，为了避免跨页时的错误，指针偏移需要这个函数处理
	{
		register int i,offset;
		if(off==0) return pnt; //没有偏移的话，就不用下面这些复杂步骤了
		for(i=0;i<PageNumber;++i)
			if(p_table[i]!=NULL)
			{
				offset = (int)(pnt-p_table[i]);
				if(offset>=0)
				{
					if((offset+off)<PageSize) return pnt+off; //在一个内存块内
					if(offset<PageSize) //找到起始地址，但偏移不在同一内存块内
					{
						offset+=off;
						off=offset & PageMask;
						offset=offset>>PageBits;
						return p_table[offset]+off;
					}
				}
			}
			return NULL;
	}
};

template<class T>
struct Mem_Inner_Struct
{
	T data;
	char flag; // 用于标记这个内存块是否被使用，1为已经使用，0为未使用
};

//页面式内存申请器，PageNmuber为最大页面数量，PageBits为页面大小缺省为（2^10）
//这个内存分配器支持内存池的基本特性，支持释放内存
template<class T,int PageNumber=256,int PageBits=10> 
class MemAlloctor
{
private:
	typedef Mem_Inner_Struct<T> Inner_Struct;
	Inner_Struct** p_table;  //缓冲区指针,指向可以使用的若干页面区

	int PageSize; //内存页面尺寸
	int PageMask; //内存页面掩码
	int TopMem; //最大可用内存
	int FreeLoc; //分配器的空闲地址
	int LastHandle;

	union inner_class //为了快速分配内存，对象大小必须超过int的大小
	{
		char data[sizeof(Inner_Struct)];
		int loc;
	};

	//初始化内存区域，使之可以被正常分配和使用，loc只是在下一段的位置，不要求精确值
	inline void initMem(Inner_Struct*  pmem,int loc)
	{
		register int i;
		inner_class* pdata=(inner_class*)(pmem);

		loc=loc>>PageBits;loc=loc<<PageBits; //得到一个初始位置
		for(i=0;i<PageSize;++i)
		{
			pdata[i].loc=loc+i+1;
			pmem[i].flag=0;
		}
	};


public:
	MemAlloctor()
	{
		typedef Inner_Struct* TT;
		if(sizeof(T)<sizeof(int)) //为了快速分配内存，对象大小必须超过int的大小
			throw "MemoryMng Class object's size is too small";
		PageSize=1;
		PageSize=PageSize<<PageBits; //得到内存页面的大小
		TopMem=PageSize*PageNumber;
		PageMask=PageSize-1; //得到掩码的值，用于快速计算偏移量
		p_table = new TT[PageNumber];
		for(int i=0;i<PageNumber;++i) p_table[i]=NULL;
		FreeLoc=0;
		LastHandle=0;
	}

	inline int size()
	{
		return LastHandle;
	};

	inline T* alloc(int *phandle=NULL) //在Alloc的时候，开始实际分配内存
	{
		if(FreeLoc>=TopMem) throw "Defined memory too small";

		register Inner_Struct*  p_base=p_table[FreeLoc>>PageBits];
		if(p_base==NULL)
		{
			p_base=(Inner_Struct*)(new char[sizeof(Inner_Struct)*PageSize]); //申请新内存空间
			initMem(p_base,FreeLoc); //初始化内存指针序列
			p_table[FreeLoc>>PageBits]=p_base; //将新申请的空间记录到页面管理器
		}
		p_base+=(FreeLoc & PageMask);
		if(phandle!=NULL) *phandle=FreeLoc;
		FreeLoc=((inner_class*)(p_base))->loc; //找到下一个未使用的区域
		p_base->flag=1; //标记这个内存块已经被使用
		if(FreeLoc>LastHandle) LastHandle=FreeLoc; //得到最后使用的一个位置
		return &(p_base->data);
	}

	//phandle参数传递回已分配对象在分配器中的索引位置
	inline T* create(int *phandle=NULL) //申请对象，做不带参数构造
	{
		T* rt=alloc(phandle);
		constructor(rt); //对申请的内存区域执行构造
		return rt;
	}

	//phandle参数传递回已分配对象在分配器中的索引位置
	template<class T2>
	inline T* make(T2& val,int *phandle=NULL) //申请对象，做带一个参数的构造
	{
		int hd;
		T* rt=alloc(&hd);
		constructor(rt,val); //对申请的内存区域执行构造
		if(phandle!=NULL) *phandle=hd;
		return rt;
	}

	
	inline int get_handle(T* pnt) //获取一个对象指针的位置编号，如果不在管理区域内，返回无效值-1
	{
		register int i,offset;
		for(i=0;i<PageNumber;++i)
			if(p_table[i]!=NULL)
			{
				offset = static_cast<int>((Inner_Struct*)(pnt) - p_table[i]);
				if(offset<PageSize && offset>=0) 
				{
					if(((Inner_Struct*)(pnt))->flag==0) return -1; //此处数据并未被申请，所以返回无效值
					return (i<<PageBits)+offset; 
				}
			}
		return -1;
	}

	inline void free(T* pnt) //仅仅对管理的对象有效
	{
		register int i,offset;
		for(i=0;i<PageNumber;++i)
			if(p_table[i]!=NULL)
			{
				offset = static_cast<int>((Inner_Struct*)(pnt) - p_table[i]);
				if(offset<PageSize && offset>=0) 
				{
					if(((Inner_Struct*)(pnt))->flag==0) return;//如果是重复删除对象，此处检测并处理
					int preFreeLoc=FreeLoc; //保留原来的空余区号
					FreeLoc=(i<<PageBits)+offset; //将空余区号更新为当前删除的位置

					destructor(pnt); //析构对象
					((inner_class*)(pnt))->loc=preFreeLoc;//链接原来的位置
					((Inner_Struct*)(pnt))->flag=0; 
					return; //返回，避免不必要的循环
				}
			}
	};

	inline void free(int loc) //仅仅对管理的对象有效
	{
		if(loc>=TopMem || loc<0)  return;
		register Inner_Struct* p_base=p_table[loc>>PageBits];
		if(p_base==NULL) return;
		if(p_base[loc & PageMask].flag==0) return;//如果是重复删除对象，此处检测并处理;
		int preFreeLoc=FreeLoc; //保留原来的空余区号
		destructor(&((p_base+(loc & PageMask))->data)); //析构对象
		FreeLoc=loc; //将空余区号更新为当前删除的位置
		((inner_class*)&(p_base[loc & PageMask].data))->loc=preFreeLoc;//链接原来的位置
		p_base[loc & PageMask].flag=0; //设置标志位，表明可用
	};

	inline void del(T* pnt) //不做析构，删除申请的内存区域，仅仅对管理的对象有效
	{
		register int i,offset;
		for(i=0;i<PageNumber;++i)
			if(p_table[i]!=NULL)
			{
				offset = static_cast<int>((Inner_Struct*)(pnt) - p_table[i]);
				if(offset<PageSize && offset>=0) 
				{
					if(((Inner_Struct*)(pnt))->flag==0) return;//如果是重复删除对象，此处检测并处理
					int preFreeLoc=FreeLoc; //保留原来的空余区号
					FreeLoc=(i<<PageBits)+offset; //将空余区号更新为当前删除的位置
					((inner_class*)(pnt))->loc=preFreeLoc;//链接原来的位置
					((Inner_Struct*)(pnt))->flag=0; 
					return; //返回，避免不必要的循环
				}
			}
	};

	inline void del(int loc) //不做析构，删除申请的内存区域，仅仅对管理的对象有效
	{
		if(loc>=TopMem)  return;
		register Inner_Struct* p_base=p_table[loc>>PageBits];
		if(p_base==NULL) return;
		if(p_base[loc & PageMask].flag==0) return;//如果是重复删除对象，此处检测并处理;
		int preFreeLoc=FreeLoc; //保留原来的空余区号
		FreeLoc=loc; //将空余区号更新为当前删除的位置
		((inner_class*)(p_base[loc & PageMask].data))->loc=preFreeLoc;//链接原来的位置
		p_base[loc & PageMask].flag=0; //设置标志位，表明可用
	};


	
	~MemAlloctor()
	{
		for(int i=0;i<PageNumber;++i) 
			if(p_table[i]!=NULL) delete[] (char*)(p_table[i]);
		delete[] p_table;
	}

	//根据对象的索引，从内存池中获得指定的对象
	inline T& operator[](int loc)
	{
		if(loc>=TopMem || loc<0) throw "Defined memory too small";
		register Inner_Struct* p_base=p_table[loc>>PageBits];
		if(p_base==NULL) throw "Handle not be used!";
		return p_base[loc & PageMask].data;
	}

	//根据对象的索引，从内存池中获得指定的对象，如果指定对象不存在，则返回空指针
	inline T* operator()(int loc)
	{
		if(loc<0 || loc>=TopMem)  return NULL;
		register Inner_Struct* p_base=p_table[loc>>PageBits];
		if(p_base==NULL) return NULL;
		return &(p_base[loc & PageMask].data);
	}

	//清除内存区域，使之可以被正常分配和使用
	inline void clear()
	{
		int oft=LastHandle & PageMask;
		int seg=LastHandle>>PageBits;
		inner_class* pdata;

		for(int i=0;i<seg;++i)
		{
			pdata=(inner_class*)(p_table[i]);
			int loc=i*PageSize;
			for(int j=0;j<PageSize;++j)
			{
				pdata[j].loc=loc+j+1;
				((Inner_Struct*)pdata)[j].flag=0;
			}
		}

		pdata=(inner_class*)(p_table[seg]);
		seg=seg*PageSize;
		for(int k=0;k<oft;++k)
		{
			pdata[k].loc=seg+k+1;
			((Inner_Struct*)pdata)[k].flag=0;
		}

		LastHandle=0;
		FreeLoc=0;
	};

	//析构全部对象，并清除内存区域，使之可以被正常分配和使用
	inline void destroy()
	{
		int i=0;
		int n=LastHandle;
		typename MemAlloctor::Inner_Struct* p_base;
		while(i<n)
		{
			p_base=p_table[i>>PageBits];
			if(p_base==NULL) throw "Handle not be used!";
			Inner_Struct* pp=p_base+(i & PageMask);
			if(pp->flag==0) {++n;++i;continue;}//如果是已经释放的内存，跳过
			destructor(&(pp->data)); //遍历，析构对象
			++i;
		}
		clear(); //清除内存区域
	};
	//遍历函数，以谓词传入执行即可
	template<class Func>
	void for_each(Func func)
	{
		int i=0;
		int n=LastHandle;
		typename MemAlloctor::Inner_Struct* p_base;
		while(i<n)
		{
			p_base=p_table[i>>PageBits];
			if(p_base==NULL) throw "Handle not be used!";
			Inner_Struct* pp=p_base+(i & PageMask);
			if(pp->flag==0) //如果是已经释放的内存，跳过
			{
				++n;
				++i;
				continue;
			}
			func(&(pp->data)); //遍历，并传递有效对象
			++i;
		}
	}

	struct iterator
	{
		int handle;
		MemAlloctor *m_base;

		iterator(){}

		iterator(const iterator &it)
		{
			handle=it.handle;
			m_base=it.m_base;
		};

		iterator& operator++()
		{
			typename MemAlloctor::Inner_Struct* p_base;
			typename MemAlloctor::Inner_Struct* pp;

			do
			{
				++handle;
				p_base=m_base->p_table[handle>>PageBits];
				if(p_base==NULL) return *this;
				pp=p_base+(handle & m_base->PageMask);
			}
			while(handle<m_base->LastHandle && pp->flag==0); //如果是已经释放的内存，跳过
			return *this;
		}

		iterator& operator--()
		{

			typename MemAlloctor::Inner_Struct* p_base;
			typename MemAlloctor::Inner_Struct* pp;
			do
			{
				--handle;
				if(handle<0) return *this;
				p_base=m_base->p_table[handle>>PageBits];
				if(p_base==NULL) return;
				pp=p_base+(handle & m_base->PageMask);
			}
			while(pp->flag==0); //如果是已经释放的内存，跳过
			return *this;
		}
		bool operator==(iterator it)
		{
			return(m_base==it.m_base && handle==it.handle);
		}
		bool operator!=(iterator it)
		{
			return(m_base!=it.m_base || handle!=it.handle);
		}
		T& operator*()
		{
			typename MemAlloctor::Inner_Struct* p_base=m_base->p_table[handle>>PageBits];
			if(p_base==NULL) throw "Cann't read Memory";
			typename MemAlloctor::Inner_Struct* pp=p_base+(handle & m_base->PageMask);
			return pp->data;
		}
		T* operator->()
		{
			typename MemAlloctor::Inner_Struct* p_base=m_base->p_table[handle>>PageBits];
			if(p_base==NULL) throw "Cann't read Memory";
			typename MemAlloctor::Inner_Struct* pp=p_base+(handle & m_base->PageMask);
			return &(pp->data);

		}
	};

	//friend MemAlloctor::iterator;
	void erase(iterator& it)
	{
	
		free(it.handle);
	}

	iterator begin()
	{
		iterator rt;
		rt.handle=0;
		rt.m_base=this; 
		return rt;
	};

	iterator end()
	{
		iterator rt;
		rt.handle=LastHandle;
		rt.m_base=this; 
		return rt;
	};
};


template<class T,int size=16>
struct sdl_set  //这是一个std::set的替代品，仅仅用于数据集合较小的情况，效率应该比std::set好一些
{
	T table[size];
	T* p_table;
	int	pend;
	int curr_size;

	typedef	T* iterator;
	sdl_set(){pend=0;p_table=table;curr_size=size;};

	sdl_set(const sdl_set &s)
	{
		if(s.curr_size<=curr_size)
		{
			for(int i=0;i<s.pend;++i)
				p_table[i]=s.p_table[i];
			pend=s.pend;
		}
		else
		{
			T* p_old_table=p_table;
			p_table=new T[s.curr_size];
			if(p_table==NULL) throw "Memory alloc error";
			if(curr_size>size) delete[] p_old_table;
			curr_size=s.curr_size;
			for(int i=0;i<s.pend;++i)
				p_table[i]=s.p_table[i];
			pend=s.pend;
		}
	};

	~sdl_set(){if(curr_size!=size) delete[] p_table;};

	inline void resize()
	{

		if(curr_size==size)
		{
			p_table=new T[curr_size+size];
			if(p_table==NULL) throw "Memory alloc error";
			memcpy(p_table,table,size);
		}
		else
		{
			T* p_old_table=p_table;
			p_table=new T[curr_size+size];
			if(p_table==NULL) throw "Memory alloc error";
			memcpy(p_table,p_old_table,curr_size);
			delete[] p_old_table;
		}
		curr_size+=size;
	}

	inline iterator insert(T &val)
	{
		int	i,j;
		if(pend==curr_size-1) resize();
		for(i=0;i<pend;++i)
		{
			if(val<p_table[i]) break;
			else if(!(p_table[i]<val)) return end();
		}
		for(j=pend-1;j>=i;--j) p_table[j+1]=p_table[j];
		p_table[i]=val;
		++pend;
		return p_table+i;
	};

	inline iterator begin(){return	p_table;};
	inline iterator end(){return p_table+pend;};
	inline bool empty(){return	(pend==0);};
	inline void clear(){pend=0;};
	inline bool operator ==(sdl_set &s)
	{
		if(s.pend!=pend) return	false;
		for(int	i=0;i<pend;++i)
			if(s.p_table[i]!=p_table[i]) return	false;
		return true;
	};

	inline bool operator =(const sdl_set &s)
	{
		if(s.curr_size<=curr_size)
		{
			for(int i=0;i<s.pend;++i)
				p_table[i]=s.p_table[i];
			pend=s.pend;
		}
		else
		{
			T* p_old_table=p_table;
			p_table=new T[s.curr_size];
			if(p_table==NULL) throw "Memory alloc error";
			if(curr_size>size) delete[] p_old_table;
			curr_size=s.curr_size;
			for(int i=0;i<s.pend;++i)
				p_table[i]=s.p_table[i];
			pend=s.pend;
		}
	};

	int get_hashvalue()
	{
		int rt;
		if(pend<65536) rt=pend<<16;
		for(int	i=0;i<pend;++i)
			rt+=p_table[i];
		return rt;
	}
};

#endif
