#ifndef __LIG_LL_PARSER_
#define __LIG_LL_PARSER_
#include <list>
#include <bitset>
#include <set>
#include <iostream>


#include "sdl_memory.h"
#include "sdl_signal.h"
/*
EBNF分析器体使用说明

   这是一个比较简单的EBNF语法分析器，分析器的速度自然不会有YACC等LR分析器快，
但这是一个足够使用的分析器，分析器使用LL(1)进行语法分析，由于其具备预测能力，
因此性能上至少与一般的LL(1)分析器类似。
   测试结果表明，一个带嵌套的简单语法结构下，处理10K左右的文本需要1.4M处理器
时钟周期，对于3.0G主频的CPU，大约是0.5毫秒，相当于每秒处理20M文本。

1：分析器的基本使用方法

	这是一个嵌入式的分析器，语法规则必须在代码中给出，而不是传统的YACC的预处理
器方式，语法规则在代码中必须以以下规则表示：

	如果使用ASCII字符集（其实目前也只支持ASCII)，建议直接使用预定义的三个对象 

	typedef sp_parser<char>::sp_node Node;
	typedef sp_parser<char>::sp_tree Item;
	typedef ScanItem<char> scan;

	Node 对应一个语法规则，Item 对应一个终结符集合，scan对应一个生成树对象

	语法规则使用C++重载的运算符来表述，其中 ‘<=’ 表述定义规则， '&' 表示连接
	‘|’ 表述可选的规则, 可以使用括号表明一个优先的语法组，但是括号必须使用
	(sub() ...)的方式，也可以定义一个新Node代替

	R0(),R1(),Rn()表示对象的重复方式，这与EBNF的定义一致，R0()表示对象可以不出现
	也可以重复无穷次，R1()表示对象必须出现一次，可以重复无穷次，Rn需要参数，给出
	对象至少出现多少次，最多重复多少次

	N0(),N1(),Nn()表示不满足对象匹配的字符的重复方式，N0()表示不匹配对象字符可以
	不出现也可以重复无穷次....

	每个Node可以定义一个回调，在进行语法匹配是，如果Node代表的规则被满足，将触发回调
	函数，回调函数需要定义在一个类中，并使用如下函数定义方式：

	void [函数名称](scan *it);
	
	Node中传入回调函数的方式为：

	[Node名称].connect(&[回调对象实例],&[回调对象类名]::[回调函数名称]);

	这个分析器原来打算将词法和语法分析器合二为一，但是在某些情况下，有词法分析会更方便（可以
	向前看n个字符），所以，这个分析器支持通过回调函数部分支持词法分析。


	在Item可以使用connect_escape函数设置一个“词法”分析函数，这个函数具备
	char function_name(const char *&pstr)的形式，在匹配Item的时候，会调用这个函数，并将当前的
	字符流指针传递到这个函数中，函数可以向前看n个字符，选择忽略（更改传入的字符流指针）或者不忽略
	（不更改字符流指针），然后返回一个字符交由Item去匹配。

	函数形式为 connect_escape('&',this,&SquirrelParser::lex_and); 
	connect_escape的第一个参数为起始字符，必须探测到这个起始字符才会开始调用函数，第二个参数为回调函数类指针
	，第三个参数为回调函数。

*/

#define JSON_FALSE -1
#define JSON_TRUE   1
#define JSON_NULL   2
#define JSON_OBJECT 3
#define JSON_STRING 4
#define JSON_ARRAY  5
#define JSON_INTEGER 6
#define JSON_FLOAT   7
#define JSON_HEX   8

using namespace std;

#define MaxChar 256
namespace lig
{
#ifdef _LIG_DEBUG
	static int perf_counter=0; //调试用的计数器
	static int stack_counter=0; //调试用堆栈指示变量
#endif

#ifndef INFINITY
#define INFINITY 0x7FFFFFFF
#endif
	
	template<class Char>
	struct ScanItem //语法生成树对象，用于减少反复扫描的堆栈对象
	{
		const Char *start; //进行匹配的字符流位置
		const Char *end; //进行匹配的字符流位置
		const Char *name; //被分析项的名称

		int matched_handle; //被匹配对象的handle
		int token_handle; //类似于词法匹配的词法handle,可用于符号表处理
		int controled_size; //这个对象块控制到的对象长度


		ScanItem *pFather;
		ScanItem *pChild;
		ScanItem *pBrother;
		ScanItem **ppLastChild; //为了提高效率，增加此项，记录最后一个“孩子”的位置
								//如果*ppLastChild==NULL,表示只有一个孩子
		inline void clear()
		{
			pChild=NULL;
			pBrother=NULL;
			pFather=NULL;
			end=NULL;
			start=NULL;
			ppLastChild=(&pChild);
			name=NULL;
			controled_size=0;
			token_handle=0;
		}

		inline ScanItem* get_lastchild()
		{
			ScanItem* rt=pChild;
			ScanItem* it=pChild;
			while(it!=NULL)
			{
				rt=it;
				it=it->pBrother; 
			}
			return rt;
		}

		inline void add_child(ScanItem *pC) //总是向前插入对象
		{
			if(pChild==NULL) 
			{
				pChild=pC;
				ppLastChild=&(pChild->pBrother); 
				pC->pFather=this; 
			}
			else 
			{
				*ppLastChild=pC;
				ppLastChild=&(pC->pBrother); 
				pC->pFather=this; 
			}
			
		}

		int backto(const Char *pstr) //将扫描记录回退到字符流指针位置
		{
			int control_block=0;
			ScanItem* pf=pChild;
			ScanItem* bk;
			while(pf!=NULL)
			{
				if(pf->start == pstr)
				{
					bk=pf;
					while(pf->pBrother!=NULL)
					{
						control_block+=pf->pBrother->controled_size;
						pf=pf->pBrother;
					}
					bk->pBrother = NULL;
					return control_block;
				}
				pf=pf->pBrother;
			}
			return 0;
		}

		inline basic_string<Char> str() //将控制的字符序列转换为标准字符串
		{
			return basic_string<Char>(start,end);
		};

		//----------------------------------------------------
		inline bool match_child_handle(int handle) //判断寻找子代中是否有与指定句柄匹配的对象
		{
			ScanItem* pf=pChild;
			while(pf!=NULL)
			{
				if(pf->matched_handle==handle) return true; 
				pf=pf->pChild;
			}
			return false;
		}
		//--------------------------------------------------
		ScanItem* get_handle(int handle) //找子代中是否有与指定句柄匹配的对象
		{
			ScanItem* pf=pChild;
			while(pf!=NULL)
			{
				if(pf->matched_handle==handle) return pf; 
				pf=pf->pChild;
			}
			return NULL;
		}

		ScanItem* get(int loc=0) //在子代中寻找第loc个对象
		{
			ScanItem* rt=pChild;
			if(loc==0)	return rt;
			while(rt!=NULL && loc>0)
			{
				rt=rt->pBrother;
				--loc;
			}
			return rt;
		}

		void offset_children_str(int offset)
		{
			start+=offset;
			end+=offset;
			ScanItem* rt=pChild;
			while(rt!=NULL)
			{
				rt->offset_children_str(offset);
				rt=rt->pBrother; 
			}
		}

		int size()
		{
			int st=0;
			ScanItem* rt=pChild;
			while(rt!=NULL)
			{
				st++;
				rt=rt->pBrother; 
			}
			return st;
		}

		ScanItem* get(const Char *name) //在子代中寻找请求名字的对象
		{
			ScanItem* rt=pChild;
			if(*name==0) return this; //空串，返回自身
			if(strcmp(name,rt->name)==0)
				return rt;
			while(rt!=NULL)
			{
				rt=rt->pBrother;
				if(rt==NULL) return NULL;
				if(strcmp(name,rt->name)==0) return rt;
			}
			throw "Cann't find object";
			return NULL;
		}

		//在子代中寻找名字为name1，在name1子代（孙代）中寻找名字为name2的对象
		ScanItem* get(const Char *name1,const Char *name2) 
		{
			ScanItem* rt=pChild;
			if(strcmp(name1,rt->name)==0) return rt->get(name2);
			while(rt!=NULL)
			{
				rt=rt->pBrother;
				if(rt==NULL) return NULL;
				if(strcmp(name1,rt->name)==0) return rt->get(name2);
			}
			throw "Cann't find object";
			return NULL;
		}
		//在后续同代中寻找名字为name1的对象
		ScanItem* next(const Char *name)
		{
			ScanItem* rt=pBrother;
			while(rt!=NULL && strcmp(name,rt->name)!=0) rt=rt->pBrother; 
			return rt;
			
		}
		//在后续同代中寻找handle对象
		ScanItem* next_handle(int handle)
		{
			ScanItem* rt=pBrother;
			while(rt!=NULL && rt->matched_handle!=handle) rt=rt->pBrother; 
			return rt;
			
		}
		//-----------------------------------------------------------
		inline bool is_str(const Char* str) //判定控制的字符串是否是指定的字符串
		{
			const Char* p=start;
			while(p!=end)
			{
				if(*p!=*str) return false;
				++p;++str;
				if(*str==0 && p==end) return true;
			}
			return false;
		}

		inline bool mystrcmp(const Char* str,const Char* strEnd=NULL) //判定控制的字符串是否是指定的字符串
		{
			const Char* p=start;
			while(p!=end)
			{
				if(*p!=*str) return false;
				++p;++str;
				if(*str==0 || str==strEnd)
				{
					if(p==end) return true;
					return false;
				}
			}
			return false;
		}

