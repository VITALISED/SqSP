//#define _LIG_DEBUG

#include <lig_httpd.h>
#include <sdl_script_support.h>
#include <sdl_db_support.h>

extern http_sq* get_template_sq(const char* requested_page,const char* requested_template);
extern http_sq* get_page_sq(const char* requested_page);


ConnList m_connDB;  //ȫ������������ڴ�����
http_conn* Request_Table[0xffff]; //��¼FCGI Request_ID�ı�,��С�ǹ̶���
const char* p_root_dir="./root"; //������ȫ�ֵĸ�Ŀ¼
int global_polling_counter=0;  //������
time_t global_polling_clock=0; //ȫ��ʱ��
void* pCstrLock=NULL;          //ȫ�����������Ƿ���CSTR�ı��뷽ʽ���

set<url_map,less_url_map> RegDB; //ȫ�ֵĻص������¼��
list<http_conn*> m_connList;

lig_httpd *pHttpd(NULL);    //����������
int MAX_POSTLENGTH=2*1024*1024;  //����ϴ��ļ�Ϊ2M�����Զ�̬�ı����ֵ
char* pTagHead[]={"Connection: ","User-Agent: ","Content-Length: ","Content-Type: ",\
"If-Modified-Since: ","Authorization: ","Referer: ","Cookie: ","Location: ","Status: ","Range: "};
int TageHeadLength=sizeof(pTagHead)/sizeof(char*);

file_types types[] = {
	{"svg",		"image/svg+xml"			},
	{"torrent",	"application/x-bittorrent"	},
	{"wav",		"audio/x-wav"			},
	{"mp3",		"audio/x-mp3"			},
	{"mid",		"audio/mid"			},
	{"m3u",		"audio/x-mpegurl"		},
	{"ram",		"audio/x-pn-realaudio"		},
	{"ra",		"audio/x-pn-realaudio"		},
	{"doc",		"application/msword",		},
	{"exe",		"application/octet-stream"	},
	{"zip",		"application/x-zip-compressed"	},
	{"xls",		"application/excel"		},
	{"tgz",		"application/x-tar-gz"		},
	{"tar.gz",	"application/x-tar-gz"		},
	{"tar",		"application/x-tar"		},
	{"gz",		"application/x-gunzip"		},
	{"arj",		"application/x-arj-compressed"	},
	{"rar",		"application/x-arj-compressed"	},
	{"rtf",		"application/rtf"		},
	{"pdf",		"application/pdf"		},
	{"jpeg",	"image/jpeg"			},
	{"png",		"image/png"			},
	{"mpg",		"video/mpeg"			},
	{"mpeg",	"video/mpeg"			},
	{"asf",		"video/x-ms-asf"		},
	{"avi",		"video/x-msvideo"		},
	{"bmp",		"image/bmp"			},
	{"jpg",		"image/jpeg"			},
	{"gif",		"image/gif"			},
	{"ico",		"image/x-icon"			},
	{"txt",		"text/plain"			},
	{"css",		"text/css"			},
	{"htm",		"text/html"			},
	{"html",	"text/html"			},
	{NULL,		NULL				}
};
//-----------------------------------------------------------------
/*
* This code implements the MD5 message-digest algorithm.
* The algorithm is due to Ron Rivest.  This code was
* written by Colin Plumb in 1993, no copyright is claimed.
* This code is in the public domain; do with it what you wish.
*
* Equivalent code is available from RSA Data Security, Inc.
* This code has been tested against that, and is equivalent,
* except that you don't need to include two pages of legalese
* with every copy.
*
* To compute the message digest of a chunk of bytes, declare an
* MD5Context structure, pass it to MD5Init, call MD5Update as
* needed on buffers full of bytes, and then call MD5Final, which
* will fill a supplied 16-byte array with the digest.
*/

/*
* MD5 crypto algorithm.
*/
typedef struct _MD5Context {
	uint32_t	buf[4];
	uint32_t	bits[2];
	unsigned char	in[64];
} _MD5_CTX;

#if __BYTE_ORDER == 1234
#define byteReverse(buf, len)	/* Nothing */
#else
/*
* Note: this code is harmless on little-endian machines.
*/
static void byteReverse(unsigned char *buf, unsigned longs)
{
	uint32_t t;
	do {
		t = (uint32_t) ((unsigned) buf[3] << 8 | buf[2]) << 16 |
			((unsigned) buf[1] << 8 | buf[0]);
		*(uint32_t *) buf = t;
		buf += 4;
	} while (--longs);
}
#endif /* __BYTE_ORDER */

/* The four core functions - F1 is optimized somewhat */

/* #define F1(x, y, z) (x & y | ~x & z) */
#define F1(x, y, z) (z ^ (x & (y ^ z)))
#define F2(x, y, z) F1(z, x, y)
#define F3(x, y, z) (x ^ y ^ z)
#define F4(x, y, z) (y ^ (x | ~z))

/* This is the central step in the MD5 algorithm. */
#define MD5STEP(f, w, x, y, z, data, s) \
	( w += f(x, y, z) + data,  w = w<<s | w>>(32-s),  w += x )

/*
* Start MD5 accumulation.  Set bit count to 0 and buffer to mysterious
* initialization constants.
*/
static void _MD5Init(_MD5_CTX *ctx)
{
	ctx->buf[0] = 0x67452301;
	ctx->buf[1] = 0xefcdab89;
	ctx->buf[2] = 0x98badcfe;
	ctx->buf[3] = 0x10325476;

	ctx->bits[0] = 0;
	ctx->bits[1] = 0;
}

/*
* The core of the MD5 algorithm, this alters an existing MD5 hash to
* reflect the addition of 16 longwords of new data.  MD5Update blocks
* the data and converts bytes into longwords for this routine.
*/
static void _MD5Transform(uint32_t buf[4], uint32_t const in[16])
{
	register uint32_t a, b, c, d;

	a = buf[0];
	b = buf[1];
	c = buf[2];
	d = buf[3];

	MD5STEP(F1, a, b, c, d, in[0] + 0xd76aa478, 7);
	MD5STEP(F1, d, a, b, c, in[1] + 0xe8c7b756, 12);
	MD5STEP(F1, c, d, a, b, in[2] + 0x242070db, 17);
	MD5STEP(F1, b, c, d, a, in[3] + 0xc1bdceee, 22);
	MD5STEP(F1, a, b, c, d, in[4] + 0xf57c0faf, 7);
	MD5STEP(F1, d, a, b, c, in[5] + 0x4787c62a, 12);
	MD5STEP(F1, c, d, a, b, in[6] + 0xa8304613, 17);
	MD5STEP(F1, b, c, d, a, in[7] + 0xfd469501, 22);
	MD5STEP(F1, a, b, c, d, in[8] + 0x698098d8, 7);
	MD5STEP(F1, d, a, b, c, in[9] + 0x8b44f7af, 12);
	MD5STEP(F1, c, d, a, b, in[10] + 0xffff5bb1, 17);
	MD5STEP(F1, b, c, d, a, in[11] + 0x895cd7be, 22);
	MD5STEP(F1, a, b, c, d, in[12] + 0x6b901122, 7);
	MD5STEP(F1, d, a, b, c, in[13] + 0xfd987193, 12);
	MD5STEP(F1, c, d, a, b, in[14] + 0xa679438e, 17);
	MD5STEP(F1, b, c, d, a, in[15] + 0x49b40821, 22);

	MD5STEP(F2, a, b, c, d, in[1] + 0xf61e2562, 5);
	MD5STEP(F2, d, a, b, c, in[6] + 0xc040b340, 9);
	MD5STEP(F2, c, d, a, b, in[11] + 0x265e5a51, 14);
	MD5STEP(F2, b, c, d, a, in[0] + 0xe9b6c7aa, 20);
	MD5STEP(F2, a, b, c, d, in[5] + 0xd62f105d, 5);
	MD5STEP(F2, d, a, b, c, in[10] + 0x02441453, 9);
	MD5STEP(F2, c, d, a, b, in[15] + 0xd8a1e681, 14);
	MD5STEP(F2, b, c, d, a, in[4] + 0xe7d3fbc8, 20);
	MD5STEP(F2, a, b, c, d, in[9] + 0x21e1cde6, 5);
	MD5STEP(F2, d, a, b, c, in[14] + 0xc33707d6, 9);
	MD5STEP(F2, c, d, a, b, in[3] + 0xf4d50d87, 14);
	MD5STEP(F2, b, c, d, a, in[8] + 0x455a14ed, 20);
	MD5STEP(F2, a, b, c, d, in[13] + 0xa9e3e905, 5);
	MD5STEP(F2, d, a, b, c, in[2] + 0xfcefa3f8, 9);
	MD5STEP(F2, c, d, a, b, in[7] + 0x676f02d9, 14);
	MD5STEP(F2, b, c, d, a, in[12] + 0x8d2a4c8a, 20);

	MD5STEP(F3, a, b, c, d, in[5] + 0xfffa3942, 4);
	MD5STEP(F3, d, a, b, c, in[8] + 0x8771f681, 11);
	MD5STEP(F3, c, d, a, b, in[11] + 0x6d9d6122, 16);
	MD5STEP(F3, b, c, d, a, in[14] + 0xfde5380c, 23);
	MD5STEP(F3, a, b, c, d, in[1] + 0xa4beea44, 4);
	MD5STEP(F3, d, a, b, c, in[4] + 0x4bdecfa9, 11);
	MD5STEP(F3, c, d, a, b, in[7] + 0xf6bb4b60, 16);
	MD5STEP(F3, b, c, d, a, in[10] + 0xbebfbc70, 23);
	MD5STEP(F3, a, b, c, d, in[13] + 0x289b7ec6, 4);
	MD5STEP(F3, d, a, b, c, in[0] + 0xeaa127fa, 11);
	MD5STEP(F3, c, d, a, b, in[3] + 0xd4ef3085, 16);
	MD5STEP(F3, b, c, d, a, in[6] + 0x04881d05, 23);
	MD5STEP(F3, a, b, c, d, in[9] + 0xd9d4d039, 4);
	MD5STEP(F3, d, a, b, c, in[12] + 0xe6db99e5, 11);
	MD5STEP(F3, c, d, a, b, in[15] + 0x1fa27cf8, 16);
	MD5STEP(F3, b, c, d, a, in[2] + 0xc4ac5665, 23);

	MD5STEP(F4, a, b, c, d, in[0] + 0xf4292244, 6);
	MD5STEP(F4, d, a, b, c, in[7] + 0x432aff97, 10);
	MD5STEP(F4, c, d, a, b, in[14] + 0xab9423a7, 15);
	MD5STEP(F4, b, c, d, a, in[5] + 0xfc93a039, 21);
	MD5STEP(F4, a, b, c, d, in[12] + 0x655b59c3, 6);
	MD5STEP(F4, d, a, b, c, in[3] + 0x8f0ccc92, 10);
	MD5STEP(F4, c, d, a, b, in[10] + 0xffeff47d, 15);
	MD5STEP(F4, b, c, d, a, in[1] + 0x85845dd1, 21);
	MD5STEP(F4, a, b, c, d, in[8] + 0x6fa87e4f, 6);
	MD5STEP(F4, d, a, b, c, in[15] + 0xfe2ce6e0, 10);
	MD5STEP(F4, c, d, a, b, in[6] + 0xa3014314, 15);
	MD5STEP(F4, b, c, d, a, in[13] + 0x4e0811a1, 21);
	MD5STEP(F4, a, b, c, d, in[4] + 0xf7537e82, 6);
	MD5STEP(F4, d, a, b, c, in[11] + 0xbd3af235, 10);
	MD5STEP(F4, c, d, a, b, in[2] + 0x2ad7d2bb, 15);
	MD5STEP(F4, b, c, d, a, in[9] + 0xeb86d391, 21);

	buf[0] += a;
	buf[1] += b;
	buf[2] += c;
	buf[3] += d;
}

/*
* Update context to reflect the concatenation of another buffer full
* of bytes.
*/
static void
_MD5Update(_MD5_CTX *ctx, unsigned char const *buf, unsigned len)
{
	uint32_t t;

	/* Update bitcount */

	t = ctx->bits[0];
	if ((ctx->bits[0] = t + ((uint32_t) len << 3)) < t)
		ctx->bits[1]++;		/* Carry from low to high */
	ctx->bits[1] += len >> 29;

	t = (t >> 3) & 0x3f;	/* Bytes already in shsInfo->data */

	/* Handle any leading odd-sized chunks */

	if (t) {
		unsigned char *p = (unsigned char *) ctx->in + t;

		t = 64 - t;
		if (len < t) {
			memcpy(p, buf, len);
			return;
		}
		memcpy(p, buf, t);
		byteReverse(ctx->in, 16);
		_MD5Transform(ctx->buf, (uint32_t *) ctx->in);
		buf += t;
		len -= t;
	}
	/* Process data in 64-byte chunks */

	while (len >= 64) {
		memcpy(ctx->in, buf, 64);
		byteReverse(ctx->in, 16);
		_MD5Transform(ctx->buf, (uint32_t *) ctx->in);
		buf += 64;
		len -= 64;
	}

	/* Handle any remaining bytes of data. */

	memcpy(ctx->in, buf, len);
}

/*
* Final wrapup - pad to 64-byte boundary with the bit pattern 
* 1 0* (64-bit count of bits processed, MSB-first)
*/
static void
_MD5Final(unsigned char digest[16], _MD5_CTX *ctx)
{
	unsigned count;
	unsigned char *p;

	/* Compute number of bytes mod 64 */
	count = (ctx->bits[0] >> 3) & 0x3F;

	/* Set the first char of padding to 0x80.  This is safe since there is
	always at least one byte free */
	p = ctx->in + count;
	*p++ = 0x80;

	/* Bytes of padding needed to make 64 bytes */
	count = 64 - 1 - count;

	/* Pad out to 56 mod 64 */
	if (count < 8) {
		/* Two lots of padding:  Pad the first block to 64 bytes */
		memset(p, 0, count);
		byteReverse(ctx->in, 16);
		_MD5Transform(ctx->buf, (uint32_t *) ctx->in);

		/* Now fill the next block with 56 bytes */
		memset(ctx->in, 0, 56);
	} else {
		/* Pad block to 56 bytes */
		memset(p, 0, count - 8);
	}
	byteReverse(ctx->in, 14);

	/* Append length in bits and transform */
	((uint32_t *) ctx->in)[14] = ctx->bits[0];
	((uint32_t *) ctx->in)[15] = ctx->bits[1];

	_MD5Transform(ctx->buf, (uint32_t *) ctx->in);
	byteReverse((unsigned char *) ctx->buf, 4);
	memcpy(digest, ctx->buf, 16);
	memset((char *) ctx, 0, sizeof(ctx));	/* In case it's sensitive */
}

/*
* END OF LICENSED MD5 CODE (Sergey)
*/

/*
* Stringify binary data. Output buffer must be twice as big as input,
* because each byte takes 2 bytes in string representation
*/
static void
bin2str(char *to, const unsigned char *p, size_t len)
{
	const char	*hex = "0123456789abcdef";

	for (;len--; p++) {
		*to++ = hex[p[0] >> 4];
		*to++ = hex[p[0] & 0x0f];
	}
	*to = '\0';
}

/*
* Return stringified MD5 hash for list of vectors.
* buf must point to at least 32-bytes long buffer
*/
static void
md5(char *buf, ...)
{
	unsigned char		hash[16];
	const unsigned char	*p;
	va_list			ap;
	_MD5_CTX		ctx;

	_MD5Init(&ctx);

	va_start(ap, buf);
	while ((p = va_arg(ap, const unsigned char *)) != NULL)
		_MD5Update(&ctx, p, strlen((char *) p));
	va_end(ap);

	_MD5Final(hash, &ctx);
	bin2str(buf, hash, sizeof(hash));
}


void create_md5(char *buf, char *value) //Add This Function By Lig,2005-12-01
{
	unsigned char		hash[16];
	_MD5_CTX		ctx;
	_MD5Init(&ctx);
	_MD5Update(&ctx, (const unsigned char*)value, strlen(value));
	_MD5Final(hash, &ctx);
	bin2str(buf, hash, sizeof(hash));
};

void create_session_hash(char *buf, char *value) //Add This Function By Lig,2008-08-26
{
	int i;
	unsigned char		hash[16];
	static const char *tb="0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789";
	_MD5_CTX		ctx;
	_MD5Init(&ctx);
	_MD5Update(&ctx, (const unsigned char*)value, strlen(value));
	_MD5Final(hash, &ctx);

	for(i=0;i<16;i+=2)
	{
		char c1=hash[i]/4;
		char c2=(hash[i]%4)*16+hash[i+1]/16;
		char c3=hash[i+1]%64;
		*(buf++)=tb[c1];
		*(buf++)=tb[c2];
		*(buf++)=tb[c3];
	}
	*buf=0;

};
//-----------�ļ��ϴ���
bool sp_str_cmp(const char* sstr,int sstr_len,const char* dstr) //���ٵ��ַ����ȽϺ���
{
	for(int i=0;i<sstr_len;++i)
	{
		if(dstr[i]==0) return false;
		if(sstr[i]!=dstr[i]) return false;
	}
	return true;
}

const char* nocase_find_str(const char *from,const char *to,const char* str) //��ָ����Χ�ڲ��ִ�Сд�ҵ��ַ���������ֵ���ַ���ĩβλ��
{
	register const char *p=from;
	register const char *s=str;
	while(p!=to && *s!=0)
	{
		if(*p==*s)
		{
			++s;
			++p;
			continue;
		}
		else
		{
			if( (*p>='A' && *p<='Z' && *s>='a' && (*p+('a'-'A'))==*s) 
				||( *p>='a' && *s>='A' && *s<='Z' && (*s+('a'-'A'))==*p ) )
			{
				++s;
				++p;
				continue;
			}
		}
		++from;
		p=from;
		s=str;
		continue;
	}
	if(*s!=0) return NULL;
	return p;
};



int str_find(const char *s,int l1,const char *t,int l2)
{
	int i,j;
	for(i=0;i<l1-l2;++i)
	{
		for(j=0;j<l2;++j)
			if(*(s+i+j)!=(*t+j)) break;
		if(j==l2) return i;
	}
	return -1;
}

void KMP_init(const char* pat, int* next)
{
	int k;next[0] = -1;           //��һ��Ԫ�ص�next����-1, ��Ϊ����(1) , ���ǲ��Ҳ���һ��k��j=0С
	for(int i=1; i<strlen(pat); i++)
	{
		k = next[i-1];     //��Ϊ���õݹ�ķ���, Ϊ�˼���next[i], �ȼ�¼��next[i-1] ���Ҽ���next[i-1]��֪
		while(pat[k+1]!=pat[i] && k>=0)
		{ // �ݹ�
            k = next[k];
        }
        if(pat[k+1]==pat[i])     //�����ȶ�����, ���ҵ�һ�Գ���Ϊk��ǰ׺�ִ��ͺ�׺�ִ�
            next[i] = k+1;       //������һ����ͬ��
        else
            next[i] = 0;          //�������
    }
}

int KMP_find(const char *s,int l1,const char *t,int l2,int *next)
{
	
	int i, j = -1, ans = -1;
	for( i=0; i<l1; ++i )
	{
		while( (j+1>0) && (t[j+1]!=s[i]) )
		{
			j = next[j] ;
		}
		if( t[j+1]==s[i] )
		{
			++j ;
		}
		if( j+1==l2 )
		{
			ans = i-l2+1 ;
			break ;
		}
	}
	return ans;

} 
enum mime_hdr_type	{MIME_INT, MIME_STRING,MIME_QUOTE_STRING};

struct mime_header 
{
	int		id;		         /* Header name length		*/
	enum mime_hdr_type	type;		 /* Header type			*/
	const char	*name;		         /* Header name			*/
	const char  *value_str;
	int value_length;
}; 
mime_header mime_headers[] =
{
	{0, MIME_STRING, "Content-Disposition: ",NULL,0 },
	{1,  MIME_QUOTE_STRING, "name=",NULL,0 },
	{2,  MIME_QUOTE_STRING, "filename=",NULL,0},
	{3, MIME_STRING, "Content-Type: ",NULL,0 },	
	{-1,  MIME_INT,	  NULL,NULL,0}
};  //Ԥ�����

