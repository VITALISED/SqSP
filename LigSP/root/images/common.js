// ֻ���������� onkeydown="onlyNum();"
function onlyNum()
{

 if(!(event.keyCode==46)&&!(event.keyCode==8)&&!(event.keyCode==37)&&!(event.keyCode==39)&&!(event.keyCode==9))
  if(!((event.keyCode>=48&&event.keyCode<=57)||(event.keyCode>=96&&event.keyCode<=105)))
    event.returnValue=false;
}

//��������
function open_myWindows(field)
{
	var winproperty = "toolbar=no,location=no,menubar=no,scrollbars=no,resizable=no,width=50,height=10,top="+screen.width+",left="+screen.height;
	window.open(field,"",winproperty);
}

//ɾ��ȷ��
function del_myWindows(msg,field)
{
	if (confirm(msg)){
		window.location.href=field;
	}
}

//
function changecolor(index,num) {
	for(var i=1;i<=num;i++) {
		eval("record"+i+".style.background=\"#f9f9f9\";");
	}
	eval("record"+index+".style.background=\"#CCCCCC\";");
}
function fun(index) {
    var num = document.forms[0].totalrecord.value;
    for(var i=1;i<=num;i++) {
        eval("record"+i+".style.background=\"#f9f9f9\";");
    }
    eval("record"+index+".style.background=\"#CCCCCC\";");
}


// ��һ������ת��Ϊ����
function var_to_obj(val)
{
	this.value=val;
}
// �ж��Ƿ����ĳ���� *** ���� !is_greater ******
function is_greater(field,crit,limit)
{
	var Ret = (is_numeric(field,-1) ) ? (field.value > limit )  : false;
	if( is_null(field,-1,"") )
	{
		Ret = true ;
	}
	if (!Ret)
		doCritCode(field,crit,"Value must be greater than "+limit);
	return(Ret);
}
// �ж���ֵ��С **** ���� !compare_num ****
function compare_num(Num1,Num2,crit,msg){
	var Ret = true;
	if (eval(Num1.value) > eval(Num2.value))
	{
		Ret = false;
	}

	if (!Ret)
		doCritCode(Num1,crit,msg);
	return(Ret);
}
// �ж��Ƿ�С��ĳ����  **** С�� !is_less ****
function is_less(field,crit,limit)
{
	var Ret = (is_numeric(field,-1) ) ? (field.value < limit )  : false;

	if( is_null(field,-1,"") )
	{
		Ret = true ;
	}

	if (!Ret)
		doCritCode(field,crit,"Value must be less than "+limit);
	return(Ret);
}
//�Ƚ��������ڵĴ�С��Num1>Num2 return:true;Num1<=Num2 return:false  **** ���� compare_date ****
function compare_date(Num1,Num2)
{
		var pos1,pos2,end;
		var para1,para2,para3,para4,para5,para6;

		//para1:��
		//para2:��
		//para3:��
		end=Num1.length;
		pos1=Num1.indexOf("/",0);
		pos2=Num1.indexOf("/",pos1+1);
		para1=Num1.substring(0,pos1);
		para2=Num1.substring(pos1+1,pos2);
		para3=Num1.substring(pos2+1,end);
		para1=parseInt(para1,36);
		para2=parseInt(para2,36);
		para3=parseInt(para3,36);
		end=Num2.length;
		pos1=Num2.indexOf("/",0);
		pos2=Num2.indexOf("/",pos1+1);
		para4=Num2.substring(0,pos1);
		para5=Num2.substring(pos1+1,pos2);
		para6=Num2.substring(pos2+1,end);
		para4=parseInt(para4,36);
		para5=parseInt(para5,36);
		para6=parseInt(para6,36);
		if(para1>para4)
		{
			return true;
		}
		else if(para1==para4)
		{
			if(para2>para5)
			{
				return true;
			}
			else if(para2==para5)
			{
				if(para3>para6)
				{
					return true;
				}
			}
		}
		return false;

}
//format float data as:*****.**
//decplaces:С��λ��
function FloatFormat(expr,decplaces)
{

        if ( expr == "" ) return "";
        if ( expr == "0" ) return "0.00";
        var str = "" + Math.round(eval(expr)*Math.pow(10,decplaces));
        while(str.length <= decplaces)
        {
                str = "0" + str;
        }

        var decpoint = str.length - decplaces;
        return str.substring(0,decpoint) + "." + str.substring(decpoint,str.length);
}

// ȡ�����ַ������ֽڳ��� ��dxj 2004-3-17��
function cnStrlen(str){
	var len;
	var i;
	len = 0;
	for (i=0;i<str.length;i++){
		if (str.charCodeAt(i)>255)
			len+=2;
		else
			len++;
	}
	return len;
}

//�ж������ַ�������С�ڹ涨�ֽڳ��ȣ�С�ڣ�true ����:flase
function less_cnLength(field,limit,crit,msg)
{
	var Ret = false;
	if(cnStrlen(field.value)>limit){
          doCritCode(field,crit,msg);
	}
	else{
		Ret = true;
	}
	return (Ret);
}


//�ж��ַ������� ***** ���� is_less ****
function is_length(field,limit,crit,msg)
{
	var Ret = false;
	if(field.value.length>limit){
		doCritCode(field,crit,msg);
		Ret = true;
	}
	else{
		Ret = false;
	}
	return (Ret);
}