		inline bool mystrcmp(const Char* str,int size) //判定控制的字符串是否是指定的字符串
		{
			int ct=0;
			const Char* p=start;
			while(p!=end)
			{
				if(*p!=*str) return false;
				++p;++str;
				if((++ct)>=size) return true;
			}
			return false;
		}

		inline bool cstr2str(char* buf,int &bufSize)
		{
			int ct=0;
			Char ch,cc;
			const Char* p=start;
			if(*p!='\"') return false;
			while(*(++p)!='\"')
			{
				if(*p=='\\')
				{
					ch=*(p++);
					switch(ch)
					{
					case 'r':ch='\r';break;
					case 'n':ch='\n';break;
					case 't':ch='\t';break;
					case 'a':ch='\a';break;
					case 'b':ch='\b';break;
					case 'f':ch='\f';break;
					case 'v':ch='\v';break;
					case '?':ch='\?';break;
					case '\"':ch='\"';break;
					case '\'':ch='\'';break;
					case '\\':ch='\\';break;
					
					case 'x':
						ch=*(p++);
						ch=(ch>='0' && ch<='9')?(ch-'0')*16:((ch>='A' && ch<='F')?(ch-'A'+10)*16:((ch>='a' && ch<='f')?(ch-'a'+10)*16:-1));;
						if(ch<0) return false;
						cc=ch;
						ch=*(p++);
						ch=(ch>='0' && ch<='9')?(ch-'0'):((ch>='A' && ch<='F')?(ch-'A'+10):((ch>='a' && ch<='f')?(ch-'a'+10):0));
						cc+=ch;
						if(ch<0) return false;
						ch=cc;;
						break;
					default:
						ch=*(p++);
						if(ch<'0' || ch>'7') return false;
						cc=(ch-'0')*64;
						ch=*(p++);
						if(ch<'0' || ch>'7') return false;
						cc+=(ch-'0')*8;
						ch=*(p++);
						if(ch<'0' || ch>'7') return false;
						cc+=(ch-'0');
						ch=cc;
						break;
					}
					buf[ct]=ch;
				}
				else buf[ct]=*p;
				if(++ct>=bufSize) return false;
			}
			buf[ct]=0;
			bufSize=ct;
			return true;
		}

		inline int cstrcmp(const Char* str,int size=-1) //判定控制的字符串(cstr编码方式)是否是指定的字符串
		{
			int ct=0;
			int rt=0;
			Char ch,cc;
			const Char* p=start;
			while(*(++p)!='\"' && *str!=0)
			{
				if(*p=='\\')
				{
					ch=*(p++);
					switch(ch)
					{
					case 'r':ch='\r';break;
					case 'n':ch='\n';break;
					case 't':ch='\t';break;
					case 'a':ch='\a';break;
					case 'b':ch='\b';break;
					case 'f':ch='\f';break;
					case 'v':ch='\v';break;
					case '?':ch='\?';break;
					case '\"':ch='\"';break;
					case '\'':ch='\'';break;
					case '\\':ch='\\';break;
					
					case 'x':
						ch=*(p++);
						ch=(ch>='0' && ch<='9')?(ch-'0')*16:((ch>='A' && ch<='F')?(ch-'A'+10)*16:((ch>='a' && ch<='f')?(ch-'a'+10)*16:-1));;
						if(ch<0) return -1;
						cc=ch;
						ch=*(p++);
						ch=(ch>='0' && ch<='9')?(ch-'0'):((ch>='A' && ch<='F')?(ch-'A'+10):((ch>='a' && ch<='f')?(ch-'a'+10):0));
						cc+=ch;
						if(ch<0) return -1;
						ch=cc;;
						break;
					default:
						ch=*(p++);
						if(ch<'0' || ch>'7') return -1;
						cc=(ch-'0')*64;
						ch=*(p++);
						if(ch<'0' || ch>'7') return -1;
						cc+=(ch-'0')*8;
						ch=*(p++);
						if(ch<'0' || ch>'7') return -1;
						cc+=(ch-'0');
						ch=cc;
						break;
					}
				}
				else ch=*p;
				rt+=ch-*str;
				if(size>0 && (++ct)>=size) return rt;
				if(rt!=0) return rt;
				++str;
			}
			rt=*p- '\"'- *str;
			return rt;
		}

		bool operator==(const Char* str){return is_str(str);} //用于字符串比较的，重载的运算符号

		bool operator!=(const Char* str){return !(is_str(str));} //用于字符串比较的，重载的运算符号

		//----------------------------------------------------------
		int to_int()
		{
			return my_atol(start,end) ;
		}
		int to_hex()
		{
			int hex=0;
			const char* pHex=start+2;
			while(pHex<end)
			{
				int ch=*pHex;
				ch=(ch>='0' && ch<='9')?(ch-'0'):((ch>='A' && ch<='F')?(ch-'A'+10):((ch>='a' && ch<='f')?(ch-'a'+10):0));
				hex=(hex*16)+ch;
				++pHex;
			}
			return hex;
		}
		const char* to_str()
		{
			int len=end-start;
			cstr2str((char*)start,len) ;
			return start;
				
		}
		double to_double()
		{
			char* the_end=const_cast<char*>(end);
			return my_strtod(start,&the_end);
		}

		static int my_atol(const char *str,const char* endptr) //来源于CSDN yinqing_yx老兄
		{
			char c = *str;
			if( !isdigit(c) ) str++;
			int value=0;
			for(;str<endptr;str++)
			{
				if( !isdigit(*str) ) break;
				value = value * 10 + (*str -'0');
			}
			return c == '-' ? -value : value ;
		};

		static double my_strtod(const char* s, char** endptr) //来源于CSDN一段代码，在此谢谢
		{
			register const char*  p     = s;
			register long double  value = 0.L;
			int                   sign  = 0;
			long double           factor;
			unsigned int          expo;

			while(isspace(*p) ) p++;//跳过前面的空格
			if(*p == '-' || *p == '+')	sign = *p++;//把符号赋给字符sign，指针后移。
			//处理数字字符
			while ( (unsigned int)(*p - '0') < 10u ) value = value*10 + (*p++ - '0');//转换整数部分
			//如果是正常的表示方式（如：1234.5678）
			if ( *p == '.' )
			{
				factor = 1.0;p++;
				while ( (unsigned int)(*p - '0') < 10u )
				{
					factor *= 0.1;
					value  += (*p++ - '0') * factor;
				}
			}
			//如果是IEEE754标准的格式（如：1.23456E+3）
			if ( (*p | 32) == 'e' )
			{
				expo   = 0;
				factor = 10.L;
				switch (*++p)
				{
				case '-':
					factor = 0.1;
				case '+':
					p++;break;
				case '0':	case '1':	case '2':	case '3':
				case '4':	case '5':	case '6':	case '7':
				case '8':	case '9':	break;
				default :	value = 0.L;p = s;goto done;
				}
				while((unsigned int)(*p - '0') < 10u ) expo = 10 * expo + (*p++ - '0');
				while (true)
				{
					if ( expo & 1 )	value *= factor;
					if ( (expo >>= 1) == 0 ) break;
					factor *= factor;
				}
			}
done:
			if (endptr != 0 ) *endptr = (char*)p;
			return (sign == '-' ? -value : value);
		}
	};

	

	template<class Tp>
	struct sp_warp
	{
		int min;
		int max;
		bool mmode;
		bool flag_search;
		int charValue;
		Tp *pObj;
	};


#ifdef _LIG_DEBUG
	static const char *start_str;
#endif
	

	template<class Char>
	struct sp_parser
	{
		typedef ScanItem<Char> scan;
		typedef bitset<MaxChar> CharMask;

		class sp_tree;
		class sp_node;

		typedef  sp_tree Leaf;
		typedef  sp_node Node;

		struct sp_closed {};//一个特殊对象，用于表示分析到这里成功返回
	
		typedef MemAlloctor<scan,256,12> ScanMemAlloctor;  //按页管理的内存区域 
		
		
		static Leaf* create_last_child(Leaf* parent) //从一个节点派生出一个子节点，而且总是最后一个子节点
		{
			Leaf* newItem = new sp_tree;

			newItem->pBrother = NULL;
			newItem->pChild = NULL;
			newItem->pFather = NULL;

			if(parent->pChild == NULL)
			{
				parent->pChild = newItem;
				newItem->pFather = parent;
				newItem->pBrother = NULL;
			}
			else
			{
				Leaf* pp = parent->pChild;
				while(pp->pBrother != NULL) pp = pp->pBrother;
				pp->pBrother = newItem;
				newItem->pFather = parent;
				newItem->pBrother = NULL;
			}
			return newItem;
		}

		static Leaf* create_first_child(Leaf* parent) //从一个节点派生出一个子节点，而且总是第一个子节点
		{
			Leaf* newItem = new sp_tree;

			newItem->pBrother = NULL;
			newItem->pChild = NULL;
			newItem->pFather = NULL;

			if(parent->pChild == NULL)
			{
				parent->pChild = newItem;
				newItem->pFather = parent;
				newItem->pBrother = NULL;
			}
			else
			{
				Leaf* pp = parent->pChild;
				parent->pChild = newItem;
				newItem->pFather = parent;
				newItem->pBrother = pp;
			}
			return newItem;
		}

