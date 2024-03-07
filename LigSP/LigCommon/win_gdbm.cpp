/*
这段代码来源于http://xmailserver.org/gdbm-win32.html， 是为了在WIN32下使用GDBM而改写的

由于原来的代码由过多的文件组成，不便于使用，因此我将原来的文件合并的一个文件内，去掉了很多无意义的AUTOCONFIG
这样，在Win32环境下，使用GDBM更加容易，我同时去掉了老UNIX的DBM接口定义，也就是说，这段代码只支持GDBM格式的调用。

由于去掉了跨平台支持，这段代码只能在WINDOWS下编译（在VC2008上实验通过），不过在UNIX/Linux下，GDBM都是直接可以
使用的libgdbm，也不需要自己做编译。

																					2009.05.11 李钢 TEDC,CAAC

This file is part of GDBM, the GNU data base manager, by Philip A. Nelson.
	Copyright (C) 1990, 1991, 1993  Free Software Foundation, Inc.

	GDBM is free software; you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation; either version 2, or (at your option)
	any later version.

	GDBM is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
	GNU General Public License for more details.

	You should have received a copy of the GNU General Public License
	along with GDBM; see the file COPYING.  If not, write to
	the Free Software Foundation, 675 Mass Ave, Cambridge, MA 02139, USA.

	You may contact the author by:
	e-mail:  phil@cs.wwu.edu
	us-mail:  Philip A. Nelson
				Computer Science Department
				Western Washington University
				Bellingham, WA 98226
	
const char * gdbm_version =	"This is GDBM version 1.7.3, as of May 19, 1994.";


//以下是一段使用GDBM的描述,来源与Internet


gdbm使用一个数据文件，与dbm不同。注意：不要使用读写函数直接操作数据文件，应该使用gdbm提供的数据操作函数访问数据。

数据、索引都使用以下结构保存（与dbm相同）：

typedef struct{
        char *dptr;
        int   dsize;
      } datum;
 

数据库的访问结构（等同于FILE）：

       typedef struct {int dummy[10];} *GDBM_FILE;

 
//打开数据库
GDBM_FILE  *gdbm_open(
       char *name;			   //用于保存数据库路径文件名
       int block_size;         //设置内存与磁盘直接io传递数据的单位，最小512字节
       int read_write;         //可设置为：GDBM_READER、GDBM_WRITER、GDBM_WRCREATER、GDBM_NEWDB
       int mode; ):              //文件打开模式。与chmod（2）、open（2）相似

//成功返回GDBM_FILE类型指针，否则返回NULL。

 

//存储数据
int gdbm_store(GDBM_FILE *database_descriptor,    //前面打开数据库操作返回的结构
       datum key,                                         //检索数据的关键字
       datum content,                                          //用于保存数据的结构
       int store_mode)                                          //如果为gdbm_insert，则数据存在时操作会失败
//与DBM相同
//如果一个只读打开的数据库调用了这个函数，则返回－1；
//其他情况返回0（操作成功）；
//注意：gdbm的存储数据大小没有限制，这与dbm、ndbm不同。



//检索数据
datum gdbm_fetch(GDBM_FILE *database_descreiptor,    //gdbm_open返回的数据结构
       datum key                                         //检索使用的关键字
);    //与DBM相同

//如果返回值的dptr字段为NULL，则没有找到数据；
//注意：gdbm自动分配dptr的存储空间（使用malloc（3C）），但不会自动释放这个空间。释放空间的责任交给了程序员完成。

 

//关闭数据库

void gdbm_close(GDBM_FILE *database_descriptor); //gdbm_open返回的数据结构

 

//其他函数简介

int gdbm_exist(GDBM_FILE dbf,datum key);     //检查数据库中是否存在key对应的数据

//返回true表示找到数据，返回false表示数据不存在。

 

int gdbm_delete(GDBM_FILE *database_descriptor,datum key); //删除索引为key的数据

//返回0表示操作成功，返回－1表示没有找到key对应的数据。

 

char *gdbm_strerror(gdbm_error errno);     //把错误代号转换为英文字符串。

//返回可打印的错误描述字符串。

 

int gdbm_setopt(GDBM_FILE dbf, int option, int value, int size); //设置已经打开的数据库的参数，可以设置系统内部cache大小、设置快速模式（此功能已过时）、打开或关闭中央空闲数据块存储池。/

//返回0表示设置操作成功，返回－1表示操作失败。

 

int gdbm_fdesc(dbf);     //多用户共享时，为了给数据库文件加锁，设置已打开的数据库的标志。

 

key=gdbm_firstkey(dbf)

nextkey=gdbm_nextkey(dbf,key); //与dbm相似，用于访问所有数据。

例如：

       key ＝ gdbm_firstkey(dbf);

       while(key.dptr)

       {

              nextkey = gdbm_nextkey(dbf,key);

              …    //某些循环中的处理操作

              key = nextkey;

       }

*/


#include <sys/types.h>
#include <stdio.h>
#include <sys/stat.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#ifdef WIN32
#include <windows.h>
#else
#include <unistd.h>
#include <sys/types.h>
#define _lseek lseek
#define _write write
#define _read read
#define _close close
#define _unlink unlink
#define _open open
#endif
#ifndef SEEK_SET
#define SEEK_SET        0
#endif

#ifndef L_SET
#define L_SET SEEK_SET
#endif

/* Do we have bcopy?  */

#define bcmp(d1, d2, n)	memcmp(d1, d2, n)
#define bcopy(d1, d2, n) memcpy(d2, d1, n)


/* Do we have fsync? */
#ifndef HAVE_FSYNC
#define fsync(f) {sync(); sync();}
#endif


/* Default block size.  Some systems do not have blocksize in their
stat record. This code uses the BSD blocksize from stat. */
#define STATBLKSIZE 1024

/* Do we have ftruncate? */
#ifdef HAVE_FTRUNCATE
#define TRUNCATE(dbf) ftruncate (dbf->desc, 0)
#else
#define TRUNCATE(dbf) _close( _open (dbf->name, O_RDWR|O_TRUNC, mode));
#endif

/* Do we have 32bit or 64bit longs? */
#ifdef LONG_64_BITS 
//	|| #ifndef INT_16_BITS
typedef int word_t;
#else
typedef long word_t;
#endif

//	Added by "Dave Roth" rothd@roth.net 970829
//	Modifications made by "Sergei Romanenko" <roman@spp.Keldysh.ru> 980602
#ifdef	WIN32
#include <io.h>

int	lock_val;
#undef	UNLOCK_FILE
#define	UNLOCK_FILE(dbf) lock_val = !UnlockFile((HANDLE) _get_osfhandle((int) dbf->desc ), 0, 0, 0xffffffff, 0xffffffff);
#undef	WRITELOCK_FILE
#define	WRITELOCK_FILE(dbf)	 lock_val = !LockFile((HANDLE) _get_osfhandle((int) dbf->desc ), 0, 0, 0xffffffff, 0xffffffff);
#undef	READLOCK_FILE
#define	READLOCK_FILE(dbf)	lock_val = !LockFile((HANDLE) _get_osfhandle((int) dbf->desc ), 0, 0, 0xffffffff, 0xffffffff);

#undef	fsync
#define fsync(dbf)  _commit(dbf)

//	Redefine open() so files are opened in binary mode
#define	open(x, y, z)	open(x, y | O_BINARY, z)
#else
#define  O_BINARY 0
#define	UNLOCK_FILE(dbf) 
#define  WRITELOCK_FILE(dbf)
#define  READLOCK_FILE(dbf)
#endif	//	WIN32
 


#define  TRUE    1
#define  FALSE   0

/* Parameters to gdbm_open. */
#define  GDBM_READER  0		/* READERS only. */
#define  GDBM_WRITER  1		/* READERS and WRITERS.  Can not create. */
#define  GDBM_WRCREAT 2		/* If not found, create the db. */
#define  GDBM_NEWDB   3		/* ALWAYS create a new db.  (WRITER) */
#define  GDBM_FAST    16	/* Write fast! => No fsyncs. */

/* Parameters to gdbm_store for simple insertion or replacement in the
case a key to store is already in the database. */
#define  GDBM_INSERT  0		/* Do not overwrite data in the database. */
#define  GDBM_REPLACE 1		/* Replace the old value with the new value. */

/* Parameters to gdbm_setopt, specifing the type of operation to perform. */
#define	 GDBM_CACHESIZE	1	/* Set the cache size. */
#define  GDBM_FASTMODE	2	/* Turn on or off fast mode. */

/* In freeing blocks, we will ignore any blocks smaller (and equal) to
IGNORE_SIZE number of bytes. */
#define IGNORE_SIZE 4

/* The number of key bytes kept in a hash bucket. */
#define SMALL    4

/* The number of bucket_avail entries in a hash bucket. */
#define BUCKET_AVAIL 6

/* The size of the bucket cache. */
#define DEFAULT_CACHESIZE  100

/* The type definitions are next.  */

/* The data and key structure.  This structure is defined for compatibility. */

typedef struct {
	char *dptr;
	int   dsize;
} datum;


/* The available file space is stored in an "avail" table.  The one with
most activity is contained in the file header. (See below.)  When that
one filles up, it is split in half and half is pushed on an "avail
stack."  When the active avail table is empty and the "avail stack" is
not empty, the top of the stack is popped into the active avail table. */

/* The following structure is the element of the avaliable table.  */
typedef struct {
	int   av_size;		/* The size of the available block. */
	off_t  av_adr;		/* The file address of the available block. */
} avail_elem;

/* This is the actual table. The in-memory images of the avail blocks are
allocated by malloc using a calculated size.  */
typedef struct {
	int   size;		/* The number of avail elements in the table.*/
	int   count;		/* The number of entries in the table. */
	off_t next_block;	/* The file address of the next avail block. */
	avail_elem av_table[1]; /* The table.  Make it look like an array.  */
} avail_block;

/* The dbm file header keeps track of the current location of the hash
directory and the free space in the file.  */

typedef struct {
	word_t header_magic;  /* 0x13579ace to make sure the header is good. */
	int   block_size;    /* The  optimal i/o blocksize from stat. */
	off_t dir;	     /* File address of hash directory table. */
	int   dir_size;	     /* Size in bytes of the table.  */
	int   dir_bits;	     /* The number of address bits used in the table.*/
	int   bucket_size;   /* Size in bytes of a hash bucket struct. */
	int   bucket_elems;  /* Number of elements in a hash bucket. */
	off_t next_block;    /* The next unallocated block address. */
	avail_block avail;   /* This must be last because of the psuedo
						array in avail.  This avail grows to fill
						the entire block. */
}  gdbm_file_header;


/* The dbm hash bucket element contains the full 31 bit hash value, the
"pointer" to the key and data (stored together) with their sizes.  It also
has a small part of the actual key value.  It is used to verify the first
part of the key has the correct value without having to read the actual
key. */

typedef struct {
	word_t hash_value;	/* The complete 31 bit value. */
	char  key_start[SMALL];	/* Up to the first SMALL bytes of the key.  */
	off_t data_pointer;	/* The file address of the key record. The
						data record directly follows the key.  */
	int   key_size;		/* Size of key data in the file. */
	int   data_size;	/* Size of associated data in the file. */
} bucket_element;