struct mime_data
{
	mime_header*  header;
	const char*   p_data;
	int data_length;

	mime_data()
	{
		header= mime_headers;
		p_data=NULL;
		data_length=-1;
	};

};



#define MIME_HEADER_LENGTH 5 //��Ҫ����MIME��ʾͷ���룬���涨���������4���˴�Ϊ4


//����ϴ��ļ�������ƱȽϼ򵥣���Щ������δ����
bool http_mime_request(const char *boundary_str,const char *buf,int buf_size,list<mime_data_pair> &mime_list);

//----------------------------------------------------------------------------------------------

struct session_item //ÿ��session�����ھ�����һ��ϵͳȫ��ҳ������ָ��
{
	int session_id; //session���ڲ���ţ���1��ʼ�Զ�������-1��ʾ����session
	char session_key[36]; //session���ַ����
	time_t time_stamp; //session��ʱ�꣬�����ж�Session�Ƿ�ʱ��Ч

	session_item()
	{
		session_id=0;
		time_stamp=0;
	}

	~session_item(){}

	const char *key_str(){return session_key;}
};

struct less_session_item
{
	bool operator()(const session_item &v1,const session_item &v2) const
	{
		return (strcmp(v1.session_key,v2.session_key)<0);
	};
};

void create_session_table(HSQUIRRELVM v,const char* session_str)
{
	SQInteger top = sq_gettop(v); //saves the stack size before the call
	sq_pushregistrytable(v);
	sq_pushstring(v,session_str,-1);
	sq_newtable(v);
	sq_createslot(v,-3);
	sq_settop(v,top); //restores the original stack size
}

bool push_session_table(HSQUIRRELVM v,const char* session_str)
{
	SQInteger top = sq_gettop(v); //saves the stack size before the call
	sq_pushregistrytable(v);
	sq_pushstring(v,session_str,-1);
	if(SQ_SUCCEEDED(sq_get(v,-2)) && sq_gettype(v,-1)==OT_TABLE)
	{
		sq_remove(v,-2);
		return true;
	}
	sq_settop(v,top); //restores the original stack size
	return false;
}

void delete_session_table(HSQUIRRELVM v,const char* session_str)
{
	SQInteger top = sq_gettop(v); //saves the stack size before the call
	sq_pushregistrytable(v);
	sq_pushstring(v,session_str,-1);
	sq_deleteslot(v,-2,SQFalse);
	sq_settop(v,top); //restores the original stack size
}

class session_manager //session�Ĺ�����
{

private:
	int session_id_counter;  //�����������ڼ�¼�Ѿ���������session����
	session_item tmp;
	set<session_item,less_session_item> session_database; //��¼session��������ݿ�
	int value_of_MAX_SESSION_ALIVE;
	char sbuf[64];

public:
	HSQUIRRELVM v;  //��httpd�ڳ�ʼ����ʱ�򣬸����������ʵ��

	session_manager()
	{
		session_id_counter=0;
		value_of_MAX_SESSION_ALIVE=1200; //ȱʡsessionʧЧʱ��Ϊ20����
	}

	~session_manager(){	};

	void set_session_alive(int t){value_of_MAX_SESSION_ALIVE=t;}

	session_item* create_session(int random_number) //����һ��session����
	{
		session_id_counter++;
		tmp.session_id=session_id_counter;
		unsigned int rdn=sdl_time::rand(); // ����� 
		snprintf(sbuf,64,"li_%d , %d_,_%d_gang_%d",session_id_counter,global_polling_counter,global_polling_clock,rdn);
		create_session_hash(tmp.session_key,sbuf);
		//���ݵ�ǰʱ�估����ֵ�����ֵ������MD5�㷨����һ��Ψһ�ַ���

		pair<set<session_item,less_session_item>::iterator,bool> rt;
		rt=session_database.insert(tmp); //������ַ�����ӵ��ڲ����ݼ�¼
		while(!rt.second) //�������ַ����Ѿ����ڣ�
		{
			session_id_counter++;
			tmp.session_id=session_id_counter;  //�仯session_id,�����ظ�session_key
			sprintf(sbuf,"%d , %d_,_%d",session_id_counter,global_polling_counter,global_polling_clock);
			create_session_hash(tmp.session_key,sbuf);;
			rt=session_database.insert(tmp); //������ַ�����ӵ��ڲ����ݼ�¼
		}

		create_session_table(v,tmp.session_key); //����һ��sq��session�� 

		int time_span;

		time_t *p_ts=(time_t*)(&((rt.first)->time_stamp));
		*p_ts=global_polling_clock;  //session�Ĵ���ʱ��, current_time defined in shttpd;

		//������Session��ʱ����Ҫ����Ƿ��й��ڵ�session,��ֹ��Чsession��פ�ڴ�
		set<session_item,less_session_item>::iterator it=session_database.begin();
		while(it!=session_database.end())
		{
			set<session_item,less_session_item>::iterator iit=it;++it;
			time_span=(int)(global_polling_clock-iit->time_stamp); //��ȡsession�ĳ���ʱ��,current_time defined in shttpd;
			//DOUT<<"Create Session Timespan="<<time_span<<endl;
			if(time_span>value_of_MAX_SESSION_ALIVE) //��ʱMAX_SESSION_ALIVE���ӣ�Session��Ч
			{

				delete_session_table(v,iit->session_key); //ɾ��session�����

				session_database.erase(iit); //ɾ����Чsession
			}
		}
		return (session_item *)&(*rt.first);  
	};

	const session_item* get_session(const char *session)
	{
		if(session==NULL) return NULL;
		strcpy(tmp.session_key,session);
		set<session_item,less_session_item>::iterator fd;
		fd=session_database.find(tmp);
		if(fd==session_database.end()) return NULL;

		time_t old_time;
		old_time=fd->time_stamp;
		*((time_t *)&(fd->time_stamp))=global_polling_clock;  //��session����ʱ��,current_time defined in shttpd
		int time_span=(int)(fd->time_stamp-old_time); //��ȡsession�ĳ���ʱ��

		if(time_span>value_of_MAX_SESSION_ALIVE) //��ʱ,Session��Ч
		{
			delete_session_table(v,fd->session_key); //ɾ��session�����
			session_database.erase(fd); //ɾ����Чsession
			return NULL; //���ؿ�ֵ
		}
		return &(*fd);
	};

	const session_item* operator[](const char *session)
	{
		return get_session(session);
	};

	void oper_time(const char *session) //�ı䵱ǰSession����ȡʱ�䣬�ӳ�Session����
	{
		const session_item* p=get_session(session);
		if(p!=NULL) *((time_t *)&(p->time_stamp))=global_polling_clock; //current time is defined in shttpd
	};

	int get_time_span(const char *session) //-1 ��ʾ��session��Ч,�鿴session��ʱЧ
	{
		time_t old_time;
		const session_item* p=get_session(session);
		if(p!=NULL)
		{
			old_time=p->time_stamp;
			*((time_t *)&(p->time_stamp))=global_polling_clock;
			return (int)(p->time_stamp-old_time);
		}
		return -1;
	};

	void delete_session(const char *session) //ɾ��һ��session
	{
		strcpy(tmp.session_key,session);
		set<session_item,less_session_item>::iterator fd;
		fd=session_database.find(tmp);

		if(fd!=session_database.end()) 
		{
			delete_session_table(v,session); //ɾ��session�����
			int tmp_session_id=fd->session_id;//׼��ɾ����session�м�¼�ĸ�������
			session_database.erase(fd);
		}
	};
};

session_manager AppData;

/*
bool run_script(HSQUIRRELVM v,string &script) 
{
	SQInteger retval=0;
	SQInteger oldtop=sq_gettop(v);
	sq_pushroottable(v);
	if(SQ_SUCCEEDED(sq_compilebuffer(v,script.data(),script.length(),_SC("inner function"),SQTrue)))
	{	
		sq_push(v,-2); //����this,�Ա㴴����Ҫ�ĺ������󣬴˴���thisΪroot
		if(SQ_SUCCEEDED(sq_call(v,1,retval,SQTrue)))
		{
			sq_settop(v,oldtop);
			return true;
		}
	}
	sq_settop(v,oldtop);
	return false;
};
*/

void http_conn::init(lig_httpd* ps)
{
	constructor(&mlist); //����ִ��mlist�Ĺ���
	constructor(&header); //����ִ��header�Ĺ���
	inBuf.init();
	outBuf.init(); 
	mFiles=NULL;
	pServer=ps;
	if(TageHeadLength>15) 
	{
		throw "header buf has defined less than 16 items";
		exit(0);
	}

	time_stamp=global_polling_clock; //��¼����ʱ��ʱ��
	link_stat=read_ready; //��ʼ״̬
	flag_header_ok=false;
	flag_json_request=false;
	flag_file_upload=false; //��δ�ļ��ϴ�
	flag_uri_translate=false; //����Ҫ�����ļ�ת��
	post_length=-1;
	header_length=-1;
	pUrlMap=NULL;

	action=&http_conn::action_UNKNOWN;
	for(int i=0;i<16;++i) pHeader[i]=NULL;
}

inline void http_conn::printf(char* fmt,...)
{
	int nWrite;
	va_list	ap;
	va_start(ap, fmt);
	nWrite=vsnprintf(outBuf.p_buf+outBuf.buf_curr,outBuf.buf_size-outBuf.buf_curr,fmt,ap);
	va_end(ap);
	while(nWrite<0 && outBuf.resize())  //�ڴ�ռ䲻�������·����ڴ棬����д
	{
		va_start(ap, fmt);
		nWrite=vsnprintf(outBuf.p_buf+outBuf.buf_curr,outBuf.buf_size-outBuf.buf_curr,fmt,ap);
		va_end(ap);
	}
	outBuf.buf_curr+=nWrite;
};

inline bool http_conn::ncasecmp(const char* str1,const char* str2)
{
	if(str1==NULL || str2==NULL) return false;
	while(((*str1>='a' && *str2<='z')?(*str1-'a'+'A'):*str1)==((*str2>='a' && *str2<='z')?(*str2-'a'+'A'):*str2) && *str1!=0 && *str2!=0){++str1;++str2;}
	if(*str1==0) return true;
	return false;
}


int http_conn::montoi(const char *s) //����shttpd�ĺ���
{
	static const char *ar[] = {
		"Jan", "Feb", "Mar", "Apr", "May", "Jun",
		"Jul", "Aug", "Sep", "Oct", "Nov", "Dec"
	};
	size_t	i;

	for(i = 0;i < 12;i++)
		if(strcmp(s,ar[i])==0) return (i);
	return (-1);
}

time_t http_conn::datetosec(const char *s) //����shttpd�ĺ���
{
	struct tm tm;
	char mon[32];
	int sec, min, hour, mday, month, year;

	(void) memset(&tm, 0, sizeof(tm));
	sec = min = hour = mday = month = year = 0;

	if (((sscanf(s, "%d/%3s/%d %d:%d:%d",
		&mday, mon, &year, &hour, &min, &sec) == 6) ||
		(sscanf(s, "%d %3s %d %d:%d:%d",
		&mday, mon, &year, &hour, &min, &sec) == 6) ||
		(sscanf(s, "%*3s, %d %3s %d %d:%d:%d",
		&mday, mon, &year, &hour, &min, &sec) == 6) ||
		(sscanf(s, "%d-%3s-%d %d:%d:%d",
		&mday, mon, &year, &hour, &min, &sec) == 6)) &&
		(month = montoi(mon)) != -1) {
			tm.tm_mday	= mday;
			tm.tm_mon	= month;
			tm.tm_year	= year;
			tm.tm_hour	= hour;
			tm.tm_min	= min;
			tm.tm_sec	= sec;
	}

	if (tm.tm_year > 1900)
		tm.tm_year -= 1900;
	else if (tm.tm_year < 70)
		tm.tm_year += 100;

	return (mktime(&tm));
};

bool http_conn::create_getfile_header()
{
	char fnbuf[256];

	mFiles=NULL;
	strcpy(fnbuf,p_root_dir);
	if(fnbuf[strlen(p_root_dir)-1]!='/' || fnbuf[strlen(p_root_dir)-1]!='\\')
	{
		fnbuf[strlen(p_root_dir)]='/';
		fnbuf[strlen(p_root_dir)+1]=0;
	}

	strcat(fnbuf,header.pUri); 

	if(stat(fnbuf,&file_state)!=0) 
	{
		send_http_msg(404,"Not Found","", "Not Found: %s",header.pUri);
		return false;
	}
	FileLength = file_state.st_size;


	if(pHeader[ID_If_Modified_Since]!=NULL) //����GET
	{
		time_t mt=datetosec(pHeader[ID_If_Modified_Since]);
		if(file_state.st_mtime<=mt)
		{
			//DOUT<<"\nFile Not Modified\n"<<endl;
			send_http_msg(304,"Not Modified","", "Not Modified: %s",header.pUri);
			return false;
		}
	}

	mFiles=fopen(fnbuf,"rb");
	if(mFiles==NULL)
	{
		//DOUT<<"OpenFile : "<<fnbuf<<" Failed!"<<endl;
		send_http_msg(404,"Not Found","", "Not Found: %s",header.pUri);
		return false;
	}
	int fnlength=strlen(fnbuf);
	while(fnlength>0 && fnbuf[fnlength]!='.') --fnlength;
	file_types* p=types;
	while(p->ext!=NULL)
	{
		if(strcmp(p->ext,fnbuf+fnlength+1)==0) break; 
		++p;
	}


	//׼����Ҫ���ļ�ͷ
	char date[64],lm[64],etag[64];
	const char* fmt = "%a, %d %b %Y %H:%M:%S GMT";
	strftime(date, sizeof(date), fmt, gmtime(&global_polling_clock));
	strftime(lm, sizeof(lm), fmt, gmtime(&(file_state.st_mtime)));
	snprintf(etag, sizeof(etag), "%lx.%lx",(unsigned long)file_state.st_mtime,(unsigned long) file_state.st_size);
	const char* mime=(p->type==NULL)?"application/octet-stream":p->type; 
	//д���ļ�ͷ
	outBuf.printf("HTTP/1.1 200 OK\r\nDate: %s\r\nLast-Modified: %s\r\nEtag: \"%s\"\r\nContent-Type: %s\r\nContent-Length: %lu\r\nConnection: Keep-Alive\
				  Server: WebServer,Written by LiGang;\r\n\r\n",
				  date, lm, etag, mime,FileLength);
	return true;
}

bool http_conn::action_UNKNOWN()
{
	link_stat=read_finished; //�����˴���״̬����˲��ټ�����
	send_http_msg(400,"Bad Request","", "Bad Request");
	return true;
}

bool http_conn::action_GET()
{
	link_stat=read_finished; //�����˴���״̬����˲��ټ�����
	if(create_getfile_header()) return write_file();
	return true;
}

bool http_conn::action_POST()
{
	link_stat=read_finished; //�����˴���״̬����˲��ټ�����
	
	send_http_msg(200,"OK","", "POST OK: %s");
	//send_http_msg(200,"OK","", "POST OK: %s",inBuf.p_buf+post_data_offset);
	return true;
}

bool http_conn::action_HEAD()
{

	link_stat=read_finished; //�����˴���״̬����˲��ټ�����
	create_getfile_header();
	if(mFiles==NULL) fclose(mFiles); //����ļ����򿪣��ر��ļ�
	return true;
}


void http_conn::send_http_msg(int status,const char *descr,const char *headers,const char *fmt, ...)
{
	va_list	ap;

	outBuf.buf_curr=snprintf(outBuf.p_buf,IO_SIZE,"HTTP/1.1 %d %s\r\n%s%s\r\n%d ",
		status,descr, headers, headers[0]=='\0'?"":"\r\n",status);
	va_start(ap, fmt);
	outBuf.buf_curr+=vsnprintf(outBuf.p_buf+outBuf.buf_curr,IO_SIZE-outBuf.buf_curr,fmt,ap);

	va_end(ap);
	link_stat=write_ready; //�Ѿ�׼����д��������

};


bool http_conn::write_file()
{
	if(FileLength<IO_SIZE-outBuf.buf_curr) //�ļ�ʣ�ಿ�ֱȻ�����ҪС
	{
		if(fread(outBuf.p_buf+outBuf.buf_curr,1,FileLength,mFiles)!=FileLength)
		{
			fclose(mFiles);
			mFiles=NULL;
			outBuf.buf_curr=0;
			send_http_msg(500,"Error","", "Error at read : %s",header.pUri);
			return false;
		}
		outBuf.buf_curr+=FileLength;
		link_stat=write_ready; //׼��д������
		fclose(mFiles);
		mFiles=NULL;
		return true;
	}
	else //�ļ��Ȼ���������д��һ����
	{
		int nRead=IO_SIZE-outBuf.buf_curr;
		if(fread(outBuf.p_buf+outBuf.buf_curr,1,nRead,mFiles)!=nRead)
		{
			fclose(mFiles);
			mFiles=NULL;
			outBuf.buf_curr=0;
			send_http_msg(500,"Error","", "Error at read : %s",header.pUri);
			return false;
		}
		outBuf.buf_curr+=nRead;
		FileLength-=nRead;
		link_stat=write_ready; //׼��д������
		return true;
	}

}

void lig_httpd::insert_request(int my_sock) 
{
		if(max_fd<my_sock) max_fd=my_sock;
		http_conn* pconn=m_connDB.alloc(); //���һ����������
		m_connList.push_back(pconn); 
		pconn->init(this); 
		pconn->socket_id=my_sock;
		//DOUT<<"join socket ="<<pconn->socket_id<<" size="<<m_connList.size()<<endl;
}


