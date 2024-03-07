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