/* A bucket is a small hash table.  This one consists of a number of
bucket elements plus some bookkeeping fields.  The number of elements
depends on the optimum blocksize for the storage device and on a
parameter given at file creation time.  This bucket takes one block.
When one of these tables gets full, it is split into two hash buckets.
The contents are split between them by the use of the first few bits
of the 31 bit hash function.  The location in a bucket is the hash
value modulo the size of the bucket.  The in-memory images of the
buckets are allocated by malloc using a calculated size depending of
the file system buffer size.  To speed up write, each bucket will have
BUCKET_AVAIL avail elements with the bucket. */

typedef struct {
	int   av_count;		   /* The number of bucket_avail entries. */
	avail_elem bucket_avail[BUCKET_AVAIL];  /* Distributed avail. */
	int   bucket_bits;	   /* The number of bits used to get here. */
	int   count;		   /* The number of element buckets full. */
	bucket_element h_table[1]; /* The table.  Make it look like an array.*/
} hash_bucket;

/* We want to keep from reading buckets as much as possible.  The following is
to implement a bucket cache.  When full, buckets will be dropped in a
least recently read from disk order.  */

/* To speed up fetching and "sequential" access, we need to implement a
data cache for key/data pairs read from the file.  To find a key, we
must exactly match the key from the file.  To reduce overhead, the
data will be read at the same time.  Both key and data will be stored
in a data cache.  Each bucket cached will have a one element data
cache.  */

typedef struct {
	word_t hash_val;
	int   data_size;
	int   key_size;
	char *dptr;
	int   elem_loc;
}  data_cache_elem;

typedef struct {
	hash_bucket *   ca_bucket;
	off_t           ca_adr;
	char		ca_changed;   /* Data in the bucket changed. */
	data_cache_elem ca_data;
} cache_elem;



/* This final structure contains all main memory based information for
a gdbm file.  This allows multiple gdbm files to be opened at the same
time by one program. */

typedef struct {
	/* Global variables and pointers to dynamic variables used by gdbm.  */

	/* The file name. */
	char *name;

	/* The reader/writer status. */
	int read_write;

	/* Fast_write is set to 1 if no fsyncs are to be done. */
	int fast_write;

	/* The fatal error handling routine. */
	void (*fatal_err)(char*);

	/* The gdbm file descriptor which is set in gdbm_open.  */
	int  desc;

	/* The file header holds information about the database. */
	gdbm_file_header *header;

	/* The hash table directory from extendible hashing.  See Fagin et al, 
	ACM Trans on Database Systems, Vol 4, No 3. Sept 1979, 315-344 */
	off_t *dir;

	/* The bucket cache. */
	cache_elem *bucket_cache;
	int cache_size;
	int last_read;

	/* Points to the current hash bucket in the cache. */
	hash_bucket *bucket;

	/* The directory entry used to get the current hash bucket. */
	word_t bucket_dir;

	/* Pointer to the current bucket's cache entry. */
	cache_elem *cache_entry;


	/* Bookkeeping of things that need to be written back at the
	end of an update. */
	char  header_changed;
	char  directory_changed;
	char  bucket_changed;
	char  second_changed;

} gdbm_file_info;

#ifdef _ARGS
#undef _ARGS
#endif

#ifdef __STDC__
#define _ARGS(args) args
#else
#define _ARGS(args) args
#endif


/* From bucket.c */
void _gdbm_new_bucket    _ARGS((gdbm_file_info *dbf, hash_bucket *bucket,
int bits ));
void _gdbm_get_bucket    _ARGS((gdbm_file_info *dbf, word_t dir_index ));
void _gdbm_split_bucket  _ARGS((gdbm_file_info *dbf, word_t next_insert ));
void _gdbm_write_bucket  _ARGS((gdbm_file_info *dbf, cache_elem *ca_entry ));

/* From falloc.c */
off_t _gdbm_alloc       _ARGS((gdbm_file_info *dbf, int num_bytes ));
void _gdbm_free         _ARGS((gdbm_file_info *dbf, off_t file_adr,int num_bytes ));
int  _gdbm_put_av_elem  _ARGS((avail_elem new_el, avail_elem av_table [],int *av_count ));

/* From findkey.c */
char *_gdbm_read_entry  _ARGS((gdbm_file_info *dbf, int elem_loc));
int _gdbm_findkey       _ARGS((gdbm_file_info *dbf, datum key,  char **dptr,word_t *new_hash_val));

/* From hash.c */
word_t _gdbm_hash _ARGS((datum key ));

/* From update.c */
void _gdbm_end_update   _ARGS((gdbm_file_info *dbf ));
int _gdbm_fatal         _ARGS((gdbm_file_info *dbf, char *val ));

/* From gdbmopen.c */
int _gdbm_init_cache	_ARGS((gdbm_file_info *dbf, int size));

/* The user callable routines.... */
void  gdbm_close	  _ARGS((gdbm_file_info *dbf ));
int   gdbm_delete	  _ARGS((gdbm_file_info *dbf, datum key ));
datum gdbm_fetch	  _ARGS((gdbm_file_info *dbf, datum key ));
gdbm_file_info *gdbm_open _ARGS((char *file, int block_size,int flags, int mode,void (*fatal_func)(char*)));
int   gdbm_reorganize	  _ARGS((gdbm_file_info *dbf ));
datum gdbm_firstkey       _ARGS((gdbm_file_info *dbf ));
datum gdbm_nextkey        _ARGS((gdbm_file_info *dbf, datum key ));
int   gdbm_store          _ARGS((gdbm_file_info *dbf, datum key,
datum content, int flags ));
int   gdbm_exists	  _ARGS((gdbm_file_info *dbf, datum key));
void  gdbm_sync		  _ARGS((gdbm_file_info *dbf));
int   gdbm_setopt	  _ARGS((gdbm_file_info *dbf, int optflag,int *optval, int optlen));

/* Now define all the routines in use. */
typedef enum {	
	GDBM_NO_ERROR,
	GDBM_MALLOC_ERROR,
	GDBM_BLOCK_SIZE_ERROR,
	GDBM_FILE_OPEN_ERROR,
	GDBM_FILE_WRITE_ERROR,
	GDBM_FILE_SEEK_ERROR,
	GDBM_FILE_READ_ERROR,
	GDBM_BAD_MAGIC_NUMBER,
	GDBM_EMPTY_DATABASE,
	GDBM_CANT_BE_READER,
	GDBM_CANT_BE_WRITER,
	GDBM_READER_CANT_DELETE,
	GDBM_READER_CANT_STORE,
	GDBM_READER_CANT_REORGANIZE,
	GDBM_UNKNOWN_UPDATE,
	GDBM_ITEM_NOT_FOUND,
	GDBM_REORGANIZE_FAILED,
	GDBM_CANNOT_REPLACE,
	GDBM_ILLEGAL_DATA,
	GDBM_OPT_ALREADY_SET,
	GDBM_OPT_ILLEGAL
}gdbm_error;

extern gdbm_error gdbm_errno;

/* Close the dbm file and free all memory associated with the file DBF.
Before freeing members of DBF, check and make sure that they were
allocated.  */
const char * const gdbm_errlist[] = {
	"No error", "Malloc error", "Block size error", "File open error",
	"File write error", "File seek error", "File read error",
	"Bad magic number", "Empty database", "Can't be reader", "Can't be writer",
	"Reader can't delete", "Reader can't store", "Reader can't reorganize",
	"Unknown update", "Item not found", "Reorganize failed", "Cannot replace",
	"Illegal data", "Option already set", "Illegal option"
};

gdbm_file_info  *_gdbm_file = NULL;

/* Memory for return data for the "original" interface. */
datum _gdbm_memory = {NULL, 0};	/* Used by firstkey and nextkey. */
char *_gdbm_fetch_val = NULL;	/* Used by fetch. */

/* The dbm error number is placed in the variable GDBM_ERRNO. */
gdbm_error gdbm_errno = GDBM_NO_ERROR;


#define ADDRESS_FUNCTION(arg) &(arg)

#if __STDC__
typedef void *pointer;
#else
typedef char *pointer;
#endif

#ifdef NULL
#undef	NULL
#endif
#define	NULL	0

/* Define STACK_DIRECTION if you know the direction of stack
growth for your system; otherwise it will be automatically
deduced at run-time.

STACK_DIRECTION > 0 => grows toward higher addresses
STACK_DIRECTION < 0 => grows toward lower addresses
STACK_DIRECTION = 0 => direction of growth unknown  */

#ifndef STACK_DIRECTION
#define	STACK_DIRECTION	0	/* Direction unknown.  */
#endif

#if STACK_DIRECTION != 0

#define	STACK_DIR	STACK_DIRECTION	/* Known at compile-time.  */

#else /* STACK_DIRECTION == 0; need run-time code.  */

static int stack_dir;		/* 1 or -1 once known.  */
#define	STACK_DIR	stack_dir

static void
find_stack_direction ()
{
	static char *addr = NULL;	/* Address of first `dummy', once known.  */
	auto char dummy;		/* To get stack address.  */

	if (addr == NULL)
	{				/* Initial entry.  */
		addr = ADDRESS_FUNCTION (dummy);

		find_stack_direction ();	/* Recurse once.  */
	}
	else
	{
		/* Second entry.  */
		if (ADDRESS_FUNCTION (dummy) > addr)
		stack_dir = 1;		/* Stack grew upward.  */
		else
		stack_dir = -1;		/* Stack grew downward.  */
	}
}

#endif /* STACK_DIRECTION == 0 */

/* An "alloca header" is used to:
(a) chain together all alloca'ed blocks;
(b) keep track of stack depth.

It is very important that sizeof(header) agree with malloc
alignment chunk size.  The following default should work okay.  */

#ifndef	ALIGN_SIZE
#define	ALIGN_SIZE	sizeof(double)
#endif

typedef union hdr
{
	char align[ALIGN_SIZE];	/* To force sizeof(header).  */
	struct
	{
		union hdr *next;		/* For chaining headers.  */
		char *deep;		/* For stack depth measure.  */
	} h;
} header;

static header *last_alloca_header = NULL;	/* -> last alloca header.  */

/* Return a pointer to at least SIZE bytes of storage,
which will be automatically reclaimed upon exit from
the procedure that called alloca.  Originally, this space
was supposed to be taken from the current stack frame of the
caller, but that method cannot be made to work for some
implementations of C, for example under Gould's UTX/32.  */