bool http_conn::parse_fastcgi() //��Ҫ����һ��requestIdӳ�������һ�������
{
	if(inBuf.buf_curr-inBuf.buf_start<sizeof(FCGI_Header)){link_stat=read_ready;return false;} //�����ֽڹ��٣��ȴ���������
	

	FCGI_BeginRequestRecord *pBeginRequestRecord;
	while(inBuf.buf_start<inBuf.buf_curr)
	{
		FCGI_Header *pHeader=(FCGI_Header*)(inBuf.p_buf+inBuf.buf_start);

		if(pHeader->version!=1) {link_stat=conn_finished;return false;} //�����FCGI�汾����ȷ����������
		int contentLength=pHeader->contentLengthB1*256+pHeader->contentLengthB0;
		int paddingLength=pHeader->paddingLength;
		int recordLength=contentLength+paddingLength+sizeof(FCGI_Header);
		int requestId=pHeader->requestIdB1*256+pHeader->requestIdB0;
		if(inBuf.buf_curr-inBuf.buf_start<recordLength) {link_stat=read_ready;return false;} //��δ����һ����¼���ȴ���������

		switch(pHeader->type)
		{
		case FCGI_BEGIN_REQUEST:
			pBeginRequestRecord=(FCGI_BeginRequestRecord*)(inBuf.p_buf+inBuf.buf_start);
			if(Request_Table[requestId]==NULL) //����һ���µ�����
			{
				int role=pBeginRequestRecord->body.roleB1*256+pBeginRequestRecord->body.roleB0;
				if(role!=FCGI_RESPONDER) //Ŀǰ�����ܹ�����FCGI_RESPONDER
				{
					link_stat=conn_finished; //Ҫ��ر�����
					return false;
				}
				
				Request_Table[requestId]=m_connDB.alloc(); //���һ����������
				Request_Table[requestId]->init(pServer); //��ʼ������  
				Request_Table[requestId]->socket_id=requestId; //�������д�뵽sock_id�У�δ��ʹ��
				Request_Table[requestId]->para_length=0; //���������ȳ�ʼ��
				Request_Table[requestId]->flag_fcgi=fgci_begin_request;
				if(pBeginRequestRecord->body.flags==FCGI_KEEP_CONN) //WebServerҪ�󱣳�����
				{
					Request_Table[requestId]->flag_fcgi_close_state=false; 
				}
				else //����Ӧ������ɴ����Ժ󣬶Ͽ�����
				{
					Request_Table[requestId]->flag_fcgi_close_state=true; 
				}
				inBuf.buf_start+=recordLength;//����Ѵ�������
				
			}
			else //����һ���Ѿ����ڵ��������Ǵ���
			{
				link_stat=conn_finished; //Ҫ��ر�����
				return false;
			}
			break;
		case FCGI_ABORT_REQUEST:
			break;
		case FCGI_END_REQUEST:
			break;
		case FCGI_PARAMS:
			if(Request_Table[requestId]==NULL) //����һ���µ�����,����Ӧ����Ϊ����
			{
				link_stat=conn_finished; //Ҫ��ر�����
				inBuf.buf_curr=0;
				inBuf.buf_start=0; //����Ѿ���ʹ�õ����� 
				return false;
			}
			Request_Table[requestId]->flag_fcgi=fcgi_read_client;
			 //���Ƕ�Ӧ�Ĵ��������Ҫ�����ݷַ��Ķ�Ӧ�Ĵ������
			{
				char *pData=(inBuf.p_buf+inBuf.buf_start)+sizeof(FCGI_Header); 
				Request_Table[requestId]->inBuf.direct_write(pData,contentLength); //�ַ�����Ӧ�Ĵ������  
				Request_Table[requestId]->para_length+=contentLength; //��¼�������Ĵ�С
				inBuf.buf_start+=recordLength;//����Ѵ�������
			
			}
			if(contentLength==0) //û��PARAMS����PARAMS�Ѿ�������ϣ���Ҫ���ӿհ�λ
			{
				Request_Table[requestId]->inBuf.direct_write("\0x00\0x00\0x00\0x00\0x00",4); //д��һ���ָ�
				Request_Table[requestId]->get_fastcgi_params(); //�������FASTCGI��������
				Request_Table[requestId]->post_data_offset = (Request_Table[requestId]->inBuf.buf_curr); //�õ�POSTӦ���ϴ���λ�ã�ע�⣬�������λ�ã���ֹinBuf�Զ������Ժ�������ڴ��ַ�仯
			}
			break;
		case FCGI_STDIN:
			if(Request_Table[requestId]==NULL) //����һ���µ�����,����Ӧ����Ϊ����
			{
				link_stat=conn_finished; //Ҫ��ر�����
				inBuf.buf_curr=0;
				inBuf.buf_start=0; //����Ѿ���ʹ�õ����� 
				return false;
			}
			 //���ǲ��Ƕ�Ӧ�Ĵ��������Ҫ�����ݷַ��Ķ�Ӧ�Ĵ������
			if(contentLength!=0) 
			{
				char *pData=(inBuf.p_buf+inBuf.buf_start)+sizeof(FCGI_Header); 
				Request_Table[requestId]->inBuf.direct_write(pData,contentLength); //�ַ�����Ӧ�Ĵ������  
				Request_Table[requestId]->flag_fcgi=fcgi_read_client;
				inBuf.buf_start+=recordLength;
			
			}
			else //û��STDIN����STDIN�Ѿ�������ϣ���Ҫ������Ӧ
			{
				inBuf.buf_start+=recordLength;
				if(Request_Table[requestId]->flag_fcgi!=fcgi_read_client)
				{
					//webserver ��������������ϣ����ٴ����˳�
					break;
				}
				Request_Table[requestId]->link_stat=read_finished; //�Ѿ���FCGI�ж�����ȫ����Ҫ�����ݣ�������������趨�����־λ 
				if(Request_Table[requestId]->flag_file_upload) //�ͻ���Ҫ���ϴ��ļ���������д���
				{
					const char* pPostData=Request_Table[requestId]->inBuf.p_buf+Request_Table[requestId]->post_data_offset;
					const char* boundarystr=Request_Table[requestId]->pHeader[ID_Content_Type];
					if(boundarystr==NULL) break; //û����ȷ�õ�content_type
					string bs;bs.append(boundarystr,Request_Table[requestId]->fcgi_content_type_length);
					boundarystr=bs.c_str();
					boundarystr=nocase_find_str(boundarystr+20,boundarystr+strlen(boundarystr),"boundary=");
					if(boundarystr!=NULL) 
						http_mime_request(boundarystr,pPostData,Request_Table[requestId]->post_length,Request_Table[requestId]->mlist);
					
					//DOUT<<"Client upload file!!"<<endl;
				}else if(!Request_Table[requestId]->flag_json_request && Request_Table[requestId]->action==&http_conn::action_POST && Request_Table[requestId]->post_length>0)	
					Request_Table[requestId]->header.decode_post(Request_Table[requestId]->inBuf.p_buf+Request_Table[requestId]->post_data_offset,Request_Table[requestId]->post_length); //����POST���ı�

				url_map* pRt=NULL;
				if(Request_Table[requestId]->pServer->check_register(Request_Table[requestId]->header.pUri,pRt)) //����Ƿ���ִ�д������
				{
					Request_Table[requestId]->pUrlMap=pRt;
					Request_Table[requestId]->pServer->VMthread.push_excuting(Request_Table[requestId]);  //�����̶߳��У�����������Ӧ�Ĵ���
				}
				else
				{
					Request_Table[requestId]->send_http_msg(404,"Not Found","", "Not Found: %s",Request_Table[requestId]->header.pUri);
					Request_Table[requestId]->link_stat=write_ready; //���һ������
					
				}
			
				link_stat=read_finished; //�Ѿ����һ�����룬��֪��Ҫ���к�������
				Request_Table[requestId]->flag_fcgi=fcgi_read_finished;
			}
			break;
		case FCGI_STDOUT:
			break;
		case FCGI_STDERR:
			break;
		case FCGI_DATA:
			break;
		case FCGI_GET_VALUES:
			break;
		case FCGI_GET_VALUES_RESULT:
			break;
		case FCGI_UNKNOWN_TYPE :
			break;
		}

	}
	inBuf.buf_curr=0;
	inBuf.buf_start=0; //����Ѿ���ʹ�õ����� 
	return true;

}

bool http_conn::parse_header()
{
	int it=0,i=0;
	post_data_offset=-1;
	flag_json_request=false;
	
	if(inBuf.buf_curr>3) for(i=0;i<inBuf.buf_curr-3;++i)
	{

		if((inBuf.p_buf[i]==13 &&  inBuf.p_buf[i+1]==10 &&
			inBuf.p_buf[i+2]==13 &&  inBuf.p_buf[i+3]==10 )) //\r\n\r\nģʽ��header,����������headerͷ
		{
			post_data_offset=i+4;
			header_length=i+4; //��¼����ͷ�ĳ���
		}
		if((inBuf.p_buf[i]==10 &&  inBuf.p_buf[i+1]==10)) //\n\nģʽ��header,��Ҫ����Ϊ�����Բ���������
		{
			post_data_offset=i+2;
			header_length=i+2; //��¼����ͷ�ĳ���
		}

		if(header_length>0)
		{

			char* pLine=inBuf.p_buf; 
			int j=0;
			for(;j<i;++j) if(inBuf.p_buf[j]=='\n'){ inBuf.p_buf[j]=0;break;}

			if(ncasecmp("GET",pLine)) action=&http_conn::action_GET;
			if(ncasecmp("POST",pLine)) action=&http_conn::action_POST;
			if(ncasecmp("HEAD",pLine)) action=&http_conn::action_HEAD;

			char *pURI=pLine;
			while(*pURI!=32 && *pURI!=0) ++pURI;
			while(*pURI==32 && *pURI!=0) ++pURI;
			char *pType=pURI;
			while(*pType!=32 && *pType!=0) ++pType;
			*pType=0;++pType;

			this->header.decode_url(pURI);

			pLine=inBuf.p_buf+j+1;
			for(;j<header_length+4;++j) //Ϊ�˵õ����һ�У�������ļ�ͷ��
			{
				if(inBuf.p_buf[j]=='\r')
				{
					inBuf.p_buf[j]=0;
					it=0;while(it<TageHeadLength && !ncasecmp(pTagHead[it],pLine)) ++it;
					if(it<TageHeadLength) 
					{
						pHeader[it]=pLine+strlen(pTagHead[it]);//ƥ��ɹ�����������
						if(it==ID_Content_Length) 
							post_length=atoi(pHeader[ID_Content_Length]); //�õ��ϴ�Ҫ�󳤶�
					}
					pLine=inBuf.p_buf+j+2;
				}

			}
			if(pHeader[ID_Content_Type]!=NULL && ncasecmp("application/jsonrequest",pHeader[ID_Content_Type]))
				flag_json_request=true; 
			
			flag_header_ok=true;
			
			return true;
		}
	}
	return false;
}

bool http_conn::read_client() //����false��ʾ���ٶ�������Ӧ�ü������ö�
{
	if(!inBuf.read_socket(socket_id)) //��socket����
	{
		link_stat=conn_finished; //״ָ̬ʾ��Ҫ��ر�����
		return false;
	}

	if(!flag_header_ok) parse_header();

	if(flag_header_ok)//�Ѿ�����������ͷ
	{
		
		if(action==&http_conn::action_POST)
		{
			if(post_length!=-1)
			{
				if(inBuf.buf_size<post_length)
					inBuf.resize(post_length); //������Ҫ���仺�� 
				if(post_length>MAX_POSTLENGTH)
				{
					link_stat=conn_finished; //״ָ̬ʾ��Ҫ��ر�����
					return false;
				}

				if(inBuf.buf_curr==post_length+header_length)
				{
					if(pHeader[ID_Content_Type]!=NULL && ncasecmp("multipart/form-data",pHeader[ID_Content_Type])) //�������ļ��ϴ�
					{
						flag_file_upload=true; 
						const char* boundarystr=nocase_find_str(pHeader[ID_Content_Type]+20,pHeader[ID_Content_Type]+strlen(pHeader[ID_Content_Type]),"boundary=");
						if(boundarystr!=NULL) 
							http_mime_request(boundarystr,inBuf.p_buf+post_data_offset,post_length,mlist);
					}
					else if(!flag_json_request)	
						header.decode_post(inBuf.p_buf+post_data_offset,post_length); //����POST���ı�
					link_stat=read_finished; //״ָ̬ʾ�����ͻ����Ѿ����
					return false;
				}
			}
			else
			{
				link_stat=conn_finished; //״ָ̬ʾ��Ҫ��ر�����
				return false;
			
			}
		}
		else
		{
			link_stat=read_finished; //״ָ̬ʾ�����ͻ����Ѿ����
			return false;
		}
	}
	time_stamp=global_polling_clock;  //��Ȼ�ڶ�������ʱ��
	return true;
}

bool http_conn::write_client()
{
	int rt=outBuf.write_socket(socket_id);
	if(rt==-1) //���ִ��������Ҫ��ֹ�������
	{
		link_stat=conn_finished; //״ָ̬ʾ��Ҫ��ر�����
		return false;
	}
	time_stamp=global_polling_clock; 
	if(rt==1) //�����Ѿ�д��
	{
		if(mFiles!=NULL && FileLength==0)//���ļ�д,�����Ѿ�д��
		{
			fclose(mFiles);
			mFiles=NULL;
			link_stat=conn_finished; //״ָ̬ʾ��Ҫ��ر�����
			return true;//������ֹ
		}
		if(mFiles!=NULL && FileLength>0)//���ļ�д,��Ҫ����д
		{
			outBuf.buf_start=0; 
			int nRead=(FileLength>IO_SIZE)?IO_SIZE:FileLength;
			if(fread(outBuf.p_buf,1,nRead,mFiles)!=nRead) 
			{
				link_stat=conn_finished; //״ָ̬ʾ��Ҫ��ر�����
				return false; //���������������ֹ����
			}
			link_stat=write_ready; //״ָ̬ʾ����ʾ��Ҫ���������Ƿ��д
			FileLength-=nRead;
			outBuf.buf_curr=nRead;
			return true;
		}
		//д��һ������
		outBuf.buf_start=0; 
		link_stat=conn_finished; //״ָ̬ʾ��Ҫ��ر�����
		return true;//������ֹ
	}
	return true;
}

void http_conn::read_fcgi_server() //����false��ʾ���ٶ�������Ӧ�ü������ö�
{
	if(!inBuf.read_socket(socket_id)) //��socket����
	{
		link_stat=conn_finished; //״ָ̬ʾ��Ҫ��ر�����
		return;
	}
	parse_fastcgi();
	
}

int http_conn::write_fcgi_server()
{
	int rt=outBuf.write_socket(socket_id);
	if(rt==-1) //���ִ��������Ҫ��ֹ�������
	{
		link_stat=conn_finished; //״ָ̬ʾ��Ҫ��ر�����
		return rt;
	}
	time_stamp=global_polling_clock; 
	if(rt==1) //�����Ѿ�д��
	{
		//д��һ������
		outBuf.buf_start=0; 
		outBuf.buf_curr=0; 
		link_stat=write_finished; //״ָ̬ʾ��д����
		return rt;
	}
	link_stat=write_ready;
	return rt;
}

static int my_atoi(char * pstr,int leng)
{
	int val=0;
	while(leng>0 && *pstr!=0)
	{
		if(*pstr>='0' && *pstr<='9')
		{
			val=val*10+(*pstr)-'0';
			--leng;++pstr;
		}
	}
	return val;
};
void  http_conn::get_fastcgi_params() //�õ�FCGI�Ļ�������ֵ
{
	unsigned char* buf=(unsigned char*)(inBuf.p_buf); 
	unsigned char* bufend=buf + para_length;
	 
	VARS tmp,tmpUri,tmpSession;
	tmpSession.value_length=0;
	tmpUri.value_length=0;

	while(buf != bufend)
	{
		if (*buf >> 7 == 0)
			tmp.name_length  = *(buf++);
		else
		{
			tmp.name_length = ((buf[0] & 0x7F) << 24) + (buf[1] << 16) + (buf[2] << 8) + buf[3];
			buf += 4;
		}
		if (*buf >> 7 == 0)
			tmp.value_length = *(buf++);
		else
		{
			tmp.value_length = ((buf[0] & 0x7F) << 24) + (buf[1] << 16) + (buf[2] << 8) + buf[3];
			buf += 4;
		}
		tmp.name=(char*)buf; 
	
		buf += tmp.name_length;
		tmp.value=(char*)buf;
		buf += tmp.value_length ;
		if(sp_str_cmp(tmp.name,tmp.name_length,"REQUEST_METHOD") )
		{
			if(sp_str_cmp(tmp.value,tmp.value_length,"GET"))  
				action=&http_conn::action_GET;  
			if(sp_str_cmp(tmp.value,tmp.value_length,"POST"))  
				action=&http_conn::action_POST; 
			if(sp_str_cmp(tmp.value,tmp.value_length,"HEAD"))  
				action=&http_conn::action_HEAD; 
			continue; //�ڲ���������д�������
		}
		if(sp_str_cmp(tmp.name,tmp.name_length,"REQUEST_URI") )
		{
			tmpUri=tmp;
			header.pUri=tmp.value+1;  
			continue; //�ڲ���������д�������
		}
		if(sp_str_cmp(tmp.name,tmp.name_length,"CONTENT_LENGTH") )
		{
			this->post_length=my_atoi(tmp.value,tmp.value_length);  
		}
		if(sp_str_cmp(tmp.name,tmp.name_length,"CONTENT_TYPE")) 
		{
			if(sp_str_cmp(tmp.value,23,"application/jsonrequest"))
				flag_json_request=true; 
		
			if(sp_str_cmp(tmp.value,19,"multipart/form-data"))
			{
				flag_file_upload=true;  //���ļ��ϴ�
				pHeader[ID_Content_Type]=tmp.value; //�õ��ϴ���CONTENT_TYPE
				fcgi_content_type_length=tmp.value_length;  
			}
			
		}
		if(sp_str_cmp(tmp.name,tmp.name_length,"_csp_key") )
		{
			tmpSession=tmp;
			header.pSession=tmp.value;  
			continue; //�ڲ���������д�������
		}
		header.varList.push_back(tmp); //д�� FASTCGI�Ļ�������  
	}
	if(tmpUri.value_length>0) header.pUri[tmpUri.value_length-1]=0;  //�õ�c���� URI�ַ���
	if(tmpSession.value_length>0) header.pUri[tmpSession.value_length]=0;  //�õ�c���� URI�ַ���
	header.decode_url(header.pUri); //����url
	
}

bool http_poll::write_callback_head(http_conn* pConn)
{
	char date[64];
	const char* fmt = "%a, %d %b %Y %H:%M:%S GMT";
	strftime(date, sizeof(date), fmt, gmtime(&global_polling_clock));

	if(m_connList.size()<MAX_KEEP_ALIVE)
	{
		pConn->outBuf.printf("HTTP/1.1 200 OK\r\nContent-Length:            \r\nDate: %s\r\nContent-Type: text/html;charset=UTF-8\r\nConnection: Keep-Alive\r\nServer: FastWebServer,Written by LiGang\r\n\r\n",
			date);
		return false;
	}
	else
	{
		pConn->outBuf.printf("HTTP/1.1 200 OK\r\nContent-Length:            \r\nDate: %s\r\nContent-Type: text/html;charset=UTF-8\r\nConnection: Close\r\nServer: FastWebServer,Written by LiGang\r\n\r\n",
			date);
		return true;
	}
}

bool http_poll::write_json_head(http_conn* pConn)
{
	pConn->outBuf.direct_write("HTTP/1.1 200 OK\r\nContent-Type: application/jsonrequest\r\nContent-Length:            \r\n\r\n");
	return true;
}

bool http_poll::create_script(const char* function_name,string &script) 
{
//	macro_state(script); //����״̬������
	SQInteger retval=0;
	SQInteger oldtop=sq_gettop(v);
	sq_pushroottable(v);
	sq_pushstring(v,function_name,-1);
	if(!SQ_SUCCEEDED(sq_get(v,-2))) //���û���ҵ���Ҫ���࣬����һ����
	{
		sq_pushstring(v,function_name,-1);
		sq_newtable(v);
		sq_createslot(v,-3); //�������࣬�ص�roottable
		sq_pushstring(v,function_name,-1);
		sq_get(v,-2); //�ҳ��Ѿ���������
	}
	if(sq_gettype(v,-1)==OT_TABLE && 
		SQ_SUCCEEDED(sq_compilebuffer(v,script.data(),script.length(),_SC(function_name),SQTrue)))
	{
		sq_push(v,-2); //����this,�Ա㴴����Ҫ�ĺ������󣬴˴���thisΪ�ҵ���class
		if(SQ_SUCCEEDED(sq_call(v,1,retval,SQTrue)))
		{
			sq_settop(v,oldtop);

			return true;
		}
	}
	sq_settop(v,oldtop);
	return false;

}

bool http_poll::set_nonblock(int sock_id)
{

#ifdef	_WIN32
	unsigned long on = 1;
	return (ioctlsocket(sock_id,FIONBIO,&on)==0);
#else
	int	flags;
	if ((flags = fcntl(sock_id, F_GETFL, 0))!=-1 && fcntl(sock_id, F_SETFL, flags | O_NONBLOCK)==0) return true;
	return false;
#endif 
}

bool http_poll::register_global_func(SQFUNCTION f,const char *fname,const char* para_mask)
{
	SQInteger top = sq_gettop(v); //saves the stack size before the call
	sq_pushroottable(v);
	sq_pushstring(v,fname,-1);
	sq_newclosure(v,f,0); //create a new function without a free var
	if(para_mask!=NULL) sq_setparamscheck(v,strlen(para_mask),para_mask); 
	sq_createslot(v,-3); 
	sq_settop(v,top); //restores the original stack size
	return true;
}

