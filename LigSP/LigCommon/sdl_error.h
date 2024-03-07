#ifndef _sdl_error_head_

#define _sdl_error_head_
#include <iostream>

using namespace std;

enum err_type
{
NOTICE=0,
COMM_ERR=1,
BIG_ERR=2,
VERY_BIG_ERR=3
};
class sdl_exception
{
public:
	sdl_exception (const char* s,err_type level=NOTICE) 
	{
		cout<<s<<endl;
		m_s=s;
		m_level=level;
	};
	err_type get_error_type(){return m_level;};
	~sdl_exception (){};
	const char* description() {return m_s;}
private:
	const char *m_s;
	err_type m_level;
};

#endif