pointer alloca (unsigned size)
{
	auto char probe;		/* Probes stack depth: */
	register char *depth = ADDRESS_FUNCTION (probe);

#if STACK_DIRECTION == 0
	if (STACK_DIR == 0)		/* Unknown growth direction.  */
	find_stack_direction ();
#endif

	/* Reclaim garbage, defined as all alloca'd storage that
	was allocated from deeper in the stack than currently. */

	{
		register header *hp;	/* Traverses linked list.  */

		for (hp = last_alloca_header; hp != NULL;)
		if ((STACK_DIR > 0 && hp->h.deep > depth)
				|| (STACK_DIR < 0 && hp->h.deep < depth))
		{
			register header *np = hp->h.next;

			free ((pointer) hp);	/* Collect garbage.  */

			hp = np;		/* -> next header.  */
		}
		else
		break;			/* Rest are not deeper.  */

		last_alloca_header = hp;	/* -> last valid storage.  */
	}

	if (size == 0)
	return NULL;		/* No allocation required.  */

	/* Allocate combined header + user data storage.  */

	{
		register pointer new_pointer = (pointer)malloc (sizeof (header) + size);
		/* Address of header.  */

		((header *) new_pointer)->h.next = last_alloca_header;
		((header *) new_pointer)->h.deep = depth;

		last_alloca_header = (header *) new_pointer;

		/* User storage begins just after header.  */

		return (pointer) ((char *) new_pointer + sizeof (header));
	}
}




static avail_elem get_elem _ARGS((int, avail_elem [], int *));
static avail_elem get_block _ARGS((int, gdbm_file_info *));
static void push_avail_block _ARGS((gdbm_file_info *));
static void pop_avail_block _ARGS((gdbm_file_info *));
static void adjust_bucket_avail _ARGS((gdbm_file_info *));
static void write_header _ARGS((gdbm_file_info *));

/* This procedure writes the header back to the file described by DBF. */

static void write_header (gdbm_file_info *dbf)
{
	int  num_bytes;	/* Return value for write. */
	off_t file_pos;	/* Return value for lseek. */

	file_pos = _lseek (dbf->desc, 0L, L_SET);
	if (file_pos != 0) _gdbm_fatal (dbf, "lseek error");
	num_bytes = _write (dbf->desc, dbf->header, dbf->header->block_size);
	if (num_bytes != dbf->header->block_size)
	_gdbm_fatal (dbf, "write error");

	/* Wait for all output to be done. */
	if (!dbf->fast_write)
	fsync (dbf->desc);
}


/* After all changes have been made in memory, we now write them
all to disk. */
void _gdbm_end_update(gdbm_file_info *dbf)
{
	int  num_bytes;	/* Return value for write. */
	off_t file_pos;	/* Return value for lseek. */


	/* Write the current bucket. */
	if (dbf->bucket_changed && (dbf->cache_entry != NULL))
	{
		_gdbm_write_bucket (dbf, dbf->cache_entry);
		dbf->bucket_changed = FALSE;
	}

	/* Write the other changed buckets if there are any. */
	if (dbf->second_changed)
	{
		if(dbf->bucket_cache != NULL)
		{
			register int index;

			for (index = 0; index < dbf->cache_size; index++)
			{
				if (dbf->bucket_cache[index].ca_changed)
				_gdbm_write_bucket (dbf, &dbf->bucket_cache[index]);
			}
		}
		dbf->second_changed = FALSE;
	}

	/* Write the directory. */
	if (dbf->directory_changed)
	{
		file_pos = _lseek (dbf->desc, dbf->header->dir, L_SET);
		if (file_pos != dbf->header->dir) _gdbm_fatal (dbf, "lseek error");
		num_bytes = _write (dbf->desc, dbf->dir, dbf->header->dir_size);
		if (num_bytes != dbf->header->dir_size)
		_gdbm_fatal (dbf, "write error");
		dbf->directory_changed = FALSE;
		if (!dbf->header_changed && !dbf->fast_write)
		fsync (dbf->desc);
	}

	/* Final write of the header. */
	if (dbf->header_changed)
	{
		write_header (dbf);
		dbf->header_changed = FALSE;
	}
}


/* If a fatal error is detected, come here and exit. VAL tells which fatal
error occured. */
int _gdbm_fatal(gdbm_file_info *dbf,char *val)
{
	if (dbf->fatal_err != NULL)
	(*dbf->fatal_err) (val);
	else
	fprintf (stderr, "gdbm fatal: %s.\n", val);
	exit (-1);
	/* NOTREACHED */
}

/* Allocate space in the file DBF for a block NUM_BYTES in length.  Return
the file address of the start of the block.  

Each hash bucket has a fixed size avail table.  We first check this
avail table to satisfy the request for space.  In most cases we can
and this causes changes to be only in the current hash bucket.
Allocation is done on a first fit basis from the entries.  If a
request can not be satisfied from the current hash bucket, then it is
satisfied from the file header avail block.  If nothing is there that
has enough space, another block at the end of the file is allocated
and the unused portion is returned to the avail block.  This routine
"guarantees" that an allocation does not cross a block boundary unless
the size is larger than a single block.  The avail structure is
changed by this routine if a change is needed.  If an error occurs,
the value of 0 will be returned.  */

off_t _gdbm_alloc (gdbm_file_info *dbf,int num_bytes)
{
	off_t file_adr;		/* The address of the block. */
	avail_elem av_el;		/* For temporary use. */

	/* The current bucket is the first place to look for space. */
	av_el = get_elem (num_bytes, dbf->bucket->bucket_avail,
	&dbf->bucket->av_count);

	/* If we did not find some space, we have more work to do. */
	if (av_el.av_size == 0)
	{
		/* Is the header avail block empty and there is something on the stack. */
		if ((dbf->header->avail.count == 0)
				&& (dbf->header->avail.next_block != 0))
		pop_avail_block (dbf);

		/* check the header avail table next */
		av_el = get_elem (num_bytes, dbf->header->avail.av_table,
		&dbf->header->avail.count);
		if (av_el.av_size == 0)
		/* Get another full block from end of file. */
		av_el = get_block (num_bytes, dbf);

		dbf->header_changed = TRUE;
	}

	/* We now have the place from which we will allocate the new space. */
	file_adr = av_el.av_adr;

	/* Put the unused space back in the avail block. */
	av_el.av_adr += num_bytes;
	av_el.av_size -= num_bytes;
	_gdbm_free (dbf, av_el.av_adr, av_el.av_size);

	/* Return the address. */
	return file_adr;

}



/* Free space of size NUM_BYTES in the file DBF at file address FILE_ADR.  Make
it avaliable for reuse through _gdbm_alloc.  This routine changes the
avail structure.  The value TRUE is returned if there were errors.  If no
errors occured, the value FALSE is returned. */

void _gdbm_free (gdbm_file_info *dbf,off_t file_adr,int num_bytes)
{
	avail_elem temp;

	/* Is it too small to worry about? */
	if (num_bytes <= IGNORE_SIZE)
	return;

	/* Initialize the avail element. */
	temp.av_size = num_bytes;
	temp.av_adr = file_adr;

	/* Is the freed space large or small? */
	if (num_bytes >= dbf->header->block_size)
	{
		if (dbf->header->avail.count == dbf->header->avail.size)
		{
			push_avail_block (dbf);
		}
		_gdbm_put_av_elem (temp, dbf->header->avail.av_table,
		&dbf->header->avail.count);
		dbf->header_changed = TRUE;
	}
	else
	{
		/* Try to put into the current bucket. */
		if (dbf->bucket->av_count < BUCKET_AVAIL)
		_gdbm_put_av_elem (temp, dbf->bucket->bucket_avail,
		&dbf->bucket->av_count);
		else
		{
			if (dbf->header->avail.count == dbf->header->avail.size)
			{
				push_avail_block (dbf);
			}
			_gdbm_put_av_elem (temp, dbf->header->avail.av_table,
			&dbf->header->avail.count);
			dbf->header_changed = TRUE;
		}
	}

	if (dbf->header_changed)
	adjust_bucket_avail (dbf);

	/* All work is done. */
	return;
}



/* The following are all utility routines needed by the previous two. */


/* Gets the avail block at the top of the stack and loads it into the
active avail block.  It does a "free" for itself! */

static void pop_avail_block (gdbm_file_info *dbf)
{
	int  num_bytes;		/* For use with the read system call. */
	off_t file_pos;		/* For use with the lseek system call. */
	avail_elem temp;

	/* Set up variables. */
	temp.av_adr = dbf->header->avail.next_block;
	temp.av_size = ( ( (dbf->header->avail.size * sizeof (avail_elem)) >> 1)
	+ sizeof (avail_block));

	/* Read the block. */
	file_pos = _lseek (dbf->desc, temp.av_adr, L_SET);
	if (file_pos != temp.av_adr)  _gdbm_fatal (dbf, "lseek error");
	num_bytes = _read (dbf->desc, &dbf->header->avail, temp.av_size);
	if (num_bytes != temp.av_size) _gdbm_fatal (dbf, "read error");

	/* We changed the header. */
	dbf->header_changed = TRUE;

	/* Free the previous avail block. */
	_gdbm_put_av_elem (temp, dbf->header->avail.av_table,
	&dbf->header->avail.count);
}


/* Splits the header avail block and pushes half onto the avail stack. */

static void push_avail_block(gdbm_file_info *dbf)
{
	int  num_bytes;
	int  av_size;
	off_t av_adr;
	int  index;
	off_t file_pos;
	avail_block *temp;
	avail_elem  new_loc;


	/* Caclulate the size of the split block. */
	av_size = ( (dbf->header->avail.size * sizeof (avail_elem)) >> 1)
	+ sizeof (avail_block);

	/* Get address in file for new av_size bytes. */
	new_loc = get_elem (av_size, dbf->header->avail.av_table,
	&dbf->header->avail.count);
	if (new_loc.av_size == 0)
	new_loc = get_block (av_size, dbf);
	av_adr = new_loc.av_adr;


	/* Split the header block. */
	temp = (avail_block *) alloca (av_size);
	/* Set the size to be correct AFTER the pop_avail_block. */
	temp->size = dbf->header->avail.size;
	temp->count = 0;
	temp->next_block = dbf->header->avail.next_block;
	dbf->header->avail.next_block = av_adr;
	for (index = 1; index < dbf->header->avail.count; index++)
	if ( (index & 0x1) == 1)	/* Index is odd. */
	temp->av_table[temp->count++] = dbf->header->avail.av_table[index];
	else
	dbf->header->avail.av_table[index>>1]
	= dbf->header->avail.av_table[index];

	/* Update the header avail count to previous size divided by 2. */
	dbf->header->avail.count >>= 1;

	/* Free the unneeded space. */
	new_loc.av_adr += av_size;
	new_loc.av_size -= av_size;
	_gdbm_free (dbf, new_loc.av_adr, new_loc.av_size);

	/* Update the disk. */
	file_pos = _lseek (dbf->desc, av_adr, L_SET);
	if (file_pos != av_adr) _gdbm_fatal (dbf, "lseek error");
	num_bytes = _write (dbf->desc, temp, av_size);
	if (num_bytes != av_size) _gdbm_fatal (dbf, "write error");

}



/* Get_elem returns an element in the AV_TABLE block which is
larger than SIZE.  AV_COUNT is the number of elements in the
AV_TABLE.  If an item is found, it extracts it from the AV_TABLE
and moves the other elements up to fill the space. If no block is 
found larger than SIZE, get_elem returns a size of zero.  This
routine does no I/O. */