bool http_poll::squirrel_excute_session_function(http_conn* pConn,const char* requested_function) //ͨ�ûص�����
{
	page_buf &io=pConn->outBuf;	
		
	CpuClock.start_clock(); 

	int top=sq_gettop(v);
	sq_pushroottable(v);
	
	if(pConn->header.pSession==NULL)
	{	
		if(AUTO_LOGIN_URI!="")
		{
			pConn->outBuf.clear();  
			pConn->printf("HTTP/1.1 307 Temporary Redirect\r\nLocation: %s\r\nServer: FastWebServer by Lig\r\n\r\nTemporary Redirect",AUTO_LOGIN_URI.c_str());
			pConn->link_stat=write_ready; 
			sq_settop(v,top);
			return false; //�������������󲻴���,����ִ�з���ű�,����
		}
		else //������AUTO_KOGIN_URI���Զ�����һ��Session
		{
			session_item *pS=AppData.create_session((int)pConn); //�����
			pConn->header.pSession=pS->session_key;
		}

	}
	
	sq_pushstring(v,requested_function,-1);
	if(SQ_SUCCEEDED(sq_get(v,-2)) && sq_gettype(v,-1)==OT_CLOSURE) //�ҵ���Ҫҳ���ִ�к���
	{
		sq_push(v,-2); //push the 'this' (in this case is the global table)
		sq_pushstring(v,pConn->header.pSession,-1); //����session��key�ַ��� 

		if(!push_session_table(v,pConn->header.pSession)) //ѹ��һ��session�����ڴ洢session��������
		{
			pConn->outBuf.clear();  
			pConn->printf("HTTP/1.1 307 Temporary Redirect\r\nLocation: %s\r\nServer: FastWebServer by Lig\r\n\r\nTemporary Redirect",AUTO_LOGIN_URI.c_str());
			pConn->link_stat=write_ready; 
			sq_settop(v,top);
			return false; //�������������󲻴���,����ִ�з���ű�,���� 
		}
		sq_pushobject(v,tObjectHttpConn);
		sq_setinstanceup(v,-1,(SQUserPointer)pConn);		
		
		if(SQ_FAILED(sq_call(v,4,SQFalse,SQTrue)))
		{
			io.direct_write("*--Squirrel Script runtime error : callback failed!--*<br>") ;
			io.direct_write("Request callback function = ");
			io.direct_write(requested_function);

		}
	}
	else
	{
		io.direct_write("*--Squirrel Script runtime error : Cann't find requested callback class!--*<br>");
		io.direct_write("Request callback function = ");
		io.direct_write(requested_function);

	}
	sq_settop(v,top);
#ifdef _LIG_DEBUG
	DOUT<<"TimeSpan="<<CpuClock.get_timespan()<<endl; 
#endif
	return true;
}

bool http_poll::squirrel_excute_function(http_conn* pConn,const char* requested_function) //ͨ�ûص�����
{
	page_buf &io=pConn->outBuf;	
	CpuClock.start_clock(); 
	int top=sq_gettop(v);
	sq_pushroottable(v);
	sq_pushstring(v,requested_function,-1);
	if(SQ_SUCCEEDED(sq_get(v,-2)) && sq_gettype(v,-1)==OT_CLOSURE) //�ҵ���Ҫҳ���ִ�к���
	{
		sq_push(v,-2); //push the 'this' (in this case is the global table)
		sq_pushstring(v,"no session..",-1); //����session��key�ַ��� 
		sq_pushnull(v); //ѹ��һ��null��Ϊsession��
		sq_pushobject(v,tObjectHttpConn);
		sq_setinstanceup(v,-1,(SQUserPointer)pConn);
					
		if(SQ_FAILED(sq_call(v,4,SQFalse,SQTrue))) //ִ��squirrel����
		{
			io.direct_write("*--Squirrel Script runtime error : callback failed!--*<br>") ;
			io.direct_write("Request callback function = ");
			io.direct_write(requested_function);
		}
		
	}
	else
	{
		io.direct_write("*--Squirrel Script runtime error : Cann't find requested callback function!--*<br>");
		io.direct_write("Request callback function = ");
		io.direct_write(requested_function);
	}
	sq_settop(v,top);
#ifdef _LIG_DEBUG
	DOUT<<"TimeSpan="<<CpuClock.get_timespan()<<endl; 
#endif
	return true;
}

bool http_poll::squirrel_callback(http_conn* pConn,const char* requested_page) //ͨ�ûص�����
{

	http_sq* psrv; //����������ҳ�����ݵ������������		
	page_buf &io=pConn->outBuf;	
	//��ҳ���¼���ݿ��У���������Ҫ��ҳ�棬���û�У����ش���
	if(requested_page!=NULL && (psrv=get_page_sq(requested_page))!=NULL)
		psrv->pHttp = pConn; 
	else
	{
		pConn->send_http_msg(304,"Not Found","","Not Found requested  %s",requested_page);
		return false;
	} 

	
	CpuClock.start_clock(); 

	int top=sq_gettop(v);
	sq_pushroottable(v);
	sq_pushstring(v,requested_page,-1);
	if(SQ_SUCCEEDED(sq_get(v,-2)) && sq_gettype(v,-1)==OT_TABLE) //�ҵ���Ҫҳ��ļ�¼��
	{
		sq_pushstring(v,requested_page,-1);
		if(SQ_SUCCEEDED(sq_get(v,-2)) && sq_gettype(v,-1)==OT_CLOSURE) //�ҵ���Ҫҳ���ִ�к���
		{
			sq_push(v,-2); //push the 'this' (in this case is the global table)

			if(pConn->header.pSession==NULL)
				sq_pushstring(v,"no session..",12); //����session��key�ַ��� 
			else
				sq_pushstring(v,pConn->header.pSession,-1); //����session��key�ַ��� 

			if(pConn->header.pSession==NULL || !push_session_table(v,pConn->header.pSession)) //ѹ��һ��session�����ڴ洢session��������
				sq_pushnull(v); //ѹ��һ��null��Ϊsession��
		
			sq_pushobject(v,tObjectHttpSq);//ѹ�������������
			sq_setinstanceup(v,-1,(SQUserPointer)psrv);
			
			io.direct_write("<script src=\"JSR.js\" type=\"text/javascript\"></script>");
			io.direct_write("<script language=\"JavaScript\">\n");
			io.direct_write("server.session_str=\"");
			io.direct_write("Haha..havn't session"); //ѹ��Session Pageʹ�õ�Session
			io.direct_write("\"</script>\n");
			if(psrv->javascript_code!="")
			{
				io.direct_write("<script language=\"JavaScript\">\n");
				io.direct_write(psrv->javascript_code.data(),psrv->javascript_code.length()); 
				io.direct_write("</script>\n");
			}

			if(SQ_FAILED(sq_call(v,4,SQFalse,SQTrue)))
			{
				io.direct_write("*--Squirrel Script runtime error : callback failed!--*<br>") ;
				io.direct_write("Request callback function = ");
				io.direct_write(requested_page);

			}
		}
		else
		{
			io.direct_write("*--Squirrel Script runtime error : Cann't find requested callback function!--*<br>");
			io.direct_write("Request callback function = ");
			io.direct_write(requested_page);
		}
	}
	else
	{
		io.direct_write("*--Squirrel Script runtime error : Cann't find requested callback class!--*<br>");
		io.direct_write("Request callback function = ");
		io.direct_write(requested_page);

	}
	sq_settop(v,top);
	int tsp=CpuClock.get_timespan();
#ifdef _LIG_DEBUG
	DOUT<<"TimeSpan="<<tsp<<endl; 
#endif
	return true;
}

bool http_poll::squirrel_session_callback(http_conn* pConn,const char* requested_page) //ͨ�ûص�����
{

	http_sq* psrv; //����������ҳ�����ݵ������������		
	page_buf &io=pConn->outBuf;	
	//��ҳ���¼���ݿ��У���������Ҫ��ҳ�棬���û�У����ش���
	if(requested_page!=NULL && (psrv=get_page_sq(requested_page))!=NULL)
		psrv->pHttp = pConn; 
	else
	{
		pConn->send_http_msg(304,"Not Found","","Not Found requested  %s",requested_page);
		return false;
	} 

	CpuClock.start_clock(); 


	int top=sq_gettop(v);
	sq_pushroottable(v);
	
	const char *url=AUTO_LOGIN_URI.c_str(); 
	if(pConn->header.pSession==NULL)
	{	
		if(AUTO_LOGIN_URI.length()>0)
		{
			pConn->outBuf.clear();  
			pConn->printf("HTTP/1.1 307 Temporary Redirect\r\nLocation: %s\r\nServer: FastWebServer by Lig\r\n\r\nTemporary Redirect",url);
			pConn->link_stat=write_ready; 
			sq_settop(v,top);
			return false; //�������������󲻴���,����ִ�з���ű�,����
		}
		else //������AUTO_KOGIN_URI���Զ�����һ��Session
		{
			session_item *pS=AppData.create_session((int)pConn); //�����
			pConn->header.pSession=pS->session_key;
		}


	}
	sq_settop(v,top+1);
	sq_pushstring(v,requested_page,-1);
	if(SQ_SUCCEEDED(sq_get(v,-2)) && sq_gettype(v,-1)==OT_TABLE) //�ҵ���Ҫҳ��ļ�¼��
	{
		sq_pushstring(v,requested_page,-1);
		if(SQ_SUCCEEDED(sq_get(v,-2)) && sq_gettype(v,-1)==OT_CLOSURE) //�ҵ���Ҫҳ���ִ�к���
		{
			sq_push(v,-2); //push the 'this' (in this case is the global table)
			
			sq_pushstring(v,pConn->header.pSession,-1); //����session��key�ַ��� 

			if(!push_session_table(v,pConn->header.pSession)) //ѹ��һ��session�����ڴ洢session��������
			{
				pConn->outBuf.clear();  
				pConn->printf("HTTP/1.1 307 Temporary Redirect\r\nLocation: %s\r\nServer: FastWebServer by Lig\r\n\r\nTemporary Redirect",url);
				pConn->link_stat=write_ready; 
				sq_settop(v,top);
				return false; //�������������󲻴���,����ִ�з���ű�,���� 
			}
				
			sq_pushobject(v,tObjectHttpSq);//ѹ�������������
			sq_setinstanceup(v,-1,(SQUserPointer)psrv);

			io.direct_write("<script src=\"JSR.js\" type=\"text/javascript\"></script>");
			io.direct_write("<script language=\"JavaScript\">\n");
			io.direct_write("server.session_str=\"");
			io.direct_write(pConn->header.pSession); //ѹ��Session Pageʹ�õ�Session
			io.direct_write("\"</script>\n");
			if(psrv->javascript_code!="")
			{
				io.direct_write("<script language=\"JavaScript\">\n");
				io.direct_write(psrv->javascript_code.c_str(),psrv->javascript_code.length()); 
				io.direct_write("</script>\n");
			}

			if(SQ_FAILED(sq_call(v,4,SQFalse,SQTrue)))
			{
				io.direct_write("*--Squirrel Script runtime error : callback failed!--*<br>") ;
				io.direct_write("Request callback function = ");
				io.direct_write(requested_page);

			}
		}
		else
		{
			io.direct_write("*--Squirrel Script runtime error : Cann't find requested callback function!--*<br>");
			io.direct_write("Request callback function = ");
			io.direct_write(requested_page);
		}
	}
	else
	{
		io.direct_write("*--Squirrel Script runtime error : Cann't find requested callback class!--*<br>");
		io.direct_write("Request callback function = ");
		io.direct_write(requested_page);

	}
	sq_settop(v,top);
#ifdef _LIG_DEBUG
	DOUT<<"TimeSpan="<<CpuClock.get_timespan()<<endl; 
#endif
	return true;
}

void http_poll::vm_exec(http_conn* pConn)
{
	url_map* rt;
	if(pConn->link_stat!=read_finished) return; //�Ѿ�ִ�е����ӣ����ٷ��������ִ�нű�
	bool flag_close_socket=false;
	if(pConn->flag_uri_translate) //Ҫ�����URIת��
	{
		const char* pURI=pConn->header.pUri;
		SqSetInstanceUP(v,tObjecthttp_header,(SQUserPointer)&(pConn->header));
		call_function(v,pURI,sqUriTranslate,tObjecthttp_header,pConn->header.pUri); //ִ�нű����������õ�����ֵ
	}
	if(pConn->pUrlMap!=NULL) //ִ�� Squirrel�ص�����
	{
		rt=(url_map*)pConn->pUrlMap;
		if(pConn->flag_json_request) 
		{
			write_json_head(pConn); //׼���ļ�ͷ
			char *pHeadBuf=pConn->outBuf.p_buf+72;  //�պõ�Content-Length
			int WrBytesCounter=pConn->outBuf.buf_curr; 
			if((this->*(rt->pJosnRequest))(pConn,rt->squirrel_function_callback_name))
			{
				*(pHeadBuf+snprintf(pHeadBuf,16,"%d",pConn->outBuf.buf_curr-WrBytesCounter))=0x20; //д��һ���ո񣬸���0x00�Ľ�β����
				
			}//����ִ���Ƿ�ɹ�����Ҫ�������
			pConn->link_stat=write_ready; //��Ӧ��Ӧ�����Ѿ�׼���ã����Է�������
		}
		else
		{
			flag_close_socket=write_callback_head(pConn); //׼���ļ�ͷ
			int WrBytesCounter=pConn->outBuf.buf_curr; 
			if(rt->pCCallback!=NULL)  //���ȿ���C�ص�
			{
				(*(rt->pCCallback))(pConn,rt->squirrel_function_callback_name);
				char *pHeadBuf=pConn->outBuf.p_buf+33;  //�պõ�Content-Length
				*(pHeadBuf+snprintf(pHeadBuf,16,"%d",pConn->outBuf.buf_curr-WrBytesCounter))=0x20; //д��һ���ո񣬸���0x00�Ľ�β����
				pConn->link_stat=write_ready; //��Ӧ��Ӧ�����Ѿ�׼���ã����Է�������
				return;
			}
			if((this->*(rt->pCallback))(pConn,rt->squirrel_function_callback_name))
			{
				char *pHeadBuf=pConn->outBuf.p_buf+33;  //�պõ�Content-Length
				*(pHeadBuf+snprintf(pHeadBuf,16,"%d",pConn->outBuf.buf_curr-WrBytesCounter))=0x20; //д��һ���ո񣬸���0x00�Ľ�β����
				pConn->link_stat=write_ready; //��Ӧ��Ӧ�����Ѿ�׼���ã����Է�������
			}
		}

	}
	else
	{
		pConn->pUrlMap=NULL;
		(pConn->*(pConn->action))(); //ִ��ȱʡ�������
		return;
	}

	if(flag_close_socket) pConn->link_stat=conn_finished; 
}



void http_poll::begin()
{
	bool flag;
	const char *url="";
	flag=SqCreateInstance(v,"AST",tObjectAST);
	flag&=SqCreateInstance(v,"http_sq",tObjectHttpSq); 
	flag&=SqCreateInstance(v,"http_conn",tObjectHttpConn); 
	flag&=SqCreateInstance(v,"http_header",tObjecthttp_header);//��ʼ����Ҫ��Squirrel����ʵ�� 
	flag&=get_function(v,"URITranslate",sqUriTranslate);       //Ѱ�ҳ�ʼ���ű��е�URL���뺯��
	sq_pushroottable(v);
	sq_pushstring(v,"AUTO_LOGIN_URI",-1);
	if(SQ_SUCCEEDED(sq_get(v,-2))) sq_getstring(v,-1,&url);
	AUTO_LOGIN_URI=url;
	AppData.v=v;
	while(flag)
	{
		excute_lock.wait();  //�������������д���в�Ϊ�յ�ʱ�����̲߳Ż��ͷ�������д����Ϊ�յ�ʱ�򣬴˴�����
#ifdef _LIG_DEBUG	
		DOUT<<"Excuting Scripts.."<<endl;
#endif
		while(!m_que.empty())
		{
			vm_exec(m_que.front()); //���������ִ��
			m_que.erase(m_que.begin());   
		}
	
	}
		
	
};


bool http_poll::json_callback(http_conn* pConn,const char* requested_page) //����Session�Ļص�����
{
	
	int parameter_number=4;
	http_sq* psrv; //����������ҳ�����ݵ������������		
	page_buf &io=pConn->outBuf;			
	//��ҳ���¼���ݿ��У���������Ҫ��ҳ�棬���û�У����ؿ�ҳ
	if((psrv=get_page_sq(requested_page))!=NULL) psrv->pHttp=pConn; 
	else
	{
		pConn->send_http_msg(304,"Not Found","","Not Found requested  %s",requested_page);
		return false;
	} 
	int top=sq_gettop(v);
	int real_post= pConn->inBuf.buf_curr - pConn->post_data_offset;
	if(pConn->post_length>0 && pConn->post_length==real_post)
	{
		pConn->inBuf.p_buf[pConn->post_data_offset + pConn->post_length]=0;  
	}
	else
	{
#ifdef _LIG_DEBUG
		DOUT<<"POST LENGTH error! post_length="<<pConn->post_length<<" real_post="<<real_post<<endl;
#endif
		pConn->send_http_msg(304,"Not Found","","POST FAILED size=%s",real_post);
                return false;
	}
#ifdef _LIG_DEBUG
	DOUT<<"JSONRequest : "<<pConn->inBuf.p_buf + pConn->post_data_offset<<endl;
#endif


	CpuClock.start_clock();
	sq_pushroottable(v);
	sq_pushstring(v,requested_page,-1);
	if(SQ_SUCCEEDED(sq_get(v,-2)) && sq_gettype(v,-1)==OT_TABLE) //�ҵ���Ҫҳ��ļ�¼��
	{
		int tabIdx=sq_gettop(v);
		const char *pJsonTxt=(const char*)(pConn->inBuf.p_buf + pConn->post_data_offset);
		if(tjson.json_parse(pJsonTxt))
		{
			Scan* ss=tjson.pRoot->pChild;//���ݱ���Ŀ��Ajax�ϴ�����淶����˳���ͬ��get("session");
			if(ss==NULL) {sq_settop(v,top);	return false;}
			Scan* rt=ss->pBrother;;  //���ݱ���Ŀ��Ajax�ϴ�����淶����˳���ͬ��get("server_func");
			if(rt!=NULL)
			{
				sq_pushstring(v,rt->start,(rt->end-rt->start)); //ѹ������ĺ�������
				if(SQ_SUCCEEDED(sq_get(v,tabIdx)) && sq_gettype(v,-1)==OT_CLOSURE) //�õ�����ĺ���
				{
					sq_push(v,tabIdx); //push "this"
					Scan* ct=rt->pBrother;//���ݱ���Ŀ��Ajax�ϴ�����淶����˳���ͬ��get("client")
					if(ct==NULL) 
						parameter_number=3; //û�д��ݲ���
					else
					{
						sq_pushobject(v,tObjectAST);//ѹ�������������
						sq_setinstanceup(v,-1,(SQUserPointer)ct);
					}
						
					AppData.get_session(ss->start);  //����ϴ���session���session���ڣ���ʱ��
					if(!push_session_table(v,ss->start)) //׼��ѹ��session��
						sq_pushnull(v); //���û��session��ѹ��һ���յ� session
					
					sq_pushobject(v,tObjectHttpSq);//ѹ�������������
					sq_setinstanceup(v,-1,(SQUserPointer)psrv);
					
					io.direct_write("{\"session\":\"");
					io.direct_write(ss->start,ss->controled_size);
					io.direct_write("\",\"action\":[");
					if(!SQ_SUCCEEDED(sq_call(v,parameter_number,SQFalse,SQTrue))) //ִ��json�ص�sq����
					{
						sq_settop(v,top);
						return false;
					}
					if(io.back()==',') io.pop(); //ɾ������Ҫ�Ķ���
					io.direct_write("]}"); 
					int time_count=CpuClock.get_timespan();
#ifdef _LIG_DEBUG
					DOUT<<"Jsonrequest TimeSpan="<<time_count<<endl;
#endif
					
				}
				else
				{
					io.direct_write("*--Squirrel Script runtime error : Cann't find requested jsoncallback function!--*<br>");
					io.direct_write("Request callback function = ");
					io.direct_write(requested_page);
				}
			}
		}
		else //�Ҳ�����Ҫ��json�ص�����
		{
			io.direct_write("*--Squirrel Script runtime error : Cann't find requested jsoncallback function!--*<br>");
			io.direct_write("Request callback function = ");
			io.direct_write(requested_page);
		}
	}
	sq_settop(v,top);
	return true;
};


