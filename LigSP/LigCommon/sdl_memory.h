#ifndef __SDL_MEMEORY_MNG__
#define __SDL_MEMEORY_MNG__
//----------------------------------------------
//------------�ṩ�ֹ�����ͽ⹹��ģ�庯��-----
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
//ҳ��ʽ�ڴ��������PageNmuberΪ���ҳ��������PageBitsΪҳ���СȱʡΪ��2^10��
template<class T,int PageNumber,int PageBits=10> 
class PageMemory
{
private:
	T** p_table;  //������ָ��,ָ�����ʹ�õ�����ҳ����
	int PageSize; //�ڴ�ҳ��ߴ�
	int PageMask; //�ڴ�ҳ������
	int TopMem; //�������ڴ�

public:
	PageMemory()
	{
		typedef T* TT;
		PageSize=1;
		PageSize=PageSize<<PageBits; //�õ��ڴ�ҳ��Ĵ�С
		TopMem=PageSize*PageNumber;
		PageMask=PageSize-1; //�õ������ֵ�����ڿ��ټ���ƫ����
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
			p_base=(T*)(new char[sizeof(T)*PageSize]); //�������ڴ�ռ�
			p_table[loc>>PageBits]=p_base; //��������Ŀռ��¼��ҳ�������
		}
		return p_base[loc & PageMask];
	}

	inline T* operator()(int loc)
	{
		if(loc>=TopMem) throw "Defined memory too small";
		register T* p_base=p_table[loc>>PageBits];
		if(p_base==NULL)
		{
			p_base=(T*)(new char[sizeof(T)*PageSize]); //�������ڴ�ռ�
			p_table[loc>>PageBits]=p_base; //��������Ŀռ��¼��ҳ�������
		}
		return p_base+(loc & PageMask);
	}

	inline T* offset(T* pnt,int off) //�����Ƿ�ҳʽ�ڴ����Ϊ�˱����ҳʱ�Ĵ���ָ��ƫ����Ҫ�����������
	{
		register int i,offset;
		if(off==0) return pnt; //û��ƫ�ƵĻ����Ͳ���������Щ���Ӳ�����
		for(i=0;i<PageNumber;++i)
			if(p_table[i]!=NULL)
			{
				offset = (int)(pnt-p_table[i]);
				if(offset>=0)
				{
					if((offset+off)<PageSize) return pnt+off; //��һ���ڴ����
					if(offset<PageSize) //�ҵ���ʼ��ַ����ƫ�Ʋ���ͬһ�ڴ����
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
	char flag; // ���ڱ������ڴ���Ƿ�ʹ�ã�1Ϊ�Ѿ�ʹ�ã�0Ϊδʹ��
};

//ҳ��ʽ�ڴ���������PageNmuberΪ���ҳ��������PageBitsΪҳ���СȱʡΪ��2^10��
//����ڴ������֧���ڴ�صĻ������ԣ�֧���ͷ��ڴ�
template<class T,int PageNumber=256,int PageBits=10> 
class MemAlloctor
{
private:
	typedef Mem_Inner_Struct<T> Inner_Struct;
	Inner_Struct** p_table;  //������ָ��,ָ�����ʹ�õ�����ҳ����

	int PageSize; //�ڴ�ҳ��ߴ�
	int PageMask; //�ڴ�ҳ������
	int TopMem; //�������ڴ�
	int FreeLoc; //�������Ŀ��е�ַ
	int LastHandle;

	union inner_class //Ϊ�˿��ٷ����ڴ棬�����С���볬��int�Ĵ�С
	{
		char data[sizeof(Inner_Struct)];
		int loc;
	};

	//��ʼ���ڴ�����ʹ֮���Ա����������ʹ�ã�locֻ������һ�ε�λ�ã���Ҫ��ȷֵ
	inline void initMem(Inner_Struct*  pmem,int loc)
	{
		register int i;
		inner_class* pdata=(inner_class*)(pmem);

		loc=loc>>PageBits;loc=loc<<PageBits; //�õ�һ����ʼλ��
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
		if(sizeof(T)<sizeof(int)) //Ϊ�˿��ٷ����ڴ棬�����С���볬��int�Ĵ�С
			throw "MemoryMng Class object's size is too small";
		PageSize=1;
		PageSize=PageSize<<PageBits; //�õ��ڴ�ҳ��Ĵ�С
		TopMem=PageSize*PageNumber;
		PageMask=PageSize-1; //�õ������ֵ�����ڿ��ټ���ƫ����
		p_table = new TT[PageNumber];
		for(int i=0;i<PageNumber;++i) p_table[i]=NULL;
		FreeLoc=0;
		LastHandle=0;
	}

	inline int size()
	{
		return LastHandle;
	};

	inline T* alloc(int *phandle=NULL) //��Alloc��ʱ�򣬿�ʼʵ�ʷ����ڴ�
	{
		if(FreeLoc>=TopMem) throw "Defined memory too small";

		register Inner_Struct*  p_base=p_table[FreeLoc>>PageBits];
		if(p_base==NULL)
		{
			p_base=(Inner_Struct*)(new char[sizeof(Inner_Struct)*PageSize]); //�������ڴ�ռ�
			initMem(p_base,FreeLoc); //��ʼ���ڴ�ָ������
			p_table[FreeLoc>>PageBits]=p_base; //��������Ŀռ��¼��ҳ�������
		}
		p_base+=(FreeLoc & PageMask);
		if(phandle!=NULL) *phandle=FreeLoc;
		FreeLoc=((inner_class*)(p_base))->loc; //�ҵ���һ��δʹ�õ�����
		p_base->flag=1; //�������ڴ���Ѿ���ʹ��
		if(FreeLoc>LastHandle) LastHandle=FreeLoc; //�õ����ʹ�õ�һ��λ��
		return &(p_base->data);
	}

	//phandle�������ݻ��ѷ�������ڷ������е�����λ��
	inline T* create(int *phandle=NULL) //���������������������
	{
		T* rt=alloc(phandle);
		constructor(rt); //��������ڴ�����ִ�й���
		return rt;
	}

	//phandle�������ݻ��ѷ�������ڷ������е�����λ��
	template<class T2>
	inline T* make(T2& val,int *phandle=NULL) //�����������һ�������Ĺ���
	{
		int hd;
		T* rt=alloc(&hd);
		constructor(rt,val); //��������ڴ�����ִ�й���
		if(phandle!=NULL) *phandle=hd;
		return rt;
	}

	
	inline int get_handle(T* pnt) //��ȡһ������ָ���λ�ñ�ţ�������ڹ��������ڣ�������Чֵ-1
	{
		register int i,offset;
		for(i=0;i<PageNumber;++i)
			if(p_table[i]!=NULL)
			{
				offset = static_cast<int>((Inner_Struct*)(pnt) - p_table[i]);
				if(offset<PageSize && offset>=0) 
				{
					if(((Inner_Struct*)(pnt))->flag==0) return -1; //�˴����ݲ�δ�����룬���Է�����Чֵ
					return (i<<PageBits)+offset; 
				}
			}
		return -1;
	}

	inline void free(T* pnt) //�����Թ���Ķ�����Ч
	{
		register int i,offset;
		for(i=0;i<PageNumber;++i)
			if(p_table[i]!=NULL)
			{
				offset = static_cast<int>((Inner_Struct*)(pnt) - p_table[i]);
				if(offset<PageSize && offset>=0) 
				{
					if(((Inner_Struct*)(pnt))->flag==0) return;//������ظ�ɾ�����󣬴˴���Ⲣ����
					int preFreeLoc=FreeLoc; //����ԭ���Ŀ�������
					FreeLoc=(i<<PageBits)+offset; //���������Ÿ���Ϊ��ǰɾ����λ��

					destructor(pnt); //��������
					((inner_class*)(pnt))->loc=preFreeLoc;//����ԭ����λ��
					((Inner_Struct*)(pnt))->flag=0; 
					return; //���أ����ⲻ��Ҫ��ѭ��
				}
			}
	};

	inline void free(int loc) //�����Թ���Ķ�����Ч
	{
		if(loc>=TopMem || loc<0)  return;
		register Inner_Struct* p_base=p_table[loc>>PageBits];
		if(p_base==NULL) return;
		if(p_base[loc & PageMask].flag==0) return;//������ظ�ɾ�����󣬴˴���Ⲣ����;
		int preFreeLoc=FreeLoc; //����ԭ���Ŀ�������
		destructor(&((p_base+(loc & PageMask))->data)); //��������
		FreeLoc=loc; //���������Ÿ���Ϊ��ǰɾ����λ��
		((inner_class*)&(p_base[loc & PageMask].data))->loc=preFreeLoc;//����ԭ����λ��
		p_base[loc & PageMask].flag=0; //���ñ�־λ����������
	};

	inline void del(T* pnt) //����������ɾ��������ڴ����򣬽����Թ���Ķ�����Ч
	{
		register int i,offset;
		for(i=0;i<PageNumber;++i)
			if(p_table[i]!=NULL)
			{
				offset = static_cast<int>((Inner_Struct*)(pnt) - p_table[i]);
				if(offset<PageSize && offset>=0) 
				{
					if(((Inner_Struct*)(pnt))->flag==0) return;//������ظ�ɾ�����󣬴˴���Ⲣ����
					int preFreeLoc=FreeLoc; //����ԭ���Ŀ�������
					FreeLoc=(i<<PageBits)+offset; //���������Ÿ���Ϊ��ǰɾ����λ��
					((inner_class*)(pnt))->loc=preFreeLoc;//����ԭ����λ��
					((Inner_Struct*)(pnt))->flag=0; 
					return; //���أ����ⲻ��Ҫ��ѭ��
				}
			}
	};

	inline void del(int loc) //����������ɾ��������ڴ����򣬽����Թ���Ķ�����Ч
	{
		if(loc>=TopMem)  return;
		register Inner_Struct* p_base=p_table[loc>>PageBits];
		if(p_base==NULL) return;
		if(p_base[loc & PageMask].flag==0) return;//������ظ�ɾ�����󣬴˴���Ⲣ����;
		int preFreeLoc=FreeLoc; //����ԭ���Ŀ�������
		FreeLoc=loc; //���������Ÿ���Ϊ��ǰɾ����λ��
		((inner_class*)(p_base[loc & PageMask].data))->loc=preFreeLoc;//����ԭ����λ��
		p_base[loc & PageMask].flag=0; //���ñ�־λ����������
	};


	
	~MemAlloctor()
	{
		for(int i=0;i<PageNumber;++i) 
			if(p_table[i]!=NULL) delete[] (char*)(p_table[i]);
		delete[] p_table;
	}

	//���ݶ�������������ڴ���л��ָ���Ķ���
	inline T& operator[](int loc)
	{
		if(loc>=TopMem || loc<0) throw "Defined memory too small";
		register Inner_Struct* p_base=p_table[loc>>PageBits];
		if(p_base==NULL) throw "Handle not be used!";
		return p_base[loc & PageMask].data;
	}

	//���ݶ�������������ڴ���л��ָ���Ķ������ָ�����󲻴��ڣ��򷵻ؿ�ָ��
	inline T* operator()(int loc)
	{
		if(loc<0 || loc>=TopMem)  return NULL;
		register Inner_Struct* p_base=p_table[loc>>PageBits];
		if(p_base==NULL) return NULL;
		return &(p_base[loc & PageMask].data);
	}

	//����ڴ�����ʹ֮���Ա����������ʹ��
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

	//����ȫ�����󣬲�����ڴ�����ʹ֮���Ա����������ʹ��
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
			if(pp->flag==0) {++n;++i;continue;}//������Ѿ��ͷŵ��ڴ棬����
			destructor(&(pp->data)); //��������������
			++i;
		}
		clear(); //����ڴ�����
	};
	//������������ν�ʴ���ִ�м���
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
			if(pp->flag==0) //������Ѿ��ͷŵ��ڴ棬����
			{
				++n;
				++i;
				continue;
			}
			func(&(pp->data)); //��������������Ч����
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
			while(handle<m_base->LastHandle && pp->flag==0); //������Ѿ��ͷŵ��ڴ棬����
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
			while(pp->flag==0); //������Ѿ��ͷŵ��ڴ棬����
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
struct sdl_set  //����һ��std::set�����Ʒ�������������ݼ��Ͻ�С�������Ч��Ӧ�ñ�std::set��һЩ
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