//�ж��ַ�������***** �涨���� **** ���� order_length ****
function order_length(field,limit,crit,msg)
{
	var Ret = false;
	if(field.value.length==limit || field.value.length==0){
		Ret = false;
	}
	else{
		doCritCode(field,crit,msg);
		Ret = true;
	}
	return (Ret);
}


//�ж��ַ�������С�ڹ涨���ȣ�С�ڣ�true ����:flase
function less_length(field,limit,crit,msg)
{
	var Ret = false;
	if(field.value.length>limit){
          doCritCode(field,crit,msg);
	}
	else{
		Ret = true;
	}
	return (Ret);
}



//ֻ������12:23 ��ʽ  true :�� flase :��
function is_HM2int(field,crit,msg)
{
	var Ret = false;
	var NumStr="0123456789";
	var chr;
        var para=field.value.indexOf(":",0);

	if( is_null(field,-1,"") )
	{
		Ret = true ;
	}


	for (i=0;i<field.value.length;++i)
	{
		chr=field.value.charAt(i);
		if (NumStr.indexOf(chr,0)==-1)
		{
			if ( chr==":")
			{
                            if((field.value.substring(para+1)>=0)&&(field.value.substring(para+1)<60))
                                    Ret=true;
                            else
                                    Ret=false;
			}

			else
			{
				Ret=false;
			}
		}
	}


	if (!Ret)
		doCritCode(field,crit,msg);
	return(Ret);
}


//ֻ����������  **** ��������false  !is_numeric ****
function is_numeric(field,crit,msg)
{
	var Ret = true;
	var NumStr="0123456789";
	var decUsed=false;
	var chr;
	for (i=0;i<field.value.length;++i)
	{
		chr=field.value.charAt(i);
		if (NumStr.indexOf(chr,0)==-1)
		{
			if ( (!decUsed) && chr==".")
			{
				decUsed=true;
			}
			else
			{
				Ret=false;
			}
		}
	}

	//start  dxj 2003/2/19
	// ����һ��  "."����Ϊ����
	if(Trim(field.value)==".")
		Ret = false;

 /*
 //�������� "."�Զ�ת��Ϊ0.0
 var tempVal = Trim(field.value);
	if(tempVal.indexOf(".")==0)
		  tempVal = "0"+tempVal;
 if(tempVal.indexOf(".")== (tempVal.length-1) )
		  tempVal = tempVal+"0";
	field.value = tempVal;
	*/
 //end

	if (!Ret)
		doCritCode(field,crit,msg);
	return(Ret);
}
 // �ж��Ƿ��Ǽ۸�
function is_price(field,crit,msg)
{
	var Ret = true;
	var NumStr="0123456789";
	var decUsed=false;
	var chr;
	for (i=0;i<field.value.length;++i)
	{
		chr=field.value.charAt(i);
		if (NumStr.indexOf(chr,0)==-1)
		{
			if ( (!decUsed) && chr==".")
			{
				decUsed=true;
			}
			else
			{
				Ret=false;
			}
		}
	}
	if(Ret)
	{
		if(decUsed&&(field.value.length-field.value.indexOf('.')<4))
		;
		else if(decUsed)
			Ret=false;
	}
	if (!Ret)
		doCritCode(field,crit,msg);
	return(Ret);
}
 // �ж��Ƿ��ǿ�true:��false:�ǿ�  **** is_null ****
function is_null(field,crit,msg)
{
//alert("test");
	Text=""+Trim(field.value);

	if(Text.length)
	{
		for(var i=0;i<Text.length;i++)
		if(Text.charAt(i)!=" ")
		break;
		if(i>=Text.length)
			Ret=true;
		else
			Ret=false;
	}
	else
		Ret=true;
	if (Ret)
		doCritCode(field,crit,msg);
	return(Ret);
}


//�ж��Ƿ�ΪСʱ��ture:��Сʱ��false:����Сʱ
function is_hour(field,crit,msg)
{
   if(!is_null(field,crit,msg+"����Ϊ��"))
      {

           if(is_int(field,crit,msg+"ӦΪ����"))
                     {


                          if(field.value<0)
   	{
   		doCritCode(field,crit,msg+"����0");
		Ret = false;
   	}
   	               else if(field.value>24)
   	{
   		doCritCode(field,crit,msg+"С��24");
   		Ret = false;
   	}

                          else
                              Ret=true;
                     }
           else
                 Ret=false;
      }
    else
       Ret=false;

       return(Ret);
}


//�ж��Ƿ�ΪСʱ��ture:��Сʱ��false:����Сʱ:�����
function is_nullhour(field,crit,msg)
{

        if(is_int(field,crit,msg+"ӦΪ����"))
        {


                          if(field.value<0)
   	{
   		doCritCode(field,crit,msg+"����0");
		Ret = false;
   	}
   	               else if(field.value>24)
   	{
   		doCritCode(field,crit,msg+"С��24");
   		Ret = false;
   	}

                          else
                              Ret=true;
        }
        else
                 Ret=false;


       return(Ret);
}


//�ж��Ƿ�Ϊ���ӣ�ture:�Ƿ��ӣ�false:���Ƿ���
function is_min(field,crit,msg)
{
   if(!is_null(field,crit,msg+"����Ϊ��"))
      {

           if(is_int(field,crit,msg+"ӦΪ����"))
                     {


                          if(field.value<0)
   	{
   		doCritCode(field,crit,msg+"����0");
		Ret = false;
   	}
   	               else if(field.value>59)
   	{
   		doCritCode(field,crit,msg+"С��59");
   		Ret = false;
   	}

                          else
                              Ret=true;
                     }
           else
                 Ret=false;
      }
    else
       Ret=false;

       return(Ret);
}


