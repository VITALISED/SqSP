//--------------------------------------------------------
//---Scanf支持--来源于Apple libc库--------------
#include "sdl_script_support.h"
#include "sdl_tcpstream.h"

static inline int isspace(char c)
{
	return (c == ' ' || c == '\t' || c == '\n' || c == '\12');
}

#define	BUF	32 	/* Maximum length of numeric string. */

/*
* Flags used during conversion.
*/
#define	LONG		0x01	/* l: long or double */
#define	SHORT		0x04	/* h: short */
#define	SUPPRESS	0x08	/* *: suppress assignment */
#define	POINTER		0x10	/* p: void * (as hex) */
#define	NOSKIP		0x20	/* [ or c: do not skip blanks */
#define	LONGLONG	0x400	/* ll: long long (+ deprecated q: quad) */
#define	SHORTSHORT	0x4000	/* hh: char */
#define	UNSIGNED	0x8000	/* %[oupxX] conversions */
#define	FLOAT		0x10000	/* %fFeEgG */
/*
* The following are used in numeric conversions only:
* SIGNOK, NDIGITS, DPTOK, and EXPOK are for floating point;
* SIGNOK, NDIGITS, PFXOK, and NZDIGITS are for integral.
*/
#define	SIGNOK		0x40	/* +/- is (still) legal */
#define	NDIGITS		0x80	/* no digits detected */

#define	DPTOK		0x100	/* (float) decimal point is still legal */
#define	EXPOK		0x200	/* (float) exponent (e+3, etc) still legal */

#define	PFXOK		0x100	/* 0x prefix is (still) legal */
#define	NZDIGITS	0x200	/* no zero digits detected */

/*
* Conversion types.
*/
#define	CT_CHAR		0	/* %c conversion */
#define	CT_CCL		1	/* %[...] conversion */
#define	CT_STRING	2	/* %s conversion */
#define	CT_INT		3	/* %[dioupxX] conversion */
#define CT_FLOAT    4
static const unsigned char* __sccl(char *, const u_char *);

