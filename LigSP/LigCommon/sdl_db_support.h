#ifndef _SQLITE_DB_SUPPORT_
#define _SQLITE_DB_SUPPORT_

#include <list>
#include <string>
#include <iostream>
#include "sdl_script_support.h"
#include "sqlite3.h"

struct sqlite_query
{
	sqlite3_stmt* mLink;
	int m_nColumn;
	bool flag_eof;
	bool flag_failed;

	struct field
	{
		sqlite3_stmt* mLink;
		int idx;

		field(sqlite3_stmt* pLink,const char* fieldname)
		{
			mLink=pLink;idx=0;
			int nColumn=sqlite3_column_count(mLink);
			for(;idx<nColumn;++idx) if(strcmp(sqlite3_column_name(mLink,idx),fieldname)==0) break;
			if(idx>=nColumn) {idx=-1;return;}
		}

		field(sqlite_query &db,const char* fieldname)
		{
			mLink=db.mLink;idx=0;
			int nColumn=sqlite3_column_count(mLink);
			for(;idx<nColumn;++idx) if(strcmp(sqlite3_column_name(mLink,idx),fieldname)==0) break;
			if(idx>=nColumn) {idx=-1;return;}
		}

		const char* operator()()
		{
			static char nullstr=0;
			if(idx==-1) return (const char*)(&nullstr);
			const char* rt_str=(const char*)sqlite3_column_text(mLink,idx);
			if(rt_str==NULL) return (const char*)(&nullstr);
			return rt_str;
		}
	};

	
	static sqlite3** get_DB()
	{
		static sqlite3* mDB;
		return &mDB;
	};

	static bool* get_db_open_flag()
	{
		static bool flag=false;
		return &flag;
	}
	

	sqlite_query(){mLink=NULL;m_nColumn=0;flag_eof=true;flag_failed=false;}

	sqlite_query(sqlite_query &q){mLink=q.mLink;m_nColumn=q.m_nColumn;flag_eof=q.flag_eof;flag_failed=q.flag_failed;}

	//sqlite_query(sqlite_db &db){mLink=NULL;m_nColumn=0;flag_eof=true;pDB=&db;flag_failed=false;}

	sqlite_query(const char *sql)
	{
	
		if(sqlite3_prepare_v2(*(get_DB()),sql,strlen(sql),&mLink,NULL)!=SQLITE_OK) 
		{
			m_nColumn=0;
			mLink=NULL;
			flag_eof=true;
			return;
		}
		m_nColumn=sqlite3_column_count(mLink);
		flag_eof=(sqlite3_step(mLink)!=SQLITE_ROW);
	}

	bool exec(const char *sql)
	{
		if(*(get_db_open_flag())==false) return false;
		if(mLink!=NULL)	sqlite3_finalize(mLink);
		if(sqlite3_prepare(*(get_DB()),sql,strlen(sql),&mLink,NULL)!=SQLITE_OK)
		{
			m_nColumn=0;
			mLink=NULL;
			flag_eof=true;
			flag_failed=true;
			return false;
		}
		flag_failed=false;
		m_nColumn=sqlite3_column_count(mLink);
		flag_eof=(sqlite3_step(mLink)!=SQLITE_ROW);
		return true;
	}

	inline bool eof(){return flag_eof;}

	inline bool fail(){return flag_failed;}


	inline bool next()
	{
		if(mLink==NULL) {flag_eof=true;return false;}
		flag_eof=(sqlite3_step(mLink)!=SQLITE_ROW);
		return (!flag_eof);
	}

	inline void operator++()
	{
		if(mLink==NULL) {flag_eof=true;return;}
		flag_eof=(sqlite3_step(mLink)!=SQLITE_ROW);
	}

	template<class ValueType> bool get(int nIdx,ValueType &value);

	template<class ValueType> bool get(const char* fieldname,ValueType &value);

	const char* operator[](int nIdx)
	{
		static char nullstr=0;
		if(nIdx>=m_nColumn) return (const char*)(&nullstr);
		const char* rt_str=(const char*)sqlite3_column_text(mLink,nIdx);
		if(rt_str==NULL) return (const char*)(&nullstr);
		return rt_str;
	}

	const char* operator[](const char* fieldname)
	{
		static char nullstr=0;
		int nIdx=0;
		for(;nIdx<m_nColumn;++nIdx) if(strcmp(sqlite3_column_name(mLink,nIdx),fieldname)==0) break;
		if(nIdx>=m_nColumn) return (const char*)(&nullstr);
		const char* rt_str=(const char*)sqlite3_column_text(mLink,nIdx);
		if(rt_str==NULL) return (const char*)(&nullstr);
		return rt_str;
	}

	~sqlite_query()
	{
		if(mLink!=NULL)	sqlite3_finalize(mLink);
	}
	//--------------------------------------------------------------------------
	