		static Leaf* insert_brother(Leaf* brother) //从一个节点插入一个兄弟节点，它总在当前节点之后
		{
			Leaf* newItem = new sp_tree;

			newItem->pBrother = NULL;
			newItem->pChild = NULL;
			newItem->pFather = NULL;

			if(brother->pBrother == NULL)
			{
				brother->pBrother = newItem;
				newItem->pFather = brother->pFather;
				newItem->pBrother = NULL;
			}
			else
			{
				Leaf* pp = brother->pBrother;
				brother->pBrother = newItem;
				newItem->pFather = brother->pFather;
				newItem->pBrother = pp;
			}
			return newItem;
		}

		class sp_tree //这是一个简易的单向指针树结构，专门用于这个分析器，效率第一啊
		{
		public:
			typedef ScanItem<Char> scan;

			int min_repeat; //最低匹配次数，0就算epsilon匹配，如果为-1表明此结点接受不匹配的字符，直到匹配为止
			int max_repeat; //最大匹配次数，0x7fffffff算是无穷次匹配
			//----------------------------
			CharMask char_mask; //用于识别字符对象的“阵列”
			const char *node_name;  //节点名称
			//----------------------------
			Char     escape_char; //转义符号,在遇到转义符号,需要执行一个回调函数,判断转义符号是否符合要求
			sdl_rt_callbase<const Char*&,Char> *escape_callback; //转义符号的回调函数
		template<typename Ts>
			void connect_escape(Char escapChar,Ts *pobj, Char (Ts::*f)(const Char*& arg)) //回调支持函数
			{
				escape_char = escapChar;
				char_mask[escapChar] = 1;
				if(escape_callback!=NULL) delete escape_callback; //删除不使用的回调
				escape_callback = new sdl_rt_function<Ts,const Char*&,Char>(pobj,f); //每次只能连接一个回调
			};
			void connect_escape(Char escapChar,Char (*f)(const Char*&)) //回调支持函数
			{
				escape_char = escapChar;
				char_mask[escapChar] = 1;
				if(escape_callback!=NULL) delete escape_callback; //删除不使用的回调
				escape_callback = new sdl_rt_callbase<const Char*&,Char>; //每次只能连接一个回调
				escape_callback->c_callback=f;
			};

			//----------------------------
			bool flag_match_mode; //用于表示匹配模式 true表示正常匹配，false表示“非”匹配
		
			int token_handle;  //用于记录符号表编码
			int handle; //节点标识
			//---------------------------------------------
			sp_node *info; //如果 info==NULL，这是一个普通节点 ，否则指向一个对象
			int required_info_handle; //如果不为-1，则要求info匹配的token_handle与这个值相等，否则认为匹配失败
			//---------------------------------------------
			sp_tree *pFather; //指向父节点
			sp_tree *pChild;  //指向子节点
			sp_tree *pBrother; //指向下一个兄弟节点，如果这个指针为空，表明已经没有兄弟节点了
			//--------------------------------------------
			
			//-------------------------------------------
			inline void set_max(int m=INFINITY){max_repeat=m;}
			inline void set_min(int m=0){min_repeat=m;}
			inline void operator()(int min=0,int max=INFINITY){min_repeat=min;max_repeat=max;}
			inline void set_except_match(){flag_match_mode=false;}; //设置为“除非...”匹配模式
			//--------------------------------------------

			
#ifdef _LIG_DEBUG
			void debug_print()
			{
				int ich=0x20;
				int nch=0x7f;
				while(ich<=nch)
				{
					if(this->char_mask[ich]) 
						DOUT<<(char)(ich);
					++ich;
				}
				DOUT<<endl;
			}
#endif		
			inline void range(Char b,Char s)
			{
				min_repeat=1;
				max_repeat=1;
				int i;
				for(i=(unsigned char)(b);i<=(unsigned char)(s);++i)
					char_mask[i]=true;
			}

			inline void not_set()
			{
				char_mask=~char_mask;
			}

		
			inline void char_set(const Char* cset)
			{
				const Char* pp=cset;
				min_repeat=1;
				max_repeat=1;
				while(*pp!=Char(0)) {char_mask[(unsigned char)(*pp)]=true;++pp;}
			}

			inline void char_set(const Char c)
			{
				min_repeat=1;
				max_repeat=1;
				char_mask[(unsigned char)(c)]=true;
			}

			inline Leaf& operator+=(Leaf &st)
			{
				char_mask|=st.char_mask;
				return *this;
			}

			bool is_node()
			{
				return false;
			}

			sp_tree()
			{
				pBrother = NULL;
				pChild = NULL;
				pFather = NULL;
				info = NULL;
				required_info_handle=-1;
				token_handle = -1;
				handle=-1;
				min_repeat=1;
				max_repeat=1;
				flag_match_mode = true;  //设置为正常匹配模式
				escape_callback = NULL;  //转义符号的回调函数置为NULL
				node_name=NULL; //缺省没有名称

			}

			sp_tree(Char ch)
			{
				pBrother = NULL;
				pChild = NULL;
				pFather = NULL;
				info = NULL;
				required_info_handle=-1;
				token_handle = -1;
				handle=-1;
				min_repeat=1;
				max_repeat=1;
				flag_match_mode = true;  //设置为正常匹配模式
				escape_callback = NULL;  //转义符号的回调函数置为NULL
				node_name=NULL; //缺省没有名称
				min_repeat=1;
				max_repeat=1;
				char_mask[ch] = true;
			}

			sp_tree(const Char *str)
			{
				pBrother = NULL;
				pChild = NULL;
				pFather = NULL;
				info = NULL;
				required_info_handle=-1;
				token_handle = -1;
				handle=-1;
				min_repeat=1;
				max_repeat=1;
				flag_match_mode = true;  //设置为正常匹配模式
				escape_callback = NULL;  //转义符号的回调函数置为NULL
				node_name=NULL; //缺省没有名称
				min_repeat=1;
				max_repeat=1;
				while(*str!=0)
				{
					char_mask[*str] = true;
					++str;
				}
			}


			sp_tree(const sp_tree &tr)
			{
				pBrother = tr.pBrother ;
				pChild = tr.pChild ;
				pFather = tr.pFather;
				info = tr.info; 
				required_info_handle = tr.required_info_handle; 
				token_handle = tr.token_handle;
				handle=tr.handle;

				min_repeat=tr.min_repeat;
				max_repeat=tr.max_repeat;
				flag_match_mode = tr.flag_match_mode;  //设置为正常匹配模式
				
				char_mask = tr.char_mask; 
			};

			~sp_tree(){}


			void delete_children() //删除子树
			{
				Leaf *it,*mt;
				if((it=pChild)==NULL) return;
				while(it!=NULL)
				{
					mt=it;
					it->delete_children();
					it=it->pBrother;
					if(mt!=NULL) delete mt;
				}
				pChild=NULL;
			};

			void delete_tree() //删除树结构
			{
				delete_children(); //删除全部子树
				if(pFather!=NULL) //不是root
				{
					Leaf *it=pFather->pChild;
					if(it==this) 
					{
						pFather->pChild=pBrother;
						return;
					}
					while(it!=NULL)
					{
						if(it->pBrother==this) 
						{
							it->pBrother=pBrother;
							delete this;
							return;
						}
						it=it->pBrother; 
					}
				}
				throw "Leaf* Structure Error!";

			};

			Leaf* clone(Leaf* father=NULL) //克隆一个树结构
			{
				Leaf *it,*mt,*first;
				if((it=pChild)==NULL) 
				{
					mt=new sp_tree;
					mt->pFather=father; 
					mt->info = info; 
					mt->token_handle = token_handle;
					return mt;
				}
				first=new sp_tree;
				first->info = info; 
				first->token_handle = token_handle;
				first->pChild = NULL; 
				Leaf *pre=NULL;
				while(it!=NULL)
				{
					mt=it->clone(first);
					if(first->pChild==NULL)
						first->pChild=mt;
					mt->pFather=first; 
					if(pre!=NULL) pre->pBrother=mt; 
					pre=mt;
					it=it->pBrother;
				}
				return first;
			}

		protected:


			bool is_include(Leaf* pLeaf) //测试一个节点是否已经包含指定节点的字符映射
			{
				bitset<MaxChar> tmp=char_mask;
				tmp|=pLeaf->char_mask;
				return (tmp==char_mask);
			}

			bool is_conflict(Leaf* pLeaf) //测试字符集是否产生冲突
			{
				const static bitset<MaxChar> zeroValue;
				bitset<MaxChar> tmp=char_mask;
				tmp&=pLeaf->char_mask;
				return (tmp!=zeroValue);
			}


		public:

			bool is_single_child() //测试本节点是否包含一个唯一的子节点
			{
				if(pChild==NULL) return false;
				if(pChild->pBrother!=NULL) return false;
				return true;
			}
			bool is_same(Leaf* pLeaf,int tMin,int tMax,bool mt) //测试两个节点是否同构
			{
				return (info==NULL && pLeaf->info==NULL && (tMin==-1)?(min_repeat==pLeaf->min_repeat):(min_repeat==tMin) &&
					(tMax==-1)?(max_repeat==pLeaf->max_repeat):(max_repeat==tMax) &&
					(tMax==-1)?(flag_match_mode==pLeaf->flag_match_mode):(mt== flag_match_mode) && 
					char_mask==pLeaf->char_mask &&
					escape_callback==pLeaf->escape_callback && 
					escape_char==pLeaf->escape_char);
			}