SQInteger Sq2Vsscanf(HSQUIRRELVM v)
{
	int inr;
	int c;			/* character from format, or conversion */
	size_t width;		/* field width, or 0 */
	char *p;		/* points into all kinds of strings */
	int n;			/* handy integer */
	int flags;		/* flags as defined above */
	char *p0;		/* saves original value of p when necessary */
	int nassigned;		/* number of fields assigned */
	int nconversions;	/* number of conversions */
	int nread;		/* number of characters consumed from fp */
	int base;		/* base argument to conversion function */
	char ccltab[256];	/* character class table for %[...] */
	char buf[BUF];		/* buffer for numeric conversions */

	/* `basefix' is used to avoid `if' tests in the integer scanner */
	static short basefix[17] =
	{ 10, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16 };

	const char *fmt0;
	const char *inp;
	SqFile *pFile;
	SQInstancePointer pCT;
	if(SQ_SUCCEEDED(sq_getinstanceup(v,1,(SQUserPointer*)&pFile,0))) //是文件读取方式
	{
		if(!SqGetVar(v,fmt0,pCT)) return 0;
		pFile->readLn();  //读入一行
		inp=pFile->linestr.c_str(); //得到输入缓冲区
	}
	else if(!SqGetVar(v,inp,fmt0,pCT)) return 0;
	if(!check_instancetype(v,"StdVector")) return 0;  //检查是否是指定的类型实例
	SqStdVectorWarp* handle=(SqStdVectorWarp*)pCT;
	vector<SQObject> &vect=*(handle->pVector);

	char* currStr=(char*)inp; //当前指针位置
	int arrayIdx=0;  //第一个数组成员
	const unsigned char *fmt = (const unsigned char*)fmt0;
	inr = strlen(inp);
	if(*inp==0 || *fmt==0) return 0; //数据不对，返回
	nassigned = 0;
	nconversions = 0;
	nread = 0;
	base = 0;		/* XXX just to keep gcc happy */
	for (;;) {
		c = *fmt++;
		if (c == 0)
		{
			int vectsize=vect.size();
			for(int i=nassigned;i<vectsize;++i) //删除多余的对象
			{
				sq_release(v,&(vect.back())); //去除引用计数
				vect.pop_back();             //弹出不需要的对象
			}
			sq_pushinteger(v,nassigned);
			return 1;
		}
		if (isspace(c)) {
			while (inr > 0 && isspace(*inp))
				nread++, inr--, inp++;
			continue;
		}
		if (c != '%')
			goto literal_ch;
		width = 0;
		flags = 0;
		/*
		* switch on the format.  continue if done;
		* break once format type is derived.
		*/
again:		c = *fmt++;
		switch (c) {
		case '%':
literal_ch:
			if (inr <= 0)
				goto input_failure;
			if (*inp != c)
				goto match_failure;
			inr--, inp++;
			nread++;
			continue;

		case '*':
			flags |= SUPPRESS;
			goto again;
		case 'l':
			if (flags & LONG) {
				flags &= ~LONG;
				flags |= LONGLONG;
			} else
				flags |= LONG;
			goto again;
		case 'q':
			flags |= LONGLONG;	/* not quite */
			goto again;
		case 'h':
			if (flags & SHORT) {
				flags &= ~SHORT;
				flags |= SHORTSHORT;
			} else
				flags |= SHORT;
			goto again;

		case '0': case '1': case '2': case '3': case '4':
		case '5': case '6': case '7': case '8': case '9':
			width = width * 10 + c - '0';
			goto again;

			/*
			* Conversions.
			*/
		case 'f': case 'F' : case 'e' : case 'E' : case 'g' : case 'G' :
			c= CT_FLOAT;
			break;
		case 'd':
			c = CT_INT;
			base = 10;
			break;

		case 'i':
			c = CT_INT;
			base = 0;
			break;

		case 'o':
			c = CT_INT;
			flags |= UNSIGNED;
			base = 8;
			break;

		case 'u':
			c = CT_INT;
			flags |= UNSIGNED;
			base = 10;
			break;

		case 'X':
		case 'x':
			flags |= PFXOK;	/* enable 0x prefixing */
			c = CT_INT;
			flags |= UNSIGNED;
			base = 16;
			break;

		case 's':
			c = CT_STRING;
			break;

		case '[':
			fmt = __sccl(ccltab, fmt);
			flags |= NOSKIP;
			c = CT_CCL;
			break;

		case 'c':
			flags |= NOSKIP;
			c = CT_CHAR;
			break;

		case 'p':	/* pointer format is like hex */
			flags |= POINTER | PFXOK;
			c = CT_INT;
			flags |= UNSIGNED;
			base = 16;
			break;

		case 'n':
			nconversions++;

			if (flags & SUPPRESS)	/* ??? */
				continue;

			//-------------------------------------------
			if(arrayIdx<vect.size())
			{
				SQObject &obj=vect[arrayIdx];
				sq_release(v,&obj);    //释放原来的对象
				obj._type=OT_INTEGER; 
				obj._unVal.nInteger=nread;  //读入值  

			}
			else
			{
				SQObject obj;
				obj._type=OT_INTEGER; 
				obj._unVal.nInteger=nread;  //读入值  
				vect.push_back(obj);
			}
			++arrayIdx;
			//----------------------------------------
			continue;
		}

		/*
		* We have a conversion that requires input.
		*/
		if (inr <= 0)
			goto input_failure;

		/*
		* Consume leading white space, except for formats
		* that suppress this.
		*/
		if ((flags & NOSKIP) == 0) {
			while (isspace(*inp)) {
				nread++;
				if (--inr > 0)
					inp++;
				else 
					goto input_failure;
			}
			/*
			* Note that there is at least one character in
			* the buffer, so conversions that do not set NOSKIP
			* can no longer result in an input failure.
			*/
		}

		/*
		* Do the conversion.
		*/
		switch (c) {

		case CT_CHAR:
			/* scan arbitrary characters (sets NOSKIP) */
			if (width == 0)
				width = 1;
			if (flags & SUPPRESS) {
				size_t sum = 0;
				for (;;) {
					if ((n = inr) < (int)width) {
						sum += n;
						width -= n;
						inp += n;
						if (sum == 0)
							goto input_failure;
						break;
					} else {
						sum += width;
						inr -= width;
						inp += width;
						break;
					}
				}
				nread += sum;
			} else {

				//-------------------------------------------
				//-----------获得指定长度的字符串-----------
				sq_pushstring(v,(const char*)inp,width);
				if(arrayIdx<vect.size())
				{
					SQObject &obj=vect[arrayIdx];
					sq_release(v,&obj); //删除原来的引用
					sq_getstackobj(v,-1,&obj); //读入字符串值
					sq_addref(v,&obj); //增加引用计数，防止函数返回字符串被销毁
				}
				else
				{
					vect.push_back(SQObject());
					SQObject &obj=vect.back();
					sq_getstackobj(v,-1,&obj); //读入字符串值 
					sq_addref(v,&obj); //增加引用计数，防止函数返回字符串被销毁
				}

				++arrayIdx;
				sq_pop(v,1);
				currStr+=width;
				//------------------------------------------------
				inr -= width;
				inp += width;
				nread += width;
				nassigned++;
			}
			nconversions++;
			break;

		case CT_CCL:
			/* scan a (nonempty) character class (sets NOSKIP) */
			if (width == 0)
				width = (size_t)~0;	/* `infinity' */
			/* take only those things in the class */
			if (flags & SUPPRESS) {
				n = 0;
				while (ccltab[(unsigned char)*inp]) {
					n++, inr--, inp++;
					if (--width == 0)
						break;
					if (inr <= 0) {
						if (n == 0)
							goto input_failure;
						break;
					}
				}
				if (n == 0)
					goto match_failure;
			} else {
				p0 = p;
				while (ccltab[(unsigned char)*inp]) {
					inr--;
					*p++ = *inp++;
					if (--width == 0)
						break;
					if (inr <= 0) {
						if (p == p0)
							goto input_failure;
						break;
					}
				}
				n = p - p0;
				if (n == 0)
					goto match_failure;
				//*p = 0;
				//-------------------------------------------
				//-----------获得变长度的字符串-----------
				sq_pushstring(v,(const char*)p0,n);
				if(arrayIdx<vect.size())
				{
					SQObject &obj=vect[arrayIdx];
					sq_release(v,&obj); //删除原来的引用
					sq_getstackobj(v,-1,&obj); //读入字符串值
				}
				else
				{
					SQObject obj;
					sq_getstackobj(v,-1,&obj); //读入字符串值 
					vect.push_back(obj);
				}
				++arrayIdx;
				sq_pop(v,1);
				currStr=(char*)inp;
				//------------------------------------------------
				nassigned++;
			}
			nread += n;
			nconversions++;
			break;

		case CT_STRING:
			/* like CCL, but zero-length string OK, & no NOSKIP */
			if (width == 0)
				width = (size_t)~0;
			if (flags & SUPPRESS) {
				n = 0;
				while (!isspace(*inp)) {
					n++, inr--, inp++;
					if (--width == 0)
						break;
					if (inr <= 0)
						break;
				}
				nread += n;
			} else {
				p0 = (p = currStr);
				while (!isspace(*inp)) {
					inr--;
					*p++ = *inp++;
					if (--width == 0)
						break;
					if (inr <= 0)
						break;
				}
				//*p = 0;
				//-------------------------------------------
				//-----------获得变长度的字符串-----------
				sq_pushstring(v,(const char*)p0,p-p0);
				if(arrayIdx<vect.size())
				{
					SQObject &obj=vect[arrayIdx];
					sq_release(v,&obj); //删除原来的引用
					sq_getstackobj(v,-1,&obj); //读入字符串值
					sq_addref(v,&obj); //增加引用计数，防止函数返回字符串被销毁
				}
				else
				{
					vect.push_back(SQObject());
					SQObject &obj=vect.back();
					sq_getstackobj(v,-1,&obj); //读入字符串值 
					sq_addref(v,&obj); //增加引用计数，防止函数返回字符串被销毁
				}

				++arrayIdx;
				sq_pop(v,1);
				currStr=(char*)inp;
				//------------------------------------------------
				nread += p - p0;
				nassigned++;
			}
			nconversions++;
			continue;
		case CT_FLOAT:
			{
				char*  p;
				int		sign  = 0;
				unsigned int expo;
				double  value = 0.L;
				double	factor;

				while(isspace(*inp) ) inp++;//跳过前面的空格
				p = (char*)inp;
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
					default :	value = 0.L;goto done;
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
				if(p==(char*)inp) goto match_failure;
				value=(sign == '-' ? -value : value);
				if(arrayIdx<vect.size())
				{
					SQObject &obj=vect[arrayIdx];
					sq_release(v,&obj); //删除原来的引用
					obj._type=OT_FLOAT; 
					obj._unVal.fFloat =(SQFloat)value;
				}
				else
				{
					SQObject obj;
					obj._type=OT_FLOAT; 
					obj._unVal.fFloat =(SQFloat)value;
					vect.push_back(obj);
				}
				++arrayIdx;
				currStr=p;
				nassigned++; //增加一个读取计数
				inp=(const char*)(currStr);
			}
			continue;
		case CT_INT:
			/* scan an integer as if by the conversion function */
#ifdef hardway
			if (width == 0 || width > sizeof(buf) - 1)
				width = sizeof(buf) - 1;
#else
			/* size_t is unsigned, hence this optimisation */
			if (--width > sizeof(buf) - 2)
				width = sizeof(buf) - 2;
			width++;
#endif
			flags |= SIGNOK | NDIGITS | NZDIGITS;
			for (p = buf; width; width--) {
				c = *inp;
				/*
				* Switch on the character; `goto ok'
				* if we accept it as a part of number.
				*/
				switch (c) {

					/*
					* The digit 0 is always legal, but is
					* special.  For %i conversions, if no
					* digits (zero or nonzero) have been
					* scanned (only signs), we will have
					* base==0.  In that case, we should set
					* it to 8 and enable 0x prefixing.
					* Also, if we have not scanned zero digits
					* before this, do not turn off prefixing
					* (someone else will turn it off if we
					* have scanned any nonzero digits).
				 */
		case '0':
			if (base == 0) {
				base = 8;
				flags |= PFXOK;
			}
			if (flags & NZDIGITS)
				flags &= ~(SIGNOK|NZDIGITS|NDIGITS);
			else
				flags &= ~(SIGNOK|PFXOK|NDIGITS);
			goto ok;

			/* 1 through 7 always legal */
		case '1': case '2': case '3':
		case '4': case '5': case '6': case '7':
			base = basefix[base];
			flags &= ~(SIGNOK | PFXOK | NDIGITS);
			goto ok;

			/* digits 8 and 9 ok iff decimal or hex */
		case '8': case '9':
			base = basefix[base];
			if (base <= 8)
				break;	/* not legal here */
			flags &= ~(SIGNOK | PFXOK | NDIGITS);
			goto ok;

			/* letters ok iff hex */
		case 'A': case 'B': case 'C':
		case 'D': case 'E': case 'F':
		case 'a': case 'b': case 'c':
		case 'd': case 'e': case 'f':
			/* no need to fix base here */
			if (base <= 10)
				break;	/* not legal here */
			flags &= ~(SIGNOK | PFXOK | NDIGITS);
			goto ok;

			/* sign ok only as first character */
		case '+': case '-':
			if (flags & SIGNOK) {
				flags &= ~SIGNOK;
				goto ok;
			}
			break;

			/* x ok iff flag still set & 2nd char */
		case 'x': case 'X':
			if (flags & PFXOK && p == buf + 1) {
				base = 16;	/* if %i */
				flags &= ~PFXOK;
				goto ok;
			}
			break;
				}

				/*
				* If we got here, c is not a legal character
				* for a number.  Stop accumulating digits.
				*/
				break;
ok:
				/*
				* c is legal: store it and look at the next.
				*/
				*p++ = c;
				if (--inr > 0)
					inp++;
				else 
					break;		/* end of input */
			}
			/*
			* If we had only a sign, it is no good; push
			* back the sign.  If the number ends in `x',
			* it was [sign] '0' 'x', so push back the x
			* and treat it as [sign] '0'.
			*/
			if (flags & NDIGITS) {
				if (p > buf) {
					inp--;
					inr++;
				}
				goto match_failure;
			}
			c = ((u_char *)p)[-1];
			if (c == 'x' || c == 'X') {
				--p;
				inp--;
				inr++;
			}
			if ((flags & SUPPRESS) == 0) {
				unsigned long res;

				*p = 0;
				if ((flags & UNSIGNED) == 0)
					res = atoi(buf);
				else
					res = atoi(buf);
				if (flags & POINTER)
				{

					//-------------------------------------------
					if(arrayIdx<vect.size())
					{
						SQObject &obj=vect[arrayIdx];
						sq_release(v,&obj); //删除原来的引用
						obj._type=OT_USERPOINTER; 
						obj._unVal.pUserPointer =(SQUserPointer)res;
					}
					else
					{
						SQObject obj;
						obj._type=OT_USERPOINTER; 
						obj._unVal.pUserPointer =(SQUserPointer)res;
						vect.push_back(obj);
					}
					++arrayIdx;
					currStr=p+1;
					//-----------------------------------------------
				}
				else
				{
					//-------------------------------------------
					if(arrayIdx<vect.size())
					{
						SQObject &obj=vect[arrayIdx];
						obj._type=OT_INTEGER; 
						obj._unVal.nInteger =(SQInteger)res;
					}
					else
					{
						SQObject obj;
						obj._type=OT_INTEGER; 
						obj._unVal.nInteger =(SQInteger)res;
						vect.push_back(obj);
					}
					++arrayIdx;
					//-----------------------------------------------
				}
				nassigned++;
			}
			nread += p - buf;
			nconversions++;
			break;

		}
	}
input_failure:
	if(nconversions == 0) return 0;
match_failure:
	int vectsize=vect.size();
	for(int i=nassigned;i<vectsize;++i) //删除多余的对象
	{
		sq_release(v,&(vect.back())); //去除引用计数
		vect.pop_back();             //弹出不需要的对象
	}
	sq_pushinteger(v,nassigned);
	return 1;
}