static avail_elem get_elem (int size,avail_elem av_table[],int *av_count)
{
	int index;			/* For searching through the avail block. */
	avail_elem val;		/* The default return value. */

	/* Initialize default return value. */
	val.av_adr = 0;
	val.av_size = 0;

	/* Search for element.  List is sorted by size. */
	index = 0;
	while (index < *av_count && av_table[index].av_size < size)
	{
		index++;
	}

	/* Did we find one of the right size? */
	if (index >= *av_count)
	return val;

	/* Ok, save that element and move all others up one. */
	val = av_table[index];
	*av_count -= 1;
	while (index < *av_count)
	{
		av_table[index] = av_table[index+1];
		index++;
	}

	return val;
}


/* This routine inserts a single NEW_EL into the AV_TABLE block in
sorted order. This routine does no I/O. */

int _gdbm_put_av_elem (avail_elem new_el,avail_elem av_table[],int *av_count)
{
	int index;			/* For searching through the avail block. */
	int index1;

	/* Is it too small to deal with? */
	if (new_el.av_size <= IGNORE_SIZE)
	return FALSE; 

	/* Search for place to put element.  List is sorted by size. */
	index = 0;
	while (index < *av_count && av_table[index].av_size < new_el.av_size)
	{
		index++;
	}

	/* Move all others up one. */
	index1 = *av_count-1;
	while (index1 >= index)
	{
		av_table[index1+1] = av_table[index1];
		index1--;
	}

	/* Add the new element. */
	av_table[index] = new_el;

	/* Increment the number of elements. */
	*av_count += 1;  

	return TRUE;
}





/* Get_block "allocates" new file space and the end of the file.  This is
done in integral block sizes.  (This helps insure that data smaller than
one block size is in a single block.)  Enough blocks are allocated to
make sure the number of bytes allocated in the blocks is larger than SIZE.
DBF contains the file header that needs updating.  This routine does
no I/O.  */

static avail_elem get_block (int size,gdbm_file_info *dbf)
{
	avail_elem val;

	/* Need at least one block. */
	val.av_adr  = dbf->header->next_block;
	val.av_size = dbf->header->block_size;

	/* Get enough blocks to fit the need. */
	while (val.av_size < size)
	val.av_size += dbf->header->block_size;

	/* Update the header and return. */
	dbf->header->next_block += val.av_size;

	/* We changed the header. */
	dbf->header_changed = TRUE;

	return val;

}


/*  When the header already needs writing, we can make sure the current
bucket has its avail block as close to 1/2 full as possible. */
static void adjust_bucket_avail (gdbm_file_info *dbf)
{
	int third = BUCKET_AVAIL / 3;
	avail_elem av_el;

	/* Can we add more entries to the bucket? */
	if (dbf->bucket->av_count < third)
	{
		if (dbf->header->avail.count > 0)
		{
			dbf->header->avail.count -= 1;
			av_el = dbf->header->avail.av_table[dbf->header->avail.count];
			_gdbm_put_av_elem (av_el, dbf->bucket->bucket_avail,
			&dbf->bucket->av_count);
			dbf->bucket_changed = TRUE;
		}
		return;
	}

	/* Is there too much in the bucket? */
	while (dbf->bucket->av_count > BUCKET_AVAIL-third
	&& dbf->header->avail.count < dbf->header->avail.size)
	{
		av_el = get_elem (0, dbf->bucket->bucket_avail, &dbf->bucket->av_count);
		_gdbm_put_av_elem (av_el, dbf->header->avail.av_table,
		&dbf->header->avail.count);
		dbf->bucket_changed = TRUE;
	}
}

/* Initializing a new hash buckets sets all bucket entries to -1 hash value. */
void _gdbm_new_bucket (gdbm_file_info *dbf,hash_bucket *bucket,int bits)
{
	int index;

	/* Initialize the avail block. */
	bucket->av_count = 0;

	/* Set the information fields first. */
	bucket->bucket_bits = bits;
	bucket->count = 0;

	/* Initialize all bucket elements. */
	for (index = 0; index < dbf->header->bucket_elems; index++)
	bucket->h_table[index].hash_value = -1;
}



/* Find a bucket for DBF that is pointed to by the bucket directory from
location DIR_INDEX.   The bucket cache is first checked to see if it
is already in memory.  If not, a bucket may be tossed to read the new
bucket.  In any case, the requested bucket is make the "current" bucket
and dbf->bucket points to the correct bucket. */

void _gdbm_get_bucket (gdbm_file_info *dbf,word_t dir_index)
{
	off_t bucket_adr;	/* The address of the correct hash bucket.  */
	int   num_bytes;	/* The number of bytes read. */
	off_t	file_pos;	/* The return address for lseek. */
	register int index;	/* Loop index. */

	/* Initial set up. */
	dbf->bucket_dir = dir_index;
	bucket_adr = dbf->dir [dir_index];

	if (dbf->bucket_cache == NULL)
	{
		if(_gdbm_init_cache(dbf, DEFAULT_CACHESIZE) == -1)
		_gdbm_fatal(dbf, "couldn't init cache");
	}

	/* Is that one is not already current, we must find it. */
	if (dbf->cache_entry->ca_adr != bucket_adr)
	{
		/* Look in the cache. */
		for (index = 0; index < dbf->cache_size; index++)
		{
			if (dbf->bucket_cache[index].ca_adr == bucket_adr)
			{
				dbf->bucket = dbf->bucket_cache[index].ca_bucket;
				dbf->cache_entry = &dbf->bucket_cache[index];
				return;
			}
		}

		/* It is not in the cache, read it from the disk. */
		dbf->last_read = (dbf->last_read + 1) % dbf->cache_size;
		if (dbf->bucket_cache[dbf->last_read].ca_changed)
		_gdbm_write_bucket (dbf, &dbf->bucket_cache[dbf->last_read]);
		dbf->bucket_cache[dbf->last_read].ca_adr = bucket_adr;
		dbf->bucket = dbf->bucket_cache[dbf->last_read].ca_bucket;
		dbf->cache_entry = &dbf->bucket_cache[dbf->last_read];
		dbf->cache_entry->ca_data.elem_loc = -1;
		dbf->cache_entry->ca_changed = FALSE;

		/* Read the bucket. */
		file_pos = _lseek (dbf->desc, bucket_adr, L_SET);
		if (file_pos != bucket_adr)
		_gdbm_fatal (dbf, "lseek error");
		num_bytes = _read (dbf->desc, dbf->bucket, dbf->header->bucket_size);
		if (num_bytes != dbf->header->bucket_size)
		_gdbm_fatal (dbf, "read error");
	}

	return;
}							  


/* Split the current bucket.  This includes moving all items in the bucket to
a new bucket.  This doesn't require any disk reads because all hash values
are stored in the buckets.  Splitting the current bucket may require
doubling the size of the hash directory.  */
void _gdbm_split_bucket (gdbm_file_info *dbf,word_t next_insert)
{
	hash_bucket *bucket[2]; 	/* Pointers to the new buckets. */

	int          new_bits;	/* The number of bits for the new buckets. */
	int	       cache_0;		/* Location in the cache for the buckets. */
	int	       cache_1;
	off_t        adr_0;		/* File address of the new bucket 0. */
	off_t        adr_1;		/* File address of the new bucket 1. */
	avail_elem   old_bucket;	/* Avail Struct for the old bucket. */

	off_t        dir_start0;	/* Used in updating the directory. */
	off_t        dir_start1;
	off_t        dir_end;

	off_t       *new_dir;		/* Pointer to the new directory. */
	off_t        dir_adr; 	/* Address of the new directory. */
	int          dir_size;	/* Size of the new directory. */
	off_t        old_adr[31]; 	/* Address of the old directories. */
	int          old_size[31]; 	/* Size of the old directories. */
	int	       old_count;	/* Number of old directories. */

	int          index;		/* Used in array indexing. */
	int          index1;		/* Used in array indexing. */
	int          elem_loc;	/* Location in new bucket to put element. */
	bucket_element *old_el;	/* Pointer into the old bucket. */
	int	       select;		/* Used to index bucket during movement. */


	/* No directories are yet old. */
	old_count = 0;

	if (dbf->bucket_cache == NULL)
	{
		if(_gdbm_init_cache(dbf, DEFAULT_CACHESIZE) == -1)
		_gdbm_fatal(dbf, "couldn't init cache");
	}

	while (dbf->bucket->count == dbf->header->bucket_elems)
	{
		/* Initialize the "new" buckets in the cache. */
		do
		{
			dbf->last_read = (dbf->last_read + 1) % dbf->cache_size;
			cache_0 = dbf->last_read;
		}      
		while (dbf->bucket_cache[cache_0].ca_bucket == dbf->bucket);
		bucket[0] = dbf->bucket_cache[cache_0].ca_bucket;
		if (dbf->bucket_cache[cache_0].ca_changed)
		_gdbm_write_bucket (dbf, &dbf->bucket_cache[cache_0]);
		do
		{
			dbf->last_read = (dbf->last_read + 1) % dbf->cache_size;
			cache_1 = dbf->last_read;
		}      
		while (dbf->bucket_cache[cache_1].ca_bucket == dbf->bucket);
		bucket[1] = dbf->bucket_cache[cache_1].ca_bucket;
		if (dbf->bucket_cache[cache_1].ca_changed)
		_gdbm_write_bucket (dbf, &dbf->bucket_cache[cache_1]);
		new_bits = dbf->bucket->bucket_bits+1;
		_gdbm_new_bucket (dbf, bucket[0], new_bits);
		_gdbm_new_bucket (dbf, bucket[1], new_bits);
		adr_0 = _gdbm_alloc (dbf, dbf->header->bucket_size); 
		dbf->bucket_cache[cache_0].ca_adr = adr_0;
		adr_1 = _gdbm_alloc (dbf, dbf->header->bucket_size);
		dbf->bucket_cache[cache_1].ca_adr = adr_1;



		/* Double the directory size if necessary. */
		if (dbf->header->dir_bits == dbf->bucket->bucket_bits)
		{
			dir_size = dbf->header->dir_size * 2;
			dir_adr  = _gdbm_alloc (dbf, dir_size);
			new_dir  = (off_t *) malloc (dir_size);
			if (new_dir == NULL) _gdbm_fatal (dbf, "malloc error");
			for (index = 0;
			index < dbf->header->dir_size/sizeof (off_t); index++)
			{
				new_dir[2*index]   = dbf->dir[index];
				new_dir[2*index+1] = dbf->dir[index];
			}

			/* Update header. */
			old_adr[old_count] = dbf->header->dir;
			dbf->header->dir = dir_adr;
			old_size[old_count] = dbf->header->dir_size;
			dbf->header->dir_size = dir_size;
			dbf->header->dir_bits = new_bits;
			old_count++;

			/* Now update dbf.  */
			dbf->header_changed = TRUE;
			dbf->bucket_dir *= 2;
			free (dbf->dir);
			dbf->dir = new_dir;
		}

		/* Copy all elements in dbf->bucket into the new buckets. */
		for (index = 0; index < dbf->header->bucket_elems; index++)
		{
			old_el = & (dbf->bucket->h_table[index]);
			select = (old_el->hash_value >> (31-new_bits)) & 1;
			elem_loc = old_el->hash_value % dbf->header->bucket_elems;
			while (bucket[select]->h_table[elem_loc].hash_value != -1)
			elem_loc = (elem_loc + 1) % dbf->header->bucket_elems;
			bucket[select]->h_table[elem_loc] = *old_el;
			bucket[select]->count += 1;
		}

		/* Allocate avail space for the bucket[1]. */
		bucket[1]->bucket_avail[0].av_adr
		= _gdbm_alloc (dbf, dbf->header->block_size);
		bucket[1]->bucket_avail[0].av_size = dbf->header->block_size;
		bucket[1]->av_count = 1;

		/* Copy the avail elements in dbf->bucket to bucket[0]. */
		bucket[0]->av_count = dbf->bucket->av_count;
		index = 0;
		index1 = 0;
		if (bucket[0]->av_count == BUCKET_AVAIL)
		{
			/* The avail is full, move the first one to bucket[1]. */
			_gdbm_put_av_elem (dbf->bucket->bucket_avail[0],
			bucket[1]->bucket_avail,
			&bucket[1]->av_count);
			index = 1;
			bucket[0]->av_count --;
		}
		for (; index < dbf->bucket->av_count; index++)
		{
			bucket[0]->bucket_avail[index1++] = dbf->bucket->bucket_avail[index];
		}

		/* Update the directory.  We have new file addresses for both buckets. */
		dir_start1 = (dbf->bucket_dir >> (dbf->header->dir_bits - new_bits)) | 1;
		dir_end = (dir_start1 + 1) << (dbf->header->dir_bits - new_bits);
		dir_start1 = dir_start1 << (dbf->header->dir_bits - new_bits);
		dir_start0 = dir_start1 - (dir_end - dir_start1);
		for (index = dir_start0; index < dir_start1; index++)
		dbf->dir[index] = adr_0;
		for (index = dir_start1; index < dir_end; index++)
		dbf->dir[index] = adr_1;


		/* Set changed flags. */
		dbf->bucket_cache[cache_0].ca_changed = TRUE;
		dbf->bucket_cache[cache_1].ca_changed = TRUE;
		dbf->bucket_changed = TRUE;
		dbf->directory_changed = TRUE;
		dbf->second_changed = TRUE;

		/* Update the cache! */
		dbf->bucket_dir = next_insert >> (31-dbf->header->dir_bits);

		/* Invalidate old cache entry. */
		old_bucket.av_adr  = dbf->cache_entry->ca_adr;
		old_bucket.av_size = dbf->header->bucket_size;
		dbf->cache_entry->ca_adr = 0;
		dbf->cache_entry->ca_changed = FALSE;

		/* Set dbf->bucket to the proper bucket. */
		if (dbf->dir[dbf->bucket_dir] == adr_0)
		{
			dbf->bucket = bucket[0];
			dbf->cache_entry = &dbf->bucket_cache[cache_0];
			_gdbm_put_av_elem (old_bucket,
			bucket[1]->bucket_avail,
			&bucket[1]->av_count);
		}
		else
		{
			dbf->bucket = bucket[1];
			dbf->cache_entry = &dbf->bucket_cache[cache_1];
			_gdbm_put_av_elem (old_bucket,
			bucket[0]->bucket_avail,
			&bucket[0]->av_count);
		}

	}

	/* Get rid of old directories. */
	for (index = 0; index < old_count; index++)
	_gdbm_free (dbf, old_adr[index], old_size[index]);
}