			static int string_match(sp_tree* pMe,const Char* &pstr) //进行简单字符串匹配，相当与正规表达式的能力
			{
				if(*pstr==0)
				{
					sp_tree* pT=pMe->pChild;
					while(pT!=NULL && pT->token_handle==-1) pT=pT->pBrother; 
					if(pT!=NULL)
						return pT->token_handle;
					return -1;
				}
				sp_tree* pC=pMe->pChild;
				while(pC!=NULL)
				{
					if(pC->char_mask[*pstr]) 
						return string_match(pC,++pstr);
					pC=pC->pBrother; 
				}
				return -1;
			};

			int token_match(const Char* pstr)
			{
				if(!char_mask[*pstr]) return -1;
				return string_match(this,pstr);
			}

			Leaf& operator<=(Char ch)
			{
				min_repeat=1;
				max_repeat=1;
				char_mask[ch] = true;
				return *this;
			}

			Leaf& operator <= (Leaf& nd)
			{
				min_repeat=1;
				max_repeat=1;
				char_mask |= nd.char_mask;
				return *this;
			}

			Leaf& operator <= (sp_warp<Leaf> wd)
			{
				min_repeat=wd.min_repeat;
				max_repeat=wd.max_repaet;
				char_mask |= wd.char_mask;
				return *this;
			}

			Leaf& operator | (Char ch)
			{
				min_repeat=1;
				max_repeat=1;
				char_mask[ch] = true;
				return *this;
			}

			Leaf& operator | (Leaf& nd)
			{
				min_repeat=1;
				max_repeat=1;
				char_mask |= nd.char_mask;
				return *this;
			}

		};

		static void add_leaf(Leaf* &m_curr,Leaf *plf,int min,int max,bool mt)
		{
			Leaf* pnext=m_curr->pChild;
			while(pnext!=NULL)
			{
				if(pnext->is_same(plf,min,max,mt)) //找到同构节点
				{
					m_curr=pnext;
					break;
				}
				else
					pnext=pnext->pBrother; 
			}
			if(pnext==NULL) //创建新的
			{
				pnext=create_last_child(m_curr); //创建了一个新节点
				pnext->min_repeat=min;
				pnext->max_repeat=max;
				pnext->flag_match_mode=true;
				pnext->escape_callback=plf->escape_callback;
				pnext->escape_char=plf->escape_char;  
				if(mt) pnext->char_mask=plf->char_mask;
				else  pnext->char_mask=~(plf->char_mask);
				pnext->info=NULL;
				m_curr=pnext;
			}

		};

		static void add_char(Leaf* &m_curr,Char ch,int min,int max,bool mt)
		{
			Leaf* pnext=m_curr->pChild;
		
			while(pnext!=NULL)
			{
				if(mt && pnext->char_mask[ch]) //找到相同的节点
				{
					m_curr=pnext;
					break;
				}
				pnext=pnext->pBrother; 
			}
			if(pnext==NULL) //此字符目前还不存在，因此要创建新的
			{
				pnext=create_last_child(m_curr); //创建了一个新节点
				pnext->char_mask[ch]=true;
				pnext->min_repeat=min;
				pnext->max_repeat=max;
				pnext->flag_match_mode=true;
				
				if(!mt) pnext->char_mask=~(pnext->char_mask);
				m_curr=pnext;
			}
		}


		static void add_str(Leaf* &m_curr,const Char *str)
		{
			Leaf* pnext=m_curr;
			Leaf* pup=m_curr;

			const Char *pstr=str;

			if(m_curr->pFather==NULL) //如果是起始节点，需要加入First 
			{
				m_curr->char_mask[*str]=true; 

			}
			else
			{
				pup=m_curr->pFather->pChild;
				while(pup!=NULL && pup->min_repeat==0)
				{
					if(pup==m_curr)
					{
						m_curr->pFather->char_mask[*str]=true; 
						break;
					}
					pup=pup->pBrother; 
				}
			}
			pup=m_curr;

			while(*pstr!=Char(0))
			{
				pup=pnext; //保留当前值
				pnext=pnext->pChild;
				while(pnext!=NULL) //查找是否已经存在同样的字符串
				{
					if(pnext->char_mask[*pstr]) 
						break;
					else
						pnext=pnext->pBrother; 
				}
				if(pnext==NULL) //此字符目前还不存在，因此要创建新的
				{
					pnext=create_first_child(pup); //创建了一个新节点
					pnext->char_mask[*pstr]=1;
					pnext->flag_match_mode=true;
				}
				++pstr; 
			}
			pnext=create_last_child(pnext);
			pnext->char_mask=0; 
			pnext->max_repeat=0; //增加一个终结符号 
			m_curr=pnext;
		};
	
		static bool add_keystr(Leaf* &m_curr,const Char *str,int tokenHandle,CharMask &TermCharSet)
		{
			return add_keystr(m_curr,str,NULL,tokenHandle,TermCharSet);
		};

		static bool add_keystr(Leaf* &m_curr,const Char *str,const Char *str_end,int tokenHandle,CharMask &TermCharSet)
		{
			Leaf* pnext=m_curr;
			Leaf* pup=m_curr;

			const Char *pstr=str;

			if(m_curr->pFather==NULL) //如果是起始节点，需要加入First 
			{
				m_curr->char_mask[*str]=true; 
			}
			else
			{
				pup=m_curr->pFather->pChild;
				while(pup!=NULL && pup->min_repeat==0)
				{
					if(pup==m_curr)
					{
						m_curr->pFather->char_mask[*str]=true; 
						break;
					}
					pup=pup->pBrother; 
				}
			}
			pup=m_curr;

			while(*pstr!=Char(0) && pstr!=str_end)
			{
				pup=pnext; //保留当前值
				pnext=pnext->pChild;
				while(pnext!=NULL) //查找是否已经存在同样的字符串
				{
					if(pnext->char_mask[*pstr]) 
						break;
					else
						pnext=pnext->pBrother; 
				}
				if(pnext==NULL) //此字符目前还不存在，因此要创建新的
				{
					pnext=create_first_child(pup); //创建了一个新节点
					pnext->char_mask[*pstr]=1;
					pnext->flag_match_mode=true;
				}
				++pstr; 
			}
			pnext=create_last_child(pnext);
			pnext->char_mask=TermCharSet;
			pnext->token_handle=tokenHandle;
			pnext->max_repeat=1; //增加一个终结符号 
			m_curr=pnext;
			return true;
		};

		struct StrPath
		{
			Leaf* pLeaf;
			bool flag_single;
		};

		static bool delete_keystr(Leaf* &m_curr,const Char *strStart,const Char* strEnd)
		{
			Leaf* pnext=m_curr;
			Leaf* pup=m_curr;

			const Char* pstr=strStart;
			StrPath tmp;

			list<StrPath> StrPathList;

			while(pstr!=strEnd)
			{
				pup=pnext; //保留当前值
				pnext=pnext->pChild;
				tmp.flag_single=true;
				while(pnext!=NULL)
				{
					if(pnext->char_mask[*pstr]) 
					{
						tmp.pLeaf=pnext;
						tmp.flag_single=pnext->is_single_child(); //测试节点后续是否为单一后续 
						StrPathList.push_back(tmp); 
						break;
					}
					pnext=pnext->pBrother; 
				}

				if(pnext==NULL) return false ;//此字符目前还不存在，因此无法删除字符串
				++pstr; 
			}

			if(StrPathList.empty()) return false;

			typename list<StrPath>::reverse_iterator rit;
			rit=StrPathList.rbegin();
			while(rit!=StrPathList.rend() && rit->flag_single) ++rit;
			if(rit!=StrPathList.rend())
			{
				rit->pLeaf->delete_tree();
			}
			else
			{
				m_curr->char_mask&=(~StrPathList.front().pLeaf->char_mask);
				StrPathList.front().pLeaf->delete_tree();
			}
			return true;
		};
		//------------------------------------------------------------------------
	
		class sp_node;

		

		static list<sp_tree*>& dbs() //记录所有有效的节点
		{
			static list<sp_tree*> mdbs;
			return mdbs;
		}

		

		static void redo_charmask(Leaf* plf,CharMask &chmask) //递归算法，寻找可能对需要更新节点的影响
		{
			
			Leaf* pb=plf->pChild ; //找到第一个子节点
			if(pb==NULL) return;
			do{
				if(pb->flag_match_mode) //判定只节点的属性
				{
					if(pb->info==NULL)
					{
						chmask|=pb->char_mask; //不是node，将first set写入即可
					}
					else
					{
						pb->char_mask=pb->info->char_mask;  
						chmask|=pb->info->char_mask;
					}
					if(pb->min_repeat==0)  redo_charmask(pb,chmask); //有epsilon，递归继续
				}
				else
				{
					if(pb->info==NULL)
					{
						chmask|=~(pb->char_mask);
					}
					else
					{
						pb->char_mask=~(pb->info->char_mask);  
						chmask|=~(pb->info->char_mask);
					}

					if(pb->min_repeat==0)  redo_charmask(pb,chmask);
				}
				pb=pb->pBrother;
			}while(pb!=NULL); //在兄弟节点中遍历
		};