/*
* Fill in the given table from the scanset at the given format
* (just after `[').  Return a pointer to the character past the
* closing `]'.  The table has a 1 wherever characters should be
* considered part of the scanset.
*/
static const unsigned char* __sccl(char *tab, const u_char *fmt)
{
	int c, n, v;

	/* first `clear' the whole table */
	c = *fmt++;		/* first char hat => negated scanset */
	if (c == '^') {
		v = 1;		/* default => accept */
		c = *fmt++;	/* get new first char */
	} else
		v = 0;		/* default => reject */

	/* XXX: Will not work if sizeof(tab*) > sizeof(char) */
	(void) memset(tab, v, 256);

	if (c == 0)
		return (fmt - 1);/* format ended before closing ] */

	/*
	* Now set the entries corresponding to the actual scanset
	* to the opposite of the above.
	*
	* The first character may be ']' (or '-') without being special;
	* the last character may be '-'.
	*/
	v = 1 - v;
	for (;;) {
		tab[c] = v;		/* take character c */
doswitch:
		n = *fmt++;		/* and examine the next */
		switch (n) {

		case 0:			/* format ended too soon */
			return (fmt - 1);

		case '-':
			/*
			* A scanset of the form
			*	[01+-]
			* is defined as `the digit 0, the digit 1,
			* the character +, the character -', but
			* the effect of a scanset such as
			*	[a-zA-Z0-9]
			* is implementation defined.  The V7 Unix
			* scanf treats `a-z' as `the letters a through
			* z', but treats `a-a' as `the letter a, the
			* character -, and the letter a'.
			*
			* For compatibility, the `-' is not considerd
			* to define a range if the character following
			* it is either a close bracket (required by ANSI)
			* or is not numerically greater than the character
			* we just stored in the table (c).
			*/
			n = *fmt;
			if (n == ']' || n < c) {
				c = '-';
				break;	/* resume the for(;;) */
			}
			fmt++;
			/* fill in the range */
			do {
				tab[++c] = v;
			} while (c < n);
			c = n;
			/*
			* Alas, the V7 Unix scanf also treats formats
			* such as [a-c-e] as `the letters a through e'.
			* This too is permitted by the standard....
			*/
			goto doswitch;
			break;

		case ']':		/* end of scanset */
			return (fmt);

		default:		/* just another character */
			c = n;
			break;
		}
	}
	/* NOTREACHED */
}