	static SQInteger Sq2sqlite_initDB(HSQUIRRELVM v)
	{
		const char* dbName;
		const char* sql=NULL;
		sqlite_query *self = NULL;
		sq_getinstanceup(v,1,(SQUserPointer *)&self,0); //标准的获取C++对象的方式

		if(SqGetVar(v,dbName) && sqlite3_open(dbName,get_DB())==SQLITE_OK)
		{
			*(get_db_open_flag())=true;
			if(sql!=NULL)
			{
				if(sqlite3_exec(*(get_DB()),sql,NULL,NULL,NULL)==SQLITE_OK) return 0;
			}
		}
		else  return sq_throwerror(v,"Create instance failed");
		return 0;
	}

	static SQInteger Sq2Constructor(HSQUIRRELVM v) //标准的Squirrel C++对象构造方式
	{
		
		int top=sq_gettop(v); 
		if(top==1)  //如果没有参数,将创建一个对象
		{
			sqlite_query *query = new sqlite_query; //引用现有的数据库连接
			if(query==NULL) return sq_throwerror(v,"Create instance failed");
			sq_setinstanceup(v,1,query);
			sq_setreleasehook(v,1,_queryobj_releasehook);
			return 0;
		}
		if(top==2 && sq_gettype(v,-1)==OT_STRING) //如果后续有SQL语句，则有一个字符串
		{
			const char* sql;
			sq_getstring(v,-1,&sql);
			sqlite_query *query = new sqlite_query; //引用现有的数据库连接
			if(query==NULL) return sq_throwerror(v,"Create instance failed");
			if(!query->exec(sql)) return sq_throwerror(v,"Excute SQL failed");
			sq_setinstanceup(v,1,query);
			sq_setreleasehook(v,1,_queryobj_releasehook);
		}
		return 0;
	}

	static SQInteger _queryobj_releasehook(SQUserPointer p, SQInteger size) //标准的Squirrel C++对象析构回调
	{
		sqlite_query *self = ((sqlite_query *)p);
		delete(self);
		return 1;
	}

	static SQInteger Sq2sqlite_querynext(HSQUIRRELVM v)
	{
		sqlite_query *self = NULL;
		sq_getinstanceup(v,1,(SQUserPointer *)&self,0); //标准的获取C++对象的方式
		if(self==NULL) return 0;
		if(self->next()) //执行实际的对象函数
			sq_pushbool(v,SQTrue);
		else
			sq_pushbool(v,SQFalse);

		return 1; //有返回值
	}

	static SQInteger Sq2sqlite_queryeof(HSQUIRRELVM v)
	{
		sqlite_query *self = NULL;
		sq_getinstanceup(v,1,(SQUserPointer *)&self,0); //标准的获取C++对象的方式
		if(self==NULL) return 0;
		if(self->eof()) //执行实际的对象函数
			sq_pushbool(v,SQTrue);
		else
			sq_pushbool(v,SQFalse);

		return 1; //有返回值
	}

	static SQInteger Sq2sqlite_queryfailed(HSQUIRRELVM v)
	{
		sqlite_query *self = NULL;
		sq_getinstanceup(v,1,(SQUserPointer *)&self,0); //标准的获取C++对象的方式
		if(self==NULL) return 0;
		if(self->fail()) //执行实际的对象函数
			sq_pushbool(v,SQTrue);
		else
			sq_pushbool(v,SQFalse);

		return 1; //有返回值
	}

	static SQInteger Sq2sqlite_bonding(HSQUIRRELVM v)
	{
		sqlite_query *self = NULL;
		sq_getinstanceup(v,1,(SQUserPointer *)&self,0); //标准的获取C++对象的方式

		for(int nIdx=0;nIdx<self->m_nColumn;++nIdx)
		{
			sq_pushstring(v,sqlite3_column_name(self->mLink,nIdx),-1);
			const char* type=sqlite3_column_decltype(self->mLink,nIdx);

			if(strcmp(type,"TEXT")==0)
			{
				sq_pushstring(v,(const char*)sqlite3_column_text(self->mLink,nIdx),-1);
				sq_newslot(v,-3,SQFalse); //修改绑定值
			}
			if(strcmp(type,"INTEGER")==0)
			{
				sq_pushinteger(v,sqlite3_column_int(self->mLink,nIdx));
				sq_newslot(v,-3,SQFalse); //修改绑定值
			}
			if(strcmp(type,"REAL")==0)
			{
				sq_pushfloat(v,sqlite3_column_double(self->mLink,nIdx));
				sq_newslot(v,-3,SQFalse); //修改绑定值
			}
			
		}
		return 0; //无返回值
	}

