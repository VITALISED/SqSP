json_parse = function () {

// This is a function that can parse a JSON text, producing a JavaScript
// data structure. It is a simple, recursive descent parser. It does not use
// eval or regular expressions, so it can be used as a model for implementing
// a JSON parser in other languages.

// We are defining the function inside of another function to avoid creating
// global variables.

    var at,     // The index of the current character
        ch,     // The current character
        escapee = {
            '"':  '"',
            '\\': '\\',
            '/':  '/',
            b:    '\b',
            f:    '\f',
            n:    '\n',
            r:    '\r',
            t:    '\t'
        },
        text,

        error = function (m) {

// Call error when something is wrong.

            throw {
                name:    'SyntaxError',
                message: m,
                at:      at,
                text:    text
            };
        },

        next = function (c) {

// If a c parameter is provided, verify that it matches the current character.

            if (c && c !== ch) {
                error("Expected '" + c + "' instead of '" + ch + "'");
            }

// Get the next character. When there are no more characters,
// return the empty string.

            ch = text.charAt(at);
            at += 1;
            return ch;
        },

        number = function () {

// Parse a number value.

            var number,
                string = '';

            if (ch === '-') {
                string = '-';
                next('-');
            }
            while (ch >= '0' && ch <= '9') {
                string += ch;
                next();
            }
            if (ch === '.') {
                string += '.';
                while (next() && ch >= '0' && ch <= '9') {
                    string += ch;
                }
            }
            if (ch === 'e' || ch === 'E') {
                string += ch;
                next();
                if (ch === '-' || ch === '+') {
                    string += ch;
                    next();
                }
                while (ch >= '0' && ch <= '9') {
                    string += ch;
                    next();
                }
            }
            number = +string;
            if (isNaN(number)) {
                error("Bad number");
            } else {
                return number;
            }
        },

        string = function () {
				
// Parse a string value.
			
            var hex,
                i,
                string = '',
                uffff;

// When parsing for string values, we must look for " and \ characters.

            if (ch === '"') {
                while (next()) {
                    if (ch === '"') {
                        next();
                        return string;
                    } else if (ch === '\\') {
                        next();
                        if (ch === 'u') {
                            uffff = 0;
                            for (i = 0; i < 4; i += 1) {
                                hex = parseInt(next(), 16);
                                if (!isFinite(hex)) {
                                    break;
                                }
                                uffff = uffff * 16 + hex;
                            }
                            string += String.fromCharCode(uffff);
                        } else if (typeof escapee[ch] === 'string') {
                            string += escapee[ch];
                        } else {
                            break;
                        }
                    } else {
                        string += ch;
                    }
                }
            }
            error("Bad string");
        },

        white = function () {

// Skip whitespace.

            while (ch && ch <= ' ') {
                next();
            }
        },

        word = function () {

// true, false, or null.

            switch (ch) {
            case 't':
                next('t');
                next('r');
                next('u');
                next('e');
                return true;
            case 'f':
                next('f');
                next('a');
                next('l');
                next('s');
                next('e');
                return false;
            case 'n':
                next('n');
                next('u');
                next('l');
                next('l');
                return null;
            }
            error("Unexpected '" + ch + "'");
        },

        value,  // Place holder for the value function.

        array = function () {

// Parse an array value.

            var array = [];

            if (ch === '[') {
                next('[');
                white();
                if (ch === ']') {
                    next(']');
                    return array;   // empty array
                }
                while (ch) {
                    array.push(value());
                    white();
                    if (ch === ']') {
                        next(']');
                        return array;
                    }
                    next(',');
                    white();
                }
            }
            error("Bad array");
        },

        object = function () {

// Parse an object value.

            var key,
                object = {};

            if (ch === '{') {
                next('{');
                white();
                if (ch === '}') {
                    next('}');
                    return object;   // empty object
                }
                while (ch) {
                    key = string();
                    white();
                    next(':');
                    object[key] = value();
                    white();
                    if (ch === '}') {
                        next('}');
                        return object;
                    }
                    next(',');
                    white();
                }
            }
            error("Bad object");
        };

    value = function () {

// Parse a JSON value. It could be an object, an array, a string, a number,
// or a word.

        white();
        switch (ch) {
        case '{':
            return object();
        case '[':
            return array();
        case '"':
            return string();
        case '-':
            return number();
        default:
            return ch >= '0' && ch <= '9' ? number() : word();
        }
    };

// Return the json_parse function. It will have access to all of the above
// functions and variables.

    return function (source, reviver) {
        var result;

        text = source;
        at = 0;
        ch = ' ';
        result = value();
        white();
        if (ch) {
            error("Syntax error");
        }

// If there is a reviver function, we recursively walk the new structure,
// passing each name/value pair to the reviver function for possible
// transformation, starting with a temporary root object that holds the result
// in an empty key. If there is not a reviver function, we simply return the
// result.

        return typeof reviver === 'function' ? function walk(holder, key) {
            var k, v, value = holder[key];
            if (value && typeof value === 'object') {
                for (k in value) {
                    if (Object.hasOwnProperty.call(value, k)) {
                        v = walk(value, k);
                        if (v !== undefined) {
                            value[k] = v;
                        } else {
                            delete value[k];
                        }
                    }
                }
            }
            return reviver.call(holder, key, value);
        }({'': result}, '') : result;
    };
}();