struct gdbm_file_info;
struct datum
{
	char *dptr;
	int   dsize;
};
#define  GDBM_READER  0		/* READERS only. */
#define  GDBM_WRITER  1		/* READERS and WRITERS.  Can not create. */
#define  GDBM_WRCREAT 2		/* If not found, create the db. */
#define  GDBM_NEWDB   3		/* ALWAYS create a new db.  (WRITER) */
#define  GDBM_FAST    16	/* Write fast! => No fsyncs. */
#define  GDBM_INSERT  0		/* Do not overwrite data in the database. */
#define  GDBM_REPLACE 1		/* Replace the old value with the new value. */

void  gdbm_close(gdbm_file_info *dbf );
int   gdbm_delete(gdbm_file_info *dbf, datum key);
datum gdbm_fetch(gdbm_file_info *dbf, datum key);
gdbm_file_info *gdbm_open(char *file, int block_size=4096,int flags=GDBM_WRCREAT,int mode=0,void (*fatal_func)(char*)=NULL);
int   gdbm_reorganize(gdbm_file_info *dbf );
datum gdbm_firstkey(gdbm_file_info *dbf );
datum gdbm_nextkey(gdbm_file_info *dbf, datum key );
int   gdbm_store(gdbm_file_info *dbf, datum key,datum content, int flags=GDBM_REPLACE);
int   gdbm_exists(gdbm_file_info *dbf, datum key);
void  gdbm_sync	(gdbm_file_info *dbf);
int   gdbm_setopt(gdbm_file_info *dbf, int optflag,int *optval, int optlen);
//----------------------------------------------------------------------------------
bool gdbm_fastget_key(gdbm_file_info *dbf,datum key,const char* &find_data,int &size);
//自己定义的一个快速获取函数,写在的win_gdbm.cpp中。
//这个函数会破坏gdbm_fetch函数的完整性，在并发访问的时候，不要混合使用这两个函数
//---------------------------------------------------------------------------------