SQInteger http_sq::Sq2Constructor(HSQUIRRELVM v) //��׼��Squirrel C++�����췽ʽ
{
	int top=sq_gettop(v);
	SQUserPointer theHandle;
	if(SqGetVar(v,theHandle))
	{
		http_sq* pobj=(http_sq*)(theHandle); //�Ӷ�ջ�л�ȡC++�����ָ��
		if(!pobj) return sq_throwerror(v,"Create instance failed");
		sq_setinstanceup(v,1,theHandle); //Ϊ��ʵ������C++����ָ��,��ʵ���ڶ�ջ�ĵײ�������idx ʹ��1
	}
	return 0;
}


SQInteger http_sq::Sq2GetVars(HSQUIRRELVM v)
{
	http_sq *self = NULL;
	sq_getinstanceup(v,1,(SQUserPointer *)&self,0); //��׼�Ļ�ȡC++����ķ�ʽ
	if(self==NULL) return 0;

	sq_newtable(v); //����һ��sq table;
	vector<VARS>::iterator it=self->pHttp->header.varList.begin();
	while(it!=self->pHttp->header.varList.end())
	{
		if(it->name==NULL) break;
		if(sp_str_cmp(it->name,it->name_length,"_csp_key")==0) continue; //�ڲ��ֶΣ����������
		sq_pushstring(v,it->name,it->name_length);
		sq_pushstring(v,it->value,it->value_length); //���ϴ��ı���д��sq table
		sq_createslot(v,-3);
		++it;
	}
	return 1; //���ذ���ȫ��
}

SQInteger http_sq::Sq2Get(HSQUIRRELVM v)
{
	int top=sq_gettop(v);
	http_sq *self = NULL;
	sq_getinstanceup(v,1,(SQUserPointer *)&self,0); //��׼�Ļ�ȡC++����ķ�ʽ

	top=sq_gettop(v);
	const char* svalue;
	sq_getstring(v,-1,&svalue);

	vector<VARS>::iterator it=self->pHttp->header.varList.begin();
	while(it!=self->pHttp->header.varList.end())
	{
		if(sp_str_cmp(it->name,it->name_length,svalue))
		{
			sq_settop(v,top);
			sq_pushstring(v,it->value,it->value_length);
			return 1;
		}
		++it;
	}
	sq_settop(v,top);
	sq_pushnull(v);
	return 1;
}

SQInteger http_sq::Sq2Write(HSQUIRRELVM v)
{
	int top=sq_gettop(v);
	const char* svalue;
	int ivalue;
	float fvalue;
	http_sq *self = NULL;
	sq_getinstanceup(v,1,(SQUserPointer *)&self,0); //��׼�Ļ�ȡC++����ķ�ʽ

	top=sq_gettop(v);
	if(self->flag_switch) //���ƿ��أ������Ƿ���clientд��
		for(int i=2;i<=top;++i)
		{
			switch(sq_gettype(v,i))
			{
			case OT_NULL:
				break;
			case OT_INTEGER:
				sq_getinteger(v,i,&ivalue);
				self->pHttp->printf("%d",ivalue);
				break;
			case OT_FLOAT:
				sq_getfloat(v,i,&fvalue);
				self->pHttp->printf("%f",fvalue);
				break;
			case OT_STRING:
				sq_getstring(v,i,&svalue);
				if(pCstrLock!=NULL)
					self->pHttp->outBuf.write_cstr(svalue);
				else
					self->pHttp->outBuf.direct_write(svalue);
				break;  
			case OT_USERPOINTER:
				sq_getuserpointer(v,i,(SQUserPointer*)&svalue);
				if(pCstrLock!=NULL)
					self->pHttp->outBuf.write_cstr(svalue);
				else
					self->pHttp->outBuf.direct_write(svalue);
				break;  
			default:
				return sq_throwerror(v,"invalid param"); //throws an exception
			}
		}
		return 0;
}

SQInteger http_sq::Sq2WriteHandle(HSQUIRRELVM v)
{
	int top=sq_gettop(v);
	http_sq *self = NULL;
	sq_getinstanceup(v,1,(SQUserPointer *)&self,0); //��׼�Ļ�ȡC++����ķ�ʽ

	top=sq_gettop(v);
	int hnd;
	if(self->flag_switch) //���ƿ��أ������Ƿ���clientд��
	{
		sq_getinteger(v,-1,&hnd);
		const char* str=_get_string(hnd);
		if(str!=NULL) self->pHttp->outBuf.direct_write(str);
	}
	return 0;
}

SQInteger http_sq::Sq2IdxWrite(HSQUIRRELVM v) //���ذ�������д���Ķ�����ַ���
{
	http_sq *pSR = NULL;
	sq_getinstanceup(v,1,(SQUserPointer *)&pSR,0); //��׼�Ļ�ȡC++����ķ�ʽ
	if(pSR->pScan==NULL) return 0; 

	size_t wIdx; //д�ֶε�����
	sq_getinteger(v,-1,(int*)(&wIdx)); //�õ���Ҫ�Ĳ���
	if(wIdx>=pSR->thePosIdx.size()) return 0; 
	Scan* ps=pSR->thePosIdx[wIdx];
	int htmlcode_length=0;
	if(ps->pChild!=NULL) htmlcode_length= ps->controled_size - ps->pChild->controled_size; 
	if(pSR->flag_switch && htmlcode_length>0) //���ƿ��أ������Ƿ���clientд��
	{
		if(pCstrLock!=NULL)
			pSR->pHttp->outBuf.write_cstr(ps->start,htmlcode_length); //�����д��
		else
			pSR->pHttp->outBuf.direct_write(ps->start,htmlcode_length); //ֱ��д��
	}
	if(ps->pChild!=NULL)
		pSR->pSegment=ps->pChild; //���������������������ַ���
	else
		pSR->pSegment=ps;

	//pSR->pLastStr=ps->end; //�������һ��δ���з������ַ��� 
	//pSR->pScan = ps->pBrother; 
	return 0; 
}

SQInteger http_sq::Sq2SegmentWrite(HSQUIRRELVM v) //����һ�����������ز���ָ��������ַ���
{
	http_sq *pSR = NULL;
	sq_getinstanceup(v,1,(SQUserPointer *)&pSR,0); //��׼�Ļ�ȡC++����ķ�ʽ
	if(pSR->pSegment==NULL) return 0; 
	const char* svalue;
	sq_getstring(v,-1,&svalue); //�õ���Ҫ�Ĳ���
	Scan* pSeg=pSR->pSegment->get(svalue);  

	if(pSeg==NULL) return 0;
	int htmlcode_length= pSeg->controled_size - pSeg->pChild->controled_size; 
	if(pSR->flag_switch) //���ƿ��أ������Ƿ���clientд��
	{
		if(pCstrLock!=NULL)
			pSR->pHttp->outBuf.write_cstr(pSeg->start,htmlcode_length); //�����Ժ�д��
		else
			pSR->pHttp->outBuf.direct_write(pSeg->start,htmlcode_length); //ֱ��д��
	}
	return 0; 
}

SQInteger http_sq::Sq2LastWrite(HSQUIRRELVM v) //д�����ַ���
{
	http_sq *pSR = NULL;
	sq_getinstanceup(v,1,(SQUserPointer *)&pSR,0); //��׼�Ļ�ȡC++����ķ�ʽ
	if(pSR->pLastStr==NULL) return 0; 
	if(pCstrLock!=NULL)
		pSR->pHttp->outBuf.write_cstr(pSR->pLastStr); //�����д��
	else
		pSR->pHttp->outBuf.direct_write(pSR->pLastStr); //ֱ��д��
	return 0; 
}

SQInteger http_sq::Sq2InnerClose(HSQUIRRELVM v) //�����
{
	http_sq *pSR = NULL;
	sq_getinstanceup(v,1,(SQUserPointer *)&pSR,0); //��׼�Ļ�ȡC++����ķ�ʽ
	pSR->flag_switch=false;
	return 0; 
}

SQInteger http_sq::Sq2InnerOpen(HSQUIRRELVM v) //�ر����
{
	http_sq *pSR = NULL;
	sq_getinstanceup(v,1,(SQUserPointer *)&pSR,0); //��׼�Ļ�ȡC++����ķ�ʽ
	pSR->flag_switch=true;
	return 0; 
}

SQInteger http_sq::Sq2GetTemplate(HSQUIRRELVM v) //��ȡһ��ģ��
{
	http_sq *pSR = NULL;
	sq_getinstanceup(v,1,(SQUserPointer *)&pSR,0); //��׼�Ļ�ȡC++����ķ�ʽ
	if(pSR->pLastStr==NULL) return 0; 
	const char* template_name;

	SQInteger top = sq_gettop(v);;
	if(SQ_SUCCEEDED(sq_getstring(v,-1,&template_name)) && template_name!=NULL) //�Ӷ�ջ�л�ȡһ������
	{
		http_sq *pTemplate=get_template_sq(pSR->function_name,template_name);
		if(pTemplate!=NULL)
		{
			pTemplate->pHttp=pSR->pHttp;  
			sq_pushroottable(v); //push the root table(were the globals of the script will be stored)
			sq_pushstring(v,"http_sq",-1);
			if(SQ_SUCCEEDED(sq_get(v,-2)) && sq_gettype(v,-1)==OT_CLASS)
			{
				if(SQ_SUCCEEDED(sq_createinstance(v,-1)))
				{
					sq_remove(v,-2);//ȥ������Ҫ��class
					sq_remove(v,-2); //ȥ������Ҫ��roottable
					sq_setinstanceup(v,-1,SQUserPointer(pTemplate)); //Ϊ��ʵ������C++����ָ��
					return 1; //���ض���ʵ��
				}
			}
		}
	}
	sq_settop(v,top); //restores the original stack size
	sq_pushnull(v);  //����һ���ն���
	return 1; 
}

SQInteger http_sq::Sq2AjaxValue(HSQUIRRELVM v)
{
	http_sq *pSR = NULL;
	sq_getinstanceup(v,1,(SQUserPointer *)&pSR,0); //��׼�Ļ�ȡC++����ķ�ʽ
	if(pSR==NULL) return 0; 
	const char* id;
	const char* value;

	if(SQ_SUCCEEDED(sq_getstring(v,-1,&value)) &&
		SQ_SUCCEEDED(sq_getstring(v,-2,&id)))
	{
		pSR->pHttp->outBuf.direct_write("{\"id\":\"",7);
		pSR->pHttp->outBuf.write_cstr(id);
		pSR->pHttp->outBuf.direct_write("\",\"value\":\"",11);
		pSR->pHttp->outBuf.write_cstr(value); 
		pSR->pHttp->outBuf.direct_write("\"}",2);
	}

	SQObjectType tp=sq_gettype(v,-1);
	if(sq_gettype(v,-1)==OT_INTEGER &&
		SQ_SUCCEEDED(sq_getstring(v,-2,&id)))
	{
		int val;sq_getinteger(v,-1,&val);
		pSR->pHttp->outBuf.direct_write("{\"id\":\"",7);
		pSR->pHttp->outBuf.write_cstr(id);
		pSR->pHttp->outBuf.direct_write("\",\"value\":\"",11);
		pSR->pHttp->outBuf.printf("%d",val);  
		pSR->pHttp->outBuf.direct_write("\"}",2);
	}

	if(sq_gettype(v,-1)==OT_FLOAT &&
		SQ_SUCCEEDED(sq_getstring(v,-2,&id)))
	{
		float val;sq_getfloat(v,-1,&val);
		pSR->pHttp->outBuf.direct_write("{\"id\":\"",7);
		pSR->pHttp->outBuf.write_cstr(id);
		pSR->pHttp->outBuf.direct_write("\",\"value\":\"",11);
		pSR->pHttp->outBuf.printf("%f",val);  
		pSR->pHttp->outBuf.direct_write("\"}",2);
	}

	if(tp==OT_TABLE &&
		SQ_SUCCEEDED(sq_getstring(v,-2,&id)))
	{
		int table_idx=sq_gettop(v);
		const char* value;
		const char* key;
		sq_pushnull(v); //���������
		while(SQ_SUCCEEDED(sq_next(v,table_idx)))
		{
			if(sq_gettype(v,-2)==OT_STRING)
			{
				sq_getstring(v,-2,&key);
				sq_tostring(v,-1);
				sq_getstring(v,-1,&value);

				pSR->pHttp->outBuf.direct_write("{\"id\":\"",7);
				pSR->pHttp->outBuf.write_cstr(id);
				pSR->pHttp->outBuf.direct_write('.');
				pSR->pHttp->outBuf.write_cstr(key); //�����ǰ׺��template����ID
				pSR->pHttp->outBuf.direct_write("\",\"value\":\"",11);
				pSR->pHttp->outBuf.write_cstr(value); 
				pSR->pHttp->outBuf.direct_write("\"},",3);
			}
			sq_pop(v,3);
		}
		sq_poptop(v);

	}
	return 0;
};

SQInteger http_sq::Sq2AjaxAlert(HSQUIRRELVM v)
{
	http_sq *pSR = NULL;
	sq_getinstanceup(v,1,(SQUserPointer *)&pSR,0); //��׼�Ļ�ȡC++����ķ�ʽ
	if(pSR==NULL) return 0; 

	const char* value;
	sq_getstring(v,-1,&value);
	pSR->pHttp->outBuf.direct_write("{\"id\":\"alert\",\"alert\":\"");
	pSR->pHttp->outBuf.write_cstr(value);
	pSR->pHttp->outBuf.direct_write("\"},");
	return 0;
};


SQInteger http_sq::Sq2AjaxURL(HSQUIRRELVM v)
{
	http_sq *pSR = NULL;
	sq_getinstanceup(v,1,(SQUserPointer *)&pSR,0); //��׼�Ļ�ȡC++����ķ�ʽ
	if(pSR==NULL) return 0; 

	const char* value;
	sq_getstring(v,-1,&value);
	pSR->pHttp->outBuf.direct_write("{\"id\":\"url\",\"URL\":\"");
	pSR->pHttp->outBuf.write_cstr(value);
	pSR->pHttp->outBuf.direct_write("\"},");
	return 0;
};

SQInteger http_sq::Sq2AjaxinnerHTML(HSQUIRRELVM v)
{
	http_sq *pSR = NULL;
	sq_getinstanceup(v,1,(SQUserPointer *)&pSR,0); //��׼�Ļ�ȡC++����ķ�ʽ
	if(pSR==NULL) return 0; 


	const char* value;
	const char* id;

	sq_getstring(v,2,&id);
	pSR->pHttp->outBuf.direct_write("{\"id\":\"");
	pSR->pHttp->outBuf.write_cstr(id);
	pSR->pHttp->outBuf.direct_write("\",\"innerHTML\":\"");
	sq_getstring(v,3,&value);
	pSR->pHttp->outBuf.write_cstr(value);
	pSR->pHttp->outBuf.direct_write("\"},");
	return 0;
};

SQInteger http_sq::Sq2AjaxnewList(HSQUIRRELVM v)
{

	http_sq *pSR = NULL;
	sq_getinstanceup(v,1,(SQUserPointer *)&pSR,0); //��׼�Ļ�ȡC++����ķ�ʽ
	if(pSR==NULL) return 0; 
	const char *name;
	const char *value;
	const char *key;

	if(SQ_SUCCEEDED(sq_getstring(v,-2,&name)) && 
		(sq_gettype(v,-1)==OT_TABLE || sq_gettype(v,-1)==OT_ARRAY))
	{
		int table_idx=sq_gettop(v);

		pSR->pHttp->outBuf.direct_write("{\"id\":\"");
		pSR->pHttp->outBuf.write_cstr(name);
		pSR->pHttp->outBuf.direct_write("\",\"list\":[");


		sq_pushnull(v); //���������
		int counter=0;
		while(SQ_SUCCEEDED(sq_next(v,table_idx)))
		{
			value=NULL;
			key=NULL;
			int handle;
			if(sq_gettype(v,-1)==OT_STRING)
				sq_getstring(v,-1,&value);
			if(sq_gettype(v,-1)==OT_INTEGER) //��������ַ�����������������ֵ��е��ַ���
			{
				sq_getinteger(v,-1,&handle); 
				value=_get_string(handle);

				pSR->pHttp->outBuf.direct_write("[\"");
				pSR->pHttp->outBuf.direct_write(handle);
				pSR->pHttp->outBuf.direct_write("\",\"");
				if(value!=NULL) //����ֵ��в鲻�����handle
					pSR->pHttp->outBuf.write_cstr(value);
				else
					pSR->pHttp->outBuf.direct_write(handle); //д�����ֱ���
				pSR->pHttp->outBuf.direct_write("\"],");
				sq_pop(v,2);
				continue;
			}
			sq_getstring(v,-2,&key);
			pSR->pHttp->outBuf.direct_write("[\"");
			if(key==NULL)
				pSR->pHttp->outBuf.direct_write(counter);
			else
				pSR->pHttp->outBuf.write_cstr(key);
			pSR->pHttp->outBuf.direct_write("\",\"");
			if(value!=NULL)
				pSR->pHttp->outBuf.write_cstr(value);
			pSR->pHttp->outBuf.direct_write("\"],");
			sq_pop(v,2);
			counter++;
		}
		if(pSR->pHttp->outBuf.back()==',') pSR->pHttp->outBuf.pop(); //ɾ������Ҫ�Ķ���
	}
	pSR->pHttp->outBuf.direct_write("]},");
	return 0;
};


SQInteger http_sq::Sq2AjaxInnerTemplate(HSQUIRRELVM v)
{
	http_sq *pSR = NULL;
	sq_getinstanceup(v,1,(SQUserPointer *)&pSR,0); //��׼�Ļ�ȡC++����ķ�ʽ
	if(pSR==NULL) return 0; 

	int parameter=sq_gettop(v);
	if(parameter<3) return 0;
	if(sq_gettype(v,2)!=OT_STRING) return 0;

	const char* id;
	if(sq_gettype(v,3)==OT_CLOSURE)
	{
		sq_getstring(v,2,&id);
		pSR->pHttp->outBuf.direct_write("{\"id\":\"");
		pSR->pHttp->outBuf.write_cstr(id);
		pSR->pHttp->outBuf.direct_write("\",\"innerHTML\":\"");
		sq_push(v,3); //������д���ջ
		sq_push(v,1); //д��this
		for(int i=4;i<=parameter;++i) sq_push(v,i); //����������ѹ���ջ
		//SQObjectType st=sq_gettype(v,1);
		//sq_push(v,1); //д��io

		if(pCstrLock==NULL) pCstrLock=(void*)(pSR); //���������뽫ʹ��cstr����
		if(SQ_FAILED(sq_call(v,parameter-2,SQFalse,SQTrue))) 
			pSR->pHttp->outBuf.direct_write("Failed call squirrel function");
		if(pCstrLock==(void*)(pSR)) pCstrLock=NULL; //��ȷ�ɱ����������������Ҫ���������,�ָ��������뷽ʽ 
		pSR->pHttp->outBuf.direct_write("\"},");
	}
	return 0;
}

SQInteger http_sq::Sq2WriteTemplate(HSQUIRRELVM v)
{
	http_sq *pSR = NULL;
	sq_getinstanceup(v,1,(SQUserPointer *)&pSR,0); //��׼�Ļ�ȡC++����ķ�ʽ
	if(pSR==NULL) return 0; 

	int parameter=sq_gettop(v);
	if(parameter<3) return 0;
	if(sq_gettype(v,2)!=OT_TABLE) return 0;

	const char* id;
	if(sq_gettype(v,3)==OT_CLOSURE)
	{
		
		sq_push(v,3); //������д���ջ
		for(int i=4;i<=parameter;++i) sq_push(v,i); //����������ѹ���ջ
		sq_push(v,2); //д��session
		sq_push(v,1); //д��io

		if(SQ_FAILED(sq_call(v,parameter-2,SQFalse,SQTrue))) 
			pSR->pHttp->outBuf.direct_write("Failed call squirrel function");
	}
	return 0;
}

