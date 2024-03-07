/*************************************************************************
SQSP veesion alpha
这是SQSP的初始化配置文件,它是一个squirrel脚本,服务器按照这个脚本进行启动
*************************************************************************/
//---------------------------------------------------------------------
//---定义需要验证页面的登录页面,如果不定义,自动使用"autologin"为登录路径
//----------------------------------------------------------------------
AUTO_LOGIN_URI<-"autologin.html"; //重定向URL
//----------------------------------------------------------------------
//----第一步骤,定义服务器的根目录及监听端口号,以及客户端连接的生命周期,不可省略
//-----------------------------------------------------------------------
ROOT_DIR<-"./root";
//定义监听80端口，主目录为./root
server<-WebServer("./root",9000,2*1024*1024,20); //FCGI端口为9000 ,此处定义客户端连接的最大超时20秒
//server<-WebServer(ROOT_DIR,80,2*1024*1024,20); //此处定义根目录、监听端口、最大文件上传尺寸，客户端连接的最大超时（秒,KeepAlive保持的时间）
//-----------------------------------------------------------------------
//----第二步骤,初始化json读写对象,读入全局数据,如果不使用，可以省略
//-----------------------------------------------------------------------
//nativeDB=read_josn("nativeDB.txt");
//----------------------------------------------
//----第三步骤,定义动态执行文件,至少定义一个
function URITranslate(header,uri)  //这是一个必须存在的翻译函数，系统会传入URL，这个函数解释以后，返回给定的URL
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
	io.write("<strong>附带的文件:</strong><input type='file' name='fileUpload' id='fileField' /><input type='submit' name='submitButton' id='button' value='提交文件' /></form>");
	if(rt!=null)
	{
		rt.fileUpload.writeFile("root/skey_"+rt.fileUpload.uploadname);
		io.write("<a href='"+"skey_"+rt.fileUpload.uploadname +"'>"+rt.fileUpload.uploadname+"</a>");
	
	}
}
server.mapFunction("uploadFile.hnut","upLoadFile");
*/

//-----------------------------------------------------------
//----第四步骤,定义一个Sqlite数据库,如果不需要数据库,可以省略
//-----------------------------------------------------------
/*
sqlite<-sql_query(); //创建一个数据库对象
sqlite.initDB("World.db3");
sql<-sql_query("CREATE TABLE IF NOT EXISTS User(id INTEGER PRIMARY KEY AUTOINCREMENT, user_name TEXT, password TEXT,sex INTEGER, e_mail TEXT, phone TEXT,mobile TEXT,money REAL);"); //在全局表中创建一个查询对象
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
以下为自定义区域，可以自由定义变量及全局函数，类。。。。
这个配置文件中定义的变量及函数都是全局的,能够在任何地方被看见并执行
----------------------------------------------------------------*/

//--------------------------------------------------------
//----最后的步骤,启动Web服务器,不可省略,而且必须放到最后执行
//--------------------------------------------------------


print("WebServer is starting..");
server.local_service(); //要求仅本机才能获得网络连接，防止数据绣楼
//server.run_web();  //启动执行WebServer
server.run_fcgi();