/* The only place where a bucket is written.  CA_ENTRY is the
cache entry containing the bucket to be written. */

void _gdbm_write_bucket(gdbm_file_info *dbf,cache_elem *ca_entry)
{
	int  num_bytes;	/* The return value for write. */
	off_t file_pos;	/* The return value for lseek. */

	file_pos = _lseek (dbf->desc, ca_entry->ca_adr, L_SET);
	if (file_pos != ca_entry->ca_adr)
	_gdbm_fatal (dbf, "lseek error");
	num_bytes = _write (dbf->desc, ca_entry->ca_bucket, dbf->header->bucket_size);
	if (num_bytes != dbf->header->bucket_size)
	_gdbm_fatal (dbf, "write error");
	ca_entry->ca_changed = FALSE;
	ca_entry->ca_data.hash_val = -1;
	ca_entry->ca_data.elem_loc = -1;
}

/* Read the data found in bucket entry ELEM_LOC in file DBF and
return a pointer to it.  Also, cache the read value. */

char * _gdbm_read_entry (gdbm_file_info *dbf,int elem_loc)
{
	int num_bytes;		/* For seeking and reading. */
	int key_size;
	int data_size;
	off_t file_pos;
	data_cache_elem *data_ca;

	/* Is it already in the cache? */
	if (dbf->cache_entry->ca_data.elem_loc == elem_loc)
	return dbf->cache_entry->ca_data.dptr;

	/* Set sizes and pointers. */
	key_size = dbf->bucket->h_table[elem_loc].key_size;
	data_size = dbf->bucket->h_table[elem_loc].data_size;
	data_ca = &dbf->cache_entry->ca_data;

	/* Set up the cache. */
	if (data_ca->dptr != NULL) free (data_ca->dptr);
	data_ca->key_size = key_size;
	data_ca->data_size = data_size;
	data_ca->elem_loc = elem_loc;
	data_ca->hash_val = dbf->bucket->h_table[elem_loc].hash_value;
	if (key_size+data_size == 0)
	data_ca->dptr = (char *) malloc (1);
	else
	data_ca->dptr = (char *) malloc (key_size+data_size);
	if (data_ca->dptr == NULL) _gdbm_fatal (dbf, "malloc error");


	/* Read into the cache. */
	file_pos = _lseek (dbf->desc,
	dbf->bucket->h_table[elem_loc].data_pointer, L_SET);
	if (file_pos != dbf->bucket->h_table[elem_loc].data_pointer)
	_gdbm_fatal (dbf, "lseek error");
	num_bytes = _read (dbf->desc, data_ca->dptr, key_size+data_size);
	if (num_bytes != key_size+data_size) _gdbm_fatal (dbf, "read error");

	return data_ca->dptr;
}



/* Find the KEY in the file and get ready to read the associated data.  The
return value is the location in the current hash bucket of the KEY's
entry.  If it is found, a pointer to the data and the key are returned
in DPTR.  If it is not found, the value -1 is returned.  Since find
key computes the hash value of key, that value */
int _gdbm_findkey (gdbm_file_info *dbf,datum key,char **dptr,word_t *new_hash_val/* The new hash value. */)
{
	word_t bucket_hash_val;	/* The hash value from the bucket. */
	char  *file_key;		/* The complete key as stored in the file. */
	int    elem_loc;		/* The location in the bucket. */
	int    home_loc;		/* The home location in the bucket. */
	int    key_size;		/* Size of the key on the file.  */

	/* Compute hash value and load proper bucket.  */
	*new_hash_val = _gdbm_hash (key);
	_gdbm_get_bucket (dbf, *new_hash_val>> (31-dbf->header->dir_bits));

	/* Is the element the last one found for this bucket? */
	if (dbf->cache_entry->ca_data.elem_loc != -1 
			&& *new_hash_val == dbf->cache_entry->ca_data.hash_val
			&& dbf->cache_entry->ca_data.key_size == key.dsize
			&& dbf->cache_entry->ca_data.dptr != NULL
			&& bcmp (dbf->cache_entry->ca_data.dptr, key.dptr, key.dsize) == 0)
	{
		/* This is it. Return the cache pointer. */
		*dptr = dbf->cache_entry->ca_data.dptr+key.dsize;
		return dbf->cache_entry->ca_data.elem_loc;
	}

	/* It is not the cached value, search for element in the bucket. */
	elem_loc = *new_hash_val % dbf->header->bucket_elems;
	home_loc = elem_loc;
	bucket_hash_val = dbf->bucket->h_table[elem_loc].hash_value;
	while (bucket_hash_val != -1)
	{
		key_size = dbf->bucket->h_table[elem_loc].key_size;
		if (bucket_hash_val != *new_hash_val
				|| key_size != key.dsize
				|| bcmp (dbf->bucket->h_table[elem_loc].key_start, key.dptr,
					(SMALL < key_size ? SMALL : key_size)) != 0) 
		{
			/* Current elem_loc is not the item, go to next item. */
			elem_loc = (elem_loc + 1) % dbf->header->bucket_elems;
			if (elem_loc == home_loc) return -1;
			bucket_hash_val = dbf->bucket->h_table[elem_loc].hash_value;
		}
		else
		{
			/* This may be the one we want.
			The only way to tell is to read it. */
			file_key = _gdbm_read_entry (dbf, elem_loc);
			if (bcmp (file_key, key.dptr, key_size) == 0)
			{
				/* This is the item. */
				*dptr = file_key+key.dsize;
				return elem_loc;
			}
			else
			{
				/* Not the item, try the next one.  Return if not found. */
				elem_loc = (elem_loc + 1) % dbf->header->bucket_elems;
				if (elem_loc == home_loc) return -1;
				bucket_hash_val = dbf->bucket->h_table[elem_loc].hash_value;
			}
		}
	}

	/* If we get here, we never found the key. */
	return -1;

}

word_t _gdbm_hash(datum key)
{
#ifdef LONG_64_BITS
	//	|| !INT_16_BITS
	unsigned int value;	/* Used to compute the hash value.  */
#else
	unsigned long value;	/* Used to compute the hash value.  */
#endif
	int   index;		/* Used to cycle through random values. */


	/* Set the initial value from key. */
	value = 0x238F13AF * key.dsize;
	for (index = 0; index < key.dsize; index++)
	value = (value + (key.dptr[index] << (index*5 % 24))) & 0x7FFFFFFF;

	value = (1103515243 * value + 12345) & 0x7FFFFFFF;  

	/* Return the value. */
	return((word_t) value);
}

datum firstkey ()
{
	datum ret_val;

	/* Free previous dynamic memory, do actual call, and save pointer to new
	memory. */
	ret_val = gdbm_firstkey (_gdbm_file);
	if (_gdbm_memory.dptr != NULL) free (_gdbm_memory.dptr);
	_gdbm_memory = ret_val;

	/* Return the new value. */
	return ret_val;
}


/* Continue visiting all keys.  The next key following KEY is returned. */

datum nextkey (datum key)
{
	datum ret_val;

	/* Make sure we have a valid key. */
	if (key.dptr == NULL)
	return key;

	/* Call gdbm nextkey with supplied value. After that, free the old value. */
	ret_val = gdbm_nextkey (_gdbm_file, key);
	if (_gdbm_memory.dptr != NULL) free (_gdbm_memory.dptr);
	_gdbm_memory = ret_val;

	/* Return the new value. */
	return ret_val;
}

/*
#ifndef	HAVE_RENAME

// Rename takes OLD_NAME and renames it as NEW_NAME.  If it can not rename
//   the file a non-zero value is returned.  OLD_NAME is guaranteed to
//   remain if it can't be renamed. It assumes NEW_NAME always exists (due
//   to being used in gdbm). 

static int rename (char* old_name, char* new_name) 
{
#ifdef	WIN32
return MoveFileEx(old_name, new_name, MOVEFILE_REPLACE_EXISTING)? 0:-1;
#else
if (unlink (new_name) != 0)   
return -1;

if (link (old_name, new_name) != 0)
return -1;

unlink (old_name);
return 0;
#endif	//	WIN32
}
#endif

*/