SQInteger  SqDBM::_queryobj_releasehook(SQUserPointer p, SQInteger size) //标准的Squirrel C++对象析构回调
{
	SqDBM* handle=(SqDBM*)p;
	if(handle!=NULL) 
	{
		if(handle->pGDBM!=NULL)
			gdbm_close((gdbm_file_info*)handle->pGDBM);
		delete handle;
	}
	return 0;
};
SQInteger SqDBM::Sq2Constructor(HSQUIRRELVM v) //标准的Squirrel C++对象构造方式
{
	SqDBM* handle;
	const char* filename;
	if(SqGetVar(v,filename))
	{
		handle=new SqDBM;
#ifdef WIN32
		handle->pGDBM=(void*)gdbm_open((char*)filename);
#else
		handle->pGDBM=(void*)gdbm_open((char*)filename,4096,GDBM_WRCREAT,0,NULL);
#endif
		if(handle->pGDBM==NULL) return 0;
		sq_setinstanceup(v,1,(SQUserPointer)handle);
		sq_setreleasehook(v,1,_queryobj_releasehook);
	}
	else
	{
		handle=new SqDBM;
		handle->pGDBM=NULL;
		sq_setinstanceup(v,1,(SQUserPointer)handle);
		sq_setreleasehook(v,1,_queryobj_releasehook);
	}
	return 0;
}

