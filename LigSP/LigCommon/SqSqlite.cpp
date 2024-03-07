//----------Sqlite DB support---------------------------
void sqlite_query::register_library(HSQUIRRELVM v)
{
	int top=sq_gettop(v);
	sq_pushroottable(v); //push the root table(were the globals of the script will be stored)
	
	sq_pushstring(v,"sql_query",-1);
	sq_newclass(v,SQFalse);
	sq_createslot(v,-3);
	sq_settop(v,top); //restores the original stack size

	register_func(v,"sql_query",Sq2Constructor,"constructor",NULL);//构造函数
	register_func(v,"sql_query",Sq2sqlite_queryexec,"exec","xs"); //bool exec("SQL str");返回为真表示有数据输出,否则表示没有数据输出
	register_func(v,"sql_query",Sq2sqlite_querynext,"next","x"); //bool next();从输出集合中读取下一条记录,如果下一条不存在,则返回false
	register_func(v,"sql_query",Sq2sqlite_queryeof,"eof","x");   //bool eof(); 判断数据集合中是否还有数据存在
	register_func(v,"sql_query",Sq2sqlite_queryfailed,"fail","x");   //bool fail(); 判断前一个SQL执行状态是否失败
	register_func(v,"sql_query",Sq2sqlite_bonding,"get","xt"); //void get(tableVar); 将当前数据输出写入一个Squirrel表中 
	register_func(v,"sql_query",Sq2sqlite_current,"current","x");//OT_TABLE current(); 将当前数据作为一个表输出 
	register_func(v,"sql_query",Sq2sqlite_insert,"insert","xst");//OT_TABLE insert(); 将当前数据输出 

	register_func(v,"sql_query",Sq2sqlite_get,"_get","xs");//OT_TABLE current(); 获取当前数据，这是一个meta方法 
	
	register_func(v,"sql_query",Sq2sqlite_initDB,"initDB","xs"); //bool exec("SQL str");返回为真表示有数据输出,否则表示没有数据输出
	
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