//�ж��Ƿ�Ϊ�룺ture:���룻false:�����룺�����
function is_sec(field,crit,msg)
{
	if(is_int(field,crit,msg+"ӦΪ����"))
        {


                          if(field.value<0)
   	{
   		doCritCode(field,crit,msg+"����0");
		Ret = false;
   	}
   	               else if(field.value>59)
   	{
   		doCritCode(field,crit,msg+"С��59");
   		Ret = false;
   	}

                          else
                              Ret=true;
        }
           else
                 Ret=false;


       return(Ret);
}



 // �ж��Ƿ��ǿ�true:��false:�ǿ�  **** is_Null_Checkbox ****
function is_Null_Checkbox(objname,crit,msg){

	  for(var l=0; l<objname.length; l++){
	  if(objname[l].checked){
		return false;
	  }}

  	objname[0].focus();
   	alert(msg);

   	return true;
}


 // �ж��Ƿ��ǿ�true:��false:�ǿ�  **** is_Null_Checkbox ****
function is_Null_Radio(objname,crit,msg)
{
	m_obj=eval('document.forms[0].'+objname);
        var size= m_obj.length;


	  for(i=0; i<size; i++)
	  {
	  	m_obj=eval('document.forms[0].'+objname+'['+i+']');
	  	if(m_obj.checked)
	  	{
			return false;
		}
	  }


	  m_obj=eval('document.forms[0].'+objname+'[0]');
  		m_obj.focus();
   		alert(msg);

   	return true;
}

//alert������ʾ
function doCritCode(field,crit,msg)
{
	if ( (-1!=crit) )
	{
		alert(msg)
		if (crit==1)
		{
			field.focus();  // focus does not work on certain netscape versions
			if (field.type == "select-one"){
			}else{
			field.select();}
		}
	}
}

// �ж��Ƿ�������true:��������false:�������� ����ʱ����
function is_int_negative(field,crit,msg){
	var Ret = true;
	var NumStr="0123456789";
	var chr;
	var i = 0;
	if(field.value.charAt(0)=="-")
		i = 1;
	for (;i<field.value.length;i++)
	{
		chr=field.value.charAt(i);
		if (NumStr.indexOf(chr,0)==-1)
		{
			Ret=false;
		}
	}
	if(field.value.charAt(0)=="-" && field.value.length==1)
		Ret = false;
	if (!Ret)
		doCritCode(field,crit,msg);
	return(Ret);
}


// �ж��Ƿ�������true:��������false:��������  **** !is_int ****
function is_int(field,crit,msg){
	var Ret = true;
	var NumStr="0123456789";
	var chr;

	for (i=0;i<field.value.length;i++)
	{
		chr=field.value.charAt(i);
		if (NumStr.indexOf(chr,0)==-1)
		{
			Ret=false;
		}
	}
	if (!Ret)
		doCritCode(field,crit,msg);
	return(Ret);
}
// �ж��Ƿ�������true:�����֣�false:��������  **** !is_int **** guoyy
function is_number(field,crit,msg){
	var Ret = true;
	var NumStr="0123456789.E";
	var chr;

	for (i=0;i<field.value.length;i++)
	{
		chr=field.value.charAt(i);
		if (NumStr.indexOf(chr,0)==-1)
		{
			Ret=false;
		}
	}
	if(field.value.indexOf("E")!=-1){
		if (field.value.indexOf("E")<=field.value.indexOf(".")){
			Ret=false;
		}
	}
	if (!Ret)
		doCritCode(field,crit,msg);
	return(Ret);
}
// �ж��Ƿ������� ����/��/�գ�   **** !is_date ****
function is_date(field,crit,msg){
	var Ret = false;
	var mark1;
	var mark2;
	var days

	if(field.value=="")
		return true;
	cd=new Date();

	if ( (mark1 = field.value.indexOf('/'))==-1)
		mark1=field.value.indexOf('/')
	if (mark1>-1)
	{
		if ( (mark2 = field.value.indexOf('/',mark1+1)) ==-1)
			mark2=field.value.indexOf('/',mark1+1);
		if ((mark2>-1)&&(mark2+1<field.value.length) )
		{
			year = new var_to_obj(field.value.substring(0,mark1));
			month = new var_to_obj(field.value.substring(mark1+1,mark2));
			day = new var_to_obj(field.value.substring(mark2+1,field.value.length));
			days = getDaysInMonth(month.value,year.value) + 1

			if (
				(is_greater(day,-1,0))&&(is_less(day,-1,days))&&
				(is_greater(month,-1,0))&&(is_less(month,-1,13))&&
				(is_greater(year,-1,1900))&&(is_less(year,-1,2500))
				)
				Ret=true;
		}
	}
	if (!Ret) doCritCode(field,crit,msg);

	return(Ret);
}

// �ж��Ƿ������ڣ���/�£�******** !is_YearMonth ****
function is_YearMonth(field,crit,msg){
	var Ret = false;
	var mark=Trim(field.value) + "/01" ;

	if(Trim(field.value)=="")
		return true;

	mark2 = new var_to_obj(mark);
	//alert(mark2.value);
	if (is_date(mark2,-1,"")){
		Ret = true ;
	}

	if (!Ret) doCritCode(field,crit,msg);

	return(Ret);
}