/*
Script: JSON.js JSONRequest.js JSTONE.js

Compatibility:
	FireFox - Version 1, 1.5, 2 and 3 (FireFox uses secure code evaluation)
	Internet Explorer - Version 5, 5.5, 6 and 7
	Opera - 8 and 9 (probably 7 too)
	Safari - Version 2 (probably 1 too)
	Konqueror - Version 3 or greater

Author:
	Andrea Giammarchi, <http://www.3site.eu>

License:
	>Copyright (C) 2007 Andrea Giammarchi - www.3site.eu
	>	
	>Permission is hereby granted, free of charge,
	>to any person obtaining a copy of this software and associated
	>documentation files (the "Software"),
	>to deal in the Software without restriction,
	>including without limitation the rights to use, copy, modify, merge,
	>publish, distribute, sublicense, and/or sell copies of the Software,
	>and to permit persons to whom the Software is furnished to do so,
	>subject to the following conditions:
	>
	>The above copyright notice and this permission notice shall be included
	>in all copies or substantial portions of the Software.
	>
	>THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED,
	>INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
	>FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
	>IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM,
	>DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE,
	>ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE
	>OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

	//-----------------------------------------------------------------------------------
	这段代码来源与Andrea Giammarchi，我将三个js代码合并成了一段，修改了原作者的的一个错误
	使用这段代码至少可以在Firefox和IE上支持有限的JSonrequest（目前不能跨源进行Jsonrequest,
	而标准的Jsonrequest是允许跨源请求的)

	JSON及Jsonrequest标准参见 http://www.json.org 及 http://www.json.org/JSONRequest.html
	或者rfc4627

	如果在firefox中安装了Jsonrequest插件，这段程序会被优先执行，而不使用插件

																   李 钢  TEDC CAAC

																	     2008.08.09

*/

JSONError = function(errstr)
{
	this.errRec = errstr;
};

JSONRequestError = function(errstr)
{
	this.errRec = errstr;
};