/* Reorganize the database.  This requires creating a new file and inserting
all the elements in the old file DBF into the new file.  The new file
is then renamed to the same name as the old file and DBF is updated to
contain all the correct information about the new file.  If an error
is detected, the return value is negative.  The value zero is returned
after a successful reorganization. */

int gdbm_reorganize (gdbm_file_info *dbf)
{
	gdbm_file_info *new_dbf;		/* The new file. */
	char *new_name;			/* A temporary name. */
	int  len;				/* Used in new_name construction. */
	datum key, nextkey, data;		/* For the sequential sweep. */
	struct stat fileinfo;			/* Information about the file. */
	int  index;				/* Use in moving the bucket cache. */


	/* Readers can not reorganize! */
	if (dbf->read_write == GDBM_READER)
	{
		gdbm_errno = GDBM_READER_CANT_REORGANIZE;
		return -1;
	}

	/* Initialize the gdbm_errno variable. */
	gdbm_errno = GDBM_NO_ERROR;

	/* Construct new name for temporary file. */
	len = strlen (dbf->name);
	new_name = (char *) alloca (len + 3);
	if (new_name == NULL)
	{
		gdbm_errno = GDBM_MALLOC_ERROR;
		return -1;
	}
	strcpy (&new_name[0], dbf->name);
	new_name[len+2] = 0;
	new_name[len+1] = '#';
	while ( (len > 0) && new_name[len-1] != '/')
	{
		new_name[len] = new_name[len-1];
		len -= 1;
	}
	new_name[len] = '#';

	/* Get the mode for the old file and open the new database.
	The "fast" mode is used because the reorganization will fail
	unless we create a complete copy of the database. */
	fstat (dbf->desc, &fileinfo);
	new_dbf = gdbm_open (new_name, dbf->header->block_size,	GDBM_WRCREAT | GDBM_FAST,fileinfo.st_mode, dbf->fatal_err);

	if (new_dbf == NULL)
	{
		gdbm_errno = GDBM_REORGANIZE_FAILED;
		return -1;
	}


	/* For each item in the old database, add an entry in the new. */
	key = gdbm_firstkey (dbf);

	while (key.dptr != NULL)
	{
		data = gdbm_fetch (dbf, key);
		if (data.dptr != NULL)
		{
			/* Add the data to the new file. */
			if (gdbm_store (new_dbf, key, data, GDBM_INSERT) != 0)
			{
				gdbm_close (new_dbf);
				gdbm_errno = GDBM_REORGANIZE_FAILED;
				_unlink (new_name);
				return -1;
			}
		}
		else
		{
			/* ERROR! Abort and don't finish reorganize. */
			gdbm_close (new_dbf);
			gdbm_errno = GDBM_REORGANIZE_FAILED;
			_unlink (new_name);
			return -1;
		}
		nextkey = gdbm_nextkey (dbf, key);
		free (key.dptr);
		free (data.dptr);
		key = nextkey;
	}

	/* Write everything. */
	_gdbm_end_update (new_dbf);
	gdbm_sync (new_dbf);


	/* Fix up DBF to have the correct information for the new file. */
	UNLOCK_FILE(dbf);
	_close (dbf->desc);
	free (dbf->header);
	free (dbf->dir);

	/* Move the new file to old name. */
#ifdef	WIN32
	gdbm_close (new_dbf);	  
#endif	//	WIN32

	if (rename (new_name, dbf->name) != 0)
	{
		gdbm_errno = GDBM_REORGANIZE_FAILED;
#ifndef	WIN32
		gdbm_close (new_dbf);
#endif	//	! WIN32
		return -1;
	}
#ifdef	WIN32
	new_dbf = gdbm_open (dbf->name, dbf->header->block_size,GDBM_WRCREAT | GDBM_FAST,fileinfo.st_mode, dbf->fatal_err);

	if (new_dbf == NULL)
	{
		gdbm_errno = GDBM_REORGANIZE_FAILED;
		return -1;
	}
#endif	//	WIN32

	if (dbf->bucket_cache != NULL) {
		for (index = 0; index < dbf->cache_size; index++) {
			if (dbf->bucket_cache[index].ca_bucket != NULL)
			free (dbf->bucket_cache[index].ca_bucket);
			if (dbf->bucket_cache[index].ca_data.dptr != NULL)
			free (dbf->bucket_cache[index].ca_data.dptr);
		}
		free (dbf->bucket_cache);
	}

	dbf->desc           = new_dbf->desc;
	dbf->header         = new_dbf->header;
	dbf->dir            = new_dbf->dir;
	dbf->bucket         = new_dbf->bucket;
	dbf->bucket_dir     = new_dbf->bucket_dir;
	dbf->last_read      = new_dbf->last_read;
	dbf->bucket_cache   = new_dbf->bucket_cache;
	dbf->cache_size     = new_dbf->cache_size;
	dbf->header_changed    = new_dbf->header_changed;
	dbf->directory_changed = new_dbf->directory_changed;
	dbf->bucket_changed    = new_dbf->bucket_changed;
	dbf->second_changed    = new_dbf->second_changed;

	free (new_dbf);

	/* Make sure the new database is all on disk. */
	fsync (dbf->desc);

	/* Force the right stuff for a correct bucket cache. */
	dbf->cache_entry    = &dbf->bucket_cache[0];
	_gdbm_get_bucket (dbf, 0);

	return 0;
}


const char * gdbm_strerror(gdbm_error error)
{
	if(((int)error < 0) || ((int)error > 18))
	{
		return("Unkown error");
	}
	else
	{
		return(gdbm_errlist[(int)error]);
	}
}

int gdbm_exists(gdbm_file_info *dbf,datum key)
{
	char *find_data;		/* dummy */
	word_t hash_val;		/* dummy */

	return (_gdbm_findkey (dbf, key, &find_data, &hash_val) >= 0);
}

void gdbm_close(gdbm_file_info *dbf)
{
	register int index;	/* For freeing the bucket cache. */					

	/* Make sure the database is all on disk. */
	if (dbf->read_write == GDBM_WRITER)
	fsync (dbf->desc);

	/* Close the file and free all malloced memory. */
	UNLOCK_FILE(dbf);
	_close (dbf->desc);
	free (dbf->name);
	if (dbf->dir != NULL) free (dbf->dir);

	if (dbf->bucket_cache != NULL) {
		for (index = 0; index < dbf->cache_size; index++) {
			if (dbf->bucket_cache[index].ca_bucket != NULL)
			free (dbf->bucket_cache[index].ca_bucket);
			if (dbf->bucket_cache[index].ca_data.dptr != NULL)
			free (dbf->bucket_cache[index].ca_data.dptr);
		}
		free (dbf->bucket_cache);
	}
	if ( dbf->header != NULL ) free (dbf->header);
	free (dbf);
}


/* Remove the KEYed item and the KEY from the database DBF.  The file on disk
is updated to reflect the structure of the new database before returning
from this procedure.  */

int gdbm_delete(gdbm_file_info *dbf,datum key)
{
	int elem_loc;		/* The location in the current hash bucket. */
	int last_loc;		/* Last location emptied by the delete.  */
	int home;		/* Home position of an item. */
	bucket_element elem;  /* The element to be deleted. */
	char *find_data;	/* Return pointer from findkey. */
	word_t hash_val;	/* Returned by findkey. */
	off_t free_adr;       /* Temporary stroage for address and size. */
	int   free_size;

	/* First check to make sure this guy is a writer. */
	if (dbf->read_write != GDBM_WRITER)
	{
		gdbm_errno = GDBM_READER_CANT_DELETE;
		return -1;
	}

	/* Initialize the gdbm_errno variable. */
	gdbm_errno = GDBM_NO_ERROR;

	/* Find the item. */
	elem_loc = _gdbm_findkey (dbf, key, &find_data, &hash_val);
	if (elem_loc == -1)
	{
		gdbm_errno = GDBM_ITEM_NOT_FOUND;
		return -1;
	}

	/* Save the element.  */
	elem = dbf->bucket->h_table[elem_loc];

	/* Delete the element.  */
	dbf->bucket->h_table[elem_loc].hash_value = -1;
	dbf->bucket->count -= 1;

	/* Move other elements to guarantee that they can be found. */
	last_loc = elem_loc;
	elem_loc = (elem_loc + 1) % dbf->header->bucket_elems;
	while (elem_loc != last_loc
	&& dbf->bucket->h_table[elem_loc].hash_value != -1)
	{
		home = dbf->bucket->h_table[elem_loc].hash_value
		% dbf->header->bucket_elems;
		if ( (last_loc < elem_loc && (home <= last_loc || home > elem_loc))
				|| (last_loc > elem_loc && home <= last_loc && home > elem_loc))

		{
			dbf->bucket->h_table[last_loc] = dbf->bucket->h_table[elem_loc];
			dbf->bucket->h_table[elem_loc].hash_value = -1;
			last_loc = elem_loc;
		}
		elem_loc = (elem_loc + 1) % dbf->header->bucket_elems;
	}

	/* Free the file space. */
	free_adr = elem.data_pointer;
	free_size = elem.key_size + elem.data_size;
	_gdbm_free (dbf, free_adr, free_size);

	/* Set the flags. */
	dbf->bucket_changed = TRUE;

	/* Clear out the data cache for the current bucket. */
	if (dbf->cache_entry->ca_data.dptr != NULL)
	{
		free (dbf->cache_entry->ca_data.dptr);
		dbf->cache_entry->ca_data.dptr = NULL;
	}
	dbf->cache_entry->ca_data.hash_val = -1;
	dbf->cache_entry->ca_data.key_size = 0;
	dbf->cache_entry->ca_data.elem_loc = -1;

	/* Do the writes. */
	_gdbm_end_update (dbf);
	return 0;  
}

datum gdbm_fetch (gdbm_file_info *dbf,datum key)
{
	datum  return_val;		/* The return value. */
	int    elem_loc;		/* The location in the bucket. */
	char  *find_data;		/* Returned from find_key. */
	word_t   hash_val;		/* Returned from find_key. */

	/* Set the default return value. */
	return_val.dptr  = NULL;
	return_val.dsize = 0;

	/* Initialize the gdbm_errno variable. */
	gdbm_errno = GDBM_NO_ERROR;

	/* Find the key and return a pointer to the data. */
	elem_loc = _gdbm_findkey (dbf, key, &find_data, &hash_val);

	/* Copy the data if the key was found.  */
	if (elem_loc >= 0)
	{
		/* This is the item.  Return the associated data. */
		return_val.dsize = dbf->bucket->h_table[elem_loc].data_size;
		if (return_val.dsize == 0)
		return_val.dptr = (char *) malloc (1);
		else
		return_val.dptr = (char *) malloc (return_val.dsize);
		if (return_val.dptr == NULL) _gdbm_fatal (dbf, "malloc error");
		bcopy (find_data, return_val.dptr, return_val.dsize);
	}

	/* Check for an error and return. */
	if (return_val.dptr == NULL) gdbm_errno = GDBM_ITEM_NOT_FOUND;
	return return_val;
}