		static void recompute_tree(Leaf* plf)
		{
			Leaf* pb=plf->pChild;
			if(pb==NULL) return;
			do
			{
				recompute_tree(pb); //递归遍历，至所有子节点
				//由于递归，这里是最底层的节点开始出现
				if(pb->is_node()) throw "Rule error"; //规则不能出现在节点内
				if(pb->info!=NULL)  //携带规则的子节点
					pb->char_mask=pb->flag_match_mode?pb->info->char_mask:~(pb->info->char_mask);   
			}while((pb=pb->pBrother)!=NULL) ;
		}

		static void recompute_firstset()
		{
			typename list<sp_tree*>::iterator it;
			bool flag_recompute=true;
			while(flag_recompute)
			{
				flag_recompute=false;
				it=dbs().begin();
				while(it!=dbs().end())
				{
					sp_tree* pnode=(*it);
					
#ifdef _LIG_DEBUG	
					if(pnode->node_name==NULL) DOUT<<"Blank : ";
					else DOUT<<pnode->node_name<<" : ";
					pnode->debug_print();
#endif
					CharMask backup=pnode->char_mask;
					pnode->char_mask.reset();
					redo_charmask(pnode,pnode->char_mask);
					if(backup!=pnode->char_mask)
					{
						flag_recompute=true;
#ifdef _LIG_DEBUG	
						DOUT<<"!!!!!!!!!!!!!!!!!!!"<<endl;
#endif
					}
					
					++it;
				}
				it=dbs().begin();
				while(it!=dbs().end())
				{
					recompute_tree(*it);
					++it;
				}
#ifdef _LIG_DEBUG	
				DOUT<<"****************************"<<endl;
#endif
			}
		}

		static void add_node(Leaf* &m_curr,Node *pst,int min,int max,bool mt)
		{
#ifdef _LIG_DEBUG
			if(pst->pChild==NULL)
			{
				if(pst->node_name!=NULL)
					DOUT<<"blank node "<<pst->node_name<<endl;
				
			};
#endif
			Leaf* pnext=m_curr->pChild;
			while(pnext!=NULL)
			{
				if(pnext->info==pst && pnext->flag_match_mode==mt
				&& pnext->max_repeat==max && pnext->min_repeat==min) //找到相同的节点
				{
					m_curr=pnext;
					//throw("express error!");
					break;
				}
				else
					pnext=pnext->pBrother; 
			}

			if(pnext==NULL) //此节点目前还不存在，因此要创建新的
			{
				pnext=create_last_child(m_curr); //创建了一个新节点

				pnext->info=pst;
				pnext->min_repeat=min;
				pnext->max_repeat=max;
				pnext->flag_match_mode=mt;
				//pnext->node_name="Insert Node";
				
				if(mt) 
					pnext->char_mask=pst->char_mask;
				else
					pnext->char_mask=~(pst->char_mask); //创建自己的First

				m_curr=pnext;
			}
		};

		struct sp_error //错误记录对象，这是一个特殊的设计
		{
			sp_node *pNode;
			const Char *pErr;
			const Char *pStart;
			string errLine;

			sp_error(){pNode=NULL;pErr=NULL;}

			string getErrorLine(int &lineNumber,int &offset)
			{
				int lineLength=0;
				const Char *p=pStart;
				const Char *pLine=NULL;
				string rt;
				lineNumber=0;
				offset=0;
				while(p<pErr)
				{
					if(*p=='\n') 
					{
						++lineNumber;
						pLine=p+1;
					}
					p++;
				}
				if(pLine==NULL) return rt;
				p=pLine;
				offset=(int)(pErr-pLine);
				while(*p!='\n' && *p!=0){++p;++lineLength;}
				
				rt.append(pLine,lineLength);
				return rt;
			}
		};

		
		//这个函数如果不包含参数，则返回最后一个出错位置和出错对象，包含第一个参数则表示初始化
		static sp_error& error(const Char *str=NULL,sp_node* pFailedNode=NULL)
		{
			static sp_error Err;
			if(pFailedNode==NULL && str==NULL) return Err; //返回错误对象

			if(str!=NULL && pFailedNode==NULL) //重置错误对象
			{
				Err.pStart=str;
				Err.pErr=str;
				Err.pNode=pFailedNode;
			}

			if(str!=NULL && pFailedNode!=NULL) //分析器写入最后的错误
			{
				if(str>Err.pErr) //遵从记录第一个分析失败位置
				{
					Err.pErr=str;
					Err.pNode=pFailedNode;
				}
			}
			return Err;
		};


		class sp_node:public sp_tree
		{
	
		public:

			Leaf* m_curr; //临时使用的指针
			sdl_callbase<scan*>* pCallBack;  //回调对象指针，回调函数支持
					
			template<typename Ts>
			void connect(Ts *pobj, void (Ts::*f)(scan* arg)) //回调支持函数
			{
				if(pCallBack!=NULL) delete pCallBack; //删除不使用的回调
				pCallBack = new  sdl_function<Ts,scan*>(pobj,f); //每次只能连接一个回调
			};

			
			void connect(void (*f)(scan*)) //回调支持函数
			{
				if(pCallBack==NULL) pCallBack = new sdl_callbase<scan*>; //每次只能连接一个回调
				pCallBack->c_callback=f;
			};


			int get_handle_id() //自动得到一个对象句柄
			{
				static int m_handle_id=1111; //使用这个值是为了纪念俺曾经的单身时代
				return (++m_handle_id);
			}

			int get_handle(){return sp_tree::handle;} //得到对象的句柄

			sp_node():sp_tree()
			{
				sp_tree::handle=get_handle_id();
				m_curr=this;
				pCallBack=NULL;
				sp_tree::node_name=NULL;
				dbs().push_back(this); //记录全部有效节点
			
			}

			sp_node(const char *name,int hnd=0):sp_tree()
			{
				if(hnd!=0) sp_tree::handle=hnd;
				else sp_tree::handle=get_handle_id();
				m_curr=this;
				pCallBack=NULL;
				sp_tree::node_name=name;
				dbs().push_back(this); //记录全部有效节点
			}

			inline int add_key(const Char* str,sp_tree &TermCharSet,const Char* str_end=NULL)
			{
				static int theHandle=19731018;
				m_curr=this;++theHandle;
				if(str_end==NULL) return add_keystr(m_curr,str,theHandle,TermCharSet.char_mask)?theHandle:-1;
				else return add_keystr(m_curr,str,str_end,theHandle,TermCharSet.char_mask)?theHandle:-1;
			}

			inline bool insert_key(const Char* str,int tHandle,sp_tree &TermCharSet)
			{
				m_curr=this;
				return add_keystr(m_curr,str,NULL,tHandle,TermCharSet.char_mask);
			}

			inline bool insert_key(const Char* str,const Char* str_end,int tHandle,sp_tree &TermCharSet)
			{
				m_curr=this;
				return add_keystr(m_curr,str,str_end,tHandle,TermCharSet.char_mask);
			}


			inline bool delete_key(const Char* str)
			{
				m_curr=this;
				return delete_keystr(m_curr,str);
			}

			inline bool delete_key(const Char* strStart,const Char* strEnd)
			{
				m_curr=this;
				return delete_keystr(m_curr,strStart,strEnd);
			}

			bool parse(const Char* &pstr,ScanMemAlloctor& ScanMemAlloctor,scan* p_root) //返回一个语法生成树的根
			{
				p_root->clear();
				p_root->start=pstr;
				error(pstr); //初始化出错对象
				return static_analysis(ScanMemAlloctor,this,pstr,p_root);
		
			};
		
			bool do_analysis(const Char* &pstr,int &start_id,ScanMemAlloctor* pScanMemAlloctor=NULL) //返回一个语法生成树的根
			{
				static ScanMemAlloctor defalutScanMemAlloctor; //缺省内存分配器
				int i=0;

				if(pScanMemAlloctor==NULL)
					pScanMemAlloctor=&defalutScanMemAlloctor;
				scan* p_root=pScanMemAlloctor->alloc(&start_id);
				p_root->clear();
				p_root->start=pstr;
				error(pstr); //初始化出错对象
				
				if(*pstr==0) return false;
				if(sp_tree::flag_match_mode) //正常匹配
					do{
						if(sp_tree::escape_callback!=NULL && *pstr==sp_tree::escape_char) //有转义序列定义
						{
							if(!sp_tree::char_mask[sp_tree::escape_callback->call(pstr)])	break;
						}
						else if(!sp_tree::char_mask[(unsigned char)*pstr])
							break; //如果不满足First,则必定不满足后续分析，中止
						if(!static_analysis(*pScanMemAlloctor,this,pstr,p_root)) 
							break;//满足了First，但分析失败，中止

					}while((++i)<sp_tree::max_repeat);
				else  
					do{	
						if(sp_tree::escape_callback!=NULL && *pstr==sp_tree::escape_char) //有转义序列定义
						{
							if(!sp_tree::char_mask[sp_tree::escape_callback->call(pstr)])
							{
								register const Char* orign_str=pstr;
								if(static_analysis(*pScanMemAlloctor,this,pstr,NULL)) //分析成功，所以要中止，并恢复分析前字符流位置，注意此种状态下不需要产生生成树
								{pstr=orign_str;break;}
							}
						}
						else if(!sp_tree::char_mask[(unsigned char)*pstr]) //不满足First,后续分析可以进行，否则继续读入字符，循环
						{
							register const Char* orign_str=pstr;
							if(static_analysis(*pScanMemAlloctor,this,pstr,NULL)) //分析成功，所以要中止，并恢复分析前字符流位置，注意此种状态下不需要产生生成树
							{pstr=orign_str;break;}
						}
					}while((++i)<sp_tree::max_repeat && *(++pstr)!=0);
				p_root->end=pstr;
				return (i>=sp_tree::min_repeat);

			};


#ifdef _LIG_DEBUG
			static void print_last_line(const char *str)
			{
				while(*str!=0)
				{
					if(*str!='\n') DOUT<<*str;
					else return;
					++str;
				}
			}
#endif