// �ж��Ƿ�������2
function is_date2(tmpy,tmpm,tmpd)
{
	year=new String (tmpy);
	month=new String (tmpm);
	day=new String (tmpd)
	if ((tmpy.length!=4) || (tmpm.length>2) || (tmpd.length>2))
	{
		return false;
	}
	if (!((1<=month) && (12>=month) && (31>=day) && (1<=day)) )
	{
		return false;
	}
	if (!( ((year % 4)==0) && ((year % 400)==0) ) && (month==2) && (day==29))
	{
		return false;
	}
	if ((month<=7) && ((month % 2)==0) && (day>=31))
	{
		return false;

	}
	if ((month>=8) && ((month % 2)==1) && (day>=31))
	{
		return false;
	}
	if ((month==2) && (day==30))
	{
		return false;
	}

	return true;
}


function doCrit(field,crit,msg)
{
	if ( (-1!=crit) )
	{
		alert(msg);
		if (crit==1)
		{
			field.focus();  // focus does not work on certain netscape versions
		}
	}
}
// �ж��Ƿ�����Ч���ݱ�ѡ��
function is_selected(field,crit,msg)
{
	value=""+field.options[field.selectedIndex].value;
	if(value=="0")
		Ret=false;
	else
		Ret=true;
	if (!Ret)
		doCrit(field,crit,msg);
	return(Ret);
}



// ����Ƿ����ַ�
// cCharacter������ֵ
function isCharacter( cCharacter )
{
	var sFormat = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz";

	if( sFormat.indexOf( cCharacter, 0 ) == -1 )
	{
		return false;
	}

	return true;
}

// ����Ƿ����������������Ƶ��ַ�
// cCharacter������ֵ
function isOtherNameCharacter( cCharacter )
{
	var sFormat = "_ -,";

	if( sFormat.indexOf( cCharacter, 0 ) == -1 )
	{
		return false;
	}

	return true;
}

// ����Ƿ��ǿ��������Ƶ��ַ�
// sValue������ֵ
function isNameCharacter(field,crit,msg)
{
	var	sValue = Trim(field.value);
	if( sValue.length == 0 )
	{
		doCritCode(field,crit,msg);
		return false;
	}
	if( sValue.length < 4 )
	{
		doCritCode(field,crit,msg);
		return false;
	}
	else if ( sValue.length > 50)
	{
		doCritCode(field,crit,msg);
		return false;
	}

	for( i = 0; i < sValue.length; i ++ )
	{
		var cCharacter = sValue.charAt( i );
		if( isDigital( cCharacter ) == false && isCharacter( cCharacter ) == false && isOtherNameCharacter( cCharacter ) == false )
		{
			doCritCode(field,crit,msg);
			return false;
		}
	}

	return true;
}

function isName(sValue)
{
	if( sValue.length == 0 )
	{
		return false;
	}
	if( sValue.length < 4 )
	{
		return false;
	}
	else if ( sValue.length > 50)
	{
		return false;
	}

	for( i = 0; i < sValue.length; i ++ )
	{
		var cCharacter = sValue.charAt( i );
		if( isDigital( cCharacter ) == false && isCharacter( cCharacter ) == false && isOtherNameCharacter( cCharacter ) == false )
		{
			return false;
		}
	}

	return true;
}
// ����Ƿ���Email
// sValue������ֵ���Ϸ���ʽΪa@b.c.d������ʽ
function is_Email( sValue )
{
	var iFirstIndex = 0;
	var iSecondIndex = sValue.indexOf( '@' );
	if( iSecondIndex == -1 )
	{
		return false;
	}
	var sTemp = sValue.substring( iFirstIndex, iSecondIndex );
	/*if( isName( sTemp ) == false )
	{
		return false;
	}*/
	iSecondIndex = sValue.indexOf( '.' );
	if( iSecondIndex == -1 || sValue.substring( sValue.length-1, sValue.length ) == '.' )
	{
		return false;
	}
	/*
	else if(  sTemp.length == sValue.length - 2 )	// The last two characters are '@' and '.'
	{
		return false;
	}
	else
	{
		var sTempValue = sValue;
		iSecondIndex = sValue.indexOf( '@' );
		while( iSecondIndex != -1 )
		{
			iFirstIndex = iSecondIndex + 1;
			sTempValue = sTempValue.substring( iFirstIndex, sTempValue.length );	// The right section of value
			iSecondIndex = sTempValue.indexOf( '.' );
			alert(sTempValue);
			//document.write( "sTempValue=" + sTempValue + "<br>" );
			sTemp = sTempValue.substring( 0, iSecondIndex );
			sTempValue = sTempValue.substring(iSecondIndex);
			alert(sTemp);
			//document.write( "sTemp=" + sTemp + "<br>" );
			if( isName( sTemp ) == false )
			{
				return false;
			}
		}

		if( isName( sTempValue ) == false )
		{
			return false;
		}
	}
	*/
	return true;
}

// ����Ƿ����ʱ�
// sValue������ֵ���Ϸ���ʽΪ��λ����
function is_zipcode( sValue )
{
	if( sValue == null )
	{
		return false;
	}

	if( sValue.length != 6 )
	{
		return false;
	}
	else
	{
		for( i = 0; i < 6; i ++ )
		{
			if( isDigital( sValue.charAt( i ) ) == false )
			{
				return false;
			}
		}
	}

	return true;
}

// ����Ƿ��������ַ���
// sValue������ֵ
function isDigitalString( sValue )
{
	if( sValue == null )
	{
		return false;
	}

	for( i = 0; i < sValue.length; i ++ )
	{
		if( isDigital( sValue.charAt( i ) ) == false )
		{
			return false;
		}
	}
	return true;
}


