FLAG=-O2
INC=-ILigCommon -ISQUIRREL3/include
SQLIB=-LSQUIRREL3/lib

webserver : SQUIRREL3/lib/libsquirrel.a ScriptSupport.o SqSupportUtil.o sqshell_util.o sqshell_base.o sqrdbg.o sqdbgserver.o win_gdbm.o lig_httpd.o LigSP.o parser.o sdl_tcpstream.o unicode.o sqlite3.o
	g++ ${FLAG} ${INC} -o webserver ScriptSupport.o SqSupportUtil.o sqshell_util.o sqshell_base.o sqrdbg.o sqdbgserver.o win_gdbm.o lig_httpd.o LigSP.o parser.o sdl_tcpstream.o unicode.o sqlite3.o -lpthread ${SQLIB} -lsquirrel -lsqstdlib -ldl
SQUIRREL3/lib/libsquirrel.a : SQUIRREL3/Makefile
	cd SQUIRREL3;make
lig_httpd.o : LigCommon/lig_httpd.cpp
	g++ ${FLAG} ${INC} -c LigCommon/lig_httpd.cpp
LigSP.o : LigCommon/LigSP.cpp
	g++ ${FLAG} ${INC} -c LigCommon/LigSP.cpp
parser.o : LigCommon/parser.cpp
	g++ ${FLAG} ${INC} -c LigCommon/parser.cpp
sdl_tcpstream.o : LigCommon/sdl_tcpstream.cpp
	g++ ${FLAG} ${INC} -c LigCommon/sdl_tcpstream.cpp
unicode.o : LigCommon/unicode.cpp
	g++ ${FLAG} ${INC} -c LigCommon/unicode.cpp
sqlite3.o : LigCommon/sqlite3.c
	gcc ${FLAG} ${INC} -c LigCommon/sqlite3.c
ScriptSupport.o : LigCommon/ScriptSupport.cpp
	g++ ${FLAG} ${INC} -c LigCommon/ScriptSupport.cpp
SqSupportUtil.o : LigCommon/SqSupportUtil.cpp
	g++ ${FLAG} ${INC} -c LigCommon/SqSupportUtil.cpp
win_gdbm.o : LigCommon/win_gdbm.cpp
	g++ ${FLAG} ${INC} -c LigCommon/win_gdbm.cpp
sqshell_util.o : LigCommon/sqshell_util.cpp
	g++ ${FLAG} ${INC} -c LigCommon/sqshell_util.cpp
sqshell_base.o : LigCommon/sqshell_base.cpp
	g++ ${FLAG} ${INC} -c LigCommon/sqshell_base.cpp
sqrdbg.o : LigCommon/sqrdbg.cpp
	g++ ${FLAG} ${INC} -c LigCommon/sqrdbg.cpp
sqdbgserver.o : LigCommon/sqdbgserver.cpp
	g++ ${FLAG} ${INC} -c LigCommon/sqdbgserver.cpp