//----------------------------------------------------------
SQInteger http_sq::Sq2DeleteLastComma(HSQUIRRELVM v)
{
	http_sq *pSR = NULL;
	sq_getinstanceup(v,1,(SQUserPointer *)&pSR,0); //��׼�Ļ�ȡC++����ķ�ʽ
	if(pSR==NULL) return 0; 
	if(pSR->pHttp->outBuf.back()==',') pSR->pHttp->outBuf.pop(); //ɾ������Ҫ�Ķ���
	return 0;
}

SQInteger http_sq::Sq2CreateSession(HSQUIRRELVM v)
{
	http_sq *pSR = NULL;
	sq_getinstanceup(v,1,(SQUserPointer *)&pSR,0); //��׼�Ļ�ȡC++����ķ�ʽ
	if(pSR==NULL) return 0; 
	session_item* p_session=AppData.create_session(global_polling_counter);
	sq_pushstring(v,p_session->session_key,-1); 
	return 1;
}

SQInteger http_sq::Sq2GetSession(HSQUIRRELVM v)
{
	http_sq *pSR = NULL;
	sq_getinstanceup(v,1,(SQUserPointer *)&pSR,0); //��׼�Ļ�ȡC++����ķ�ʽ
	if(pSR==NULL) return 0; 
	const char *session_name;
	sq_getstring(v,2,&session_name);
	sq_pushregistrytable(v);
	int sesssionIdx=sq_gettop(v);
	sq_pushstring(v,session_name,strlen(session_name));
	if(SQ_FAILED(sq_get(v,sesssionIdx)) && sq_gettype(v,-1)==OT_TABLE) //ѹ��session��
	{
		sq_pushnull(v);
	}
	return 1;
}

SQInteger http_sq::Sq2Redirect(HSQUIRRELVM v) //ҳ���ض���
{
	http_sq *pSR = NULL;
	sq_getinstanceup(v,1,(SQUserPointer *)&pSR,0); //��׼�Ļ�ȡC++����ķ�ʽ
	if(pSR==NULL) return 0; 
	const char* value;
	sq_getstring(v,-2,&value);
	string tag="Location: ";
	tag+=value;
	if(value!=NULL) pSR->pHttp->send_http_msg(307,"Temporary Redirect",tag.c_str(),"Lig FastWebServer redirect to %s",value);  
	return 0;
}

SQInteger http_sq::Sq2AjaxEnable(HSQUIRRELVM v) 
{
	http_sq *pSR = NULL;
	sq_getinstanceup(v,1,(SQUserPointer *)&pSR,0); //��׼�Ļ�ȡC++����ķ�ʽ
	if(pSR==NULL) return 0; 
	const char* id0;
	const char* id1;

	if(sq_gettop(v)==2)
	{
		sq_getstring(v,2,&id0);
		pSR->pHttp->outBuf.direct_write("{\"id\":\"");
		pSR->pHttp->outBuf.write_cstr(id0);
		pSR->pHttp->outBuf.direct_write("\",\"disable\":\"");
		pSR->pHttp->outBuf.write_cstr("false");
		pSR->pHttp->outBuf.direct_write("\"},");
	}
	if(sq_gettop(v)==3)
	{
		sq_getstring(v,2,&id0);
		pSR->pHttp->outBuf.direct_write("{\"id\":\"");
		pSR->pHttp->outBuf.write_cstr(id0);
		sq_getstring(v,3,&id1);
		if(strcmp(id1,"readOnly")==0)
		{
			pSR->pHttp->outBuf.direct_write("\",\"readOnly\":\"");
			pSR->pHttp->outBuf.write_cstr("true");
		}
		else
		{
			pSR->pHttp->outBuf.write_cstr(".");
			pSR->pHttp->outBuf.write_cstr(id1);
			pSR->pHttp->outBuf.direct_write("\",\"disabled\":\"");
			pSR->pHttp->outBuf.write_cstr("false");
		}
		
		pSR->pHttp->outBuf.direct_write("\"},");
	}
	return 0;
};

SQInteger http_sq::Sq2AjaxDisable(HSQUIRRELVM v) 
{
	http_sq *pSR = NULL;
	sq_getinstanceup(v,1,(SQUserPointer *)&pSR,0); //��׼�Ļ�ȡC++����ķ�ʽ
	if(pSR==NULL) return 0; 
	const char* id0;
	const char* id1;

	if(sq_gettop(v)==2)
	{
		if(sq_gettype(v,2)!=OT_STRING) return 0;
		sq_getstring(v,2,&id0);
		pSR->pHttp->outBuf.direct_write("{\"id\":\"");
		pSR->pHttp->outBuf.write_cstr(id0);
		pSR->pHttp->outBuf.direct_write("\",\"disabled\":\"");
		pSR->pHttp->outBuf.write_cstr("true");
		pSR->pHttp->outBuf.direct_write("\"},");
	}
	if(sq_gettop(v)==3)
	{
		if(sq_gettype(v,2)!=OT_STRING) return 0;
		if(sq_gettype(v,3)!=OT_STRING) return 0;
		sq_getstring(v,2,&id0);
		pSR->pHttp->outBuf.direct_write("{\"id\":\"");
		pSR->pHttp->outBuf.write_cstr(id0);
		sq_getstring(v,3,&id1);
		if(strcmp(id1,"readOnly")==0)
		{
			pSR->pHttp->outBuf.direct_write("\",\"readOnly\":\"");
			pSR->pHttp->outBuf.write_cstr("false");
		}
		else
		{
			pSR->pHttp->outBuf.write_cstr(".");
			pSR->pHttp->outBuf.write_cstr(id1);
			pSR->pHttp->outBuf.direct_write("\",\"disabled\":\"");
			pSR->pHttp->outBuf.write_cstr("true");
		}
		
		pSR->pHttp->outBuf.direct_write("\"},");
	}
	return 0;
};

extern void utf8togb(char *s); //UTF8תGB2312���ⲿ����
SQInteger http_sq::Sq2HTTPWriteUpload2File(HSQUIRRELVM v)
{
	int top=sq_gettop(v);
	
	if(top!=2)
	{
		sq_throwerror(v,"parameter's number error,required 1 string");
		sq_settop(v,top);
		sq_pushbool(v,SQFalse);
		return 1; 
	}
	
	const char* fn;
	const char* dn;

	sq_getstring(v,-1,&fn);
	sq_pushstring(v,"uploadname",-1);
	if(SQ_FAILED(sq_get(v,-3)))
	{
		sq_throwerror(v,"object error!!");
		sq_settop(v,top);
		sq_pushbool(v,SQFalse);
		return 1; 
	}
	sq_getstring(v,-1,&dn);


	int data_length;
	const char* p_data;
	sq_pushstring(v,"length",-1);
	if(SQ_FAILED(sq_get(v,-4)))
	{
		sq_throwerror(v,"object error!!");
		sq_settop(v,top);
		sq_pushbool(v,SQFalse);
		return 1; 
	}
	sq_getinteger(v,-1,&data_length);

	sq_pushstring(v,"data",-1);
	if(SQ_FAILED(sq_get(v,-5)))
	{
		sq_throwerror(v,"object error!!");
		sq_settop(v,top);
		sq_pushbool(v,SQFalse);
		return 1; 
	}
	sq_getuserpointer(v,-1,(SQUserPointer *)(&p_data));
	
	sq_settop(v,top);
	sq_pushbool(v,SQTrue);

		
#ifdef WIN32  //Windows�����£��ļ���������UTF8,�˴�ת��ΪGBK
	utf8togb((char*)fn);
#endif
	FILE* fs=fopen(fn,"wb");
	if(fs!=NULL)
		fwrite(p_data,data_length,1,fs);
	fclose(fs);

	return 1; 
}

SQInteger http_sq::Sq2GetHTTP(HSQUIRRELVM v) 
{
	http_sq *pSR = NULL;
	sq_getinstanceup(v,1,(SQUserPointer *)&pSR,0); //��׼�Ļ�ȡC++����ķ�ʽ
	if(pSR==NULL) return 0; 
	http_conn* self=pSR->pHttp; 
	if(self->flag_file_upload) //�ļ��ϴ�
	{
		sq_newtable(v); //����һ�����ر�
		list<mime_data_pair>::iterator it=self->mlist.begin();
		while(it!=self->mlist.end())
		{
#ifdef _LIG_DEBUG
			DOUT<<"CC:"<<it->name.c_str()<<endl;
#endif
			sq_pushstring(v,it->name.data(),it->name.length());
			sq_newtable(v); 
			//--------------------------------------------
			
			sq_pushstring(v,"uploadname",-1);
			sq_pushstring(v,it->filename.data(),it->filename.length());
			sq_createslot(v,-3);

			sq_pushstring(v,"data",-1);
			sq_pushuserpointer(v,(SQUserPointer)it->p_data);
			sq_createslot(v,-3);

			sq_pushstring(v,"length",-1);
			sq_pushinteger(v,it->data_length);
			sq_createslot(v,-3);

			sq_pushstring(v,"value",-1);
			sq_pushstring(v,it->value.data(),it->value.length() );
			sq_createslot(v,-3);

			sq_pushstring(v,"filename",-1);
			sq_pushstring(v,it->storename.data(),it->storename.length() );
			sq_createslot(v,-3);

			sq_pushstring(v,"writeFile",-1);
			sq_newclosure(v,Sq2HTTPWriteUpload2File,0);
			sq_createslot(v,-3);
			//----------------------------------------------------
			sq_createslot(v,-3);  //���������б�

			++it;
		}
	}
	else sq_pushnull(v); //����һ���ձ�
	return 1;
};
bool register_global_func(HSQUIRRELVM v,SQFUNCTION f,const char *fname,const char* para_mask)
{
	SQInteger top = sq_gettop(v); //saves the stack size before the call
	sq_pushroottable(v);
	SQInteger wkIdx = sq_gettop(v);
	sq_pushstring(v,fname,-1);
	sq_newclosure(v,f,0); //create a new function without a free var
	if(para_mask!=NULL) sq_setparamscheck(v,strlen(para_mask),para_mask); 
	sq_createslot(v,-3); 
	sq_settop(v,top); //restores the original stack size
	return true;
};

bool http_sq::push_obj(HSQUIRRELVM v,http_sq *pObj) //��������ѹ��squirrel��ջ����Ϊ���󴫵ݸ��ű�������
{
	SQInteger top = sq_gettop(v);
	sq_pushroottable(v); //push the root table(were the globals of the script will be stored)
	sq_pushstring(v,"http_sq",-1);
	if(SQ_SUCCEEDED(sq_get(v,-2)) && sq_gettype(v,-1)==OT_CLASS)
	{
		if(SQ_SUCCEEDED(sq_createinstance(v,-1)))
		{
			sq_remove(v,-2);//ȥ������Ҫ��class
			sq_remove(v,-2); //ȥ������Ҫ��roottable
			top = sq_gettop(v);
			sq_pushstring(v,"constructor",-1);
			if(SQ_SUCCEEDED(sq_get(v,-2)))
			{
				sq_push(v,-2); //push the 'this' (in this case is the global table)
				sq_pushuserpointer(v,SQUserPointer(pObj)); //push the 'this' (in this case is the global table)
				sq_call(v,2,SQFalse,SQTrue);
			}
			sq_settop(v,top);
			return true;
		}
	}
	sq_settop(v,top); //restores the original stack size
	return false;

};

void http_sq::register_library(HSQUIRRELVM v)
{
	SQInteger top = sq_gettop(v); //saves the stack size before the call
	sq_pushroottable(v); //push the root table(were the globals of the script will be stored)
	sq_pushstring(v,"http_sq",-1);
	sq_newclass(v,SQFalse);
	sq_createslot(v,-3);
	sq_settop(v,top); //restores the original stack size

	//'o' null, 'i' integer, 'f' float, 'n' integer or float, 's' string, 't' table, 'a' array, 'u' userdata, 
	//'c' closure and nativeclosure, 'g' generator, 'p' userpointer, 'v' thread, 'x' instance(class instance), 'y' class, 'b' bool. and '.' any type
	register_func(v,"http_sq",Sq2Constructor,"constructor",NULL);//���캯��
	//--private------------------------------------------------
	register_func(v,"http_sq",Sq2Write,"write");
	register_func(v,"http_sq",Sq2WriteHandle,"handle","xi");
	register_func(v,"http_sq",Sq2WriteTemplate,"template");
	register_func(v,"http_sq",Sq2Get,"get","xs");
	register_func(v,"http_sq",Sq2Get,"_get","xs");
	register_func(v,"http_sq",Sq2GetVars,"readVars","x");

	register_func(v,"http_sq",Sq2InnerClose,"innerClose","x");
	register_func(v,"http_sq",Sq2InnerOpen,"innerOpen","x");

	register_func(v,"http_sq",Sq2IdxWrite,"idxWrite","xi");
	register_func(v,"http_sq",Sq2SegmentWrite,"segmentWrite","xs");
	register_func(v,"http_sq",Sq2LastWrite,"lastWrite","x");
	register_func(v,"http_sq",Sq2GetTemplate,"getTemplate","xs");
	//-----public-------------------------------------------------
	register_func(v,"http_sq",Sq2AjaxValue,"ajaxValue","xs.");
	register_func(v,"http_sq",Sq2AjaxinnerHTML,"ajaxHTML","xss");
	register_func(v,"http_sq",Sq2AjaxInnerTemplate,"ajaxTemplate");
	register_func(v,"http_sq",Sq2AjaxnewList,"ajaxList","xs.");
	register_func(v,"http_sq",Sq2AjaxAlert,"ajaxMessage","xs");
	register_func(v,"http_sq",Sq2AjaxAlert,"ajaxAlert","xs");
	register_func(v,"http_sq",Sq2AjaxURL,"ajaxURL","xs");
	register_func(v,"http_sq",Sq2AjaxEnable,"ajaxEnable");  //������Ԫ�ؽ�����߽���
	register_func(v,"http_sq",Sq2AjaxDisable,"ajaxDisable");

	register_func(v,"http_sq",Sq2DeleteLastComma,"deleteLastComma","x");
	register_func(v,"http_sq",Sq2Redirect,"redirect","xs");
	register_func(v,"http_sq",Sq2CreateSession,"createSession","x");
	register_func(v,"http_sq",Sq2GetSession,"getSession","xs");//��ָ����session�ַ����еõ�session��
	register_func(v,"http_sq",Sq2GetHTTP,"getUpload","x");
	register_func(v,"http_sq",Sq2GetHTTP,"writeFile");
	
}


void http_conn::register_library(HSQUIRRELVM v)
{
	SQInteger top = sq_gettop(v); //saves the stack size before the call
	sq_pushroottable(v); //push the root table(were the globals of the script will be stored)
	sq_pushstring(v,"http_conn",-1);
	sq_newclass(v,SQFalse);
	sq_createslot(v,-3);
	sq_settop(v,top); //restores the original stack size

	//'o' null, 'i' integer, 'f' float, 'n' integer or float, 's' string, 't' table, 'a' array, 'u' userdata, 
	//'c' closure and nativeclosure, 'g' generator, 'p' userpointer, 'v' thread, 'x' instance(class instance), 'y' class, 'b' bool. and '.' any type
	register_func(v,"http_conn",Sq2Constructor,"constructor",NULL);//���캯��
	//--private------------------------------------------------
	register_func(v,"http_conn",Sq2Write,"write");
	register_func(v,"http_conn",Sq2Get,"getUpload","x");
	register_func(v,"http_conn",Sq2Get,"writeFile");
};


SQInteger http_conn::Sq2Constructor(HSQUIRRELVM v) //��׼��Squirrel C++�����췽ʽ
{
	int top=sq_gettop(v);
	SQUserPointer theHandle;
	sq_getuserpointer(v,-1,&theHandle);
	http_conn* pobj=(http_conn*)(theHandle); //�Ӷ�ջ�л�ȡC++�����ָ��

	if(!pobj) return sq_throwerror(v,"Create instance failed");
	sq_setinstanceup(v,1,theHandle); //Ϊ��ʵ������C++����ָ��,��ʵ���ڶ�ջ�ĵײ�������idx ʹ��1

	return 0;
}

bool http_conn::push_obj(HSQUIRRELVM v,http_conn *pObj) //��������ѹ��squirrel��ջ����Ϊ���󴫵ݸ��ű�������
{
	SQInteger top = sq_gettop(v);
	sq_pushroottable(v); //push the root table(were the globals of the script will be stored)
	sq_pushstring(v,"http_conn",-1);
	if(SQ_SUCCEEDED(sq_get(v,-2)) && sq_gettype(v,-1)==OT_CLASS)
	{
		if(SQ_SUCCEEDED(sq_createinstance(v,-1)))
		{
			sq_remove(v,-2);//ȥ������Ҫ��class
			sq_remove(v,-2); //ȥ������Ҫ��roottable
			top = sq_gettop(v);
			sq_pushstring(v,"constructor",-1);
			if(SQ_SUCCEEDED(sq_get(v,-2)))
			{
				sq_push(v,-2); //push the 'this' (in this case is the global table)
				sq_pushuserpointer(v,SQUserPointer(pObj)); //push the 'this' (in this case is the global table)
				sq_call(v,2,SQFalse,SQTrue);
			}
			sq_settop(v,top);
			return true;
		}
	}
	sq_settop(v,top); //restores the original stack size
	return false;

};

SQInteger http_conn::Sq2WriteUpload2File(HSQUIRRELVM v)
{
	int top=sq_gettop(v);
	http_conn *self = NULL;
	sq_getinstanceup(v,1,(SQUserPointer *)&self,0); //��׼�Ļ�ȡC++����ķ�ʽ

	if(top!=2)
	{
		sq_throwerror(v,"parameter's number error,required 1 string");
		sq_settop(v,top);
		sq_pushbool(v,SQFalse);
		return 1; 
	}
	
	const char* fn;
	const char* dn;
	sq_getstring(v,-1,&fn);
	sq_pushstring(v,"uploadname",-1);
	if(SQ_FAILED(sq_get(v,-3)))
	{
		sq_throwerror(v,"object error!!");
		sq_settop(v,top);
		sq_pushbool(v,SQFalse);
		return 1; 
	}
	sq_getstring(v,-1,&dn);


	int data_length;
	const char* p_data;
	sq_pushstring(v,"length",-1);
	if(SQ_FAILED(sq_get(v,-4)))
	{
		sq_throwerror(v,"object error!!");
		sq_settop(v,top);
		sq_pushbool(v,SQFalse);
		return 1; 
	}
	sq_getinteger(v,-1,&data_length);

	sq_pushstring(v,"data",-1);
	if(SQ_FAILED(sq_get(v,-5)))
	{
		sq_throwerror(v,"object error!!");
		sq_settop(v,top);
		sq_pushbool(v,SQFalse);
		return 1; 
	}
	sq_getuserpointer(v,-1,(SQUserPointer *)(&p_data));

	ofstream off;
	off.open(fn,ios::binary);
	if(off.good())
	{
		off.write(p_data,data_length); 
	}
	off.close();
		
	sq_pushstring(v,"filename",-1);
	sq_pushstring(v,fn,-1);
	if(SQ_FAILED(sq_set(v,-6))) //�����д�봫����ļ�����
	{
		sq_throwerror(v,"object error!!");
		sq_settop(v,top);
		sq_pushbool(v,SQFalse);
		return 1; 
	}
	sq_settop(v,top);
	sq_pushbool(v,SQTrue);
	return 1; 
}