JSON = new function(){
	
	this.encode = function(){
		var	self = arguments.length ? arguments[0] : this,
			result, tmp;
		if(self === null)
			result = "null";
		else if(self !== undefined && (tmp = $[typeof self](self))) {
			switch(tmp){
				case	Array:
					result = [];
					for(var	i = 0, j = 0, k = self.length; j < k; j++) {
						if(self[j] !== undefined && (tmp = JSON.encode(self[j])))
							result[i++] = tmp;
					};
					result = "[".concat(result.join(","), "]");
					break;
				case	Boolean:
					result = String(self);
					break;
				case	Date:
					result = '"'.concat(self.getFullYear(), '-', d(self.getMonth() + 1), '-', d(self.getDate()), 'T', d(self.getHours()), ':', d(self.getMinutes()), ':', d(self.getSeconds()), '"');
					break;
				case	Function:
					break;
				case	Number:
					result = isFinite(self) ? String(self) : "null";
					break;
				case	String:
					result = '"'.concat(self.replace(rs, s).replace(ru, u), '"');
					break;
				default:
					var	i = 0, key;
					result = [];
					for(key in self) {
						if(self[key] !== undefined && (tmp = JSON.encode(self[key])))
							result[i++] = '"'.concat(key.replace(rs, s).replace(ru, u), '":', tmp);
					};
					result = "{".concat(result.join(","), "}");
					break;
			}
		};
		return result;
	};
	
	this.toDate = function(){
		var	self = arguments.length ? arguments[0] : this,
			result;
		if(rd.test(self)){
			result = new Date;
			result.setHours(i(self, 11, 2));
			result.setMinutes(i(self, 14, 2));
			result.setSeconds(i(self, 17, 2));
			result.setMonth(i(self, 5, 2) - 1);
			result.setDate(i(self, 8, 2));
			result.setFullYear(i(self, 0, 4));
		}
		else if(rt.test(self))
			result = new Date(self * 1000);
		return result;
	};
	
	/* Section: Properties - Private */
	var	c = {"\b":"b","\t":"t","\n":"n","\f":"f","\r":"r",'"':'"',"\\":"\\","/":"/"},
		d = function(n){return n<10?"0".concat(n):n},
		e = function(c,f,e){e=eval;delete eval;if(typeof eval==="undefined")eval=e;f=eval(""+c);eval=e;return f},
		i = function(e,p,l){return 1*e.substr(p,l)},
		p = ["","000","00","0",""],
		rc = null,
		rd = /^[0-9]{4}\-[0-9]{2}\-[0-9]{2}T[0-9]{2}:[0-9]{2}:[0-9]{2}$/,
		rs = /(\x5c|\x2F|\x22|[\x0c-\x0d]|[\x08-\x0a])/g,
		rt = /^([0-9]+|[0-9]+[,\.][0-9]{1,3})$/,
		ru = /([\x00-\x07]|\x0b|[\x0e-\x1f])/g,
		s = function(i,d){return "\\".concat(c[d])},
		u = function(i,d){
			var	n=d.charCodeAt(0).toString(16);
			return "\\u".concat(p[n.length],n)
		},
		v = function(k,v){return $[typeof result](result)!==Function&&(v.hasOwnProperty?v.hasOwnProperty(k):v.constructor.prototype[k]!==v[k])},
		$ = {
			"boolean":function(){return Boolean},
			"function":function(){return Function},
			"number":function(){return Number},
			"object":function(o){return o instanceof o.constructor?o.constructor:null},
			"string":function(){return String},
			"undefined":function(){return null}
		},
		$$ = function(m){
			function $(c,t){t=c[m];delete c[m];try{e(c)}catch(z){c[m]=t;return 1}};
			return $(Array)&&$(Object)
		};
	try{rc=new RegExp('^("(\\\\.|[^"\\\\\\n\\r])*?"|[,:{}\\[\\]0-9.\\-+Eaeflnr-u \\n\\r\\t])+?$')}
	catch(z){rc=/^(true|false|null|\[.*\]|\{.*\}|".*"|\d+|\d+\.\d+)$/}
};

 JSONRequest = new function(){

	this.cancel = function(i){
		if(i-- > 0 && i < queue.length && queue[i].timeout)
			cancel(i, new JSONRequestError("cancelled"));
		changeDelay(10);
	};
	
	this.get = function(url, done){
		var	i = queue.length;
		method = "get";
		try{
			this.post(url, {}, done);
			queue[i].data = null;
			return i+1;
		} catch(e) {
			throw e
		}
	};
	
	this.post = function(url, send, done, timeout){
		var	i = queue.length;
		try {
			prepare(url, send, done, timeout || 10000);
			return i+1;
		} catch(e) {
			method = "post";
			queue[i] = {"timeout":0};
			changeDelay(500, 512);
			throw e;
		}
	};
		
	function cancel(l, JSONRequestError){
		clearTimeout(queue[l].timeout);
		queue[l].xhr.onreadystatechange = function(){};
		queue[l].xhr.abort();
		if(JSONRequestError)
			queue[l].done(l + 1, null, JSONRequestError);
		queue[l] = {"timeout":0};
	};
	
	
	function changeDelay(a, b){
		delay += (a + Math.floor(Math.random() * b || 0));
		if(delay < 0)
			delay = 0;
	};
	
	function prepare(url, send, done, timeout){
		var	i = queue.length,
			uri = url.indexOf(document.domain);
		if(uri > 8 || (uri === -1 && re.test(uri)))
			throw new JSONRequestError("bad URL");
		else if(parseInt(timeout) !== timeout || timeout < 0)
			throw new JSONRequestError("bad timeout");
		else if(done.constructor !== Function || done.length !== 3)
			throw new JSONRequestError("bad function");
		else {
			try {
				queue[i] = {"data":JSON.encode(send), "done":done, "method":method, "send":true, "timeout":timeout, "url":url, "xhr":xhr()};
				if(queue[i].data === undefined || (send.constructor !== Array && send.constructor !== Object))
					throw new JSONRequestError;
			}
			catch(e) {
				throw new JSONRequestError("bad data")
			}
		};
	};
		
	function request(){
		var	request = queue[l],
			data = null,
			xhr;
		if(delay < 0)
			delay = 0;
		if(!delay && request && request.timeout && request.send) {
			request.send = false;
			request.timeout = setTimeout(function(){
				changeDelay(500, 512);
				cancel(l++, new JSONRequestError("no response"));
			}, request.timeout);
			xhr = request.xhr;
			xhr.open(request.method, request.url, true);
			xhr.setRequestHeader("Content-Type", "application/jsonrequest");
			if(request.method === "post") {
				//data = "JSONRequest=".concat(encodeURIComponent(request.data));
				data=request.data;
				xhr.setRequestHeader("Content-Length", data.length);
				xhr.setRequestHeader("Content-Encoding", "identity");
			};
			xhr.onreadystatechange = function(){
				if(xhr.readyState === 4) {
					if(xhr.status === 200) {
						clearTimeout(request.timeout);
						try {
							if(xhr.getResponseHeader("Content-Type") === "application/jsonrequest") {
								//alert(xhr.responseText);
								try{
								request.done(l+1, json_parse(xhr.responseText));
								}catch(err)
								{
									alert(err.message);
								}
								//request.done(l+1, JSON.decode(xhr.responseText));
								cancel(l);
								changeDelay(-10);
							}
							else
								throw new JSONRequestError;
						}
						catch(e) {
							changeDelay(500, 512);
							cancel(l, new JSONRequestError("bad response"));
						}
					}
					else {
						changeDelay(500, 512);
						cancel(l, new JSONRequestError("not ok"));
					};
					l++;
				}
			};
			xhr.send(data);
		}
		else if(request && !request.timeout && l < queue.length - 1)
			l++;
		if(delay)
			delay -= 10;
	};
	
	function xhr(){
		return window.XMLHttpRequest ? new XMLHttpRequest : new ActiveXObject("Microsoft.XMLHTTP");
	};
	
	var	delay = 0,
		l = 0,
		method = "post",
		queue = [],
		re = /^(\s*)http/;
	setInterval(request, 10);
};

