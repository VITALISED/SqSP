#ifndef __LIG_LL_PARSER_
#define __LIG_LL_PARSER_
#include <list>
#include <bitset>
#include <set>
#include <iostream>


#include "sdl_memory.h"
#include "sdl_signal.h"
/*
EBNF��������ʹ��˵��

   ����һ���Ƚϼ򵥵�EBNF�﷨�����������������ٶ���Ȼ������YACC��LR�������죬
������һ���㹻ʹ�õķ�������������ʹ��LL(1)�����﷨������������߱�Ԥ��������
���������������һ���LL(1)���������ơ�
   ���Խ��������һ����Ƕ�׵ļ��﷨�ṹ�£�����10K���ҵ��ı���Ҫ1.4M������
ʱ�����ڣ�����3.0G��Ƶ��CPU����Լ��0.5���룬�൱��ÿ�봦��20M�ı���

1���������Ļ���ʹ�÷���

	����һ��Ƕ��ʽ�ķ��������﷨��������ڴ����и����������Ǵ�ͳ��YACC��Ԥ����
����ʽ���﷨�����ڴ����б��������¹����ʾ��

	���ʹ��ASCII�ַ�������ʵĿǰҲֻ֧��ASCII)������ֱ��ʹ��Ԥ������������� 

	typedef sp_parser<char>::sp_node Node;
	typedef sp_parser<char>::sp_tree Item;
	typedef ScanItem<char> scan;

	Node ��Ӧһ���﷨����Item ��Ӧһ���ս�����ϣ�scan��Ӧһ������������

	�﷨����ʹ��C++���ص������������������ ��<=�� ����������� '&' ��ʾ����
	��|�� ������ѡ�Ĺ���, ����ʹ�����ű���һ�����ȵ��﷨�飬�������ű���ʹ��
	(sub() ...)�ķ�ʽ��Ҳ���Զ���һ����Node����

	R0(),R1(),Rn()��ʾ������ظ���ʽ������EBNF�Ķ���һ�£�R0()��ʾ������Բ�����
	Ҳ�����ظ�����Σ�R1()��ʾ����������һ�Σ������ظ�����Σ�Rn��Ҫ����������
	�������ٳ��ֶ��ٴΣ�����ظ����ٴ�

	N0(),N1(),Nn()��ʾ���������ƥ����ַ����ظ���ʽ��N0()��ʾ��ƥ������ַ�����
	������Ҳ�����ظ������....

	ÿ��Node���Զ���һ���ص����ڽ����﷨ƥ���ǣ����Node����Ĺ������㣬�������ص�
	�������ص�������Ҫ������һ�����У���ʹ�����º������巽ʽ��

	void [��������](scan *it);
	
	Node�д���ص������ķ�ʽΪ��

	[Node����].connect(&[�ص�����ʵ��],&[�ص���������]::[�ص���������]);

	���������ԭ�����㽫�ʷ����﷨�������϶�Ϊһ��������ĳЩ����£��дʷ�����������㣨����
	��ǰ��n���ַ��������ԣ����������֧��ͨ���ص���������֧�ִʷ�������


	��Item����ʹ��connect_escape��������һ�����ʷ���������������������߱�
	char function_name(const char *&pstr)����ʽ����ƥ��Item��ʱ�򣬻�������������������ǰ��
	�ַ���ָ�봫�ݵ���������У�����������ǰ��n���ַ���ѡ����ԣ����Ĵ�����ַ���ָ�룩���߲�����
	���������ַ���ָ�룩��Ȼ�󷵻�һ���ַ�����Itemȥƥ�䡣

	������ʽΪ connect_escape('&',this,&SquirrelParser::lex_and); 
	connect_escape�ĵ�һ������Ϊ��ʼ�ַ�������̽�⵽�����ʼ�ַ��ŻῪʼ���ú������ڶ�������Ϊ�ص�������ָ��
	������������Ϊ�ص�������

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
	static int perf_counter=0; //�����õļ�����
	static int stack_counter=0; //�����ö�ջָʾ����
#endif

#ifndef INFINITY
#define INFINITY 0x7FFFFFFF
#endif
	
	template<class Char>
	struct ScanItem //�﷨�������������ڼ��ٷ���ɨ��Ķ�ջ����
	{
		const Char *start; //����ƥ����ַ���λ��
		const Char *end; //����ƥ����ַ���λ��
		const Char *name; //�������������

		int matched_handle; //��ƥ������handle
		int token_handle; //�����ڴʷ�ƥ��Ĵʷ�handle,�����ڷ��ű���
		int controled_size; //����������Ƶ��Ķ��󳤶�


		ScanItem *pFather;
		ScanItem *pChild;
		ScanItem *pBrother;
		ScanItem **ppLastChild; //Ϊ�����Ч�ʣ����Ӵ����¼���һ�������ӡ���λ��
								//���*ppLastChild==NULL,��ʾֻ��һ������
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

		inline void add_child(ScanItem *pC) //������ǰ�������
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

		int backto(const Char *pstr) //��ɨ���¼���˵��ַ���ָ��λ��
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

		inline basic_string<Char> str() //�����Ƶ��ַ�����ת��Ϊ��׼�ַ���
		{
			return basic_string<Char>(start,end);
		};

		//----------------------------------------------------
		inline bool match_child_handle(int handle) //�ж�Ѱ���Ӵ����Ƿ�����ָ�����ƥ��Ķ���
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
		ScanItem* get_handle(int handle) //���Ӵ����Ƿ�����ָ�����ƥ��Ķ���
		{
			ScanItem* pf=pChild;
			while(pf!=NULL)
			{
				if(pf->matched_handle==handle) return pf; 
				pf=pf->pChild;
			}
			return NULL;
		}

		ScanItem* get(int loc=0) //���Ӵ���Ѱ�ҵ�loc������
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

		ScanItem* get(const Char *name) //���Ӵ���Ѱ���������ֵĶ���
		{
			ScanItem* rt=pChild;
			if(*name==0) return this; //�մ�����������
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

		//���Ӵ���Ѱ������Ϊname1����name1�Ӵ����������Ѱ������Ϊname2�Ķ���
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
		//�ں���ͬ����Ѱ������Ϊname1�Ķ���
		ScanItem* next(const Char *name)
		{
			ScanItem* rt=pBrother;
			while(rt!=NULL && strcmp(name,rt->name)!=0) rt=rt->pBrother; 
			return rt;
			
		}
		//�ں���ͬ����Ѱ��handle����
		ScanItem* next_handle(int handle)
		{
			ScanItem* rt=pBrother;
			while(rt!=NULL && rt->matched_handle!=handle) rt=rt->pBrother; 
			return rt;
			
		}
		//-----------------------------------------------------------
		inline bool is_str(const Char* str) //�ж����Ƶ��ַ����Ƿ���ָ�����ַ���
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

		inline bool mystrcmp(const Char* str,const Char* strEnd=NULL) //�ж����Ƶ��ַ����Ƿ���ָ�����ַ���
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

		inline bool mystrcmp(const Char* str,int size) //�ж����Ƶ��ַ����Ƿ���ָ�����ַ���
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

		inline int cstrcmp(const Char* str,int size=-1) //�ж����Ƶ��ַ���(cstr���뷽ʽ)�Ƿ���ָ�����ַ���
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

		bool operator==(const Char* str){return is_str(str);} //�����ַ����Ƚϵģ����ص��������

		bool operator!=(const Char* str){return !(is_str(str));} //�����ַ����Ƚϵģ����ص��������

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

		static int my_atol(const char *str,const char* endptr) //��Դ��CSDN yinqing_yx����
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

		static double my_strtod(const char* s, char** endptr) //��Դ��CSDNһ�δ��룬�ڴ�лл
		{
			register const char*  p     = s;
			register long double  value = 0.L;
			int                   sign  = 0;
			long double           factor;
			unsigned int          expo;

			while(isspace(*p) ) p++;//����ǰ��Ŀո�
			if(*p == '-' || *p == '+')	sign = *p++;//�ѷ��Ÿ����ַ�sign��ָ����ơ�
			//���������ַ�
			while ( (unsigned int)(*p - '0') < 10u ) value = value*10 + (*p++ - '0');//ת����������
			//����������ı�ʾ��ʽ���磺1234.5678��
			if ( *p == '.' )
			{
				factor = 1.0;p++;
				while ( (unsigned int)(*p - '0') < 10u )
				{
					factor *= 0.1;
					value  += (*p++ - '0') * factor;
				}
			}
			//�����IEEE754��׼�ĸ�ʽ���磺1.23456E+3��
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

		struct sp_closed {};//һ������������ڱ�ʾ����������ɹ�����
	
		typedef MemAlloctor<scan,256,12> ScanMemAlloctor;  //��ҳ������ڴ����� 
		
		
		static Leaf* create_last_child(Leaf* parent) //��һ���ڵ�������һ���ӽڵ㣬�����������һ���ӽڵ�
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

		static Leaf* create_first_child(Leaf* parent) //��һ���ڵ�������һ���ӽڵ㣬�������ǵ�һ���ӽڵ�
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

		static Leaf* insert_brother(Leaf* brother) //��һ���ڵ����һ���ֵܽڵ㣬�����ڵ�ǰ�ڵ�֮��
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

		class sp_tree //����һ�����׵ĵ���ָ�����ṹ��ר�����������������Ч�ʵ�һ��
		{
		public:
			typedef ScanItem<Char> scan;

			int min_repeat; //���ƥ�������0����epsilonƥ�䣬���Ϊ-1�����˽����ܲ�ƥ����ַ���ֱ��ƥ��Ϊֹ
			int max_repeat; //���ƥ�������0x7fffffff���������ƥ��
			//----------------------------
			CharMask char_mask; //����ʶ���ַ�����ġ����С�
			const char *node_name;  //�ڵ�����
			//----------------------------
			Char     escape_char; //ת�����,������ת�����,��Ҫִ��һ���ص�����,�ж�ת������Ƿ����Ҫ��
			sdl_rt_callbase<const Char*&,Char> *escape_callback; //ת����ŵĻص�����
		template<typename Ts>
			void connect_escape(Char escapChar,Ts *pobj, Char (Ts::*f)(const Char*& arg)) //�ص�֧�ֺ���
			{
				escape_char = escapChar;
				char_mask[escapChar] = 1;
				if(escape_callback!=NULL) delete escape_callback; //ɾ����ʹ�õĻص�
				escape_callback = new sdl_rt_function<Ts,const Char*&,Char>(pobj,f); //ÿ��ֻ������һ���ص�
			};
			void connect_escape(Char escapChar,Char (*f)(const Char*&)) //�ص�֧�ֺ���
			{
				escape_char = escapChar;
				char_mask[escapChar] = 1;
				if(escape_callback!=NULL) delete escape_callback; //ɾ����ʹ�õĻص�
				escape_callback = new sdl_rt_callbase<const Char*&,Char>; //ÿ��ֻ������һ���ص�
				escape_callback->c_callback=f;
			};

			//----------------------------
			bool flag_match_mode; //���ڱ�ʾƥ��ģʽ true��ʾ����ƥ�䣬false��ʾ���ǡ�ƥ��
		
			int token_handle;  //���ڼ�¼���ű����
			int handle; //�ڵ��ʶ
			//---------------------------------------------
			sp_node *info; //��� info==NULL������һ����ͨ�ڵ� ������ָ��һ������
			int required_info_handle; //�����Ϊ-1����Ҫ��infoƥ���token_handle�����ֵ��ȣ�������Ϊƥ��ʧ��
			//---------------------------------------------
			sp_tree *pFather; //ָ�򸸽ڵ�
			sp_tree *pChild;  //ָ���ӽڵ�
			sp_tree *pBrother; //ָ����һ���ֵܽڵ㣬������ָ��Ϊ�գ������Ѿ�û���ֵܽڵ���
			//--------------------------------------------
			
			//-------------------------------------------
			inline void set_max(int m=INFINITY){max_repeat=m;}
			inline void set_min(int m=0){min_repeat=m;}
			inline void operator()(int min=0,int max=INFINITY){min_repeat=min;max_repeat=max;}
			inline void set_except_match(){flag_match_mode=false;}; //����Ϊ������...��ƥ��ģʽ
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
				flag_match_mode = true;  //����Ϊ����ƥ��ģʽ
				escape_callback = NULL;  //ת����ŵĻص�������ΪNULL
				node_name=NULL; //ȱʡû������

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
				flag_match_mode = true;  //����Ϊ����ƥ��ģʽ
				escape_callback = NULL;  //ת����ŵĻص�������ΪNULL
				node_name=NULL; //ȱʡû������
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
				flag_match_mode = true;  //����Ϊ����ƥ��ģʽ
				escape_callback = NULL;  //ת����ŵĻص�������ΪNULL
				node_name=NULL; //ȱʡû������
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
				flag_match_mode = tr.flag_match_mode;  //����Ϊ����ƥ��ģʽ
				
				char_mask = tr.char_mask; 
			};

			~sp_tree(){}


			void delete_children() //ɾ������
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

			void delete_tree() //ɾ�����ṹ
			{
				delete_children(); //ɾ��ȫ������
				if(pFather!=NULL) //����root
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

			Leaf* clone(Leaf* father=NULL) //��¡һ�����ṹ
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


			bool is_include(Leaf* pLeaf) //����һ���ڵ��Ƿ��Ѿ�����ָ���ڵ���ַ�ӳ��
			{
				bitset<MaxChar> tmp=char_mask;
				tmp|=pLeaf->char_mask;
				return (tmp==char_mask);
			}

			bool is_conflict(Leaf* pLeaf) //�����ַ����Ƿ������ͻ
			{
				const static bitset<MaxChar> zeroValue;
				bitset<MaxChar> tmp=char_mask;
				tmp&=pLeaf->char_mask;
				return (tmp!=zeroValue);
			}


		public:

			bool is_single_child() //���Ա��ڵ��Ƿ����һ��Ψһ���ӽڵ�
			{
				if(pChild==NULL) return false;
				if(pChild->pBrother!=NULL) return false;
				return true;
			}
			bool is_same(Leaf* pLeaf,int tMin,int tMax,bool mt) //���������ڵ��Ƿ�ͬ��
			{
				return (info==NULL && pLeaf->info==NULL && (tMin==-1)?(min_repeat==pLeaf->min_repeat):(min_repeat==tMin) &&
					(tMax==-1)?(max_repeat==pLeaf->max_repeat):(max_repeat==tMax) &&
					(tMax==-1)?(flag_match_mode==pLeaf->flag_match_mode):(mt== flag_match_mode) && 
					char_mask==pLeaf->char_mask &&
					escape_callback==pLeaf->escape_callback && 
					escape_char==pLeaf->escape_char);
			}

			static int string_match(sp_tree* pMe,const Char* &pstr) //���м��ַ���ƥ�䣬�൱��������ʽ������
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
				if(pnext->is_same(plf,min,max,mt)) //�ҵ�ͬ���ڵ�
				{
					m_curr=pnext;
					break;
				}
				else
					pnext=pnext->pBrother; 
			}
			if(pnext==NULL) //�����µ�
			{
				pnext=create_last_child(m_curr); //������һ���½ڵ�
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
				if(mt && pnext->char_mask[ch]) //�ҵ���ͬ�Ľڵ�
				{
					m_curr=pnext;
					break;
				}
				pnext=pnext->pBrother; 
			}
			if(pnext==NULL) //���ַ�Ŀǰ�������ڣ����Ҫ�����µ�
			{
				pnext=create_last_child(m_curr); //������һ���½ڵ�
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

			if(m_curr->pFather==NULL) //�������ʼ�ڵ㣬��Ҫ����First 
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
				pup=pnext; //������ǰֵ
				pnext=pnext->pChild;
				while(pnext!=NULL) //�����Ƿ��Ѿ�����ͬ�����ַ���
				{
					if(pnext->char_mask[*pstr]) 
						break;
					else
						pnext=pnext->pBrother; 
				}
				if(pnext==NULL) //���ַ�Ŀǰ�������ڣ����Ҫ�����µ�
				{
					pnext=create_first_child(pup); //������һ���½ڵ�
					pnext->char_mask[*pstr]=1;
					pnext->flag_match_mode=true;
				}
				++pstr; 
			}
			pnext=create_last_child(pnext);
			pnext->char_mask=0; 
			pnext->max_repeat=0; //����һ���ս���� 
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

			if(m_curr->pFather==NULL) //�������ʼ�ڵ㣬��Ҫ����First 
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
				pup=pnext; //������ǰֵ
				pnext=pnext->pChild;
				while(pnext!=NULL) //�����Ƿ��Ѿ�����ͬ�����ַ���
				{
					if(pnext->char_mask[*pstr]) 
						break;
					else
						pnext=pnext->pBrother; 
				}
				if(pnext==NULL) //���ַ�Ŀǰ�������ڣ����Ҫ�����µ�
				{
					pnext=create_first_child(pup); //������һ���½ڵ�
					pnext->char_mask[*pstr]=1;
					pnext->flag_match_mode=true;
				}
				++pstr; 
			}
			pnext=create_last_child(pnext);
			pnext->char_mask=TermCharSet;
			pnext->token_handle=tokenHandle;
			pnext->max_repeat=1; //����һ���ս���� 
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
				pup=pnext; //������ǰֵ
				pnext=pnext->pChild;
				tmp.flag_single=true;
				while(pnext!=NULL)
				{
					if(pnext->char_mask[*pstr]) 
					{
						tmp.pLeaf=pnext;
						tmp.flag_single=pnext->is_single_child(); //���Խڵ�����Ƿ�Ϊ��һ���� 
						StrPathList.push_back(tmp); 
						break;
					}
					pnext=pnext->pBrother; 
				}

				if(pnext==NULL) return false ;//���ַ�Ŀǰ�������ڣ�����޷�ɾ���ַ���
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

		

		static list<sp_tree*>& dbs() //��¼������Ч�Ľڵ�
		{
			static list<sp_tree*> mdbs;
			return mdbs;
		}

		

		static void redo_charmask(Leaf* plf,CharMask &chmask) //�ݹ��㷨��Ѱ�ҿ��ܶ���Ҫ���½ڵ��Ӱ��
		{
			
			Leaf* pb=plf->pChild ; //�ҵ���һ���ӽڵ�
			if(pb==NULL) return;
			do{
				if(pb->flag_match_mode) //�ж�ֻ�ڵ������
				{
					if(pb->info==NULL)
					{
						chmask|=pb->char_mask; //����node����first setд�뼴��
					}
					else
					{
						pb->char_mask=pb->info->char_mask;  
						chmask|=pb->info->char_mask;
					}
					if(pb->min_repeat==0)  redo_charmask(pb,chmask); //��epsilon���ݹ����
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
			}while(pb!=NULL); //���ֵܽڵ��б���
		};

		static void recompute_tree(Leaf* plf)
		{
			Leaf* pb=plf->pChild;
			if(pb==NULL) return;
			do
			{
				recompute_tree(pb); //�ݹ�������������ӽڵ�
				//���ڵݹ飬��������ײ�Ľڵ㿪ʼ����
				if(pb->is_node()) throw "Rule error"; //�����ܳ����ڽڵ���
				if(pb->info!=NULL)  //Я��������ӽڵ�
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
				&& pnext->max_repeat==max && pnext->min_repeat==min) //�ҵ���ͬ�Ľڵ�
				{
					m_curr=pnext;
					//throw("express error!");
					break;
				}
				else
					pnext=pnext->pBrother; 
			}

			if(pnext==NULL) //�˽ڵ�Ŀǰ�������ڣ����Ҫ�����µ�
			{
				pnext=create_last_child(m_curr); //������һ���½ڵ�

				pnext->info=pst;
				pnext->min_repeat=min;
				pnext->max_repeat=max;
				pnext->flag_match_mode=mt;
				//pnext->node_name="Insert Node";
				
				if(mt) 
					pnext->char_mask=pst->char_mask;
				else
					pnext->char_mask=~(pst->char_mask); //�����Լ���First

				m_curr=pnext;
			}
		};

		struct sp_error //�����¼��������һ����������
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

		
		//�����������������������򷵻����һ������λ�úͳ�����󣬰�����һ���������ʾ��ʼ��
		static sp_error& error(const Char *str=NULL,sp_node* pFailedNode=NULL)
		{
			static sp_error Err;
			if(pFailedNode==NULL && str==NULL) return Err; //���ش������

			if(str!=NULL && pFailedNode==NULL) //���ô������
			{
				Err.pStart=str;
				Err.pErr=str;
				Err.pNode=pFailedNode;
			}

			if(str!=NULL && pFailedNode!=NULL) //������д�����Ĵ���
			{
				if(str>Err.pErr) //��Ӽ�¼��һ������ʧ��λ��
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

			Leaf* m_curr; //��ʱʹ�õ�ָ��
			sdl_callbase<scan*>* pCallBack;  //�ص�����ָ�룬�ص�����֧��
					
			template<typename Ts>
			void connect(Ts *pobj, void (Ts::*f)(scan* arg)) //�ص�֧�ֺ���
			{
				if(pCallBack!=NULL) delete pCallBack; //ɾ����ʹ�õĻص�
				pCallBack = new  sdl_function<Ts,scan*>(pobj,f); //ÿ��ֻ������һ���ص�
			};

			
			void connect(void (*f)(scan*)) //�ص�֧�ֺ���
			{
				if(pCallBack==NULL) pCallBack = new sdl_callbase<scan*>; //ÿ��ֻ������һ���ص�
				pCallBack->c_callback=f;
			};


			int get_handle_id() //�Զ��õ�һ��������
			{
				static int m_handle_id=1111; //ʹ�����ֵ��Ϊ�˼�������ĵ���ʱ��
				return (++m_handle_id);
			}

			int get_handle(){return sp_tree::handle;} //�õ�����ľ��

			sp_node():sp_tree()
			{
				sp_tree::handle=get_handle_id();
				m_curr=this;
				pCallBack=NULL;
				sp_tree::node_name=NULL;
				dbs().push_back(this); //��¼ȫ����Ч�ڵ�
			
			}

			sp_node(const char *name,int hnd=0):sp_tree()
			{
				if(hnd!=0) sp_tree::handle=hnd;
				else sp_tree::handle=get_handle_id();
				m_curr=this;
				pCallBack=NULL;
				sp_tree::node_name=name;
				dbs().push_back(this); //��¼ȫ����Ч�ڵ�
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

			bool parse(const Char* &pstr,ScanMemAlloctor& ScanMemAlloctor,scan* p_root) //����һ���﷨�������ĸ�
			{
				p_root->clear();
				p_root->start=pstr;
				error(pstr); //��ʼ���������
				return static_analysis(ScanMemAlloctor,this,pstr,p_root);
		
			};
		
			bool do_analysis(const Char* &pstr,int &start_id,ScanMemAlloctor* pScanMemAlloctor=NULL) //����һ���﷨�������ĸ�
			{
				static ScanMemAlloctor defalutScanMemAlloctor; //ȱʡ�ڴ������
				int i=0;

				if(pScanMemAlloctor==NULL)
					pScanMemAlloctor=&defalutScanMemAlloctor;
				scan* p_root=pScanMemAlloctor->alloc(&start_id);
				p_root->clear();
				p_root->start=pstr;
				error(pstr); //��ʼ���������
				
				if(*pstr==0) return false;
				if(sp_tree::flag_match_mode) //����ƥ��
					do{
						if(sp_tree::escape_callback!=NULL && *pstr==sp_tree::escape_char) //��ת�����ж���
						{
							if(!sp_tree::char_mask[sp_tree::escape_callback->call(pstr)])	break;
						}
						else if(!sp_tree::char_mask[(unsigned char)*pstr])
							break; //���������First,��ض������������������ֹ
						if(!static_analysis(*pScanMemAlloctor,this,pstr,p_root)) 
							break;//������First��������ʧ�ܣ���ֹ

					}while((++i)<sp_tree::max_repeat);
				else  
					do{	
						if(sp_tree::escape_callback!=NULL && *pstr==sp_tree::escape_char) //��ת�����ж���
						{
							if(!sp_tree::char_mask[sp_tree::escape_callback->call(pstr)])
							{
								register const Char* orign_str=pstr;
								if(static_analysis(*pScanMemAlloctor,this,pstr,NULL)) //�����ɹ�������Ҫ��ֹ�����ָ�����ǰ�ַ���λ�ã�ע�����״̬�²���Ҫ����������
								{pstr=orign_str;break;}
							}
						}
						else if(!sp_tree::char_mask[(unsigned char)*pstr]) //������First,�����������Խ��У�������������ַ���ѭ��
						{
							register const Char* orign_str=pstr;
							if(static_analysis(*pScanMemAlloctor,this,pstr,NULL)) //�����ɹ�������Ҫ��ֹ�����ָ�����ǰ�ַ���λ�ã�ע�����״̬�²���Ҫ����������
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
				++StackLevel; //����һ�Σ�����һ����ջ����
#endif
				if(pScan!=NULL && pMe->node_name!=NULL) //���û�д������ϲ����������󣬻���û�����֣��򲻲�������������
				{
					pl_scan=MemAlloctor.alloc();; //����һ���µ�����������
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
					while(pnext!=NULL) //���ֵܽڵ��б�����һ��ƥ��ɹ������ٱ���
					{
//-----------------------------------------------------------------------------------
						int i=0;
						if(pnext->info!=NULL) //���ǰ������ӽڵ����
						{
							
							if(pnext->flag_match_mode) //Normal Match
							{
								
								do{
									if(!pnext->info->char_mask[(unsigned char)*pstr]) //��info�Ľڵ㣬ֱ�ӿ���info��first���ϣ�����Ľ������ڶ�̬����
										break; //���������First,��ض������������������ֹ
									if(!static_analysis(MemAlloctor,pnext->info,pstr,(pl_scan==NULL)?pScan:pl_scan)) 
										break;//������First��������ʧ�ܣ���ֹ
								
								}while((++i)<pnext->max_repeat);
							}
							else  //Except Match
							{
								do{	
									if(pnext->info->char_mask[(unsigned char)*pstr]) //��info�Ľڵ㣬ֱ�ӿ���info��first���ϣ�����Ľ������ڶ�̬����
									{
										const Char* sub_orign_str=pstr;
										if(static_analysis(MemAlloctor,pnext->info,pstr,NULL)) //�����ɹ�������Ҫ��ֹ�����ָ�����ǰ�ַ���λ�ã�ע�����״̬�²���Ҫ����������
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
						else //�����Ǽ򵥵��ַ�ƥ����
						{
							while(i!=pnext->max_repeat)
							{
								if(pnext->escape_callback!=NULL && *pstr==pnext->escape_char) //��ת�����ж���
								{
									if(!pnext->char_mask[pnext->escape_callback->call(pstr)]) break;
								}
								else if(!pnext->char_mask[(unsigned char)*pstr]) break;
								++i;if(*(++pstr)==Char(0)) break;
							}
							
							if(pnext->max_repeat==0) break; //�����ս���ţ���Ҫ�ɹ�����
								
							if(i>=pnext->min_repeat) 
							{
								if(pnext->token_handle!=-1) //���Ƿ��ű���սᣬ��Ҫ����һ���ַ�
								{
									if(pl_scan!=NULL) pl_scan->token_handle=pnext->token_handle;
									--pstr; //�����ַ�
								}
								break;
							}
						}
//----------------------------------------------------------------------------------------
						pnext=pnext->pBrother; 
					}
					if(pnext==NULL) //����ʧ�ܣ�ƥ��Ҳʧ��
					{
#ifdef _LIG_DEBUG
						if(pMe->node_name!=NULL && pScan!=NULL)
						{
							
							for(int is=0;is<StackLevel;++is) DOUT<<"  ";
							DOUT<<" the "<<pMe->node_name<<" Failed!"<<endl;
							
						}
				
#endif
						if(pMe->node_name!=NULL) error(pstr,pMe); //��¼����λ��
#ifdef _LIG_DEBUG
						reread_bytes+=(int)(pstr-orign_str);
#endif
						pstr=orign_str; //�ָ�������ָ��
						MemAlloctor.free(pl_scan);
#ifdef _LIG_DEBUG
						--StackLevel; //�˳���������ջ
#endif
						return false; 
					}
					pnext=pnext->pChild;//���Ӵ��ڵ��б���

				}while(pnext!=NULL); 

				if(pScan!=NULL &&  pl_scan!=NULL) //ƥ��ɹ�
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
						pl_scan->controled_size==pl_scan->pChild->controled_size) //�ظ���������
					{
						pScan->add_child(pl_scan->pChild); //ǰ�� 
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
				--StackLevel; //�˳���������ջ
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
				add_str(newLeaf,(const Char*)(st.pObj)); //��������ǿ��ת���������˴�����ǿ��ת����ȥ
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


//GR0����ƥ��涨�Ķ�������Σ�0�����ޣ�
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
	}//R0����ƥ��0�����޴ζ���

	
	template<class Tp>
	static sp_warp<Tp> R1(Tp &obj,int n=INFINITY)
	{
		sp_warp<Tp> rt;
		rt.max=n;
		rt.min=1;
		rt.mmode=true;//Normal Match
	
		rt.pObj=&obj;
		return rt;
	}//R0����ƥ��1�����޴ζ���

	template<class Tp>
	static sp_warp<Tp> Rn(Tp &obj,int minA,int maxA=INFINITY)
	{
		sp_warp<Tp> rt;
		rt.max=maxA;
		rt.min=minA;
		rt.mmode=true;//Normal Match
		
		rt.pObj=&obj;
		return rt;
	}//Rn����ƥ��n�����޴ζ���

	template<class Tp>
	static sp_warp<Tp> N0(Tp &obj,int n=INFINITY)
	{
		sp_warp<Tp> rt;
		rt.max=n;
		rt.min=0;
		rt.mmode=false;//Except match;
		
		rt.pObj=&obj;
		return rt;
	}//N0����ƥ�����Ҫ����ַ�ƥ��0�����޴�

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
	}//N0����

	template<class Tp>
	static sp_warp<Tp> N1(Tp &obj,int n=INFINITY)
	{
		sp_warp<Tp> rt;
		rt.max=n;
		rt.min=1;
		rt.mmode=false;//Except match;
		
		rt.pObj=&obj;
		return rt;
	}//N0����ƥ�����Ҫ����ַ�ƥ��1�����޴�

	template<class Char>
	static sp_warp<Char*> NS0(const Char *str,int n=INFINITY)
	{
		sp_warp<Char*> rt;
		rt.max=n;
		rt.min=0;
		rt.mmode=false;//Except match;
		
		rt.pObj=(Char**)(str); //����һ��ǿ��ת������Ҫע��
		return rt;
	}//NS0����ƥ��涨�ַ������ַ�ƥ��0�����޴�

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
	}//NS0����ƥ��涨�ַ������ַ�ƥ��1�����޴�

	

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
					pScan->matched_handle=JSON_ARRAY; //�޸�����
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
					pScan->matched_handle=JSON_OBJECT; //�޸�����
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
			else //��չJson��ʽ
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
	

	
	class Reader //�ļ���ȡ��
	{
	private:
		char* pOrignBuf;
		char* pStartBuf;
		int BufLength;
		//---------------
		bool flag_case_insensitive; //Ϊtrue�ٶ�Ҫ���Сд�޹�,�Ὣ�ļ�����ȫ��ת��ΪСд,����ת��
		
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

		void set_case_sensitive() //�ļ��Ǵ�Сд�йص�,��ת���ļ��ڵĴ�Сд
		{
			flag_case_insensitive=false;
		}

		void set_case_insensitive() //�ļ��Ǵ�Сд�޹ص�,����д��ĸ��ת��ΪСд��ĸ
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
				 //����һ��C������������һ�α��������ı�������ʱ���ʹ��
				
				pOrign=pOrignBuf;
			
				pStartBuf=new char[BufLength+16];
				memcpy(pStartBuf,pOrignBuf,BufLength); //����һ������
				for(int i=BufLength;i<BufLength+16;i++) pStartBuf[i]=0; //����һ��C������������һ�α��������ı�������ʱ���ʹ��
				pStrStart=pStartBuf+int(pOrign-pOrignBuf); //�õ�ʵ��ʹ�õ�ָ��
				

				if(flag_case_insensitive) //����ı��Ǵ�Сд�޹صģ�Ϊ�˷��㣬ȫ��תΪСд
					for(int i=0;i<BufLength;i++) 
						if(pStartBuf[i]>='A' && pStartBuf[i]<='Z') pStartBuf[i]+=('a'-'A'); //ȫ��תΪСд

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