gdbm_file_info* gdbm_open (char *file,int  block_size,int  flags,int  mode,void (*fatal_func)(char*))
{
	gdbm_file_info *dbf;		/* The record to return. */
	struct stat file_stat;	/* Space for the stat information. */
	int         len;		/* Length of the file name. */
	int         num_bytes;	/* Used in reading and writing. */
	off_t       file_pos;		/* Used with seeks. */
	int	      lock_val;         /* Returned by the flock call. */
	int	      file_block_size;	/* Block size to use for a new file. */
	int 	      index;		/* Used as a loop index. */
	char        need_trunc;	/* Used with GDBM_NEWDB and locking to avoid
							truncating a file from under a reader. */

#ifdef WIN32
	if(mode==0) mode=_S_IREAD | _S_IWRITE;
#endif
	/* Initialize the gdbm_errno variable. */
	gdbm_errno = GDBM_NO_ERROR;

	/* Allocate new info structure. */
	dbf = (gdbm_file_info *) malloc (sizeof (gdbm_file_info));
	if (dbf == NULL)
	{
		gdbm_errno = GDBM_MALLOC_ERROR;
		return NULL;
	}

	/* Initialize some fields for known values.  This is done so gdbm_close
	will work if called before allocating some structures. */
	dbf->dir  = NULL;
	dbf->bucket = NULL;
	dbf->header = NULL;
	dbf->bucket_cache = NULL;
	dbf->cache_size = 0;

	/* Save name of file. */
	len = strlen (file);
	dbf->name = (char *) malloc (len + 1);
	if (dbf->name == NULL)
	{
		free (dbf);
		gdbm_errno = GDBM_MALLOC_ERROR;
		return NULL;
	}
	strcpy (dbf->name, file);

	/* Initialize the fatal error routine. */
	dbf->fatal_err = fatal_func;

	/* Check for fast writers. */
	if (flags & GDBM_FAST)
	{
		dbf->fast_write = TRUE;
		flags -= GDBM_FAST;
	}
	else
	{
		dbf->fast_write = FALSE;
	}

	/* Open the file. */
	need_trunc = FALSE;
	if (flags == GDBM_READER)
	{
		dbf->desc = _open (dbf->name, O_RDONLY | O_BINARY, 0);
	}
	else if (flags == GDBM_WRITER)
	{
		dbf->desc = _open (dbf->name, O_RDWR | O_BINARY, 0);
	}
	else if (flags == GDBM_NEWDB)
	{
		dbf->desc = _open (dbf->name, O_RDWR | O_CREAT| O_BINARY, mode);
		flags = GDBM_WRITER;
		need_trunc = TRUE;
	}
	else
	{
		dbf->desc = _open (dbf->name, O_RDWR | O_CREAT | O_BINARY, mode);
		flags = GDBM_WRITER;
	}
	if (dbf->desc < 0)
	{
		free (dbf->name);
		free (dbf);
		gdbm_errno = GDBM_FILE_OPEN_ERROR;
		return NULL;
	}

	/* Get the status of the file. */
	fstat (dbf->desc, &file_stat);

	/* Lock the file in the approprate way. */
	if (flags == GDBM_READER)
	{
		if (file_stat.st_size == 0)
		{
			_close (dbf->desc);
			free (dbf->name);
			free (dbf);
			gdbm_errno = GDBM_EMPTY_DATABASE;
			return NULL;
		}
		/* Sets lock_val to 0 for success.  See systems.h. */
		READLOCK_FILE(dbf);
	}
	else
	{
		/* Sets lock_val to 0 for success.  See systems.h. */
		WRITELOCK_FILE(dbf);
	}
	
	if (lock_val != 0)
	{
		_close (dbf->desc);
		free (dbf->name);
		free (dbf);
		if (flags == GDBM_READER)
		gdbm_errno = GDBM_CANT_BE_READER;
		else
		gdbm_errno = GDBM_CANT_BE_WRITER;
		return NULL;
	}
	
	/* Record the kind of user. */
	dbf->read_write = flags;

	/* If we do have a write lock and it was a GDBM_NEWDB, it is 
	now time to truncate the file. */
	if (need_trunc && file_stat.st_size != 0)
	{
		TRUNCATE (dbf);
		fstat (dbf->desc, &file_stat);
	}

	/* Decide if this is a new file or an old file. */
	if (file_stat.st_size == 0)
	{

		/* This is a new file.  Create an empty database.  */

		/* Start with the blocksize. */
		if (block_size < 512)
		file_block_size = STATBLKSIZE;
		else
		file_block_size = block_size;

		/* Get space for the file header. */
		dbf->header = (gdbm_file_header *) malloc (file_block_size);
		if (dbf->header == NULL)
		{
			gdbm_close (dbf);
			gdbm_errno = GDBM_MALLOC_ERROR;
			return NULL;
		}

		/* Set the magic number and the block_size. */
		dbf->header->header_magic = 0x13579ace;
		dbf->header->block_size = file_block_size;

		/* Create the initial hash table directory.  */
		dbf->header->dir_size = 8 * sizeof (off_t);
		dbf->header->dir_bits = 3;
		while (dbf->header->dir_size < dbf->header->block_size)
		{
			dbf->header->dir_size <<= 1;
			dbf->header->dir_bits += 1;
		}

		/* Check for correct block_size. */
		if (dbf->header->dir_size != dbf->header->block_size)
		{
			gdbm_close (dbf);
			gdbm_errno = GDBM_BLOCK_SIZE_ERROR;
			return NULL;
		}

		/* Allocate the space for the directory. */
		dbf->dir = (off_t *) malloc (dbf->header->dir_size);
		if (dbf->dir == NULL)
		{
			gdbm_close (dbf);
			gdbm_errno = GDBM_MALLOC_ERROR;
			return NULL;
		}
		dbf->header->dir = dbf->header->block_size;

		/* Create the first and only hash bucket. */
		dbf->header->bucket_elems =
		(dbf->header->block_size - sizeof (hash_bucket))
		/ sizeof (bucket_element) + 1;
		dbf->header->bucket_size  = dbf->header->block_size;
#if !defined(sgi)
		dbf->bucket = (hash_bucket *) (alloca (dbf->header->bucket_size));
#else	/* sgi */
		/* The SGI C compiler doesn't accept the previous form. */
		{
			hash_bucket *ptr;
			ptr = (hash_bucket *) (alloca (dbf->header->bucket_size));
			dbf->bucket = ptr;
		}
#endif	/* sgi */
		if (dbf->bucket == NULL)
		{
			gdbm_close (dbf);
			gdbm_errno = GDBM_MALLOC_ERROR;
			return NULL;
		}
		_gdbm_new_bucket (dbf, dbf->bucket, 0);
		dbf->bucket->av_count = 1;
		dbf->bucket->bucket_avail[0].av_adr = 3*dbf->header->block_size;
		dbf->bucket->bucket_avail[0].av_size = dbf->header->block_size;

		/* Set table entries to point to hash buckets. */
		for (index = 0; index < dbf->header->dir_size / sizeof (off_t); index++)
		dbf->dir[index] = 2*dbf->header->block_size;

		/* Initialize the active avail block. */
		dbf->header->avail.size
		= ( (dbf->header->block_size - sizeof (gdbm_file_header))
		/ sizeof (avail_elem)) + 1;
		dbf->header->avail.count = 0;
		dbf->header->avail.next_block = 0;
		dbf->header->next_block  = 4*dbf->header->block_size;

		/* Write initial configuration to the file. */
		/* Block 0 is the file header and active avail block. */
		num_bytes = write (dbf->desc, dbf->header, dbf->header->block_size);
		if (num_bytes != dbf->header->block_size)
		{
			gdbm_close (dbf);
			gdbm_errno = GDBM_FILE_WRITE_ERROR;
			return NULL;
		}

		/* Block 1 is the initial bucket directory. */
		num_bytes = write (dbf->desc, dbf->dir, dbf->header->dir_size);
		if (num_bytes != dbf->header->dir_size)
		{
			gdbm_close (dbf);
			gdbm_errno = GDBM_FILE_WRITE_ERROR;
			return NULL;
		}

		/* Block 2 is the only bucket. */
		num_bytes = write (dbf->desc, dbf->bucket, dbf->header->bucket_size);
		if (num_bytes != dbf->header->bucket_size)
		{
			gdbm_close (dbf);
			gdbm_errno = GDBM_FILE_WRITE_ERROR;
			return NULL;
		}

		/* Wait for initial configuration to be written to disk. */
		fsync (dbf->desc);
	}
	else
	{
		/* This is an old database.  Read in the information from the file
		header and initialize the hash directory. */

		gdbm_file_header partial_header;  /* For the first part of it. */

		/* Read the partial file header. */
		num_bytes = read (dbf->desc, &partial_header, sizeof (gdbm_file_header));
		if (num_bytes != sizeof (gdbm_file_header))
		{
			gdbm_close (dbf);
			gdbm_errno = GDBM_FILE_READ_ERROR;
			return NULL;
		}

		/* Is the magic number good? */
		if (partial_header.header_magic != 0x13579ace)
		{
			gdbm_close (dbf);
			gdbm_errno = GDBM_BAD_MAGIC_NUMBER;
			return NULL;
		}

		/* It is a good database, read the entire header. */
		dbf->header = (gdbm_file_header *) malloc (partial_header.block_size);
		if (dbf->header == NULL)
		{
			gdbm_close (dbf);
			gdbm_errno = GDBM_MALLOC_ERROR;
			return NULL;
		}
		bcopy (&partial_header, dbf->header, sizeof (gdbm_file_header));
		num_bytes = read (dbf->desc, &dbf->header->avail.av_table[1],
		dbf->header->block_size-sizeof (gdbm_file_header));
		if (num_bytes != dbf->header->block_size-sizeof (gdbm_file_header))
		{
			gdbm_close (dbf);
			gdbm_errno = GDBM_FILE_READ_ERROR;
			return NULL;
		}

		/* Allocate space for the hash table directory.  */
		dbf->dir = (off_t *) malloc (dbf->header->dir_size);
		if (dbf->dir == NULL)
		{
			gdbm_close (dbf);
			gdbm_errno = GDBM_MALLOC_ERROR;
			return NULL;
		}

		/* Read the hash table directory. */
		file_pos = lseek (dbf->desc, dbf->header->dir, L_SET);
		if (file_pos != dbf->header->dir)
		{
			gdbm_close (dbf);
			gdbm_errno = GDBM_FILE_SEEK_ERROR;
			return NULL;
		}

		num_bytes = read (dbf->desc, dbf->dir, dbf->header->dir_size);
		if (num_bytes != dbf->header->dir_size)
		{
			gdbm_close (dbf);
			gdbm_errno = GDBM_FILE_READ_ERROR;
			return NULL;
		}

	}

	/* Finish initializing dbf. */
	dbf->last_read = -1;
	dbf->bucket = NULL;
	dbf->bucket_dir = 0;
	dbf->cache_entry = NULL;
	dbf->header_changed = FALSE;
	dbf->directory_changed = FALSE;
	dbf->bucket_changed = FALSE;
	dbf->second_changed = FALSE;

	/* Everything is fine, return the pointer to the file
	information structure.  */
	return dbf;

}