	static SQInteger Sq2sqlite_current(HSQUIRRELVM v)
	{
		sqlite_query *self = NULL;
		sq_getinstanceup(v,1,(SQUserPointer *)&self,0); //标准的获取C++对象的方式

		sq_newtable(v); //创建一个输出表
		for(int nIdx=0;nIdx<self->m_nColumn;++nIdx)
		{
			const char* fname=sqlite3_column_name(self->mLink,nIdx);
			sq_pushstring(v,fname,-1);
			const char* type=sqlite3_column_decltype(self->mLink,nIdx);
			if(strcmp(type,"TEXT")==0)
			{
				sq_pushstring(v,(const char*)sqlite3_column_text(self->mLink,nIdx),-1);
				sq_newslot(v,-3,SQFalse); //创建绑定值
			}
			if(strcmp(type,"INTEGER")==0)
			{
				sq_pushinteger(v,sqlite3_column_int(self->mLink,nIdx));
				sq_newslot(v,-3,SQFalse); //创建绑定值
			}
			if(strcmp(type,"REAL")==0)
			{
				sq_pushfloat(v,sqlite3_column_double(self->mLink,nIdx));
				sq_newslot(v,-3,SQFalse); //创建绑定值
			}
			
		}
		return 1; //返回得到的数据表
	}

	static SQInteger Sq2sqlite_get(HSQUIRRELVM v)
	{
		sqlite_query *self = NULL;
		sq_getinstanceup(v,1,(SQUserPointer *)&self,0); //标准的获取C++对象的方式
		const char *fieldstr;

		int top=sq_gettop(v);
		sq_getstring(v,-1,&fieldstr);
		for(int nIdx=0;nIdx<self->m_nColumn;++nIdx)
		{
			const char* fname=sqlite3_column_name(self->mLink,nIdx);
			if(strcmp(fname,fieldstr)!=0) continue;

			const char* type=sqlite3_column_decltype(self->mLink,nIdx);
			if(strcmp(type,"TEXT")==0)
				sq_pushstring(v,(const char*)sqlite3_column_text(self->mLink,nIdx),-1);
			
			if(strcmp(type,"INTEGER")==0)
				sq_pushinteger(v,sqlite3_column_int(self->mLink,nIdx));

			if(strcmp(type,"REAL")==0)
				sq_pushfloat(v,sqlite3_column_double(self->mLink,nIdx));
						
		}
		if(top==sq_gettop(v)) sq_pushnull(v);
		return 1; //返回得到的数据
	}

	static SQInteger Sq2sqlite_insert(HSQUIRRELVM v)
	{
		sqlite_query *self = NULL;
		sq_getinstanceup(v,1,(SQUserPointer *)&self,0); //标准的获取C++对象的方式
		
		const char* tablename=NULL;
		sq_getstring(v,-2,&tablename);
		if(tablename==NULL) return 0;

		string sql="INSERT INTO ";
		sql+=tablename;
		sql+=" (";

		string values=" VALUES(";
		
		const char* key;
		char vbuf[32];

		int table_idx=sq_gettop(v);
		sq_pushnull(v); //放入迭代子
		while(SQ_SUCCEEDED(sq_next(v,table_idx)))
		{
			switch(sq_gettype(v,-1))
			{
			case OT_STRING:
				const char* valueStr;
				sq_getstring(v,-1,&valueStr);
				values+='\"';
				values+=valueStr;
				values+="\",";
				sq_getstring(v,-2,&key);
				sql+=key;
				sql+=',';
				break;
			case OT_INTEGER: //是整数
				int valueInt;
				sq_getinteger(v,-1,&valueInt);
				sprintf(vbuf,"%d",valueInt);
				values+=vbuf;
				values+=',';
				sq_getstring(v,-2,&key);
				sql+=key;
				sql+=',';
				break;
			case OT_FLOAT: //是浮点数
				SQFloat valueFloat;
				sq_getfloat(v,-1,&valueFloat);
				sprintf(vbuf,"%f",valueFloat);
				values+=vbuf;
				values+=',';
				sq_getstring(v,-2,&key);
				sql+=key;
				sql+=',';
				break;
			}
			sq_pop(v,2); //弹出不再需要的数据
		}
		sq_poptop(v); //弹出不再需要的迭代子
		if(sql[sql.length()-1]==',') //有数据输出
		{
			sql.erase(sql.length()-1,1);
			values.erase(values.length()-1,1); //去除不需要的逗号
			sql+=") ";
			values+="); ";
			sql+=values;
			self->exec(sql.c_str());
		}
		return 0;
	}

	static SQInteger Sq2sqlite_queryexec(HSQUIRRELVM v)
	{
		sqlite_query *self = NULL;
		sq_getinstanceup(v,1,(SQUserPointer *)&self,0); //标准的获取C++对象的方式

		const char* sql;
		if(SqGetVar(v,sql) && self->exec(sql)) //执行实际的对象函数
		{
			sq_pushbool(v,SQTrue);
		}
		else
			sq_pushbool(v,SQFalse);

		return 1; //有返回值
	}

	static void register_library(HSQUIRRELVM v);
	
};



#endif