			static bool static_analysis(ScanMemAlloctor &MemAlloctor,sp_node* pMe,const Char* &pstr,scan* pScan)
			{
				const Char* orign_str=pstr;
				Leaf* pnext=pMe->pChild;
				scan* pl_scan;
							
#ifdef _LIG_DEBUG	
				static int StackLevel=0;
				static int reread_bytes=0;
				if(pMe->node_name!=NULL && pScan!=NULL) //&& strcmp(pMe->node_name,"otherWord")==0)
				{
					if(pMe->node_name!=NULL && strcmp(pMe->node_name,"String")==0) 
							DOUT<<endl;
					for(int is=0;is<StackLevel;++is) DOUT<<"  ";
					DOUT<<"Come in "<<pMe->node_name<<"  |";
					pMe->debug_print();
					print_last_line(pstr);
					DOUT<<endl; 
				}
				++StackLevel; //进入一次，增加一个堆栈计数
#endif
				if(pScan!=NULL && pMe->node_name!=NULL) //如果没有传递来上层生成树对象，或者没有名字，则不产生生成树对象
				{
					pl_scan=MemAlloctor.alloc();; //产生一个新的生成树对象
					pl_scan->clear();
					pl_scan->start=pstr;
					pl_scan->matched_handle=pMe->handle;
		
				}else pl_scan=NULL;
#ifdef _LIG_DEBUG
				int debug_for_chain_number=0;
#endif
				do
				{
#ifdef _LIG_DEBUG
					++debug_for_chain_number;
#endif

#ifdef _LIG_DEBUG
					int debug_for_item_number=0;
#endif
					while(pnext!=NULL) //在兄弟节点中遍历，一旦匹配成功，不再遍历
					{
//-----------------------------------------------------------------------------------
						int i=0;
						if(pnext->info!=NULL) //这是包含附加节点对象
						{
							
							if(pnext->flag_match_mode) //Normal Match
							{
								
								do{
									if(!pnext->info->char_mask[(unsigned char)*pstr]) //有info的节点，直接看其info的first集合，这个改进有利于动态分析
										break; //如果不满足First,则必定不满足后续分析，中止
									if(!static_analysis(MemAlloctor,pnext->info,pstr,(pl_scan==NULL)?pScan:pl_scan)) 
										break;//满足了First，但分析失败，中止
								
								}while((++i)<pnext->max_repeat);
							}
							else  //Except Match
							{
								do{	
									if(pnext->info->char_mask[(unsigned char)*pstr]) //有info的节点，直接看其info的first集合，这个改进有利于动态分析
									{
										const Char* sub_orign_str=pstr;
										if(static_analysis(MemAlloctor,pnext->info,pstr,NULL)) //分析成功，所以要中止，并恢复分析前字符流位置，注意此种状态下不需要产生生成树
										{
#ifdef _LIG_DEBUG
											reread_bytes+=(int)(pstr-sub_orign_str);
#endif
											pstr=sub_orign_str;
											break;
										}
									}
								}while((++i)<pnext->max_repeat && *(++pstr)!=0);
							}	
							if(i>=pnext->min_repeat) break;
						}
						else //这里是简单的字符匹配了
						{
							while(i!=pnext->max_repeat)
							{
								if(pnext->escape_callback!=NULL && *pstr==pnext->escape_char) //有转义序列定义
								{
									if(!pnext->char_mask[pnext->escape_callback->call(pstr)]) break;
								}
								else if(!pnext->char_mask[(unsigned char)*pstr]) break;
								++i;if(*(++pstr)==Char(0)) break;
							}
							
							if(pnext->max_repeat==0) break; //这是终结符号，需要成功返回
								
							if(i>=pnext->min_repeat) 
							{
								if(pnext->token_handle!=-1) //这是符号表的终结，需要回退一个字符
								{
									if(pl_scan!=NULL) pl_scan->token_handle=pnext->token_handle;
									--pstr; //回退字符
								}
								break;
							}
						}
//----------------------------------------------------------------------------------------
						pnext=pnext->pBrother; 
					}
					if(pnext==NULL) //遍历失败，匹配也失败
					{
#ifdef _LIG_DEBUG
						if(pMe->node_name!=NULL && pScan!=NULL)
						{
							
							for(int is=0;is<StackLevel;++is) DOUT<<"  ";
							DOUT<<" the "<<pMe->node_name<<" Failed!"<<endl;
							
						}
				
#endif
						if(pMe->node_name!=NULL) error(pstr,pMe); //记录出错位置
#ifdef _LIG_DEBUG
						reread_bytes+=(int)(pstr-orign_str);
#endif
						pstr=orign_str; //恢复数据流指针
						MemAlloctor.free(pl_scan);
#ifdef _LIG_DEBUG
						--StackLevel; //退出函数则退栈
#endif
						return false; 
					}
					pnext=pnext->pChild;//在子代节点中遍历

				}while(pnext!=NULL); 

				if(pScan!=NULL &&  pl_scan!=NULL) //匹配成功
				{

#ifdef _LIG_DEBUG
					if(pMe->node_name!=NULL && pScan!=NULL)
					{
						for(int is=0;is<StackLevel;++is) DOUT<<"  ";
						DOUT<<" the "<<pMe->node_name<<" OK!"<<endl;
					}
#endif
					pl_scan->name=pMe->node_name;
					pl_scan->controled_size=(int)(pstr-pl_scan->start);
					pl_scan->end=pstr;
					if(pl_scan->pChild!=NULL && 
						pl_scan->end==pl_scan->pChild->end &&
						pl_scan->controled_size==pl_scan->pChild->controled_size) //重复的生成树
					{
						pScan->add_child(pl_scan->pChild); //前移 
						MemAlloctor.free(pl_scan);
					}
					else
						pScan->add_child(pl_scan);
					if(pMe->pCallBack!=NULL)
						pMe->pCallBack->call(pl_scan);
				}
#ifdef _LIG_DEBUG
				else if(pMe->node_name!=NULL && pScan!=NULL)
				{
					for(int i=0;i<StackLevel;++i) DOUT<<"  ";
					
					DOUT<<" the "<<pMe->node_name<<" OK!"<<" Reread="<<reread_bytes<<endl;
					--StackLevel;
				}
				
#endif 
#ifdef _LIG_DEBUG
				--StackLevel; //退出函数则退栈
#endif 
				return true;
			};


			sp_node& operator<=(Node &st)
			{
				m_curr=this;
				add_node(m_curr,&st,1,1,true);
				return *this;
			}

			sp_node& operator<=(Leaf &st)
			{
				m_curr=this;
				add_leaf(m_curr,&st,1,1,true);
				return *this;
			}

			sp_node& operator<=(Char ch)
			{
				sp_tree::char_mask[ch] = true;
				m_curr=this;
				add_char(m_curr,ch,1,1,true);
				return *this;
			}

			sp_node& operator<=(sp_warp<Char> st)
			{
				sp_tree::char_mask[(Char)(st.charValue)] = true;
				m_curr=this;
				add_char(m_curr,(Char)(st.charValue),st.min,st.max,st.mmode);
				return *this;
			}

			sp_node& operator<=(sp_warp<Node> st)
			{
				m_curr=this;
				add_node(m_curr,st.pObj,st.min,st.max,st.mmode);
				return *this;
			}

			sp_node& operator<=(sp_warp<Leaf> st)
			{
				m_curr=this;
				add_leaf(m_curr,st.pObj,st.min,st.max,st.mmode);
				return *this;
			}

			sp_node& operator<=(const Char *str)
			{
				m_curr=this;
				add_str(m_curr,str);
				return *this;
			}


			sp_node& operator | (Node &st)
			{
				m_curr=this;
				add_node(m_curr,&st,1,1,true);
				return *this;
			}

			sp_node& operator | (Leaf &st)
			{
				m_curr=this;
				add_leaf(m_curr,&st,1,1,true);
				return *this;
			}

			sp_node& operator | (Char ch)
			{
				sp_tree::char_mask[ch] = true;
				m_curr=this;
				add_char(m_curr,ch,1,1,true);
				return *this;
			}

			sp_node& operator | (sp_warp<Node> st)
			{
				m_curr=this;
				add_node(m_curr,st.pObj,st.min,st.max,st.mmode);
				return *this;
			}

			sp_node& operator | (sp_warp<Leaf> st)
			{
				m_curr=this;
				add_leaf(m_curr,st.pObj,st.min,st.max,st.mmode);
				return *this;
			}

			sp_node& operator | (sp_warp<Char> st)
			{
				sp_tree::char_mask[(Char)(st.charValue)] = true;
				m_curr=this;
				add_char(m_curr,(Char)(st.charValue),st.min,st.max,st.mmode);
				return *this;
			}

			sp_node& operator | (const Char *str)
			{
				m_curr=this;
				add_str(m_curr,str);
				return *this;
			}

			sp_node& operator & (sp_closed t)
			{
				add_term(m_curr);
				return *this;
			}

			sp_node& operator & (Char ch)
			{
				add_char(m_curr,ch,1,1,true);
				return *this;
			}