SQInteger http_conn::Sq2Get(HSQUIRRELVM v)
{
	int top=sq_gettop(v);
	http_conn *self = NULL;
	sq_getinstanceup(v,1,(SQUserPointer *)&self,0); //��׼�Ļ�ȡC++����ķ�ʽ
	if(self->flag_file_upload) //�ļ��ϴ�
	{
		sq_newtable(v); //����һ�����ر�
		list<mime_data_pair>::iterator it=self->mlist.begin();
		while(it!=self->mlist.end())
		{
#ifdef _LIG_DEBUG
			DOUT<<"CC:"<<it->name.c_str()<<endl;
#endif
			sq_pushstring(v,it->name.data(),it->name.length());
			sq_newtable(v); 
			//--------------------------------------------
			
			sq_pushstring(v,"uploadname",-1);
			sq_pushstring(v,it->filename.data(),it->filename.length());
			sq_createslot(v,-3);

			sq_pushstring(v,"data",-1);
			sq_pushuserpointer(v,(SQUserPointer)it->p_data);
			sq_createslot(v,-3);

			sq_pushstring(v,"length",-1);
			sq_pushinteger(v,it->data_length);
			sq_createslot(v,-3);

			sq_pushstring(v,"value",-1);
			sq_pushstring(v,it->value.data(),it->value.length() );
			sq_createslot(v,-3);

			sq_pushstring(v,"filename",-1);
			sq_pushstring(v,it->storename.data(),it->storename.length() );
			sq_createslot(v,-3);

			sq_pushstring(v,"writeFile",-1);
			sq_newclosure(v,Sq2WriteUpload2File,0);
			sq_createslot(v,-3);
			//----------------------------------------------------
			sq_createslot(v,-3);  //���������б�

			++it;
		}
	}
	else sq_pushnull(v); //����һ���ձ�
	return 1;
}

SQInteger http_conn::Sq2WriteFile(HSQUIRRELVM v)
{
	int top=sq_gettop(v);
	int args=1;
	bool flag_writen=false;
	http_conn *self = NULL;
	sq_getinstanceup(v,1,(SQUserPointer *)&self,0); //��׼�Ļ�ȡC++����ķ�ʽ
	
	if(self->flag_file_upload) //�ļ��ϴ�
	{
		list<mime_data_pair>::iterator it=self->mlist.begin();
		while(it!=self->mlist.end())
		{
			const char* fn;
			if(it->data_length>0 && args<top)
			{
				if(sq_gettype(v,args)!=OT_STRING)
				{
					sq_throwerror(v,"parameter is not matched,required string");
					sq_pushbool(v,SQFalse);
					return 1;
				}
				sq_getstring(v,args,&fn);
				ofstream off;
				off.open(fn,ios::binary);
				if(off.good())
				{
					off.write(it->p_data,it->data_length); 
					flag_writen=true;
					it->storename=fn; 
				}
				off.close();
				++args;
			}
			++it;
		}
		
	}
	if(flag_writen) sq_pushbool(v,SQTrue);
	else sq_pushbool(v,SQFalse);
	return 1; 
}

SQInteger http_conn::Sq2Write(HSQUIRRELVM v)
{
	int top=sq_gettop(v);
	http_conn *self = NULL;
	sq_getinstanceup(v,1,(SQUserPointer *)&self,0); //��׼�Ļ�ȡC++����ķ�ʽ

	for(int i=2;i<=top;++i)
	{
		switch(sq_gettype(v,i))
		{
		case OT_NULL:
			break;
		case OT_INTEGER:
			int ivalue;
			sq_getinteger(v,i,&ivalue);
			self->outBuf.printf("%d",ivalue);
			break;
		case OT_FLOAT:
			float fvalue;
			sq_getfloat(v,i,&fvalue);
			self->outBuf.printf("%f",fvalue);
			break;
		case OT_STRING:
			const char* svalue;
			sq_getstring(v,i,&svalue);
			self->outBuf.direct_write(svalue);
			break;  

		default:

			return sq_throwerror(v,"invalid param"); //throws an exception
		}
	}
	return 0;
}


string root_dir; //�����������ж�ȡ��·��
//----------Sqlite DB support---------------------------
void sqlite_query::register_library(HSQUIRRELVM v)
{
	int top=sq_gettop(v);
	sq_pushroottable(v); //push the root table(were the globals of the script will be stored)
	
	sq_pushstring(v,"sql_query",-1);
	sq_newclass(v,SQFalse);
	sq_createslot(v,-3);
	sq_settop(v,top); //restores the original stack size

	register_func(v,"sql_query",Sq2Constructor,"constructor",NULL);//���캯��
	register_func(v,"sql_query",Sq2sqlite_queryexec,"exec","xs"); //bool exec("SQL str");����Ϊ���ʾ���������,�����ʾû���������
	register_func(v,"sql_query",Sq2sqlite_querynext,"next","x"); //bool next();����������ж�ȡ��һ����¼,�����һ��������,�򷵻�false
	register_func(v,"sql_query",Sq2sqlite_queryeof,"eof","x");   //bool eof(); �ж����ݼ������Ƿ������ݴ���
	register_func(v,"sql_query",Sq2sqlite_queryfailed,"fail","x");   //bool fail(); �ж�ǰһ��SQLִ��״̬�Ƿ�ʧ��
	register_func(v,"sql_query",Sq2sqlite_bonding,"get","xt"); //void get(tableVar); ����ǰ�������д��һ��Squirrel���� 
	register_func(v,"sql_query",Sq2sqlite_current,"current","x");//OT_TABLE current(); ����ǰ������Ϊһ������� 
	register_func(v,"sql_query",Sq2sqlite_insert,"insert","xst");//OT_TABLE insert(); ����ǰ������� 

	register_func(v,"sql_query",Sq2sqlite_get,"_get","xs");//OT_TABLE current(); ��ȡ��ǰ���ݣ�����һ��meta���� 
	
	register_func(v,"sql_query",Sq2sqlite_initDB,"initDB","xs"); //bool exec("SQL str");����Ϊ���ʾ���������,�����ʾû���������
	
};

template<>bool sqlite_query::get(int nIdx, int &value) 
{
	if(nIdx>=m_nColumn) return false;
	value=sqlite3_column_int(mLink,nIdx);
	return true;
};
template<>bool sqlite_query::get(int nIdx, const char* &value) 
{
	if(nIdx>=m_nColumn) return false;
	value=(const char*)sqlite3_column_text(mLink,nIdx);
	return true;
};
template<>bool sqlite_query::get(int nIdx, double &value) 
{
	if(nIdx>=m_nColumn) return false;
	value=sqlite3_column_double(mLink,nIdx);
	return true;
};

template<>bool sqlite_query::get(const char* fieldname, int &value) 
{
	int nIdx=0;
	for(;nIdx<m_nColumn;++nIdx) if(strcmp(sqlite3_column_name(mLink,nIdx),fieldname)==0) break;
	if(nIdx>=m_nColumn) return false;
	value=sqlite3_column_int(mLink,nIdx);
	return true;
};
template<>bool sqlite_query::get(const char* fieldname, const char* &value) 
{
	int nIdx=0;
	for(;nIdx<m_nColumn;++nIdx) if(strcmp(sqlite3_column_name(mLink,nIdx),fieldname)==0) break;
	if(nIdx>=m_nColumn) return false;
	value=(const char*)sqlite3_column_text(mLink,nIdx);
	return true;
};
template<>bool sqlite_query::get(const char* fieldname, double &value) 
{
	int nIdx=0;
	for(;nIdx<m_nColumn;++nIdx) if(strcmp(sqlite3_column_name(mLink,nIdx),fieldname)==0) break;
	if(nIdx>=m_nColumn) return false;
	value=sqlite3_column_double(mLink,nIdx);
	return true;
};
//--------------------------------------------------------------------------------------
void lig_httpd::register_library(HSQUIRRELVM v)
{
	int top=sq_gettop(v);
	sq_pushroottable(v); //push the root table(were the globals of the script will be stored)
	sq_pushstring(v,"WebServer",-1);
	sq_newclass(v,SQFalse);
	sq_createslot(v,-3);
	sq_settop(v,top); //restores the original stack size

	register_func(v,"WebServer",Sq2ReturnRootPath,"root","x");
	register_func(v,"WebServer",Sq2Constructor,"constructor","xsiii"); //����Ϊ"��Ŀ¼","�����˿�","����ϴ��ߴ�","��ʱʱ�䣨��)"
	register_func(v,"WebServer",Sq2JoinPage,"joinPage","xs-s");
	register_func(v,"WebServer",Sq2JoinSessionPage,"joinSessionPage","xs-s");

	//joinPage��ҳ���ļ���������URL��Ŀ¼�µģ���ҳ�������ȱʡHTMLĿ¼�£�
	//����еڶ��������ڶ�������Ϊ�����ҳ���URI�����û�еڶ�������ʹ��ҳ���ļ�����ΪURI
	//joinSessionPage������Sessionʱ���Զ��ض���AUTOLOGIN���õĵ�½URI

	//'o' null, 'i' integer, 'f' float, 'n' integer or float, 's' string, 't' table, 'a' array, 'u' userdata, 
	//'c' closure and nativeclosure, 'g' generator, 'p' userpointer, 'v' thread, 'x' instance(class instance), 'y' class, 'b' bool. and '.' any type
	//------------------------------------------------------------
	register_func(v,"WebServer",Sq2MapFunction,"mapFunction","xss");
	register_func(v,"WebServer",Sq2MapSessionFunction,"mapSessionFunction","xss");
	//mapFunction(url,func); url��Ӧһ��/url,func����Ӧ�ô���session,io����
	//------------------------------------------------------------
	register_func(v,"WebServer",Sq2RunWebServer,"run_web","x");
	register_func(v,"WebServer",Sq2RunFCGIServer,"run_fcgi","x");
	//-------------------------------------------------------------
	register_func(v,"WebServer",Sq2LocalService,"local_service","x"); //��ֹ���ط��������ܷǱ�������������
	register_func(v,"WebServer",Sq2SetDefaultPage,"default_page","xs"); //��ֹ���ط��������ܷǱ�������������

	
};

SQInteger lig_httpd::Sq2MapScript(HSQUIRRELVM v) //��һ��URLӳ�䵽һ������ֱ��ִ�е�squirrel�ļ���
{
	lig_httpd *pServer = NULL;
	sq_getinstanceup(v,1,(SQUserPointer *)&pServer,0); //��׼�Ļ�ȡC++����ķ�ʽ
	if(pServer==NULL) return 0; 
	//������δ���
	return 0;
}
SQInteger lig_httpd::Sq2MapSessionScript(HSQUIRRELVM v) //��һ��URLӳ�䵽һ������ֱ��ִ�е�squirrel�ļ��ϣ�ӳ���ʱ���Զ����session�Ƿ���Ч
{
	lig_httpd *pServer = NULL;
	sq_getinstanceup(v,1,(SQUserPointer *)&pServer,0); //��׼�Ļ�ȡC++����ķ�ʽ
	if(pServer==NULL) return 0; 
	//������δ���
	return 0;
}
SQInteger lig_httpd::Sq2MapFunction(HSQUIRRELVM v) //��һ��URLӳ�䵽һ������ֱ��ִ�е�squirrel������
{
	lig_httpd *pServer = NULL;
	sq_getinstanceup(v,1,(SQUserPointer *)&pServer,0); //��׼�Ļ�ȡC++����ķ�ʽ
	if(pServer==NULL) return 0; 

	const char* url;
	const char* func;
	sq_getstring(v,-2,&url);
	sq_getstring(v,-1,&func);
	pServer->register_sqfunction(url,func);
	return 0;

}
SQInteger lig_httpd::Sq2MapSessionFunction(HSQUIRRELVM v) //��һ��URLӳ�䵽һ������ֱ��ִ�е�squirrel�����ϣ�ӳ���ʱ���Զ����session�Ƿ���Ч
{
	lig_httpd *pServer = NULL;
	sq_getinstanceup(v,1,(SQUserPointer *)&pServer,0); //��׼�Ļ�ȡC++����ķ�ʽ
	if(pServer==NULL) return 0; 
	const char* url;
	const char* func;
	sq_getstring(v,-2,&url);
	sq_getstring(v,-1,&func);
	pServer->register_session_sqfunction(url,func);
		
	return 0;

}

SQInteger lig_httpd::_queryobj_releasehook(SQUserPointer p, SQInteger size) //��׼��Squirrel C++���������ص�
{
	lig_httpd* handle=(lig_httpd*)p;
	if(handle!=NULL) 
	{
		delete handle;
	}
	return 0;
}


SQInteger lig_httpd::Sq2Constructor(HSQUIRRELVM v) //��׼��Squirrel C++�����췽ʽ
{
	
	int port,time_out;
	const char* proot;
	if(!SqGetVar(v,proot,port,MAX_POSTLENGTH,time_out)) return 0;
	
	root_dir=proot;
	p_root_dir=root_dir.c_str(); 

	if(time_out<1) return sq_throwerror(v,"time_out set error");
	if(port<2 || port>65535)  return sq_throwerror(v,"listen port error");
	if(pHttpd==NULL) //ֻ�ܴ���һ��������
	{
		pHttpd=new lig_httpd; //�����������ʵ��
		if(!pHttpd) return sq_throwerror(v,"Create instance failed");
		pHttpd->init(port);
		pHttpd->v=v; //���������

	}
	pHttpd->VMthread.v=v; //����������ݸ��ű�ִ���߳�
	sq_setinstanceup(v,1,(SQUserPointer)(pHttpd)); //Ϊ��ʵ������C++����ָ��,��ʵ���ڶ�ջ�ĵײ�������idx ʹ��1
	sq_setreleasehook(v,1,_queryobj_releasehook);  //�������������������󣬴˴�Ҫ��ɾ������
	return 0;
}

SQInteger lig_httpd::Sq2RunWebServer(HSQUIRRELVM v) //��ʼ��Web Server
{
	lig_httpd *pServer = NULL;
	sq_getinstanceup(v,1,(SQUserPointer *)&pServer,0); //��׼�Ļ�ȡC++����ķ�ʽ
	if(pServer==NULL) return 0; 
	lig_httpd &server=*pServer;
	server.wait_connect(); 
	return 0;
}
SQInteger lig_httpd::Sq2SetDefaultPage(HSQUIRRELVM v) //��ʼ��Web Server
{
	lig_httpd *pServer = NULL;
	sq_getinstanceup(v,1,(SQUserPointer *)&pServer,0); //��׼�Ļ�ȡC++����ķ�ʽ
	if(pServer==NULL) return 0; 
	const char* strs;
	if(SqGetVar(v,strs))
		pServer->defaulf_page=strs;
	return 0;
}

SQInteger lig_httpd::Sq2LocalService(HSQUIRRELVM v) //��ʼ��Web Server
{
	lig_httpd *pServer = NULL;
	sq_getinstanceup(v,1,(SQUserPointer *)&pServer,0); //��׼�Ļ�ȡC++����ķ�ʽ
	if(pServer==NULL) return 0; 
	pServer->flag_local_listen=true;
	return 0;
}

SQInteger lig_httpd::Sq2RunFCGIServer(HSQUIRRELVM v) //��ʼ��FastCGI Server
{
	lig_httpd *pServer = NULL;
	sq_getinstanceup(v,1,(SQUserPointer *)&pServer,0); //��׼�Ļ�ȡC++����ķ�ʽ
	if(pServer==NULL) return 0; 
	lig_httpd &server=*pServer;
	server.wait_fcgi(); 
	return 0;
}

SQInteger lig_httpd::Sq2JoinPage(HSQUIRRELVM v) //ע��ҳ��
{
	lig_httpd *pServer = NULL;
	if(SQ_FAILED(sq_getinstanceup(v,1,(SQUserPointer *)&pServer,0))) return 0; //��׼�Ļ�ȡC++����ķ�ʽ
	if(pServer==NULL) return 0; 
	lig_httpd &server=*pServer;

	string url;
	const char* page;
	
	if(SqGetVar(v,page) && page!=NULL)
	{
		while((*page=='\\' || *page=='/') && *page!=0) ++page; //ɾ������Ҫ���ļ�ͷ
		url="/";url+=page;
	}else if(SqGetVar(v,page,url) && page!=NULL)
		while((*page=='\\' || *page=='/') && *page!=0) ++page; //ɾ������Ҫ���ļ�ͷ
	else {sq_throwerror(v,"parameter error!");exit(0);}
	
	const char* name=create_page(server,url.c_str(),page,true);
	//ȱʡΪGB2312���Զ�תΪUTF-8,���UTF-8�ı������ļ�ͷ�������ᱻ��ת
	if(name==NULL)
	{
#ifdef _LIG_DEBUG
		DOUT<<"create "<<page<<" failed, will exit!"<<endl;
#endif
		return 0;
		//throw("Create Http page failed, Maybe web ROOT_DIR isn't exist!");
	}
	sq_pushroottable(v);
	sq_pushstring(v,name,-1);
	if(SQ_SUCCEEDED(sq_get(v,-2)) && sq_gettype(v,-1)!=OT_TABLE)
	{
#ifdef _LIG_DEBUG
		DOUT<<"process "<<page<<" failed, will exit!"<<endl;
#endif
		return 0;
		//throw("Process http page failed, Will exit!");
	}
	return 1;
};


SQInteger lig_httpd::Sq2JoinSessionPage(HSQUIRRELVM v) //ע��ҳ��
{
	lig_httpd *pServer = NULL;
	sq_getinstanceup(v,1,(SQUserPointer *)&pServer,0); //��׼�Ļ�ȡC++����ķ�ʽ
	if(pServer==NULL) return 0; 
	lig_httpd &server=*pServer;

	string url;
	const char* page;
	
	if(SqGetVar(v,page) && page!=NULL)
	{
		while((*page=='\\' || *page=='/') && *page!=0) ++page; //ɾ������Ҫ���ļ�ͷ
		url="/";url+=page;
	}else if(SqGetVar(v,page,url) && page!=NULL)
		while((*page=='\\' || *page=='/') && *page!=0) ++page; //ɾ������Ҫ���ļ�ͷ
	else {sq_throwerror(v,"parameter error!");exit(0);}

	const char* name=create_session_page(server,url.c_str(),page,true);
	//ȱʡΪGB2312���Զ�תΪUTF-8,���UTF-8�ı������ļ�ͷ�������ᱻ��ת
	if(name==NULL)
	{
#ifdef _LIG_DEBUG
		DOUT<<"create "<<page<<" failed, will exit!"<<endl;
#endif
		throw("Create Http page failed, Maybe web ROOT_DIR isn't exist!");
	}
	sq_pushroottable(v);
	sq_pushstring(v,name,-1);
	if(SQ_SUCCEEDED(sq_get(v,-2)) && sq_gettype(v,-1)!=OT_TABLE)
	{
#ifdef _LIG_DEBUG
		DOUT<<"process "<<page<<" failed, will exit!"<<endl;
#endif
		throw("Process Http page failed, Will exit!");
	}
	return 1;
};




void lig_httpd::wait_connect()
{
	
	VMthread.start_thread();
	start_thread();
	this->set_listen(); 

	fd_set incoming;
	timeval tv;
	sockaddr addr;
	int addr_length = sizeof (addr );
	int new_socket;
	while(!GlobalExitFlag)
	{
		FD_ZERO(&incoming);
		FD_SET(sock_id,&incoming);
		tv.tv_sec=5;
		tv.tv_usec=0; 
		int rt;
		if(rt=select(sock_id+1,&incoming,NULL,NULL,&tv)<0)
		{
#ifdef WIN32
			Sleep(milliseconds);  //���rd_set,wr_setΪ�գ�Windows��ֱ�ӷ��أ�������������
#endif
		}
		time(&global_polling_clock); //��ʱ����ȫ��ʱ��
		
		if(!FD_ISSET(sock_id,&incoming)) //it's time out 
		{
			//DOUT<<"Time="<<global_polling_clock<<endl;
			//sleep(5);
			continue;
		}
#ifdef WIN32
		new_socket = ::accept(sock_id, ( sockaddr * ) &addr, (int * ) &addr_length );
#else
		if(FD_ISSET(sock_id,&incoming))
			new_socket = ::accept(sock_id, ( sockaddr * ) &addr,((socklen_t*) &addr_length));
		else
		{
			//DOUT<<"Unknown request!!"<<endl;
			sleep(5);
			continue;
		}
#endif
		static int in_coming_counter=0;
		//DOUT<<"Accept User : "<<in_coming_counter++<<endl;
		if(new_socket<= 0)
		{
			//DOUT<<"Socket = "<<new_socket<<" will exited!!"<<endl;
			//throw sdl_exception("Socket accept failed!! --li da gang");
			return;
		}

		if(max_fd<sock_id ) max_fd = sock_id;
		if(!set_nonblock(new_socket)) //��socket����Ϊ������״̬
		{
				//DOUT<<"Cann't set to non-block socket!\n";
		}
		//DOUT<<"Will insert socket id="<<new_socket<<endl;
		insert_request(new_socket); //���û�����������
	}
};