//Trim����ȥ��һ�ַ������ߵĿո�
function Trim(his)
{
   //�ҵ��ַ�����ʼλ��
   Pos_Start = -1;
   for(var i=0;i<his.length;i++)
   {
     if(his.charAt(i)!=" ")
      {
         Pos_Start = i;
         break;
      }
   }
   //�ҵ��ַ�������λ��
   Pos_End = -1;
   for(var i=his.length-1;i>=0;i--)
   {
     if(his.charAt(i)!=" ")
      {
         Pos_End = i;
         break;
      }
   }
   //���ص��ַ���
   Str_Return = ""
   if(Pos_Start!=-1 && Pos_End!=-1)
   {
		for(var i=Pos_Start;i<=Pos_End;i++)
		{
			   Str_Return = Str_Return + his.charAt(i);
		}
   }
   return Str_Return;
}

//IsDigital�����ж�һ���ַ����Ƿ�������(int or long)���
function isDigital(str)
{
  for(ilen=0;ilen<str.length;ilen++)
  {
    if(str.charAt(ilen) < '0' || str.charAt(ilen) > '9' )
    {
       return false;
    }
  }
  return true;
}
//IsFloat�����ж�һ���ַ����Ƿ�������(int or long or float)���
function is_float(str)
{
  flag_Dec = 0
  for(ilen=0;ilen<str.length;ilen++)
  {
    if(str.charAt(ilen) == '.')
    {
       flag_Dec++;
	   if(flag_Dec > 1)
          return false;
       else
          continue;
    }
    if(str.charAt(ilen) < '0' || str.charAt(ilen) > '9' )
    {
       return false;
    }
  }
  return true;
}
//IsTelephone�����ж�һ���ַ����Ƿ������ֻ�'-','*','()'���
function is_telephone(str)
{
  for(ilen=0;ilen<str.length;ilen++)
  {
    if(str.charAt(ilen) < '0' || str.charAt(ilen) > '9' )
    {
		if((str.charAt(ilen)!='-')&&(str.charAt(ilen)!='*')&&(str.charAt(ilen)!='(')&&(str.charAt(ilen)!=')'))
        return false;
    }
  }
  return true;
}
//���ڸ�ʽת��2/18/2000 ----2000-2-18
	function dateTransfer(strdate)
	{

		var pos1,pos2,end;
		var para1,para2,para3;
		var newdate;
		newdate="";
		//para1:��
		//para2:��
		//para3:��
		if(Trim(strdate)=="")
		{
			return(newdate);
		}
		end=strdate.length;
		pos1=strdate.indexOf("/",0);
		pos2=strdate.indexOf("/",pos1+1);
		para1=strdate.substring(0,pos1);
		para2=strdate.substring(pos1+1,pos2);
		para3=strdate.substring(pos2+1,end);
		newdate=para3+"-"+para1+"-"+para2;
		return(newdate);
	}
//ת������2000-10-20 ---->10/20/2000
function transferDate(str)
{
  var m=4;
  var strlen=str.length
  var n=strlen-1;
  if(Trim(str)=="")
  {
		return(str);
  }
  while (n>=strlen-2)
  {
   if(str.charAt(n)=="-")
    {
      break;
    }
   n=n-1
  }
  trimstr=str.substring(m+1,n)+"/"+ str.substring(n+1,strlen)+"/"+str.substring(0,m)
  return(trimstr);
}

//����Ƿ�������
function is_password( sValue )
{
	if( sValue == null )
	{
		return false;
	}

	for( i = 0; i < sValue.length; i ++ )
	{
		var cCharacter = sValue.charAt( i );
		if( isDigital( cCharacter ) == false && isCharacter( cCharacter ) == false )
		{
			return false;
		}
	}

	return true;
}

//�ж��Ƿ�Ϊ����ĺ���
//����˵����Year--���
//          ����ֵ:��������꣬����true�����򷵻�false.

function isLeapYear (Year) {
if (((Year % 4)==0) && ((Year % 100)!=0) || ((Year % 400)==0)) {
return (true);
} else { return (false); }
}

//ȡ��ÿ�������ĺ���
//����˵����month--��;year--��
//          ����ֵ��days--����
function getDaysInMonth(month,year)  {
var days;
if (month==1 || month==3 || month==5 || month==7 || month==8 || month==10 || month==12)  days=31;
else if (month==4 || month==6 || month==9 || month==11) days=30;
else if (month==2)  {
if (isLeapYear(year)) { days=29; }
else { days=28; }
}
return (days);
}
function writeRegard()
{
	var d = new Date();
	var hour = parseInt(d.getHours());
	if (hour>6 && hour<12)
		document.write("�����");
	else if (hour>12 && hour<18)
		document.write("�����");
	else
		document.write("���Ϻ�");
}


  function ratifyTime(str){

		  var Ret = false;
		  //Сʱ�ָ��

          var str1=str.value.substring(3,2);
		  //���ӷָ��
		  var str2=str.value.substring(6,5);
			var phour;
		  var pmin;
		  var psec;

          if(str1!=":"||str2!=":")
		  {
			alert("ʱ��ĸ�ʽӦΪ��01:01:01");
		    str.focus();
			return(false);
		  };


		  phour=parseInt(str.value.substring(2,0));//Сʱ
		  pmin=parseInt(str.value.substring(5,3));//����
		  psec=parseInt(str.value.substring(8,6));//��

		  if(phour>23||phour<0){
			alert("ʱ���Сʱλ���ܳ���23������С��0");
			str.focus();
			return(false);
		  };
          if(pmin>59||pmin<0){
			alert("ʱ��ķ���λ���ܳ���59������С��0");
			str.focus();
			return(false);
		  };
		  if(psec>59||psec<0){
			alert("ʱ�����λ���ܳ���59������С��0");
			str.focus();
			return(false);
		  };


	      return(true);
		}