SQInteger SqDBM::Sq2JsonParse(HSQUIRRELVM v)
{
	const char* key;
	SqDBM* handle;

	if(SQ_SUCCEEDED(sq_getinstanceup(v,1,(SQUserPointer*)&handle,NULL))
		&& handle->pGDBM!=NULL 
		&& SqGetVar(v,key))
	{
		datum keyd;
		const char* pVal;
		int valSize;
		keyd.dptr=(char*)key;
		keyd.dsize=strlen(key);
		if(gdbm_fastget_key((gdbm_file_info*)handle->pGDBM,keyd,pVal,valSize))
		{
			sq_pushstring(v,pVal,valSize);
			return 1;
		}
	}
	return 0;
}
/*
#define JSON_FALSE -1
#define JSON_TRUE   1
#define JSON_NULL   2
#define JSON_OBJECT 3
#define JSON_STRING 4
#define JSON_ARRAY  5
#define JSON_INTEGER 6
#define JSON_FLOAT   7
#define JSON_PAIR   8
*/

SQInteger SqDBM::Sq2Get(HSQUIRRELVM v)
{
	const char* key;
	SqDBM* handle;
	
	if(SQ_SUCCEEDED(sq_getinstanceup(v,1,(SQUserPointer*)&handle,NULL))
		&& handle->pGDBM!=NULL 
		&& SqGetVar(v,key))
	{
		datum keyd;
		const char* pVal;
		int valSize;
		keyd.dptr=(char*)key;
		keyd.dsize=strlen(key);
		if(gdbm_fastget_key((gdbm_file_info*)handle->pGDBM,keyd,pVal,valSize))
		{
			sq_pushstring(v,pVal,valSize);
			return 1;
		}
	}
	return 0;
};