void lig_httpd::wait_fcgi()
{
	fd_set incoming;

	VMthread.start_thread();
	set_listen(); 
	
	timeval tv;
	http_conn *pConn=new http_conn; //����һ��ר����FCGI�Ķ������������Ϊ��δ��֧�ֶ�WebServer����һ��FCGI��Ƶ�
	pConn->init(this); 
	while(!GlobalExitFlag)
	{
		
		FD_ZERO(&incoming);
		FD_SET(sock_id,&incoming);
		sockaddr_in addr;
		int addr_length = sizeof (addr );
		tv.tv_sec=5;tv.tv_usec=0; 
		if(select(sock_id+1,&incoming,NULL,NULL,&tv)<0)
		{
#ifdef _WIN32
			Sleep(milliseconds);  //���rd_set,wr_setΪ�գ�Windows��ֱ�ӷ��أ�������������
#endif
		}
		time(&global_polling_clock); //��ʱ����ȫ��ʱ��
		if(!FD_ISSET(sock_id,&incoming)) 
		{
			//DOUT<<"Time="<<global_polling_clock<<endl;
			continue;
		}
#ifdef WIN32
		int new_socket = ::accept(sock_id, ( sockaddr * ) &addr, (int * ) &addr_length );
#else
		int new_socket = ::accept(sock_id, ( sockaddr * ) &addr,((socklen_t*) &addr_length));
#endif
		const char *pip=inet_ntoa(addr.sin_addr); 
	
		static int in_coming_counter=0;
#ifdef _LIG_DEBUG
		DOUT<<"Accept FCGI : "<<pip<<"  counter="<<in_coming_counter++<<endl;
#endif
		if(new_socket<= 0)
		{
			//throw sdl_exception("Socket accept failed!! --li da gang");
			return;
		}

		if(max_fd<new_socket ) max_fd = new_socket;
		if(!set_nonblock(new_socket)) //��socket����Ϊ������״̬
		{
#ifdef _LIG_DEBUG
				DOUT<<"Cann't set to non-block socket!\n";
#endif
		}

		pConn->socket_id=new_socket;
		pConn->link_stat=write_finished; //��ʼ�׶Σ���Ҫ�ٶ�д�����Ѿ����
		pConn->inBuf.clear();
		pConn->outBuf.clear();  
		
		bool flag_fcgi_link_ok=true;
		int select_rt;
		FD_ZERO(&wr_set);
		while(flag_fcgi_link_ok)
		{
			FD_ZERO(&rd_set);
			FD_SET(new_socket,&rd_set);
		
			if((select_rt=select(max_fd + 1, &rd_set, &wr_set, NULL, &tv)) < 0) //�������߳�������
			{
	#ifdef _WIN32
				Sleep(milliseconds);  //���rd_set,wr_setΪ�գ�Windows��ֱ�ӷ��أ�������������
				continue;
	#endif
			}

			if(select_rt>0 && pConn->link_stat==write_ready && FD_ISSET(new_socket,&wr_set))  //����д
			{
				//DOUT<<"Come in Write.....Socket="<<pConn->socket_id<<endl;
				if(pConn->write_fcgi_server()==1 && pConn->flag_fcgi_close_state) ; //д����Ժ�pConn->link_statӦ�ô��� write_finished״̬
				{
						//DOUT<<"WebServer request close conn : "<<pConn->socket_id<<endl;
#ifdef _WIN32
						::closesocket(pConn->socket_id);
#else
						close(pConn->socket_id);
#endif
						break; //���½���ȴ�socket�ȴ�����
				}
				if(pConn->link_stat==conn_finished) //д������Դ�ڶԷ��������رջ��ߴ���
				{
						//DOUT<<"Close Socket : "<<pConn->socket_id<<endl;
#ifdef _WIN32
						::closesocket(pConn->socket_id);
#else
						close(pConn->socket_id);
#endif
						flag_fcgi_link_ok=false;
						break; //����д�����˳�
				}

	

			}

			if(select_rt>0 && FD_ISSET(new_socket,&rd_set)) //�û�����׼����
			{
				//DOUT<<"Come in FCGI Read.....Socket="<<pConn->socket_id<<endl;
				pConn->read_fcgi_server(); //һ�������ݽ��룬������ж����������������ȡ���ݣ�״̬�ı�Ϊread_finished
				if(pConn->link_stat==conn_finished) //���������ִ���״̬���Ϊconn_finished��Ҫ��ر����ӣ��˴��ر�
				{
					//DOUT<<"Close Socket : "<<pConn->socket_id<<endl;
#ifdef _WIN32
					::closesocket(pConn->socket_id);
#else
					close(pConn->socket_id);
#endif
					flag_fcgi_link_ok=false;
					break; //����д�����˳�
				}
			}

			if(pConn->link_stat==read_finished) //read���Ѿ�ȷ������������������Ҫ�ҵ����Ǹ����������
			{
				ConnList::iterator it=m_connDB.begin();
				while(it!=m_connDB.end()) //�����Ѿ�����������
				{
					tv.tv_sec=0;tv.tv_usec=1000; //�ٶ��߳�δִ����ϣ�����select��ʱӦ�����õñȽ�С�������ڿ�����Ӧ  
					http_conn *pExcute=(http_conn*)(&(*it));
					if(pExcute->link_stat==write_ready) //����ű�����Ѿ�ִ����ϣ�����Ҳ�Ѿ�д��
					{
						FCGI_Header header;
						int contLength=1024-sizeof(FCGI_Header);
						//-----------------------------------
						header.version=0x01;  //��дFCGI���ͷ
						header.type=FCGI_STDOUT;
						header.requestIdB1=pExcute->socket_id / 256;
						header.requestIdB0=pExcute->socket_id % 256;
						header.contentLengthB1=contLength / 256;
						header.contentLengthB0=contLength % 256;
						header.paddingLength=0;
						header.reserved=0;
						//-----------------------------------
						int i;for(i=0;i<(pExcute->outBuf.buf_curr)/contLength;++i)  //ע�⣬�˴��������ҳ��Ϊ�˱��ִ���Ч�ʣ�ÿ��FCGI STDOUT����С����Ϊ1K  
						{
							pConn->outBuf.direct_write((char*)&(header),sizeof(header)); 
							pConn->outBuf.direct_write(it->outBuf.p_buf+(i*contLength),contLength);   
						}
						int writeLength=pExcute->outBuf.buf_curr % contLength; //�õ�ʣ����ֽ�
						header.contentLengthB1=writeLength / 256;
						header.contentLengthB0=writeLength % 256;
						pConn->outBuf.direct_write((char*)&(header),sizeof(header)); 
						pConn->outBuf.direct_write(pExcute->outBuf.p_buf+(i*contLength),writeLength); 
						//-----��д������
						FCGI_EndRequestBody endBody;
						header.type=FCGI_END_REQUEST;
						header.contentLengthB1=0;
						header.contentLengthB0=sizeof(FCGI_EndRequestBody); 
						endBody.protocolStatus=FCGI_REQUEST_COMPLETE;
						endBody.appStatusB0=0;
						endBody.appStatusB1=0;
						endBody.appStatusB2=0;
						endBody.appStatusB3=0; 
						pConn->outBuf.direct_write((char*)&(header),sizeof(header)); 
						pConn->outBuf.direct_write((char*)&(endBody),sizeof(endBody)); 
						pConn->link_stat=write_ready; //���������д������ 
						//------------------------------
						//--������ϣ�׼���������Ҫ������
						Request_Table[pExcute->socket_id]=NULL; //������ע�����ɾ������Ҫ�ļ�¼
						m_connDB.erase(it); //���Զ�ִ������
						tv.tv_sec=1;tv.tv_usec=0; //�Ѿ������������select��ʱ�������ó�һЩ
						break; //�Ѿ�������д����Ҫ��socket�������д������
					}
					++it;
				}
			} //���û�д���,pConn->link_statӦ�û���read_finished״̬
				
			FD_ZERO(&wr_set);if(pConn->link_stat==write_ready) FD_SET(new_socket,&wr_set); //Ҫ�����д
			

		}	
		
	}
	delete pConn;
};

void lig_httpd::begin()
{
	http_conn *pConn;
	url_map* pRt;
#ifdef _LIG_DEBUG
	DOUT<<"Web Server is starting..."<<endl;
#endif
	timeval ttv;
	while(!GlobalExitFlag)
	{
		list<http_conn*>::iterator it;

		FD_ZERO(&rd_set);
		FD_ZERO(&wr_set);
re_que:
		it = m_connList.begin();
		bool flag_socket_ok=false;
		while(it != m_connList.end())
		{
			pConn=*it;
			if(pConn->link_stat==conn_finished || //�Ѿ����Ϊ������socket
				(global_polling_clock-pConn->time_stamp)>time_out)  //���� timeout�޷����������ݣ��ر�����
			{
				//DOUT<<"Close Socket : "<<pConn->socket_id<<endl;
#ifdef _WIN32
				::closesocket(pConn->socket_id);
#else
				close(pConn->socket_id);
#endif
				list<http_conn*>::iterator eit=it++;
				m_connDB.free(*eit); //�����������
				m_connList.erase(eit); //�Ӷ�����ɾ��
				continue;
			}
			if(pConn->link_stat==read_ready)  //����״̬���ж��Ƿ���������
			{
				FD_SET(pConn->socket_id,&rd_set);
				flag_socket_ok=true;
			}
			if(pConn->link_stat==write_ready)  //����״̬���ж��Ƿ����д����
			{
				FD_SET(pConn->socket_id,&wr_set);
				flag_socket_ok=true;
			}
			++it;
		}
		if(!flag_socket_ok) 
		{
#ifdef WIN32
			Sleep(25);
#else
			usleep(25);
#endif			
			continue;
		}
		//DOUT<<"Will come in select...size="<<m_connList.size()<<endl;	
		ttv.tv_sec=0;ttv.tv_usec=5000;
		if(select(max_fd + 1, &rd_set, &wr_set, NULL, &ttv) < 0) //�������߳�������
		{
#ifdef _WIN32
			Sleep(25);  //���rd_set,wr_setΪ�գ�Windows��ֱ�ӷ��أ������������ӵȴ�ʱ��
			goto re_que;
#endif
		}
		//DOUT<<"Select OK..."<<endl;
		it = m_connList.begin();  //�����н�����е����ӽ��б���
		
		while(it != m_connList.end())
		{
			pConn=*it;
		
			if(pConn->link_stat==read_ready && FD_ISSET(pConn->socket_id,&rd_set)) //�û�����׼����
			{
				//DOUT<<"Come in Read.....Socket="<<pConn->socket_id<<endl;
				pConn->read_client();
				if(pConn->link_stat==read_finished)//����������ͷ
				{
					if(pConn->header.pUri==NULL)
					{
#ifdef _LIG_DEBUG
						DOUT<<"Bad Request.."<<endl;
#endif
						pConn->link_stat=conn_finished; 
						++it;
						continue;
					}

					if(*(pConn->header.pUri)==0) //���û������д��ȱʡҳ���ļ�
						strcpy(pConn->header.pUri,defaulf_page.c_str());

					if(check_register(pConn->header.pUri,pRt))
					{
						pConn->pUrlMap=(void*)(&(*pRt));
						VMthread.push_excuting(pConn);
						continue;
					}

					if(memcmp(pConn->header.pUri,"hnut/",5)==0)
					{
						const char* pURI=pConn->header.pUri+5;
						strcpy(pConn->header.pUri,pConn->header.pUri+5);
#ifdef _LIG_DEBUG
						DOUT<<"It's a excuted file,url="<<pConn->header.pUri<<endl;
#endif
						pURI=pConn->header.pUri;
						char *pStr=(char*)pURI;
						while(*pStr!=0 && *pStr!='_') pStr++;
						bool flag_uri_translate=false;
						if(*pStr!=0) {flag_uri_translate=true;*pStr=0;}
						if(check_register(pConn->header.pUri,pRt))
						{
							if(flag_uri_translate) *pStr='_';
							pConn->flag_uri_translate=flag_uri_translate; //�����Ƿ�Ҫ�����URIת�� 
							pConn->pUrlMap=(void*)(&(*pRt));
							VMthread.push_excuting(pConn);
							continue;
						}

					}
				
					pConn->pUrlMap=NULL;
					(pConn->*(pConn->action))(); //ִ��ȱʡ�������
					pConn->time_stamp=global_polling_clock;
				}
			}

			if(pConn->link_stat==write_ready && FD_ISSET(pConn->socket_id,&wr_set))  //����д
			{
				//DOUT<<"Come in Write.....Socket="<<pConn->socket_id<<endl;
				pConn->write_client(); 
			}
			
			++it;
		}

	}
}

const char*  http_conn::query_val(const char* name)
{
	vector<VARS>::iterator it=header.varList.begin();
	while(it!=header.varList.end())
	{
		if(it->name==NULL || it->value_length==0) return NULL;
		if(strcmp(it->name,name)==0) 
			return it->value;
		++it;
	}
	return NULL;
}
bool lig_httpd::set_listen()
{
	sockaddr_in addr;

	if(port>0) 
	{
		sock_id = socket ( AF_INET, SOCK_STREAM, 0 );
		set_nonblock(sock_id);  //Ҫ�����socket�Ƿ�������
		if (sock_id<=0) 
		{
			throw sdl_exception("Create Socket Error!! --li gang");
			return false;
		}

		int on = 1;
		if ( setsockopt (sock_id, SOL_SOCKET, SO_REUSEADDR, ( const char* ) &on, sizeof ( on ) ) == -1 )
		{
			throw sdl_exception("Create Socket Error!! --li gang");
			return false;
		}

		addr.sin_family = AF_INET;
		if(flag_local_listen)
			addr.sin_addr.s_addr = inet_addr("127.0.0.1");
		else
			addr.sin_addr.s_addr = INADDR_ANY;
		addr.sin_port = htons ( port );
		int bind_return = ::bind(sock_id,( struct sockaddr * ) &addr, sizeof (addr ) );
		if( bind_return == -1 )  
		{ 
			throw sdl_exception("Socket Can't bind the port!! --li  gang");
			return false;
		}
		int listen_return = ::listen ( sock_id, MAXLISTENS);
		if( listen_return == -1 )
		{
			throw sdl_exception("Socket Can't listen!! --li  gang");
			return false;
		}

		if(max_fd<sock_id ) max_fd = sock_id;


		return true;
	}
	return false;
}

void lig_httpd::init(int PORT,int timeout)
{
	http_poll::init(PORT);
	time_out=timeout;
	
#ifdef WIN32

	WORD wVersionRequested;
	WSADATA wsaData;
	int err;

	wVersionRequested = MAKEWORD( 2, 2 );

	err = WSAStartup( wVersionRequested, &wsaData );
	if ( err != 0 )
	{
		/* Tell the user that we could not find a usable */
		/* WinSock DLL. 								 */
		throw sdl_exception("Can't find Winsocket2!! --li gang",VERY_BIG_ERR);
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
			throw sdl_exception("Can't find Winsocket2!! --li gang");
			WSACleanup( );
			return; 
	}
	
#endif

}
//----------------------------------------------------------------------------------------------------------
bool http_mime_request(const char *boundary_str,const char *buf,int buf_size,list<mime_data_pair> &mime_list)
{

	register char ch;
	string tmpstr;
	mime_data me;

	int cache[64];
	for(int i=0;i<64;++i) cache[i]=0;
	int str_length=strlen(boundary_str);
	if(str_length>64) return false;
	KMP_init(boundary_str,cache);
	
	pair<int,int> sg;

	int loc=0,fd=0;
	sg.first=-1;
	while(loc<buf_size)
	{
		if(sg.first==-1)
		{
			//fd=str_find(buf+loc,buf_size-loc,boundary_str,str_length);
			fd=KMP_find(buf+loc,buf_size-loc,boundary_str,str_length,cache);
			if(fd==-1) break;
			loc+=fd+str_length;sg.first=loc;
		}
		//fd=str_find(buf+loc,buf_size-loc,boundary_str,str_length);
		fd=KMP_find(buf+loc,buf_size-loc,boundary_str,str_length,cache);
		if(fd==-1) break;
		loc+=fd;sg.second=loc;
		{
			//------------------------------------------------------------------
			mime_header* ph=me.header;
			const char* p_begin=buf+sg.first+2; //����/r/n
			const char* p_end=p_begin;
			while((*p_end!='\r' || *(p_end+1)!='\n' || *(p_end+2)!='\r' || *(p_end+3)!='\n') && p_end<p_begin+sg.second-4) 
				++p_end; //�ҵ�MIMEͷ��λ��  
			if(p_end==p_begin+sg.second-4) return false; //�Ҳ�����ʼλ��
			
			while(ph->id!=-1)
			{
				ph->value_str=nocase_find_str(p_begin,p_end,ph->name);
				if(ph->value_str!=NULL)
				{
					ph->value_length=0;
					while(*(ph->value_str)==' ' || *(ph->value_str)=='\t') ++ph->value_str; //ȥ���ո�

					if(ph->type!=MIME_QUOTE_STRING)
					{
						ch = *(ph->value_str+ph->value_length);	
						while(ch != '\r' && ch!= '\n' &&  ch!=';' && ch!=',')  //�ҵ����ݳ���
						{
							++ph->value_length;
							ch=*(ph->value_str+ph->value_length);	
						}
					}

					if(ph->type==MIME_QUOTE_STRING)
					{
						++ph->value_str; //������һ��˫����
						ch = *(ph->value_str+ph->value_length);	
						while(ch != '\"')  //ֱ���ڶ���˫���ţ��ҵ����ݳ���
						{
							++ph->value_length;
							ch=*(ph->value_str+ph->value_length);	
						}
					}
				}
				ph++;
			}
		//-----------------------------------------------------------
			me.p_data=p_end+4;  //������ʼλ����2�� \r\n֮����������4���ֽ�
			me.data_length=(sg.second-sg.first)-(p_end-p_begin); 
			me.data_length-=10; //-8 ����Ϊһ��3������/r/n , boundary�ַ����������ַ���ռλ��ʣ�������ַ��в��ܽ���
		
			mime_list.push_back(mime_data_pair());
			ph=me.header; //�ָ���ʼλ��
			
			while(ph->id!=-1)
			{
				if(ph->value_length>0)
				{
				
					if(ph->id==0)
					{
						tmpstr="";
						tmpstr.append(ph->value_str,ph->value_length);
						if(tmpstr!="form-data") break;
					}
					if(ph->id==1)  mime_list.back().name.append(ph->value_str,ph->value_length); 
					if(ph->id==2)  mime_list.back().filename.append(ph->value_str,ph->value_length);
				}
				ph->value_length=0;
				++ph;
			}; //������������
			if(me.p_data!=NULL)
			{
				if(mime_list.back().filename!="") //ȷ�����ļ�
				{
					mime_list.back().value=mime_list.back().filename; 
					mime_list.back().p_data=me.p_data;
					mime_list.back().data_length=me.data_length;
					/*
					ofstream off;
					off.open("tlig.txt",ios::binary);
					off.write(me.p_data,me.data_length); 
					*/
				}
				else //ֻ��һ������ݶ���
				{
					me.data_length=0;
					mime_list.back().value.append(me.p_data,me.data_length);    
				}
			}
			mime_data_pair dp=mime_list.back();
		}
		loc+=str_length;sg.first=loc;
		
	}
	return true;
}

SQInteger Sq2_http_header_addvar(HSQUIRRELVM v) 
{
	http_header* handle;
	const char* key;
	const char* value;
	if(SQ_SUCCEEDED(sq_getinstanceup(v,1,(SQUserPointer*)(&handle),0))
		&& SqGetVar(v,key,value)) handle->add_vars(key,value);
	return 0;
}