			sp_node& operator & (Node &st)
			{
				add_node(m_curr,&st,1,1,true);
				return *this;
			};

			sp_node& operator & (Leaf &st)
			{
				add_leaf(m_curr,&st,1,1,true);
				return *this;
			};

			sp_node& operator & (sp_warp<Node> st)
			{
				add_node(m_curr,st.pObj,st.min,st.max,st.mmode);
				return *this;
			}

			sp_node& operator & (sp_warp<Leaf> st)
			{
				add_leaf(m_curr,st.pObj,st.min,st.max,st.mmode);
				return *this;
			}

			sp_node& operator & (sp_warp<Char> st)
			{
			
				add_char(m_curr,(Char)(st.charValue),st.min,st.max,st.mmode);
				
				return *this;
			}

			sp_node& operator & (sp_warp<Char*> st)
			{
				Node* newNode=new Node;
				Leaf* newLeaf=static_cast<Leaf*>(newNode);
				add_str(newLeaf,(const Char*)(st.pObj)); //由于这是强制转换而来，此处必须强制转换回去
				add_node(m_curr,newNode,st.min,st.max,st.mmode);
				return *this;
			}

			sp_node& operator & (const Char *str)
			{
				add_str(m_curr,str);
				return *this;
			}

		};	

		static sp_node& sub(const char* name=NULL)
		{
			sp_node* newNode=new sp_node(name);
			return *newNode;
		}

		static sp_tree& item()
		{
			sp_tree* newItem=new sp_tree();
			return *newItem;
		}

	
		static void update_firstset(sp_node& node)
		{
			node.char_mask.reset();
			redo_charmask(&node,node.char_mask);
			recompute_tree(&node);
		}

	};


//GR0代表匹配规定的对象任意次（0到无限）
#define GR0(X) R0(sp_parser<char>::sub()<=X)
#define GR1(X) R1(sp_parser<char>::sub()<=X)
#define OPT(X) Rn(sp_parser<char>::sub()<=X,0,1)
#define OPTS(X) R0(sp_parser<char>::sub()<=X)

#define CHARS(X) (sp_parser<char>::item()<=X)
#define OBJS(X) (sp_parser<char>::sub()<=X)

#define IR0(X) R0(sp_parser<char>::item()<=X)
#define IR1(X) R1(sp_parser<char>::item()<=X)

	template<class Tp>
	static sp_warp<Tp> R0(Tp &obj,int n=INFINITY)
	{
		sp_warp<Tp> rt;
		rt.max=n;
		rt.min=0;
		rt.mmode=true;//Normal Match
		
		rt.pObj=&obj;
		return rt;
	}//R0代表匹配0到无限次对象

	
	template<class Tp>
	static sp_warp<Tp> R1(Tp &obj,int n=INFINITY)
	{
		sp_warp<Tp> rt;
		rt.max=n;
		rt.min=1;
		rt.mmode=true;//Normal Match
	
		rt.pObj=&obj;
		return rt;
	}//R0代表匹配1到无限次对象

	template<class Tp>
	static sp_warp<Tp> Rn(Tp &obj,int minA,int maxA=INFINITY)
	{
		sp_warp<Tp> rt;
		rt.max=maxA;
		rt.min=minA;
		rt.mmode=true;//Normal Match
		
		rt.pObj=&obj;
		return rt;
	}//Rn代表匹配n到无限次对象

	template<class Tp>
	static sp_warp<Tp> N0(Tp &obj,int n=INFINITY)
	{
		sp_warp<Tp> rt;
		rt.max=n;
		rt.min=0;
		rt.mmode=false;//Except match;
		
		rt.pObj=&obj;
		return rt;
	}//N0代表不匹配对象要求的字符匹配0到无限次

	static sp_warp<char> R0(char obj,int n=INFINITY)
	{
		sp_warp<char> rt;
		rt.max=n;
		rt.min=0;
		rt.mmode=true;
		
		rt.charValue=obj;
		return rt;
	}

	static sp_warp<char> R1(char obj,int n=INFINITY)
	{
		sp_warp<char> rt;
		rt.max=n;
		rt.min=1;
		rt.mmode=true;
	
		rt.charValue=obj;
		return rt;
	}
	
	static sp_warp<char> N0(char obj,int n=INFINITY)
	{
		sp_warp<char> rt;
		rt.max=n;
		rt.min=0;
		rt.mmode=false;//Except match;
	
		rt.charValue=obj;
		return rt;
	}//N0代表不

	template<class Tp>
	static sp_warp<Tp> N1(Tp &obj,int n=INFINITY)
	{
		sp_warp<Tp> rt;
		rt.max=n;
		rt.min=1;
		rt.mmode=false;//Except match;
		
		rt.pObj=&obj;
		return rt;
	}//N0代表不匹配对象要求的字符匹配1到无限次

	template<class Char>
	static sp_warp<Char*> NS0(const Char *str,int n=INFINITY)
	{
		sp_warp<Char*> rt;
		rt.max=n;
		rt.min=0;
		rt.mmode=false;//Except match;
		
		rt.pObj=(Char**)(str); //这是一个强制转换，需要注意
		return rt;
	}//NS0代表不匹配规定字符串的字符匹配0到无限次

	template<class Char>
	static sp_warp<Char*> NS1(const Char *str,int n=INFINITY)
	{
		sp_warp<Char*> rt;
		rt.max=n;
		rt.min=1;
		rt.mmode=false;//Except match;
		rt.flag_search=false;
		rt.pObj=(Char**)(str);
		return rt;
	}//NS0代表不匹配规定字符串的字符匹配1到无限次

	

	typedef sp_parser<char>::sp_node Node;
	typedef sp_parser<char>::sp_tree Item;
	typedef sp_parser<char>::sp_closed Term;
	typedef sp_parser<char>::ScanMemAlloctor AST;
	typedef ScanItem<char> Scan;
	typedef sp_parser<char> Parser;

	struct JsonParser
	{
		
		AST mem;
		Scan* pRoot;

		JsonParser()
		{
			pRoot=mem.alloc(); 
		}

		void init()
		{
			mem.clear();
			pRoot=mem.alloc(); 
		}

		inline void skip_space(const char* &pTxt)
		{
			bool flag=true;
			while(flag)
			{
				flag=false;
				while(*pTxt==' ' || *pTxt=='\t' || *pTxt=='\r' || *pTxt=='\n') ++pTxt;  //skip space
				if(*pTxt=='/' && *(pTxt+1)=='/'){pTxt+=2; while(*pTxt!=0 && *pTxt!='\n') ++pTxt;pTxt++;flag=true;} //skip cpp memo
				if(*pTxt=='/' && *(pTxt+1)=='*'){pTxt+=2; while(*pTxt!=0 && *pTxt!='*' && *(pTxt+1)!='/') ++pTxt;pTxt+=2;flag=true;} //skip c memo
			}
		}
		bool json_number(const char* &pTxt,Scan* pScan)
		{
			pScan->start=pTxt;
			pScan->matched_handle=JSON_INTEGER;

			if(*pTxt=='-' && (*(++pTxt)<'0' || *pTxt>'9')) 
				goto failed_return;

			if(*pTxt=='0' && *(pTxt+1)=='x' )
			{
				pTxt+=2;
				while((*pTxt>='0' && *pTxt<='9') || (*pTxt>='A' && *pTxt<='F') || (*pTxt>='a' && *pTxt<='f')) {if(*(++pTxt)==0) break;}
				pScan->controled_size=pTxt-pScan->start;
				pScan->end=pTxt;
				pScan->matched_handle=JSON_HEX;
				return true;
			}
			while(*(++pTxt)!=0 && *pTxt>='0' && *pTxt<='9'); //integer

			if(*pTxt=='.')
			{
				pScan->matched_handle=JSON_FLOAT;
				if(*(++pTxt)<'0' || *pTxt>'9') goto failed_return;
				while(*(++pTxt)!=0 && *pTxt>='0' && *pTxt<='9');

				if(*pTxt=='E' || *pTxt=='e')
				{
					if(*(++pTxt)=='+' || *pTxt=='-') //E-1,E+123..
					{
						if(*(++pTxt)<'0' || *pTxt>'9') goto failed_return;
					}
					else if(*(++pTxt)<'0' || *pTxt>'9') goto failed_return; 
					while(*(++pTxt)!=0 && *pTxt>='0' && *pTxt<='9');//E1,E234...
				}
				pScan->controled_size=pTxt-pScan->start;
				pScan->end=pTxt;
				return true;
			}

			if(*pTxt=='E' || *pTxt=='e')
			{
				if(*(++pTxt)=='+' || *pTxt=='-') //E-1,E+123..
				{
					if(*(++pTxt)<'0' || *pTxt>'9') goto failed_return;
				}
				else if(*(++pTxt)<'0' || *pTxt>'9') goto failed_return;
				while(*(++pTxt)!=0 && *pTxt>='0' && *pTxt<='9');//E1,E234...
			}
			pScan->controled_size=pTxt-pScan->start;
			pScan->end=pTxt;
			return true;
failed_return:
			pTxt=pScan->start;
			return false;
		}
		bool json_array(const char* &pTxt,Scan* pScan)
		{
			pScan->start=pTxt;
			Scan *pValue;
			++pTxt;  //skip '['
start_element:
			skip_space(pTxt);
			pValue=mem.alloc();
			pValue->clear();
			if(json_parse(pTxt,pValue))
			{
				pScan->add_child(pValue); 
				skip_space(pTxt);
				if(*pTxt==',' || *pTxt==';') {++pTxt;goto start_element;}
				if(*pTxt==']') 
				{
					++pTxt;
					pScan->matched_handle=JSON_ARRAY; //修改类型
					pScan->controled_size=pTxt-pScan->start;
					pScan->end=pTxt;
					return true;
				}
			}
			mem.del(pValue);
			pTxt=pScan->start;
			return false;
		}
		bool json_object(const char* &pTxt,Scan* pScan)
		{
			pScan->start=pTxt;
			++pTxt;  //skip '{'
rescan_pair:
			skip_space(pTxt);
			Scan* pPair=mem.alloc(); 
			pPair->clear(); 
			if(json_pair(pTxt,pPair))
			{
				if(*pTxt==',' || *pTxt==';') 
				{
					++pTxt; //skip ,
					pScan->add_child(pPair); //add child 
					goto rescan_pair;
				}
				if(*pTxt=='}')
				{
					++pTxt; //skip }
					pScan->add_child(pPair); //add child 
					pScan->end=pTxt;
					pScan->matched_handle=JSON_OBJECT; //修改类型
					pScan->controled_size=pTxt-pScan->start;
					return true;
				}
			}
			return false;
		}
		bool json_pair(const char* &pTxt,Scan* pScan)
		{
			if(json_string(pTxt,pScan))
			{
				pScan->name=pScan->start;
				*((char*)pScan->end)=0;  //set  cstr for name
				skip_space(pTxt);
				if(*pTxt==':' || *pTxt=='=') 
				{
					++pTxt;
					if(json_parse(pTxt,pScan))
					{
						skip_space(pTxt);
						return true;
					}
				}
			}
			else //扩展Json格式
			{
				const char* pend=pTxt;
				while(*pend!=0 && *pend!=':' && *pend!='=' && *pend!=' ' && *pend!='\t' && *pend!='\r' && *pend!='\n') ++pend;
				pScan->name=pTxt;
				pScan->end=pend;
				if(*pend==0) return false;
				pTxt=pend;
				skip_space(pTxt);
				if(*pTxt==':' || *pTxt=='=') 
				{
					++pTxt;
					*((char*)pScan->end)=0;  //set  cstr for name
					if(json_parse(pTxt,pScan))
					{
						skip_space(pTxt);
						return true;
					}
				}
				
			}
			return false;
		}
		