//ʱ��Ƚϴ�С�������ȷ���0�����ڷ���1��С�ڷ���2
function compareTime(str1,str2){
		  var phour1;
		  var pmin1;
		  var psec1;

		  var phour2;
		  var pmin2;
		  var psec2;

		  phour1=parseInt(str1.value.substring(2,0));//Сʱ
		  pmin1=parseInt(str1.value.substring(5,3));//����
		  psec1=parseInt(str1.value.substring(8,6));//��


		  phour2=parseInt(str2.value.substring(2,0));//Сʱ
		  pmin2=parseInt(str2.value.substring(5,3));//����
		  psec2=parseInt(str2.value.substring(8,6));//��

		  if(phour1==phour2)
		  {
		    if(pmin1==pmin2)
			{
			  if(psec1==psec2)
			  {
			    return(0);
			  }
			  else
			  {
			    if(psec1>psec2)
				{
			      return(1);
                }
				else
				{
				  return(2);
				};
			  };
			}
			else
			{
			  if(pmin1>pmin2)
			  {
			    return(1);
			  }
			  else
			  {
			    return(2);
			  };
			};
		  }
		  else
		  {
		    if(phour1>phour2)
			{
			  return(1);
			}
			else
			{
			  return(2);
			};
		  };
		}


//��ӡҳ��
                function printwindow(reports,cssposition)
                {
                        var str = new String(reports);
                        var array = str.split(",");
                        var win2=window.open("","report","width=600,height=400,menubar=no,scrollbars,toolbar=0,left=0,top=0");
                        win2.document.writeln("<html><head>");
                        win2.document.writeln("<link rel='stylesheet' href='"+cssposition+"/css/style.css' type='text/css'>");
                        win2.document.writeln("<meta http-equiv='Content-Type' content='text/html; charset=gb2312'>");
                        win2.document.writeln("<title>����</title>");
                        win2.document.writeln("</head><body>");
                        win2.document.writeln("<p>");
                        win2.document.writeln("<table width=100% border=0 cellspacing=0 cellpadding=0 align=\"center\">");
                        win2.document.writeln("<tr><td>");
                        for(var i=0; i<array.length; i++){
                                win2.document.writeln(document.all(array[i]).outerHTML);                        }
                        win2.document.writeln("</td></tr>");
                        win2.document.writeln("<input type=\"button\" value=\"��ӡ����\" onclick=\"javascript:print()\">");
                        win2.document.writeln("&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;")
                        win2.document.writeln("<input type=\"button\" value=\"��  ��\" onclick=\"javascript:self.close()\">");
                        win2.document.writeln("<p>");
                        win2.document.writeln("</body></html>");
                        win2.document.close();
                        win2.focus();
}



//����ӵ�Validate script

//����Ƿ�ΪIP��ַ
//<INPUT type="text" id=txtIPAddress value="255.255.255.255">
//<input type=text name=result value="?">
//<INPUT type="button" value="IPvalue"
//onclick= "document.f1.result.value=verifyIP(document.f1.txtIPAddress.value)">

function verifyIP (IPvalue) {
errorString = "";
theName = "IPaddress";

var ipArray=IPvalue.split(".");

if (IPvalue == "0.0.0.0")
  errorString = errorString + theName + ': '+IPvalue+' is a special IP address and cannot be used here.(4)';
else if (IPvalue == "255.255.255.255")
  errorString = errorString + theName + ': '+IPvalue+' is a special IP address and cannot be used here.(5)';
if (!ipArray||!ipArray[0]||!ipArray[1]||!ipArray[2]||!ipArray[3]||ipArray.length>4)
  errorString = errorString + theName + ': '+IPvalue+' is not a valid IP address.(1)';
else {
  for (i = 0; i < 4; i++) {
    thisSegment = ipArray[i];
    if (thisSegment > 255) {
      errorString = errorString + theName + ': '+IPvalue+' is not a valid IP address.(2)';
      i = 4;
    }
    if ((i == 0) && (thisSegment > 255)) {
      errorString = errorString + theName + ': '+IPvalue+' is a special IP address and cannot be used here.(3)';
      i = 4;
    }
  }
}
extensionLength = 3;
if (errorString == "")
  alert ("That is a valid IP address.(0)");
else {
  alert (errorString);
  return false;
}
  return false;
}

//ȥ�����еĿո�,�������ڲ�����ͷβ
function ignoreSpaces(string) {
var temp = "";
string = '' + string;
splitstring = string.split(" ");
for(i = 0; i < splitstring.length; i++)
temp += splitstring[i];
return temp;
}

//���ֱ��д��:name count----Name Count
function changeCase(frmObj) {
var index;
var tmpStr;
var tmpChar;
var preString;
var postString;
var strlen;
tmpStr = frmObj.value.toLowerCase();
strLen = tmpStr.length;
if (strLen > 0)  {
for (index = 0; index < strLen; index++)  {
if (index == 0)  {
tmpChar = tmpStr.substring(0,1).toUpperCase();
postString = tmpStr.substring(1,strLen);
tmpStr = tmpChar + postString;
}
else {
tmpChar = tmpStr.substring(index, index+1);
if (tmpChar == " " && index < (strLen-1))  {
tmpChar = tmpStr.substring(index+1, index+2).toUpperCase();
preString = tmpStr.substring(0, index+1);
postString = tmpStr.substring(index+2,strLen);
tmpStr = preString + tmpChar + postString;
         }
      }
   }
}
frmObj.value = tmpStr;
}