SQInteger SqDBM::Sq2Set(HSQUIRRELVM v)
{
	const char* key;
	const char* value;
	SqDBM* handle;

	if(SQ_SUCCEEDED(sq_getinstanceup(v,1,(SQUserPointer*)&handle,NULL))
		&& handle->pGDBM!=NULL )
	{
		datum keyd,vald;
		if(SqGetVar(v,key,value))
		{
			keyd.dptr=(char*)key;
			keyd.dsize=strlen(key);
			vald.dptr=(char*)value;
			vald.dsize=strlen(value);
			gdbm_store((gdbm_file_info*)handle->pGDBM,keyd,vald);
			return 0;
		}
		else if(SQ_SUCCEEDED(sq_getstring(v,-1,&key)))
		{
			SQObjectType type=sq_gettype(v,-1);
			if(type==OT_TABLE || OT_ARRAY)
			{
				handle->mLine.clear();
				if(type==OT_TABLE) fwrite_json_object(handle->mLine,v); 
				if(type==OT_ARRAY) fwrite_json_array(handle->mLine,v); 
				keyd.dptr=(char*)key;
				keyd.dsize=strlen(key);
				vald.dptr=handle->mLine.c_str();
				vald.dsize=handle->mLine.length();
				gdbm_store((gdbm_file_info*)handle->pGDBM,keyd,vald);
				return 0;
			}
		}

	}
	return 0;
}