		bool json_string(const char* &pTxt,Scan* pScan)
		{
			if(*pTxt!='\"') return false;
		
			char *pstr=(char*)(++pTxt);
			char *pend=pstr;
			while(*pend!=0)
			{
				if(*pend=='\\' && *(pend+1)=='\\') 
				{
					pend+=2;
					continue;
				}
				if(*pend=='\\' && *(pend+1)=='\"') 
				{
					pend+=2;
					continue;
				}
				if(*pend=='\"') break;
				++pend;
			}
			int reduce_counter=0;
			while(pstr<pend)
			{
				if(*pstr=='\\') switch(*(pstr+1))
				{
					case 't':
						*pstr='\t';
						memcpy(pstr+1,pstr+2,pend-pstr-2);
						*(pend-1)=0;
						reduce_counter++;
						++pstr;
						break;
					case 'r':
						*pstr='\r';
						memcpy(pstr+1,pstr+2,pend-pstr-2);
						*(pend-1)=0;
						reduce_counter++;
						++pstr;
						break;
					case 'n':
						*pstr='\n';
						memcpy(pstr+1,pstr+2,pend-pstr-2);
						*(pend-1)=0;
						reduce_counter++;
						++pstr;
						break;
					case 'b':
						*pstr='\b';
						memcpy(pstr+1,pstr+2,pend-pstr-2);
						*(pend-1)=0;
						reduce_counter++;
						++pstr;
						break;
					case 'f':
						*pstr='\f';
						memcpy(pstr+1,pstr+2,pend-pstr-2);
						*(pend-1)=0;
						reduce_counter++;
						++pstr;
						break;
					case '\\':
						*pstr='\\';
						memcpy(pstr+1,pstr+2,pend-pstr-2);
						*(pend-1)=0;
						reduce_counter++;
						++pstr;
						break;
					case '/':
						*pstr='\\';
						memcpy(pstr+1,pstr+2,pend-pstr-2);
						*(pend-1)=0;
						reduce_counter++;
						++pstr;
						break;
					case 'u':
						break;
					default:
						return false;
				}
				++pstr;
			}
			pScan->matched_handle=JSON_STRING;
			pScan->start=pTxt;
			pScan->end=pend;
			pScan->controled_size=(pend-pTxt)-reduce_counter;
			
			*pend=0; //cstr here
			pTxt=pend+1;
			return true;
			
		}

		bool json_parse(const char* &pTxt,Scan* pScan=NULL)
		{
			int i=0;
			if(pScan==NULL) 
			{
				pScan=pRoot;
				pScan->clear(); 
			}
			skip_space(pTxt);
			while(*pTxt!=0)
			{
				switch(*pTxt)
				{
				case ' ' : case '\t' : case '\n' : case '\r':
					break;
				case '\"' :
					return json_string(pTxt,pScan);
				case '{' :
					return json_object(pTxt,pScan);
				case '[' :
					return json_array(pTxt,pScan);
				case '0' : case '1' : case '2' : case '3' : case '4' : case '5' : case '6' : case '7' :
				case '8' : case '9' : case '-' :
					return json_number(pTxt,pScan);
				case 't' :
					if(*(pTxt+1)=='r' && *(pTxt+2)=='u' && *(pTxt+3)=='e')
					{
						pScan->matched_handle=JSON_TRUE;
						pScan->start=pTxt;
						pScan->end=pTxt+4;
						pScan->controled_size=4; 
						pTxt+=4;
						return true;
					}
					break;
				case 'n' :
					if(*(pTxt+1)=='u' && *(pTxt+2)=='l' && *(pTxt+3)=='l')
					{
						pScan->matched_handle=JSON_NULL;
						pScan->start=pTxt;
						pScan->end=pTxt+4;
						pScan->controled_size=4; 
						pTxt+=4;
						return true;
					}
					break;
				case 'f' :
					if(*(pTxt+1)=='a' && *(pTxt+2)=='l' && *(pTxt+3)=='s' && *(pTxt+4)=='e')
					{
						pScan->matched_handle=JSON_FALSE;
						pScan->start=pTxt;
						pScan->end=pTxt+5;
						pScan->controled_size=5; 
						pTxt+=5;
						return true;
					}
					break;
				default:
					return false;
				}
				++pTxt;
			}
			return false;
		}
	};
	

	
	class Reader //文件读取器
	{
	private:
		char* pOrignBuf;
		char* pStartBuf;
		int BufLength;
		//---------------
		bool flag_case_insensitive; //为true假定要求大小写无关,会将文件内容全部转换为小写,否则不转换
		
		//-----------------------
		const char *pStrStart;
		const char *pOrign;
	public:
		const char *pStr;
		int offset;

		inline const char* ostr(const char* pstr)
		{
			return(pOrign+(int)(pstr-pStrStart));
		}

		Reader()
		{
			pOrignBuf=NULL;
			pStartBuf=NULL;
			flag_case_insensitive=false;
		};

		Reader(const char *fileName,bool case_insensitive=false)
		{
			flag_case_insensitive=case_insensitive;
				read(fileName);
		};

		~Reader()
		{
			if(pStartBuf!=NULL) delete[] pStartBuf;
			if(pOrignBuf!=NULL) delete[] pOrignBuf;
		}

		void set_case_sensitive() //文件是大小写有关的,不转换文件内的大小写
		{
			flag_case_insensitive=false;
		}

		void set_case_insensitive() //文件是大小写无关的,将大写字母都转换为小写字母
		{
			flag_case_insensitive=true;
		}

		bool read(const char *fileName)
		{
			FILE *fs;
			if((fs=fopen(fileName,"rb"))!=NULL)
			{
				int curpos = ftell(fs);
				fseek(fs, 0, SEEK_END);
				BufLength = ftell(fs);
				fseek(fs, curpos, SEEK_SET);
				
				pOrignBuf=new char[BufLength+16];
				
				fread(pOrignBuf,1,BufLength,fs);
				for(int i=BufLength;i<BufLength+16;i++) pOrignBuf[i]=0;
				 //增加一个C结束符，并加一段保护区域，文本分析的时候会使用
				
				pOrign=pOrignBuf;
			
				pStartBuf=new char[BufLength+16];
				memcpy(pStartBuf,pOrignBuf,BufLength); //拷贝一个备份
				for(int i=BufLength;i<BufLength+16;i++) pStartBuf[i]=0; //增加一个C结束符，并加一段保护区域，文本分析的时候会使用
				pStrStart=pStartBuf+int(pOrign-pOrignBuf); //得到实际使用的指针
				

				if(flag_case_insensitive) //如果文本是大小写无关的，为了方便，全部转为小写
					for(int i=0;i<BufLength;i++) 
						if(pStartBuf[i]>='A' && pStartBuf[i]<='Z') pStartBuf[i]+=('a'-'A'); //全部转为小写

				pStr=pStrStart;
				offset=(int)(pOrign-pStrStart);
				fclose(fs);
				return true;
			}
			return false;
		}

	};
};
#endif