//�Զ��ָ�λ��

function currencyFormat(fld, milSep, decSep, e) {
var sep = 0;
var key = '';
var i = j = 0;
var len = len2 = 0;
var strCheck = '0123456789';
var aux = aux2 = '';
var whichCode = (window.Event) ? e.which : e.keyCode;
if (whichCode == 13) return true;  // Enter
key = String.fromCharCode(whichCode);  // Get key value from key code
if (strCheck.indexOf(key) == -1) return false;  // Not a valid key
len = fld.value.length;
for(i = 0; i < len; i++)
if ((fld.value.charAt(i) != '0') && (fld.value.charAt(i) != decSep)) break;
aux = '';
for(; i < len; i++)
if (strCheck.indexOf(fld.value.charAt(i))!=-1) aux += fld.value.charAt(i);
aux += key;
len = aux.length;
if (len == 0) fld.value = '';
if (len == 1) fld.value = '0'+ decSep + '0' + aux;
if (len == 2) fld.value = '0'+ decSep + aux;
if (len > 2) {
aux2 = '';
for (j = 0, i = len - 3; i >= 0; i--) {
if (j == 3) {
aux2 += milSep;
j = 0;
}
aux2 += aux.charAt(i);
j++;
}
fld.value = '';
len2 = aux2.length;
for (i = len2 - 1; i >= 0; i--)
fld.value += aux2.charAt(i);
fld.value += decSep + aux.substr(len - 2, len);
}
return false;
}

//ת���ɻ��Ҹ�ʽ

function formatCurrency(num) {
num = num.toString().replace(/\$|\,/g,'');
if(isNaN(num))
num = "0";
sign = (num == (num = Math.abs(num)));
num = Math.floor(num*100+0.50000000001);
cents = num%100;
num = Math.floor(num/100).toString();
if(cents<10)
cents = "0" + cents;
for (var i = 0; i < Math.floor((num.length-(1+i))/3); i++)
num = num.substring(0,num.length-(4*i+3))+','+
num.substring(num.length-(4*i+3));
return (((sign)?'':'-') + '$' + num + '.' + cents);
}


//��button����resetǿ����������ָ�δ״̬
//Example: onclick="selectreset(document.form1, mytest)"
function selectreset(formname,selectareaname){
  with(formname){
     for(var j=0; j<selectareaname.options.length; j++){
       selectareaname.options[j].selected=false;
     }
  }
}

//�������ѡ
//Examp:onclick="selectedcheck(document.form1,mytest,'this is error')"
function selectedcheck(formname,selectareaname,msg){
  with(formname){
       var selectedflag=0;
       for(var l=0; l<selectareaname.options.length; l++){
          if(selectareaname.options[l].selected){
            selectedflag=1;
            break;
          }
       }
	   if(selectedflag){ return true; }else{ alert(msg);return false;}
    }
}

//�������ܱ�ѡ��
//Examp:onclick="selecteduncheck(document.form1,mytest,'this is error')"
function selecteduncheck(formname,selectareaname,msg){
  with(formname){
       var unselectedflag=1;
       for(var l=0; l<selectareaname.options.length; l++){
          if(selectareaname.options[l].selected){
            unselectedflag=0;
            break;
          }
       }
	   if(unselectedflag){ return true; }else{ alert(msg);return false;}
    }
}

//�ж������е��ĸ���ѡ��
//EX:onclick="selectedOptioName(document.form1,mytest,'��������')"
function selectedOptioName(formname, selectareaname, selectedoptionttext){
   with(formname){
       for(var l=0; l<selectareaname.options.length; l++){
          if(selectareaname.options[selectareaname.options.selectedIndex].text==selectedoptionttext){
            alert(" ��ѡ�����: "+selectedoptionttext);
			//������Լ�����Ҫ���õķ���
			return true;
          }
       }
   }
}

//�ж��ĸ���ѡťѡѡ��
//���ݴ����valuenameֵ�ж��ĸ�����ѡ��״̬
//Exam:onclick="whichradioselected(document.form1,testradio,'1')
function whichradioselected(formname, radioname,valuename){
   with(formname){
	  for(var l=0; l<radioname.length; l++)
      if(radioname[l].checked&&radioname[l].value==valuename){
	    alert("��ѡ�е��ǵ�"+(l+1) +"����ѡť!");
		//������Լ�����Ҫ���õķ���
		return true;
	  }
   }
}

//�жϸ�ѡť�еĵڼ����Ƿ�ѡ��
//���ݴ����valuenameֵ�ж��ĸ�����ѡ��״̬
//Exam:onclick="checkboxselected(document.form1,testcheckbox,'1')
function checkboxselected(formname, checkboxname,valuename){
   with(formname){
	  for(var l=0; l<checkboxname.length; l++)
	  if(checkboxname[l].checked&&checkboxname[l].value==valuename){
	     alert("��ѡ�е��ǵ�"+(l+1) +"����ѡť!");
		//������Լ�����Ҫ���õķ���
		return true;
	  }
   }
}

