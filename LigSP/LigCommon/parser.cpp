
#include <iostream>
#include <list>
#include <fstream>


#include <lig_parser.h>
#include <lig_httpd.h>
#include <cpu.h>


using namespace std;
using namespace lig;



class CaseParser //处理大小写无关的文件
{
private:
	char* pOrignBuf;
	char* pStartBuf;
	int BufLength;
	//---------------
	bool flag_case_insensitive; //为true假定要求大小写无关,会将文件内容全部转换为小写,否则不转换
	bool flag_gb2utf8; //为true则假定文件是GB2312编码并将文件内容转换为UTF-8,否则不进行转换
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

	CaseParser()
	{
		pOrignBuf=NULL;
		pStartBuf=NULL;
		flag_case_insensitive=true;
		flag_gb2utf8=true;
	};

	CaseParser(const char *fileName,bool case_insensitive=true,bool encodeGB2312=false)
	{
		flag_case_insensitive=case_insensitive;
		flag_gb2utf8=encodeGB2312;
		read(fileName);
	};

	~CaseParser()
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

	
	bool read(const char *fileName,bool encodeGB2312=true)
	{
		FILE *fs;
		flag_gb2utf8=encodeGB2312;
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
		
	
			if(BufLength>3 && (unsigned char)pOrignBuf[0]==0xef && (unsigned char)pOrignBuf[1]==0xbb && (unsigned char)pOrignBuf[2]==0xbf)
			{
				pOrign+=3; //Windows Notepad会在UTF8文本文件中加入这个标识头，我们不需要去除
				flag_gb2utf8=false; //如果有这个标识头，我们认为文件是UTF8的，即使声明为GB2312也不再转码
			}

			if(flag_gb2utf8) //需要从GB2312转换到UTF8
			{
				int UTF8Length=BufLength/2*3+16; //最大为原文本的3/2倍
				pStartBuf=new char[UTF8Length];
				BufLength=gb2utf8(pOrign,pStartBuf,UTF8Length); //执行GB2312到UTF8的转换
				pStrStart=pStartBuf;
				for(int i=BufLength;i<BufLength+16;i++) pStartBuf[i]=0;//增加一个C结束符，并加一段保护区域，文本分析的时候会使用

				delete pOrignBuf; //删除未转换的原始数据
				pOrignBuf=new char[BufLength+16];
				memcpy(pOrignBuf,pStrStart,BufLength); //拷贝转换以后的数据
				for(int i=BufLength;i<BufLength+16;i++) pOrignBuf[i]=0;
				pOrign=pOrignBuf;

			}
			else //不需要转换
			{
				pStartBuf=new char[BufLength+16];
				memcpy(pStartBuf,pOrignBuf,BufLength); //拷贝一个备份
				for(int i=BufLength;i<BufLength+16;i++) pStartBuf[i]=0; //增加一个C结束符，并加一段保护区域，文本分析的时候会使用
				pStrStart=pStartBuf+int(pOrign-pOrignBuf); //得到实际使用的指针
			}	

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


struct template_rec
{
	string template_name;
	string template_script;
	http_sq template_srv;
};

struct csp_parser
{
	int template_level; //模板深度
	int pre_level;
	int ast_id;        //页面生成树在内存中的起始位置
	//---------------------------
	string page_name; //被解析页面的名称，注意,名称不含后缀
	char function_name[256];
	//--------------------------
	http_sq page_srv; //用于脚本程序运行的对象
	string script;    //分析页面以后产生的脚本程序
	string jscript;   //分析页面产生的JavaScript脚本
	//--------------------------
	CaseParser pf;  //文件读取器
	//-------------------------------
	list<template_rec> template_list; //记录已解析模板的列表
	//------------------------------
	csp_parser();
	void onTemplateEnd(Scan *it);
	
	void onTemplateHead(Scan *it);
	void onTemplate(Scan *it);
	bool analysis_csp(const char* filename,bool encodeGB2312=false);
};

namespace LigSPDef
{
	Item sp;
	Item term;
	Item Alpha;
	Item Num;
	Item Word;
	Item ExpressItem;
	Item SquirrelExpress;
	Item StateChar;
	//----------------------------


	Node Name("Name");
	Node sqName("sqName");
	Node String("String");
	Node script_code("script_code");  
	Node script_head("script_head");  
	Node macro_head("macro_head");  
	Node macro_end("macro_end"); 
	Node macro_keyword("macro_keyword");
	Node html_macro("html_macro");
	Node if_head("if_head");
	Node if_end("if_end");
	Node if_else("if_else");
	Node define_express("define_express");
	Node id_express("id_express");
	Node var_express("var_express");
	Node sq_express("sq_express");
	Node exec_macro("exec_macro");
	Node memo("memo");   
	Node segment("segment");   
	Node segment_full("segment_full"); 
	Node Eseg("ESegment");
	Node All("All"); 
	Node ExecuteParse("ExecuteParse");    
	Node VarKey("VarKey");
};
//--------------------------
//Node StateVar("StateVar");
//Node StateExpress("StateExpress");
//---------------------------


/*

<!--template(...)-->           定义一个HTML模板,其参数是Squirrel的变量
<!--end template-->    
$...                        模板定义变量输出
<!--if_template(...)-->        根据表达式做模板判定，其表达式为Squirrel的表达式
<!--else_template-->
<!--end_if-->
<%  %>			             squirrel嵌入代码,代码直接在页面中执行
$!..    $!{...}             squirrel表达式(模板变量)写入页面或者模板
$@template_name()              执行一个模板
${...}                       模板控件的绑定标识


*/
using namespace LigSPDef;
void init_parser()
{
	term  <= ' ' | '\t' | '\r' | '\n' | '|' | '-' | '=' | '\"' | '\'' | '<' | '>';
	sp <= ' ' | '\t' | '\r' | '\n';

	Alpha.range('A','Z');
	Alpha.range('a','z');
	Num.range('0','9');
	Word.char_set("_.");
	Word <= Alpha | Num;
	
	
	SquirrelExpress.char_set("_.[]()<>?:!=/+-*;^&|"); 
	SquirrelExpress<=Alpha | Num;
	ExpressItem.char_set(")");
	String <= N0(ExpressItem);

	Name <= Alpha & R0(Word);
	sqName <= '(' & N0(')') & ')';
	
	script_code <= "<%" & NS0("%>") & "%>";
	script_head <= "<!--head" & NS0("%>") & "-->";
	macro_head <= "<!--template" & R1(sp) & Name & R0(sp) & '(' & R0(sp) & Rn(Name,0,1) & GR0(R0(sp) & ',' & R0(sp) & Name) & R0(sp) & ')' & NS0("-->") & "-->";
	var_express <=  "$" & VarKey;
	id_express <= "$id={" & R1(Word) & '}'; 
	id_express <= "${" & R1(Word) & '}'; 
	define_express <= "$[" & R1(Word) & ']'; 
	sq_express <= "$!" & R1(SquirrelExpress);
	sq_express <= "$!{" & NS1("}") & "}";
	exec_macro <= "$@" &  Name  & sqName;
	macro_end <= "<!--end" & NS0("-->") & "-->";
	macro_keyword <= "-->" | "template" | "end" | "head";
	memo <= "<!--" & N0(macro_keyword) & "-->";

	if_head <= "<!--if_template" & R0(sp) & '(' & String & ')' & R0(sp) & "-->";
	if_end <= "<!--end_if" & R0(sp) & "-->";
	if_else <= "<!--else_template" & R0(sp) & "-->";

	html_macro <= macro_head & R0(segment) & Eseg;
	segment_full <=  "<!--" | "<%" | var_express | sq_express | exec_macro | id_express | define_express | "#$";
	
	Eseg<=N0(segment_full) & macro_end;
	segment <= N0(segment_full) & html_macro;
	segment <= N0(segment_full) & var_express;
	segment <= N0(segment_full) & sq_express;
	segment <= N0(segment_full) & exec_macro;
	segment <= N0(segment_full) & script_code;
	segment <= N0(segment_full) & script_head;
	segment <= N0(segment_full) & if_head;
	segment <= N0(segment_full) & if_else;
	segment <= N0(segment_full) & if_end;
	segment <= N0(segment_full) & id_express;
	segment <= N0(segment_full) & define_express;
	segment <= N0(segment_full) & memo;
	
	
	ExecuteParse <=  R1(segment);
	Parser::recompute_firstset();

	
}


ostream& operator<<(ostream& out,ScanItem<char> *it)
{
	const char* p=it->start;
	while(p<it->end && *p!=0)
	{
		out.put(*p);
		++p;
	}
	return out;
}

csp_parser::csp_parser()
{
	template_level=0;
	init_parser();
};

//----------------------------------------


void csp_parser::onTemplate(Scan *tmpt)
{
	Scan* rt=tmpt->get("macro_head","Name");
	if(rt==NULL) return;

	template_rec tmp;
	tmp.template_srv.pRoot =tmpt; //模板的起点，目前没有使用2008.09.05
	
	Scan* it=rt;
	string script="this.";
	string tpname;
	tpname.append(pf.ostr(it->start),it->controled_size);
	script+=tpname;

	string boundVar="\n\tvar rt";
	boundVar+="={";
	string boundStr="function get";
	boundStr+=tpname; //模板绑定的表,用于产生javascript代码,可以方便的上传数据
	boundStr+="()\n {\n\t";
	boundStr+="\n\tvar obj;";
	int boundLoc=boundStr.length(); 
	
	tmp.template_name.append(pf.ostr(it->start),it->controled_size);
	script+="_exec <- function(";
	it=it->next("Name");
	bool flag_have_parameter=false; //判断模板是否带参数
	while(it!=NULL)
	{
		flag_have_parameter=true;
		script.append(pf.ostr(it->start),it->controled_size);
		it=it->next("Name");
		if(it!=NULL) script+=',';
	}
	if(flag_have_parameter) //模板有参数
		script+=",session,io)\n{\n\tlocal io=io.getTemplate(\"";
	else //模板没有参数
		script+="session,io)\n{\n\tlocal io=io.getTemplate(\"";
	script+=tpname;
	script+="\");\n";
	
	it=tmpt->get("segment"); //首先寻找有分割的模板输出起始位置
	if(it==NULL) it=tmpt->get("ESegment"); //如果没有分割，寻找模板未分割起始位置
	if(it!=NULL)
	{
		tmp.template_srv.pStart=it; //记录模板输出的起点,这个值反映在squirrel中使用
		tmp.template_srv.thePosIdx.clear();
		Scan* iPage=it;
		while(iPage!=NULL)  //此处填写模板各段输出的索引
		{
			tmp.template_srv.thePosIdx.push_back(iPage); 
			iPage=iPage->pBrother; 
		}
	}
	
	string IfExpressStr; //记录上一个if表达式的内容，用于else_if

	int sIdx=0; //Squirrel写的索引
	char IdxBuf[16];
	while(it!=NULL)
	{
	
		script+="\tio.idxWrite(";
		sprintf(IdxBuf,"%d",sIdx++);
		script+=IdxBuf;
		script+=");\n";
		Scan* bit=it;;
		if(strcmp(it->name,"segment")==0) bit=it->pChild;
		if(bit==NULL) break;
		if(strcmp(bit->name,"var_express")==0)
		{
			script+="\tio.write(";
			script.append(pf.ostr(bit->pChild->start),bit->pChild->controled_size); //Var_key
			script+=");\n";
		}
		if(strcmp(bit->name,"script_code")==0)
		{
			script.append(pf.ostr(bit->start + 3),(int)(bit->end - bit->start - 5));  
			script+="\n"; //这里发生过一个有趣的bug,如果代码是注释，不换行，可能注释掉自动产生的代码
		}
		if(strcmp(bit->name,"script_var")==0)
		{
			script+="\tio.write(";
			script.append(pf.ostr(bit->pChild->start),bit->pChild->controled_size);
			script+=");\n";
		}
		
		if(strcmp(bit->name,"if_head")==0)
		{
			Scan* itp=bit;
			script+="\tif(!(";
			Scan* ivar=itp->get("String");
			IfExpressStr="";
			IfExpressStr.append(pf.ostr(ivar->start),ivar->controled_size); 
			script.append(pf.ostr(ivar->start),ivar->controled_size);
			script+=")) io.innerClose();\n";
			
		}

		if(strcmp(bit->name,"if_else")==0)
		{
			script+="\tio.innerOpen();\n\tif(";
			script+=IfExpressStr;
			script+=") io.innerClose();\n";
			
		}

		if(strcmp(bit->name,"if_end")==0)
		{
			script+="\tio.innerOpen();\n";
		}

		if(strcmp(bit->name,"sq_express")==0)
		{
			Scan* itp=bit;
			script+="\tio.write(";
			script.append(pf.ostr(itp->start+2),itp->controled_size-2);
			script+=")\n";
		}

		if(strcmp(bit->name,"id_express")==0)
		{
			Scan* itp=bit;
			script+="\tio.write(\"id = '";
			script+=tpname;
			script+='.';
			string idep;
			if(*(pf.ostr(itp->start+2))=='d' && *(pf.ostr(itp->start+3))=='=') //判断两种模式的自动ID拼写方法
				idep.append(pf.ostr(itp->start+5),itp->controled_size-6); 
			else
				idep.append(pf.ostr(itp->start+2),itp->controled_size-3);
			script.append(idep);
			script+="'\");\n";
			
			boundVar.append(idep);
			boundVar+=":\"\",";

			boundStr+="\n\tobj=document.getElementById(\"";
			boundStr+=tpname;
			boundStr+='.';
			boundStr.append(idep);
			boundStr+="\");\n\tif(obj!=null){\n\t";
			boundStr+="if(obj.selectedIndex!=null) rt.";
			boundStr.append(idep);
			boundStr+="=obj.options[obj.selectedIndex].text;\n\telse "; 
			boundStr+="rt.";
			boundStr.append(idep);
			boundStr+=" = obj.value;\n\t};\n";

			
		}

		it=it->pBrother; 
	}
	
	script+="};\n";
	//---------------------
	script+="do_";
	script+=tpname;
	script+="<-";
	script+=tpname;
	script+="_exec.bindenv(this);\n\n";
	
	if(boundStr[boundStr.length()-1]=='\n') //模板有绑定
	{
		boundStr+="\treturn rt;\n};\n";
		boundVar.erase(boundVar.length()-1,1);
		boundVar+="};\n";
		boundStr.insert(boundLoc,boundVar); 
	}
	else boundStr=""; //模板没有绑定，不需要绑定表

	jscript+=boundStr; //JavaScript的绑定
	tmp.template_script=script; 
	template_list.push_back(tmp); 

	//----------------------------------------------------------
	rt=rt->next("Name");
	while(rt!=NULL)
	{
		VarKey.delete_key(rt->start,rt->end);
		Parser::update_firstset(var_express);
		rt=rt->next("Name");
	}

}



void csp_parser::onTemplateHead(Scan *it)
{
	static int handle=1234;
	Scan* rt=it->get("Name");
	if(rt==NULL) return;
	rt=rt->next("Name");
	while(rt!=NULL)
	{
		VarKey.insert_key(rt->start,rt->end,++handle,term);
		Parser::update_firstset(var_express);
		Parser::update_firstset(VarKey);
		//DOUT<<rt->str().c_str()<<","<<endl; 
		rt=rt->next("Name");
	}
	template_level++;
}

void csp_parser::onTemplateEnd(Scan *it)
{
	template_level--;
}


struct innerTable
{
	const char *request_page; //请求的页面
	csp_parser ul;
};

struct lessTable
{
	inline bool operator()(const innerTable &v1,const innerTable &v2)const
	{
		return(strcmp(v1.request_page,v2.request_page)<0);  
	}
};

static set<innerTable,lessTable> mRegDB; //记录页面与页面分析结果的数据库

bool csp_parser::analysis_csp(const char* filename,bool encodeGB2312)
{
	static Parser::ScanMemAlloctor cspMem;

	int ast=0;
	if(!pf.read(filename,encodeGB2312) 
		|| pf.pStr==NULL) return false;

	page_name=function_name;
	html_macro.connect(this,&csp_parser::onTemplate);
	macro_head.connect(this,&csp_parser::onTemplateHead);
	macro_end.connect(this,&csp_parser::onTemplateEnd); 
	
	page_srv.function_name=function_name; //得到名字
	page_srv.pRoot = cspMem.alloc(&ast);
	page_srv.pRoot->clear(); 
	
	CpuClock.start_clock(); 
	if(ExecuteParse.parse(pf.pStr,cspMem,page_srv.pRoot))
	{
		int tspan=CpuClock.get_timespan();
#ifdef _LIG_DEBUG
		DOUT<<"Anasys "<<filename<<" : Time Span="<<tspan;
		DOUT<<" Used Memory="<<ast<<endl;;
#endif
		page_srv.pRoot->offset_children_str(pf.offset);  //将分析结果文本指针指向原始文本位置
	}
	else
	{
		if(pf.pStr==NULL) //这是一个不包含任何已定义Tag的HTML页面
		{
#ifdef _LIG_DEBUG
			DOUT<<"Blank Page"<<endl;
#endif
		}
		ast=ast_id; //分析失败，生成树恢复到原来的位置，防止内存被无效占用
		int errline,offset;
		string err=Parser::error().getErrorLine(errline,offset);
#ifdef _LIG_DEBUG
		DOUT<<"Parse "<<filename<<" Error at Line "<<errline<<endl;
		DOUT<<err.c_str()<<endl;
#endif
	}

	Scan* it=page_srv.pRoot->pChild;
	if(it==NULL) return false;
	
	page_srv.pStart=it; //此处定义Squirrel输出的起点
	page_srv.thePosIdx.clear();
	Scan* iPage=it;
	if(strcmp(it->name,"ExecuteParse")==0) iPage=it->pChild;
	while(iPage!=NULL)  //此处填写页面各段输出的索引
	{
		page_srv.thePosIdx.push_back(iPage); 
		page_srv.pLastStr=iPage->end; //保留最后一段未进行分析的字符串 
		iPage=iPage->pBrother; 
	}
	
	
	list<template_rec> &tlist=template_list;
	list<template_rec>::iterator tp=tlist.begin();
	while(tp!=tlist.end())
	{
		//注册模板
		tp->template_srv.function_name=function_name; //所有在页面内的模板，function_name都一致  
		script+=tp->template_script; 
		++tp;
	}

	script+="this.";
	script+=function_name;
	script+="<-function(skey,session,io)\n{\n";
	int script_head_pos=script.length(); 
	
	int sIdx=0; //Squirrel写的索引
	char IdxBuf[16];
	Scan* ait=it;
	if(strcmp(ait->name,"ExecuteParse")==0) ait=ait->pChild;
	while(ait!=NULL)
	{
		it=ait;
		if(strcmp(it->name,"segment")==0) it=ait->pChild;
		
		if(it==NULL) return false;
		script+="\tio.idxWrite(";
		sprintf(IdxBuf,"%d",sIdx++);
		script+=IdxBuf;
		script+=");\n";
		if(strcmp(it->name,"script_code")==0)
		{
			script.append(it->start + 2,(int)(it->end - it->start - 4));   
			script+="\n";
		}
		if(strcmp(it->name,"script_head")==0)
		{
			string headstr;
			headstr.append(it->start+6,it->controled_size-8);
			script.insert(script_head_pos,headstr);
			script_head_pos+=headstr.length(); 
		}

		if(strcmp(it->name,"if_head")==0)
		{
			Scan* itp=it;
			script+="\tif(!(";
			Scan* ivar=itp->get("String");
			script.append(ivar->start,ivar->controled_size);
			script+=")) io.innerClose();\n";
			
		}

		if(strcmp(it->name,"if_end")==0)
		{
			script+="\tio.innerOpen();\n";
		}

		if(strcmp(it->name,"sq_express")==0)
		{
			Scan* itp=it;
			script+="\tio.write(";
			script.append(itp->start+2,itp->controled_size-2);
			script+=");\n";
		}

		if(strcmp(it->name,"exec_macro")==0)
		{
			Scan* rt=it->get("Name");
			if(rt==NULL) return false;

			script+="\tdo_";
			script.append(rt->start,rt->controled_size);
			rt=rt->next("sqName");
			script.append(rt->start,rt->controled_size-1);
			bool flag_comma=false;
			for(int fi=1;fi<rt->controled_size-1;++fi)
				if(*(rt->start+fi)!=' ' && *(rt->start+fi)!='\t' && *(rt->start+fi)!='\n' && *(rt->start+fi)!='\r') {flag_comma=true;break;}

			if(flag_comma)	script+=",session,io);\n";
			else script+="session,io);\n";
		}

		ait=ait->pBrother; 
	}
	script+="\tio.lastWrite();\n";
	script+="}\n";

	page_srv.javascript_code = jscript; 

	ofstream of;
	string outname=filename;
	outname+=".txt";
	of.open(outname.c_str()); 
	of<<script.c_str()<<endl; 
	of.close(); 
		
	return true;
};


http_sq* get_page_sq(const char* requested_page)
{
	innerTable tmp;
	tmp.request_page = requested_page;
	set<innerTable,lessTable>::iterator rt;
	rt = mRegDB.find(tmp);
	if(rt==mRegDB.end()) return NULL;
	const_cast<http_sq*>(&(rt->ul.page_srv))->reset();  //重置变量，用于输出
	return const_cast<http_sq*>(&(rt->ul.page_srv));
};

http_sq* get_template_sq(const char* requested_page,const char* requested_template)
{
	innerTable tmp;
	tmp.request_page = requested_page;
	set<innerTable,lessTable>::iterator rt;
	rt = mRegDB.find(tmp);
	if(rt==mRegDB.end()) return NULL;
	list<template_rec> &tlist=*(const_cast<list<template_rec>*>(&(rt->ul.template_list)));
	list<template_rec>::iterator it=tlist.begin();
	while(it!=tlist.end())
	{
		if(it->template_name==requested_template)
		{
			it->template_srv.reset();   //重置对象
			return const_cast<http_sq*>(&(it->template_srv));//返回找到的对象
		}
		++it;
	}
	return NULL;
}

const char* create_page(httpd& server,const char* url,const char* filename,bool encodeGB2312)
{
	innerTable tmp;

	char fn[256];
	const char* p=filename;
	if(*p=='/') ++p;//return false; else ++p;
	int i=0;int j=0;
	while(*p!=0  && i<255) //得到一个Squireel对应页面的函数名称
	{
		if(*p!='/' && *p!='\\' && *p!='.') //去掉可能引起Squirrel的字符
			fn[j++]=*p;
		++i;++p;
	}
	fn[j]=0;
	tmp.request_page=fn; 
	
	pair<set<innerTable,lessTable>::iterator,bool> rt=mRegDB.insert(tmp);
	if(!rt.second) return NULL;
	csp_parser& ul=(csp_parser&)(rt.first->ul);  
	strcpy(ul.function_name,fn);
	
	string real_filename=p_root_dir; //获得定义的根目录 
	int li=real_filename.length()-1;
	while(isspace(real_filename[li])) {real_filename.erase(li,1);li--;} //删去不必要的空格
	if(real_filename[li]=='/' || real_filename[li]=='\\')
		real_filename+=filename;
	else
	{
		real_filename+='/';
		real_filename+=filename;
	} //得到完整文件名

	if(ul.analysis_csp(real_filename.c_str(),encodeGB2312) &&
		server.register_page(url,ul.function_name,ul.script))
	{
		*((const char**)&(rt.first->request_page))=ul.function_name;
		return ul.function_name;
	}
	mRegDB.erase(rt.first); 
 	return NULL;
}

const char* create_session_page(httpd& server,const char* url,const char* filename,bool encodeGB2312)
{
	innerTable tmp;
	char fn[256];
	const char* p=filename;
	if(*p=='/') ++p;//return false; else ++p;
	int i=0;int j=0;
	while(*p!=0  && i<255) 
	{
		if(*p!='/' && *p!='\\' && *p!='.')
			fn[j++]=*p;
		++i;++p;
	}
	fn[j]=0;
	tmp.request_page=fn; 
	
	pair<set<innerTable,lessTable>::iterator,bool> rt=mRegDB.insert(tmp);
	if(!rt.second) return NULL;
	csp_parser& ul= *((csp_parser*)&(rt.first->ul));  
	strcpy(ul.function_name,fn);
	*((const char**)&(rt.first->request_page))=ul.function_name;  
 
	string real_filename=p_root_dir; //获得定义的根目录 
	int li=real_filename.length()-1;
	while(isspace(real_filename[li])) {real_filename.erase(li,1);li--;} //删去不必要的空格
	if(real_filename[li]=='/' || real_filename[li]==0x5c)
		real_filename+=filename;
	else
	{
		real_filename+='/';
		real_filename+=filename;
	}

	if(ul.analysis_csp(real_filename.c_str(),encodeGB2312) &&
		server.register_session_page(url,ul.function_name,ul.script))
	{
		*((const char**)&(rt.first->request_page))=ul.function_name;  
		return ul.function_name;
	}
	mRegDB.erase(rt.first); 
 	return NULL;
}

CaseParser init; 
const char* get_init_script(const char* init_fn,bool encodeGB2312=false) //文件是UTF-8的,如果是GB2312,请设定flagUseGB2312为true
{
	init.set_case_sensitive(); //配置文件是大小写相关的
	if(init.read(init_fn,encodeGB2312)) return init.pStr;  
	return NULL;
}