function JSTONE(JSON, free, clear){
	this.clear = function(){
		for(var	key = k.split("."), i = 0, l = key.length - 1, t = get(); i < l; i++) {
			if(!t[key[i]])t[key[i]] = {};
			t = t[key[i]];
		};	t[key[i]] = {};
		sync();
	};
	this.read = function(key){
		var	o = find(key);
		return	o.o[o.key]
	};
	this.write = function(key, value){
		var	o = find(key);
			o.o[o.key] = value;
		sync();
	};

	var	find = function(key){
			for(var i = 0, l = (key = k.concat(".", key).split(".")).length - 1, t = get(); i < l; i++) {
				if(!t[key[i]])t[key[i]] = {};
				t = t[key[i]];
			};	return {o:t, key:key.pop()}
		},
		get = function(){return	o = window.name ? JSON.decode(window.name) : o},
		sync = function(){window.name = JSON.encode(o)},
		unload = clear ? function(){window.name = ""} : null,
		k = !free ? location.href.split("/").slice(2,3)[0].replace(/:[0-9]+/, "") : "o.o",
		o = {};
	get();
	if(unload)
		window.addEventListener ? window.addEventListener("unload", unload, false) : window.attachEvent("on".concat("unload"), unload);
};

/*
返回值的基本编码规则是
{
	session:"session_str"
	action:[
	//这里并排对象即可

	]
}
*/