//�жϸ�ѡ�����ܹ�ֻ����minicount��maxcountͬʱ���ڱ�ѡ��
//Ex: onclick="checkboxselectedcount(document.form1,testcheckbox,1,3)"
function checkboxselectedcount(formname, checkboxname,minicount,maxcount){
   with(formname){
	  var selectedcount=0;
	  for(var l=0; l<checkboxname.length; l++)
	  if(checkboxname[l].checked){
	    selectedcount++;
	  }
	  if(selectedcount>=minicount&&selectedcount<=maxcount){
	     return true;
	  }else{
	     alert("�Բ���!���ѡ��Ҫ���� "+minicount+" �� "+maxcount+" ��ѡ��! ");
	  }
   }
}

//��������ֻ����������:
//Example: <input type=text name=txtPostalCode
//onKeypress="if (event.keyCode < 45 || event.keyCode > 57) event.returnValue = false;">

//�������̲�����������,ֻ��������������ַ�:
//Example: <input type=text name=txtPostalCode
//onKeypress="if (event.keyCode > 45 && event.keyCode< 57) event.returnValue = false;">

//�жϴ��ڷǷ��ַ���: ������:<, >,', " , &
function stringfilter(str){
    for(var i=0; i<str.value.length; i++){
	   if(str.value.charAt(i)=='<'||str.value.charAt(i)=='>'||str.value.charAt(i)=='&'||str.value.charAt(i)=="'"||str.value.charAt(i)=='"'){
	      alert("���ڷǷ��ַ�:<, >,'&");
		  return false;
	   }else{
	      return true;
	   }
	}
}

//ͨ�������������ܾ�����ָ�����ַ�
//��������!@#$%^&*!@#$%^&
//<input type="text" name=comments
//onKeypress="if ((event.keyCode > 32 && event.keyCode < 48) || (event.keyCode > 57 && event.keyCode < 65) || (event.keyCode > 90 && event.keyCode < 97)) event.returnValue = false;">


//�ж�ʱ��������ʱ��Ϊ������תΪСʱ��ʽ  20040309 guoyy
//
function intToHH(field,crit,msg){
	var Ret = true;
	var NumStr="0123456789:";
	var chr;
    var para=field.value.indexOf(":",0);

	for (i=0;i<field.value.length;i++)
	{
		chr=field.value.charAt(i);

		if (NumStr.indexOf(chr,0)==-1)
		{
			Ret=false;
		}
	}



	if(field.value.indexOf(":")<0){
		field.value = field.value + ":00";
	}
	if(Ret){
	alert(field.value.substring(para+1));
		if(parseInt(field.value.substring(para+1)>59))
										Ret=false;
										//(parseInt(field.value.substring(0,para))>24) || (
	}

	if (!Ret)
		doCritCode(field,crit,msg);
	return(Ret);

}




//����������Ԫ��

function myListAdd(list1,list2)
{

	m_obj1=eval('document.forms[0].'+list1);
	m_obj2=eval('document.forms[0].'+list2);


	if(m_obj1.selectedIndex<0){
		alert(" û�б�ѡ���");
		return;
	}


	var size = m_obj2.length;
	for(i=0;i<size;i++){
		if(m_obj1.options[m_obj1.selectedIndex].value==m_obj2.options[i].value){
			alert(m_obj1.options[m_obj1.selectedIndex].text + "�Ѿ���ѡ�У�");
			return ;
		}
	}

	m_obj2.length = size+1;
	m_obj2.options[size].text=m_obj1.options[m_obj1.selectedIndex].text;
	m_obj2.options[size].value=m_obj1.options[m_obj1.selectedIndex].value;

}

//ɾ��������ѡ�е�Ԫ��
function myListDel(list)
{
	m_obj=eval('document.forms[0].'+list);
		  for (i=m_obj.length-1; i>=0; i--)
		  {
			  if (m_obj.options[i].selected)
			  {
			  	m_obj.options[i] = null;
			  }
		  }
}



//�ж��ַ���ֻ����Ӣ�Ļ����֣��������ֵ��������ֽ�
function myStrLen(field,crit,msg)
{
	str=field.value;
	var len;
	var i;
	len = 0;
	var Ret = false;
	for (i=0;i<str.length;i++)
	{
		if (str.charCodeAt(i)>255)
                {
                	len+=2;
			doCritCode(field,crit,msg);
			return (Ret);
                }else{
			len++;
                }
	}
        Ret = true;
	return Ret;
}


/********************************** date ******************************************/
/**
*У���ַ����Ƿ�Ϊ������
*����ֵ��
*���Ϊ�գ�����У��ͨ����           ����true
*����ִ�Ϊ�����ͣ�У��ͨ����       ����true
*������ڲ��Ϸ���                   ����false    �ο���ʾ��Ϣ���������ʱ�䲻�Ϸ�����yyyy-MM-dd��
*/
function checkIsValidDate(str)
{
    //���Ϊ�գ���ͨ��У��
    if(str == "")
        return true;
    var pattern = /^((\\d{4})|(\\d{2}))-(\\d{1,2})-(\\d{1,2})$/g;
    if(!pattern.test(str))
        return false;
    var arrDate = str.split("-");
    if(parseInt(arrDate[0],10) < 100)
        arrDate[0] = 2000 + parseInt(arrDate[0],10) + "";
    var date =  new Date(arrDate[0],(parseInt(arrDate[1],10) -1)+"",arrDate[2]);
    if(date.getYear() == arrDate[0]
       && date.getMonth() == (parseInt(arrDate[1],10) -1)+""
       && date.getDate() == arrDate[2])
        return true;
    else
        return false;
}