/* initialize the bucket cache. */
int _gdbm_init_cache(gdbm_file_info *dbf,int size)
{
	register int index;

	if (dbf->bucket_cache == NULL)
	{
		dbf->bucket_cache = (cache_elem *) malloc(sizeof(cache_elem) * size);
		if(dbf->bucket_cache == NULL)
		{
			gdbm_errno = GDBM_MALLOC_ERROR;
			return(-1);
		}
		dbf->cache_size = size;

		for(index = 0; index < size; index++)
		{
			(dbf->bucket_cache[index]).ca_bucket
			= (hash_bucket *) malloc (dbf->header->bucket_size);
			if ((dbf->bucket_cache[index]).ca_bucket == NULL)
			{
				gdbm_errno = GDBM_MALLOC_ERROR;
				return(-1);
			}
			(dbf->bucket_cache[index]).ca_adr = 0;
			(dbf->bucket_cache[index]).ca_changed = FALSE;
			(dbf->bucket_cache[index]).ca_data.hash_val = -1;
			(dbf->bucket_cache[index]).ca_data.elem_loc = -1;
			(dbf->bucket_cache[index]).ca_data.dptr = NULL;
		}
		dbf->bucket = dbf->bucket_cache[0].ca_bucket;
		dbf->cache_entry = &dbf->bucket_cache[0];
	}
	return(0);
}

extern char *_gdbm_read_entry _ARGS((gdbm_file_info *, int));


/* Find and read the next entry in the hash structure for DBF starting
at ELEM_LOC of the current bucket and using RETURN_VAL as the place to
put the data that is found. */

static void get_next_key (gdbm_file_info *dbf,int elem_loc,datum *return_val)
{
	int   found;			/* Have we found the next key. */
	char  *find_data;		/* Data pointer returned by find_key. */

	/* Find the next key. */
	found = FALSE;
	while (!found)
	{
		/* Advance to the next location in the bucket. */
		elem_loc++;
		if (elem_loc == dbf->header->bucket_elems)
		{
			/* We have finished the current bucket, get the next bucket.  */
			elem_loc = 0;

			/* Find the next bucket.  It is possible several entries in
			the bucket directory point to the same bucket. */
			while (dbf->bucket_dir < dbf->header->dir_size / sizeof (off_t)
			&& dbf->cache_entry->ca_adr == dbf->dir[dbf->bucket_dir])
			dbf->bucket_dir++;

			/* Check to see if there was a next bucket. */
			if (dbf->bucket_dir < dbf->header->dir_size / sizeof (off_t))
			_gdbm_get_bucket (dbf, dbf->bucket_dir);	      
			else
			/* No next key, just return. */
			return ;
		}
		found = dbf->bucket->h_table[elem_loc].hash_value != -1;
	}

	/* Found the next key, read it into return_val. */
	find_data = _gdbm_read_entry (dbf, elem_loc);
	return_val->dsize = dbf->bucket->h_table[elem_loc].key_size;
	if (return_val->dsize == 0)
	return_val->dptr = (char *) malloc (1);
	else
	return_val->dptr = (char *) malloc (return_val->dsize);
	if (return_val->dptr == NULL) _gdbm_fatal (dbf, "malloc error");
	bcopy (find_data, return_val->dptr, return_val->dsize);
}


/* Start the visit of all keys in the database.  This produces something in
hash order, not in any sorted order.  */

datum gdbm_firstkey (gdbm_file_info *dbf)
{
	datum return_val;		/* To return the first key. */

	/* Set the default return value for not finding a first entry. */
	return_val.dptr = NULL;

	/* Initialize the gdbm_errno variable. */
	gdbm_errno = GDBM_NO_ERROR;

	/* Get the first bucket.  */
	_gdbm_get_bucket (dbf, 0);

	/* Look for first entry. */
	get_next_key (dbf, -1, &return_val);

	return return_val;
}


/* Continue visiting all keys.  The next key following KEY is returned. */

datum gdbm_nextkey (gdbm_file_info *dbf,datum key)
{
	datum  return_val;		/* The return value. */
	int    elem_loc;		/* The location in the bucket. */
	char  *find_data;		/* Data pointer returned by _gdbm_findkey. */
	word_t hash_val;		/* Returned by _gdbm_findkey. */

	/* Initialize the gdbm_errno variable. */
	gdbm_errno = GDBM_NO_ERROR;

	/* Set the default return value for no next entry. */
	return_val.dptr = NULL;

	/* Do we have a valid key? */
	if (key.dptr == NULL) return return_val;

	/* Find the key.  */
	elem_loc = _gdbm_findkey (dbf, key, &find_data, &hash_val);
	if (elem_loc == -1) return return_val;

	/* Find the next key. */  
	get_next_key (dbf, elem_loc, &return_val);

	return return_val;
}

int gdbm_setopt(gdbm_file_info *dbf,int optflag,int *optval,int optlen)
{
	switch(optflag)
	{
	case GDBM_CACHESIZE:
		/* here, optval will point to the new size of the cache. */
		if (dbf->bucket_cache != NULL)
		{
			gdbm_errno = GDBM_OPT_ALREADY_SET;
			return(-1);
		}

		return(_gdbm_init_cache(dbf, ((*optval) > 9) ? (*optval) : 10));

	case GDBM_FASTMODE:
		/* here, optval will point to either true or false. */
		if ((*optval != TRUE) && (*optval != FALSE))
		{
			gdbm_errno = GDBM_OPT_ILLEGAL;
			return(-1);
		}

		dbf->fast_write = *optval;
		break;

	default:
		gdbm_errno = GDBM_OPT_ILLEGAL;
		return(-1);
	}

	return(0);
}

int gdbm_store (gdbm_file_info *dbf,datum key,datum content,int flags)
{
	word_t new_hash_val;		/* The new hash value. */
	int  elem_loc;		/* The location in hash bucket. */
	off_t file_adr;		/* The address of new space in the file.  */
	off_t file_pos;		/* The position after a lseek. */
	int  num_bytes;		/* Used for error detection. */
	off_t free_adr;		/* For keeping track of a freed section. */
	int  free_size;

	/* char *write_data; */	/* To write both key and data in 1 call. */
	/* char *src;	*/		/* Used to prepare write_data. */
	/* char *dst;	*/		/* Used to prepare write_data. */
	/* int   cnt;	*/		/* Counter for loops to fill write_data. */
	int   new_size;		/* Used in allocating space. */
	char *temp;			/* Used in _gdbm_findkey call. */


	/* First check to make sure this guy is a writer. */
	if (dbf->read_write != GDBM_WRITER)
	{
		gdbm_errno = GDBM_READER_CANT_STORE;
		return -1;
	}

	/* Check for illegal data values.  A NULL dptr field is illegal because
	NULL dptr returned by a lookup procedure indicates an error. */
	if ((key.dptr == NULL) || (content.dptr == NULL))
	{
		gdbm_errno = GDBM_ILLEGAL_DATA;
		return -1;
	}

	/* Initialize the gdbm_errno variable. */
	gdbm_errno = GDBM_NO_ERROR;

	/* Look for the key in the file.
	A side effect loads the correct bucket and calculates the hash value. */
	elem_loc = _gdbm_findkey (dbf, key, &temp, &new_hash_val);


	/* Did we find the item? */
	if (elem_loc != -1)
	{
		if (flags == GDBM_REPLACE)
		{
			/* Just replace the data. */
			free_adr = dbf->bucket->h_table[elem_loc].data_pointer;
			free_size = dbf->bucket->h_table[elem_loc].key_size
			+ dbf->bucket->h_table[elem_loc].data_size;
			_gdbm_free (dbf, free_adr, free_size);
		}
		else
		{
			gdbm_errno = GDBM_CANNOT_REPLACE;
			return 1;
		}
	}


	/* Get the file address for the new space.
	(Current bucket's free space is first place to look.) */
	new_size = key.dsize+content.dsize;
	file_adr = _gdbm_alloc (dbf, new_size);

	/* If this is a new entry in the bucket, we need to do special things. */
	if (elem_loc == -1)
	{
		if (dbf->bucket->count == dbf->header->bucket_elems)
		{
			/* Split the current bucket. */
			_gdbm_split_bucket (dbf, new_hash_val);
		}

		/* Find space to insert into bucket and set elem_loc to that place. */
		elem_loc = new_hash_val % dbf->header->bucket_elems;
		while (dbf->bucket->h_table[elem_loc].hash_value != -1)
		{  elem_loc = (elem_loc + 1) % dbf->header->bucket_elems; }

		/* We now have another element in the bucket.  Add the new information.*/
		dbf->bucket->count += 1;
		dbf->bucket->h_table[elem_loc].hash_value = new_hash_val;
		bcopy (key.dptr, dbf->bucket->h_table[elem_loc].key_start,
		(SMALL < key.dsize ? SMALL : key.dsize));
	}


	/* Update current bucket data pointer and sizes. */
	dbf->bucket->h_table[elem_loc].data_pointer = file_adr;
	dbf->bucket->h_table[elem_loc].key_size = key.dsize;
	dbf->bucket->h_table[elem_loc].data_size = content.dsize;

	/* Write the data to the file. */
	file_pos = _lseek (dbf->desc, file_adr, L_SET);
	if (file_pos != file_adr) _gdbm_fatal (dbf, "lseek error");
	num_bytes = write (dbf->desc, key.dptr, key.dsize);
	if (num_bytes != key.dsize) _gdbm_fatal (dbf, "write error");
	num_bytes = _write (dbf->desc, content.dptr, content.dsize);
	if (num_bytes != content.dsize) _gdbm_fatal (dbf, "write error");

	/* Current bucket has changed. */
	dbf->cache_entry->ca_changed = TRUE;
	dbf->bucket_changed = TRUE;

	/* Write everything that is needed to the disk. */
	_gdbm_end_update (dbf);
	return 0;

}

void gdbm_sync(gdbm_file_info *dbf)
{

	/* Initialize the gdbm_errno variable. */
	gdbm_errno = GDBM_NO_ERROR;

	/* Do the sync on the file. */
	fsync (dbf->desc);

}

bool gdbm_fastget_key(gdbm_file_info *dbf,datum key,const char* &find_data,int &size)
{
	//--------------------------------------------------
	
	int     elem_loc;		// The location in the bucket. 
	word_t  hash_val;		// Returned from find_key. 

	size = 0;
	// Initialize the gdbm_errno variable. 
	gdbm_errno = GDBM_NO_ERROR;

	// Find the key and return a pointer to the data. 
	elem_loc = _gdbm_findkey (dbf, key,(char**)&find_data,&hash_val);

	// Copy the data if the key was found. 
	if (elem_loc >= 0)
	{
		//This is the item.  Return the associated data. 
		size = dbf->bucket->h_table[elem_loc].data_size;
		if (size == 0) return false;
		return true;
	}
	return false;
}