var server = new function() 
{
	this.request_number=0;
	this.session_str="";
	
	this.agent=function(requestNumber,value,exception)
	{
		if(value==null) return;
		if(value.session!=null) 
			this.session_str=value.session; //记录下传的_session值
		if(value.action!=null) //服务器端给出执行要求
		{
			for(var i=0;i<value.action.length;++i)
			{
				var obj=value.action[i];
				var element;
				if(obj["badSession"]!=null) //{id:"object_id",innerHTML:"html text"}
				{
					alert("Session failed,please relogin..");
				}

				if(obj["URL"]!=null) //{id:"object_id",URL:"new url"}
				{
					document.location=obj.URL
				}

				if(obj["innerHTML"]!=null) //{id:"object_id",innerHTML:"html text"}
				{
					//alert(obj.innerHTML);
					element=document.getElementById(obj.id);
					if(element!=null)
						element.innerHTML=obj.innerHTML;
				}
				if(obj["value"]!=null)  //{id:"object_id",value:"value text"}
				{
					element=document.getElementById(obj.id);
					if(element!=null)
					{
						if(element.options!=null) //ie not support select value
						{
							for(var ei=0;ei<element.options.length;++ei)
								if(element.options[ei].text==obj.value)
									element.selectedIndex=ei;
						}else element.value=obj.value;
					}
				}
				if(obj["alert"]!=null)  //{id:"alert",alert:"value text"}
				{
					alert(obj.alert);
				}
				if(obj["list"]!=null) //{id:"object_id",list:[["id1","text1],["id2",text2],...]}
				{
					element=document.getElementById(obj.id);
					if(element!=null)
					{
						var j,l;
						l=element.length;
						for(j=0;j<l;++j) element.remove(0);  
						for(j=0;j<obj.list.length;++j)
							element.options.add(new Option(obj.list[j][1],obj.list[j][0]));
					}
				}
				if(obj["disabled"]!=null)  //{id:"object_id",disabled:"true/false"}
				{
					
					element=document.getElementById(obj.id);
					if(element!=null)
					{
						if(obj.disabled=="true")
							element.disabled=true;
						else
							element.disabled=false;
					}
				}
				
				if(obj["readOnly"]!=null)  //{id:"object_id",readOnly:"true/false"}
				{
					
					element=document.getElementById(obj.id);
					if(element!=null)
					{
						if(obj.readOnly=="true")
							element.readOnly=true;
						else
							element.readOnly=false;
					}
				}
									
				//在这里随便丰富的Ajax特性吧！
			}
		}
	}
	//---------------------------------------------------------------
	this.call=function(func_name,object)
	{
		var jo={session:"",server_func:"",client:""};

		if(this.session_str=="")
			jo.session="request session..";
		else
			jo.session=this.session_str;
		jo.server_func=func_name;
		
		tp=typeof(object);
		if(tp=="string")
		{
			jo.client={value:""};
			jo.client.value=object;
		}else jo.client=object;
		
		
		try
		{
			JSONRequest.post(document.location+"",jo,server.agent,60000);
		}
		catch(err)
		{
			alert(err.errRec);
		}
	}
	
	this.raise=function(func_name)
	{
		var jo={session:"",server_func:"",client:""};

		if(this.session_str=="")
			jo.session="request session..";
		else
			jo.session=this.session_str;
		jo.server_func=func_name;
		jo.client={value:""};
		
		try
		{
			JSONRequest.post(document.location+"",jo,server.agent,60000);
		}
		catch(err)
		{
			alert(err.errRec);
		}
	}

	this.session=function(str)
	{
		this.session_str=str;
	}
}
//-----------------------------------------------------------------------------
