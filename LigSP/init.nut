/*************************************************************************
SQSP veesion alpha
����SQSP�ĳ�ʼ�������ļ�,����һ��squirrel�ű�,��������������ű���������
*************************************************************************/
//---------------------------------------------------------------------
//---������Ҫ��֤ҳ��ĵ�¼ҳ��,���������,�Զ�ʹ��"autologin"Ϊ��¼·��
//----------------------------------------------------------------------
AUTO_LOGIN_URI<-"autologin.html"; //�ض���URL
//----------------------------------------------------------------------
//----��һ����,����������ĸ�Ŀ¼�������˿ں�,�Լ��ͻ������ӵ���������,����ʡ��
//-----------------------------------------------------------------------
ROOT_DIR<-"./root";
//�������80�˿ڣ���Ŀ¼Ϊ./root
server<-WebServer("./root",9000,2*1024*1024,20); //FCGI�˿�Ϊ9000 ,�˴�����ͻ������ӵ����ʱ20��
//server<-WebServer(ROOT_DIR,80,2*1024*1024,20); //�˴������Ŀ¼�������˿ڡ�����ļ��ϴ��ߴ磬�ͻ������ӵ����ʱ����,KeepAlive���ֵ�ʱ�䣩
//-----------------------------------------------------------------------
//----�ڶ�����,��ʼ��json��д����,����ȫ������,�����ʹ�ã�����ʡ��
//-----------------------------------------------------------------------
//nativeDB=read_josn("nativeDB.txt");
//----------------------------------------------
//----��������,���嶯ִ̬���ļ�,���ٶ���һ��
function URITranslate(header,uri)  //����һ��������ڵķ��뺯����ϵͳ�ᴫ��URL��������������Ժ󣬷��ظ�����URL
{
	
	return uri;
}
server.joinPage("sample01.html","sample01.hnut");
server.joinPage("sample02.html","sample02.hnut");
server.joinPage("sample03.html","sample03.hnut");
server.joinPage("sample04.html","sample04.hnut");
server.joinPage("sample05.html","sample05.hnut");
server.joinPage("sample06.html","sample06.hnut");
server.joinPage("sample07.html","sample07.hnut");
server.joinPage("sample08.html","sample08.hnut");

/*

function upLoadFile(skey,session,io)
{
	local rt=io.getUpload()
	io.write("<form method='post' enctype='multipart/form-data'>");
	io.write("<strong>�������ļ�:</strong><input type='file' name='fileUpload' id='fileField' /><input type='submit' name='submitButton' id='button' value='�ύ�ļ�' /></form>");
	if(rt!=null)
	{
		rt.fileUpload.writeFile("root/skey_"+rt.fileUpload.uploadname);
		io.write("<a href='"+"skey_"+rt.fileUpload.uploadname +"'>"+rt.fileUpload.uploadname+"</a>");
	
	}
}
server.mapFunction("uploadFile.hnut","upLoadFile");
*/

//-----------------------------------------------------------
//----���Ĳ���,����һ��Sqlite���ݿ�,�������Ҫ���ݿ�,����ʡ��
//-----------------------------------------------------------
/*
sqlite<-sql_query(); //����һ�����ݿ����
sqlite.initDB("World.db3");
sql<-sql_query("CREATE TABLE IF NOT EXISTS User(id INTEGER PRIMARY KEY AUTOINCREMENT, user_name TEXT, password TEXT,sex INTEGER, e_mail TEXT, phone TEXT,mobile TEXT,money REAL);"); //��ȫ�ֱ��д���һ����ѯ����
user<-{}
user.user_name<-"lig";
user.password<-"ldghj1";
user.sex<-"male";
user.e_mail<-"lig@tedc.cn";
user.phone<-"13801316550";
user.mobile<-"13801316550";
user.money<-0.0;

sql.insert("User",user);
*/
/*----------------------------------------------------------------
����Ϊ�Զ������򣬿������ɶ��������ȫ�ֺ������ࡣ������
��������ļ��ж���ı�������������ȫ�ֵ�,�ܹ����κεط���������ִ��
----------------------------------------------------------------*/

//--------------------------------------------------------
//----���Ĳ���,����Web������,����ʡ��,���ұ���ŵ����ִ��
//--------------------------------------------------------


print("WebServer is starting..");
server.local_service(); //Ҫ����������ܻ���������ӣ���ֹ������¥
//server.run_web();  //����ִ��WebServer
server.run_fcgi();

