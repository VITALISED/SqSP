// Squirrel Shell
// Copyright (c) 2006-2009, Constantin "Dinosaur" Makshin
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation; either version 3 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program. If not, see <http://www.gnu.org/licenses/>.

#include "common.h"
#include <string.h>

#if defined(SHELL_PLATFORM_WINDOWS)
typedef HANDLE SysHandle;
#else
#	define  INVALID_HANDLE_VALUE (-1)
#	include <sys/stat.h>
#	include <sys/types.h>
#	include <sys/wait.h>
#	include <dirent.h>
#	include <grp.h>
#	include <pwd.h>
#	include <signal.h>
#	include <time.h>
#	include <utime.h>

typedef	int SysHandle;
#endif

#define MAX_ARGS		130 // Maximum number of command line arguments passed to child process
#define MAX_ENVS		129 // Maximum number of environment variables passed to child process
#define FILETIME_CREATE 0
#define FILETIME_ACCESS 1
#define FILETIME_MODIFY 2
#define FILETIME_CHANGE 3

extern SQInteger TimeToInt (unsigned, unsigned, unsigned, unsigned, unsigned, unsigned);
extern void		 IntToTime (SQInteger, unsigned&, unsigned&, unsigned&, unsigned&, unsigned&, unsigned&);

#if !defined(SHELL_PLATFORM_WINDOWS)
// Copy data from one file to another.
static void CopyFile (const SQChar* from, const SQChar* to, SQBool doNotOverwrite)
{
	if (doNotOverwrite && !access(to, F_OK))
		return;

	FILE *src  = fopen(from, "rb"),
		 *dest = fopen(to, "wb");
	if (src && dest)
	{
		unsigned char buf[4096];
		for (;;)
		{
			size_t n = fread(buf, 1, sizeof(buf), src);
			if (!n)
				break;
			fwrite(buf, 1, n, dest);
		}
	}
	if (dest)
		fclose(dest);
	if (src)
		fclose(src);
}
#endif

// Remove directory with all its contents
static void RmDirRecursively (const SQChar* path)
{
	SQChar tmp[MAX_PATH];

#if defined(SHELL_PLATFORM_WINDOWS)
	// Append wildcard to the end of path
#if (_MSC_VER >= 1400)
	_snprintf_s(tmp, MAX_PATH, MAX_PATH - 1, "%s\\*", path);
#else
	_snprintf(tmp, MAX_PATH - 1, "%s\\*", path);
#endif

	// List files
	WIN32_FIND_DATA fd;
	HANDLE			fh = FindFirstFile(tmp, &fd);
	if (fh != INVALID_HANDLE_VALUE)
	{
		// First 2 entries will be "." and ".." - skip them
		FindNextFile(fh, &fd);
		while (FindNextFile(fh, &fd))
		{
#if (_MSC_VER >= 1400)
			_snprintf_s(tmp, MAX_PATH, MAX_PATH - 1, "%s/%s", path, fd.cFileName);
#else
			_snprintf(tmp, MAX_PATH - 1, "%s/%s", path, fd.cFileName);
#endif

			DWORD attrs = GetFileAttributes(tmp);
			if (attrs == INVALID_FILE_ATTRIBUTES)
				continue;

			if (attrs & FILE_ATTRIBUTE_DIRECTORY)
				RmDirRecursively(tmp);
			else
				DeleteFile(tmp);
		}
		FindClose(fh);
	}
	RemoveDirectory(path);
#else // SHELL_PLATFORM_WINDOWS
	DIR* dir = opendir(path);
	if (dir)
	{
		// As in Windows, first 2 entries are "." and ".."
		readdir(dir);
		readdir(dir);

		dirent* de;
		while ((de = readdir(dir)) != NULL)
		{
			snprintf(tmp, MAX_PATH - 1, "%s/%s", path, de->d_name);
			struct stat fileStat;
			if (stat(tmp, &fileStat))
				continue;

			if (S_ISDIR(fileStat.st_mode))
				RmDirRecursively(tmp);
			else
				remove(tmp);
		}
		closedir(dir);
	}
	rmdir(path);
#endif // SHELL_PLATFORM_WINDOWS
}

// Close system handle
static inline void CloseSysHandle (SysHandle& handle)
{
	if (handle != INVALID_HANDLE_VALUE)
	{
#if defined(SHELL_PLATFORM_WINDOWS)
		CloseHandle(handle);
#else
		close(handle);
#endif
		handle = INVALID_HANDLE_VALUE;
	}
}

// Pass input stream from Squirrel
static bool PassInput (HSQUIRRELVM sqvm,SysHandle handles[2])
{
	const SQChar* streamData;
	sq_getstring(sqvm, 4, &streamData);

#if defined(SHELL_PLATFORM_WINDOWS)
	DWORD numWrittenBytes;
	WriteFile(handles[1], streamData, DWORD(sq_getsize(sqvm, 4)), &numWrittenBytes, NULL);
#else
	write(handles[1], streamData, sq_getsize(sqvm, 4));
#endif

	CloseSysHandle(handles[1]);
	return true;
}

// Pass output/error stream to Squirrel
static bool PassOutput (HSQUIRRELVM sqvm,SysHandle handles[2], const SQChar* variable)
{
	CloseSysHandle(handles[1]);

	bool	  res		 = false;
	SQInteger streamSize = 0;
	SQChar*   streamData = NULL;

#if defined(SHELL_PLATFORM_WINDOWS)
	streamSize = GetFileSize(handles[0], NULL);
	if (streamSize)
	{
		streamData = (SQChar*)malloc(streamSize);
		if (!streamData)
		{
			PrintError("ERROR: Not enough memory.\n");
			goto finish;
		}

		DWORD numReadBytes;
		if (!ReadFile(handles[0], streamData, DWORD(streamSize), &numReadBytes, NULL) || !numReadBytes)
		{
			PrintError("ERROR: Failed to get stream data.\n");
			goto finish;
		}
		streamSize = numReadBytes;
	}
#else
#define GROW_BY 256

	int numReadBytes;
	do
	{
		streamData = (SQChar*)realloc(streamData, streamSize + GROW_BY);
		if (!streamData)
		{
			PrintError("ERROR: Not enough memory.\n");
			goto finish;
		}
		numReadBytes =  read(handles[0], streamData + streamSize, GROW_BY);
		streamSize   += numReadBytes;
	} while (numReadBytes > 0);
#endif

	sq_pushroottable(sqvm);
	sq_pushstring(sqvm, variable, -1);
	if (streamSize)
		sq_pushstring(sqvm, streamData, streamSize);
	else
		sq_pushstring(sqvm, "", -1);
	sq_set(sqvm, -3);
	sq_pop(sqvm, 1); // Pop root table
	res = true;

finish:
	free(streamData);
	CloseSysHandle(handles[0]);
	return res;
}

// bool mkdir(string path[, integer mode = 0755])
// bool mkdir(string path[, string mode = "755"])
// Create new directory
static SQInteger MkDir (HSQUIRRELVM sqvm)
{
	const SQChar* path;
	sq_getstring(sqvm, 2, &path);

#if defined(SHELL_PLATFORM_WINDOWS)
	sq_pushbool(sqvm, CreateDirectory(ConvPath(path, SQTrue), NULL));
#else
	SQInteger mode = 0755;
	if (sq_gettop(sqvm) == 3)
	{
		if (sq_gettype(sqvm, 3) == OT_INTEGER)
			sq_getinteger(sqvm, 3, &mode);
		else
		{
			const SQChar* tmp;
			sq_getstring(sqvm, 3, &tmp);
			mode = SQInteger(strtoul(tmp, NULL, 8));
		}
	}
	sq_pushbool(sqvm, !mkdir(ConvPath(path, SQTrue), mode_t(mode)));
#endif
	return 1;
}

// string getcwd()
// Get current directory
static SQInteger GetCWD (HSQUIRRELVM sqvm)
{
	SQChar path[MAX_PATH];

#if defined(SHELL_PLATFORM_WINDOWS)
	GetCurrentDirectory(MAX_PATH, path);
#else
	getcwd(path, MAX_PATH - 1); // FIXME: Does getcwd() write terminating '\0'?
	path[MAX_PATH - 1] = 0;
#endif

	sq_pushstring(sqvm, ConvPath(path, SQTrue), -1);
	return 1;
}

// bool chdir(string path)
// Set current directory
static SQInteger ChDir (HSQUIRRELVM sqvm)
{
	const SQChar* path;
	sq_getstring(sqvm, 2, &path);

#if defined(SHELL_PLATFORM_WINDOWS)
	sq_pushbool(sqvm, SetCurrentDirectory(ConvPath(path, SQTrue)));
#else
	sq_pushbool(sqvm, !chdir(ConvPath(path, SQTrue)));
#endif

	return 1;
}

// bool exist(string path)
// Check whether file or directory exists or not
static SQInteger Exist (HSQUIRRELVM sqvm)
{
	const SQChar* path;
	sq_getstring(sqvm, 2, &path);

#if defined(SHELL_PLATFORM_WINDOWS)
	sq_pushbool(sqvm, (GetFileAttributes(ConvPath(path, SQTrue)) != INVALID_FILE_ATTRIBUTES));
#else
	sq_pushbool(sqvm, !access(ConvPath(path, SQTrue), F_OK));
#endif

	return 1;
}

// copy(string from, string to[, bool overwrite = false])
// Copy file
static SQInteger Copy (HSQUIRRELVM sqvm)
{
	const SQChar *from,
				 *to;
	SQBool		 ovr = SQFalse;
	sq_getstring(sqvm, 2, &from);
	sq_getstring(sqvm, 3, &to);
	if (sq_gettop(sqvm) == 4)
		sq_getbool(sqvm, 4, &ovr);

	// Copy paths in OS's native format to temporary buffers (in functions with several path arguments ConvPath()'s
	// static buffer may cause problems)
	SQChar src[MAX_PATH], dest[MAX_PATH];
	strncpy_s(src, sizeof(src), ConvPath(from, SQTrue), MAX_PATH);
	strncpy_s(dest, sizeof(dest), ConvPath(to, SQTrue), MAX_PATH);
	CopyFile(src, dest, !ovr);

#if !defined(SHELL_PLATFORM_WINDOWS)
	// Copy timestamps and permissions
	struct stat fileStat;
	if (!stat(src, &fileStat))
	{
		utimbuf fileTime = { fileStat.st_atime, fileStat.st_mtime };
		chmod(dest, fileStat.st_mode);
		utime(dest, &fileTime);
	}
#endif
	return 0;
}

// move(string from, string to)
// Move file
static SQInteger Move (HSQUIRRELVM sqvm)
{
	const SQChar *from,
				 *to;
	sq_getstring(sqvm, 2, &from);
	sq_getstring(sqvm, 3, &to);
	SQChar src[MAX_PATH], dest[MAX_PATH];
	strncpy_s(src, sizeof(src), ConvPath(from, SQTrue), MAX_PATH);
	strncpy_s(dest, sizeof(dest), ConvPath(to, SQTrue), MAX_PATH);

#if defined(SHELL_PLATFORM_WINDOWS)
	MoveFile(src, dest);
#else
	rename(src, dest);
#endif
	return 0;
}

// remove(string path)
// Delete file
static SQInteger Remove (HSQUIRRELVM sqvm)
{
	const SQChar* path;
	sq_getstring(sqvm, 2, &path);

#if defined(SHELL_PLATFORM_WINDOWS)
	DeleteFile(ConvPath(path, SQTrue));
#else
	remove(ConvPath(path, SQTrue));
#endif
	return 0;
}

// rmdir(string path[, bool recursive = false])
// Remove directory (must be empty unless 'recursive' is 'true')
static SQInteger RmDir (HSQUIRRELVM sqvm)
{
	const SQChar* path;
	sq_getstring(sqvm, 2, &path);

	SQChar tmp[MAX_PATH];
	strncpy_s(tmp, sizeof(tmp), ConvPath(path, SQFalse), MAX_PATH);

	// Remove trailing slashes
	for (SQChar* c = &tmp[sq_getsize(sqvm, 2) - 1]; (c >= tmp) && (*c == '/'); --c)
		*c = 0;

	SQBool recursive = SQFalse;
	if (sq_gettop(sqvm) == 3)
		sq_getbool(sqvm, 3, &recursive);

	if (recursive)
		RmDirRecursively(ConvPath(tmp, SQTrue));
	else
	{
#if defined(SHELL_PLATFORM_WINDOWS)
		RemoveDirectory(tmp);
#else
		rmdir(tmp);
#endif
	}
	return 0;
}

// string getenv(string name)
// Get value of environment variable
static SQInteger GetEnv (HSQUIRRELVM sqvm)
{
	if (!sq_getsize(sqvm, 2))
	{
		sq_pushnull(sqvm);
		return 1;
	}

	const SQChar* name;
	sq_getstring(sqvm, 2, &name);

#if defined(SHELL_PLATFORM_WINDOWS)
	SQChar res[32767];
	DWORD  size = sizeof(res) / sizeof(SQChar);
	if (GetEnvironmentVariable(name, res, size))
#else
	SQChar* res = getenv(name);
	if (res)
#endif
		sq_pushstring(sqvm, res, -1);
	else
		sq_pushnull(sqvm);
	return 1;
}

// bool setenv(string name, string value)
// Set value of environment variable
static SQInteger SetEnv (HSQUIRRELVM sqvm)
{
	if (!sq_getsize(sqvm, 2))
	{
		sq_pushbool(sqvm, SQFalse);
		return 1;
	}

	const SQChar *name, *value;
	sq_getstring(sqvm, 2, &name);
	sq_getstring(sqvm, 3, &value);

#if defined(SHELL_PLATFORM_WINDOWS)
	sq_pushbool(sqvm, SetEnvironmentVariable(name, value));
#else
	sq_pushbool(sqvm, !setenv(name, value, true));
#endif
	return 1;
}

// delenv(string name)
// Remove environment variable
static SQInteger DelEnv (HSQUIRRELVM sqvm)
{
	if (!sq_getsize(sqvm, 2))
		return 0;

	const SQChar* name;
	sq_getstring(sqvm, 2, &name);

#if defined(SHELL_PLATFORM_WINDOWS)
	SetEnvironmentVariable(name, NULL);
#else
	unsetenv(name);
#endif
	return 0;
}
static SQInteger StartProcess (HSQUIRRELVM sqvm)
{
	if (!sq_getsize(sqvm, 2))
	{
		sq_pushinteger(sqvm, -1);
		return 1;
	}

	const SQChar* tmp;
	sq_getstring(sqvm, 2, &tmp);

	SQChar path[MAX_PATH];
	strncpy_s(path, sizeof(path), ConvPath(tmp, SQTrue), MAX_PATH);
#if defined(SHELL_PLATFORM_WINDOWS)
	// In CreateProcess() documentation said that if path doesn't contain extension, ".exe" is assumed, but it doesn't
	// happen for some reason...
	SQChar *bs	= strrchr(path, '\\'),
		   *ext = strrchr(path, '.');
	if (!ext || (ext < bs))
		strcat_s(path, MAX_PATH, ".exe");

	// Also, CreateProcess() doesn't search for files in PATH environment variable and some other places. So do it here
	// Don't search if path is specified
	if (!bs)
		SearchPath(NULL, path, NULL, MAX_PATH, path, NULL);
#endif

	SQChar	  *args[MAX_ARGS] = { path, NULL };
	SQInteger i,
			  n = 1,
			  size;

	// Build list of command line arguments
	if ((sq_gettop(sqvm) >= 3) && sq_getsize(sqvm, 3))
	{
		size = sq_getsize(sqvm, 3);
		if (size > (MAX_ARGS - 2))
			size = MAX_ARGS - 2;

		sq_pushnull(sqvm);
		for (i = 0; i < size && SQ_SUCCEEDED(sq_next(sqvm, 3)); ++i)
		{
			if (sq_gettype(sqvm, -1) != OT_STRING)
			{
				sq_pop(sqvm, 3); // Pop the key, value and iterator
				return sq_throwerror(sqvm, "all elements of 'args' parameter in run() function must be strings");
			}

			sq_getstring(sqvm, -1, &tmp);
			size_t argSize = strlen(tmp) + 1;
			args[n] = (SQChar*)malloc(argSize);
			if (!args[n])
			{
				PrintError("ERROR: Not enough memory.\n");
				break;
			}

			strncpy_s(args[n], argSize, tmp, argSize);
			++n;
			sq_pop(sqvm, 2); // Pop the key and value
		}
		sq_pop(sqvm, 1); // Pop the iterator
		args[n] = NULL;
	}

	// Determine what streams should be redirected
	SQBool redirIn	= SQFalse,
		   redirOut = SQFalse,
		   redirErr = SQFalse;
	if ((sq_gettop(sqvm) >= 4) && sq_getsize(sqvm, 4))
		redirIn = SQTrue;
	if (sq_gettop(sqvm) >= 5)
		sq_getbool(sqvm, 5, &redirOut);
	if (sq_gettop(sqvm) == 6)
		sq_getbool(sqvm, 6, &redirErr);

	SysHandle newInput[2]  = { INVALID_HANDLE_VALUE, INVALID_HANDLE_VALUE },
			  newOutput[2] = { INVALID_HANDLE_VALUE, INVALID_HANDLE_VALUE },
			  newError[2]  = { INVALID_HANDLE_VALUE, INVALID_HANDLE_VALUE };
	SQInteger res		   = -1;

#if defined(SHELL_PLATFORM_WINDOWS)
	// Here we need to pack all arguments to one string...
	size = SQInteger(strlen(path) + 3);	// At least there'll be quoted path to executable and termination '\0'
	char* cmdLine = NULL;
	for (i = 0; i < n; ++i)
		size += SQInteger(strlen(args[i]) + 3);	// Each argument'll be separated by space and quotes

	cmdLine = (char*)malloc(size);
	if (!cmdLine)
		PrintError("ERROR: Not enough memory.\n");
	else
	{
#if (_MSC_VER >= 1400)
		sprintf_s(cmdLine, size, "\"%s\"", path);
#else
		sprintf(cmdLine, "\"%s\"", path);
#endif

		for (i = 1; i < n; ++i)
		{
			strcat_s(cmdLine, size, " \"");
			strcat_s(cmdLine, size, args[i]);
			strcat_s(cmdLine, size, "\"");
		}
	}

	// Create pipes for requested redirections
	SECURITY_ATTRIBUTES sa;
	memset(&sa, 0, sizeof(SECURITY_ATTRIBUTES));
	sa.nLength		  = sizeof(SECURITY_ATTRIBUTES);
	sa.bInheritHandle = TRUE;

	if (redirIn)
	{
		if (!CreatePipe(&newInput[0], &newInput[1], &sa, 0))
		{
			PrintError("ERROR: Failed to create input pipe.\n");
			goto finish;
		}
		SetHandleInformation(newInput[1], HANDLE_FLAG_INHERIT, 0);
	}
	else
		newInput[0] = GetStdHandle(STD_INPUT_HANDLE);

	if (redirOut)
	{
		if (!CreatePipe(&newOutput[0], &newOutput[1], &sa, 0))
		{
			PrintError("ERROR: Failed to create output pipe.\n");
			goto finish;
		}
		SetHandleInformation(newOutput[0], HANDLE_FLAG_INHERIT, 0);
	}
	else
		newOutput[1] = GetStdHandle(STD_OUTPUT_HANDLE);

	if (redirErr)
	{
		if (!CreatePipe(&newError[0], &newError[1], &sa, 0))
		{
			PrintError("ERROR: Failed to create error pipe.\n");
			goto finish;
		}
		SetHandleInformation(newError[0], HANDLE_FLAG_INHERIT, 0);
	}
	else
		newError[1] = GetStdHandle(STD_ERROR_HANDLE);

	// Run requested process
	_RPT1(_CRT_WARN, "--- Running '%s'...\n", cmdLine);
	STARTUPINFO			si;
	PROCESS_INFORMATION pi;
	memset(&si, 0, sizeof(STARTUPINFO));
	si.cb		  = sizeof(STARTUPINFO);
	si.dwFlags	  = STARTF_USESTDHANDLES;
	si.hStdInput  = newInput[0];
	si.hStdOutput = newOutput[1];
	si.hStdError  = newError[1];
	if (CreateProcess(path, cmdLine, NULL, NULL, TRUE, 0, NULL, NULL, &si, &pi))
	{
		CloseHandle(pi.hThread);

		// Pass input
		if (redirIn && !PassInput(sqvm,newInput))
		{
			// Child process won't get input, so kill it
			TerminateProcess(pi.hProcess, UINT(-1));
		}

		//WaitForSingleObject(pi.hProcess, INFINITE);

		//DWORD exitCode;
		//res = GetExitCodeProcess(pi.hProcess, &exitCode) ? SQInteger(exitCode) : 0;
		CloseHandle(pi.hProcess);

		// Pass output
		if (redirOut && !PassOutput(sqvm,newOutput, "OUTPUT"))
			goto finish;

		// Pass error
		if (redirOut && !PassOutput(sqvm,newError, "ERROR"))
			goto finish;
	}
	else
		PrintError("ERROR: Failed to run '%s'.\n", path);

finish:
	free(cmdLine);
#else // SHELL_PLATFORM_WINDOWS
	if (redirIn && pipe(newInput))
	{
		PrintError("ERROR: Failed to create input pipe.\n");
		goto finish;
	}

	if (redirOut && pipe(newOutput))
	{
		PrintError("ERROR: Failed to create output pipe.\n");
		goto finish;
	}

	if (redirErr && pipe(newError))
	{
		PrintError("ERROR: Failed to create error pipe.\n");
		goto finish;
	}

	pid_t pid;
	pid = fork();
	if (pid < 0)
		PrintError("ERROR: Failed to fork.\n");
	else if (pid == 0)
	{
		// Duplicate handles and run process
		if (redirIn)
		{
			dup2(newInput[0], STDIN_FILENO);
			CloseSysHandle(newInput[0]);
		}

		if (redirOut)
		{
			dup2(newOutput[1], STDOUT_FILENO);
			CloseSysHandle(newOutput[1]);
		}

		if (redirErr)
		{
			dup2(newError[1], STDERR_FILENO);
			CloseSysHandle(newError[1]);
		}

		execvp(path, args);
		// If we got here, execvp() failed.
		PrintError("ERROR: Failed to run '%s'.\n", path);
		_exit(EXIT_FAILURE);
	}
	else
	{
		// Pass input
		if (redirIn && !PassInput(sqvm,newInput))
		{
			// Child process won't get input, so kill it
			kill(pid, SIGKILL);
		}

		// Get child's exit status
		int ces;
		waitpid(pid, &ces, 0);
		res = ces;

		// Pass output
		if (redirOut && !PassOutput(sqvm,newOutput, "OUTPUT"))
			goto finish;

		// Pass error
		if (redirErr && !PassOutput(sqvm,newError, "ERROR"))
			goto finish;
	}

finish:
#endif // SHELL_PLATFORM_WINDOWS
	for (i = 1; i < n; ++i)
		free(args[i]);

	// Close pipes
	if (redirIn)
	{
		CloseSysHandle(newInput[0]);
		CloseSysHandle(newInput[1]);
	}
	if (redirOut)
	{
		CloseSysHandle(newOutput[0]);
		CloseSysHandle(newOutput[1]);
	}
	if (redirErr)
	{
		CloseSysHandle(newError[0]);
		CloseSysHandle(newError[1]);
	}
	sq_pushinteger(sqvm, res);
	return 1;
}
// integer run(string path[, strarray args[, string input = null[, bool redirOut[, bool redirErr]]]])
// Run another program or script
// 'path' is passed as first argument automatically, so there's no need to set is manually
// Return child exit code or -1 if failed
static SQInteger Run (HSQUIRRELVM sqvm)
{
	if (!sq_getsize(sqvm, 2))
	{
		sq_pushinteger(sqvm, -1);
		return 1;
	}

	const SQChar* tmp;
	sq_getstring(sqvm, 2, &tmp);

	SQChar path[MAX_PATH];
	strncpy_s(path, sizeof(path), ConvPath(tmp, SQTrue), MAX_PATH);
#if defined(SHELL_PLATFORM_WINDOWS)
	// In CreateProcess() documentation said that if path doesn't contain extension, ".exe" is assumed, but it doesn't
	// happen for some reason...
	SQChar *bs	= strrchr(path, '\\'),
		   *ext = strrchr(path, '.');
	if (!ext || (ext < bs))
		strcat_s(path, MAX_PATH, ".exe");

	// Also, CreateProcess() doesn't search for files in PATH environment variable and some other places. So do it here
	// Don't search if path is specified
	if (!bs)
		SearchPath(NULL, path, NULL, MAX_PATH, path, NULL);
#endif

	SQChar	  *args[MAX_ARGS] = { path, NULL };
	SQInteger i,
			  n = 1,
			  size;

	// Build list of command line arguments
	if ((sq_gettop(sqvm) >= 3) && sq_getsize(sqvm, 3))
	{
		size = sq_getsize(sqvm, 3);
		if (size > (MAX_ARGS - 2))
			size = MAX_ARGS - 2;

		sq_pushnull(sqvm);
		for (i = 0; i < size && SQ_SUCCEEDED(sq_next(sqvm, 3)); ++i)
		{
			if (sq_gettype(sqvm, -1) != OT_STRING)
			{
				sq_pop(sqvm, 3); // Pop the key, value and iterator
				return sq_throwerror(sqvm, "all elements of 'args' parameter in run() function must be strings");
			}

			sq_getstring(sqvm, -1, &tmp);
			size_t argSize = strlen(tmp) + 1;
			args[n] = (SQChar*)malloc(argSize);
			if (!args[n])
			{
				PrintError("ERROR: Not enough memory.\n");
				break;
			}

			strncpy_s(args[n], argSize, tmp, argSize);
			++n;
			sq_pop(sqvm, 2); // Pop the key and value
		}
		sq_pop(sqvm, 1); // Pop the iterator
		args[n] = NULL;
	}

	// Determine what streams should be redirected
	SQBool redirIn	= SQFalse,
		   redirOut = SQFalse,
		   redirErr = SQFalse;
	if ((sq_gettop(sqvm) >= 4) && sq_getsize(sqvm, 4))
		redirIn = SQTrue;
	if (sq_gettop(sqvm) >= 5)
		sq_getbool(sqvm, 5, &redirOut);
	if (sq_gettop(sqvm) == 6)
		sq_getbool(sqvm, 6, &redirErr);

	SysHandle newInput[2]  = { INVALID_HANDLE_VALUE, INVALID_HANDLE_VALUE },
			  newOutput[2] = { INVALID_HANDLE_VALUE, INVALID_HANDLE_VALUE },
			  newError[2]  = { INVALID_HANDLE_VALUE, INVALID_HANDLE_VALUE };
	SQInteger res		   = -1;

#if defined(SHELL_PLATFORM_WINDOWS)
	// Here we need to pack all arguments to one string...
	size = SQInteger(strlen(path) + 3);	// At least there'll be quoted path to executable and termination '\0'
	char* cmdLine = NULL;
	for (i = 0; i < n; ++i)
		size += SQInteger(strlen(args[i]) + 3);	// Each argument'll be separated by space and quotes

	cmdLine = (char*)malloc(size);
	if (!cmdLine)
		PrintError("ERROR: Not enough memory.\n");
	else
	{
#if (_MSC_VER >= 1400)
		sprintf_s(cmdLine, size, "\"%s\"", path);
#else
		sprintf(cmdLine, "\"%s\"", path);
#endif

		for (i = 1; i < n; ++i)
		{
			strcat_s(cmdLine, size, " \"");
			strcat_s(cmdLine, size, args[i]);
			strcat_s(cmdLine, size, "\"");
		}
	}

	// Create pipes for requested redirections
	SECURITY_ATTRIBUTES sa;
	memset(&sa, 0, sizeof(SECURITY_ATTRIBUTES));
	sa.nLength		  = sizeof(SECURITY_ATTRIBUTES);
	sa.bInheritHandle = TRUE;

	if (redirIn)
	{
		if (!CreatePipe(&newInput[0], &newInput[1], &sa, 0))
		{
			PrintError("ERROR: Failed to create input pipe.\n");
			goto finish;
		}
		SetHandleInformation(newInput[1], HANDLE_FLAG_INHERIT, 0);
	}
	else
		newInput[0] = GetStdHandle(STD_INPUT_HANDLE);

	if (redirOut)
	{
		if (!CreatePipe(&newOutput[0], &newOutput[1], &sa, 0))
		{
			PrintError("ERROR: Failed to create output pipe.\n");
			goto finish;
		}
		SetHandleInformation(newOutput[0], HANDLE_FLAG_INHERIT, 0);
	}
	else
		newOutput[1] = GetStdHandle(STD_OUTPUT_HANDLE);

	if (redirErr)
	{
		if (!CreatePipe(&newError[0], &newError[1], &sa, 0))
		{
			PrintError("ERROR: Failed to create error pipe.\n");
			goto finish;
		}
		SetHandleInformation(newError[0], HANDLE_FLAG_INHERIT, 0);
	}
	else
		newError[1] = GetStdHandle(STD_ERROR_HANDLE);

	// Run requested process
	_RPT1(_CRT_WARN, "--- Running '%s'...\n", cmdLine);
	STARTUPINFO			si;
	PROCESS_INFORMATION pi;
	memset(&si, 0, sizeof(STARTUPINFO));
	si.cb		  = sizeof(STARTUPINFO);
	si.dwFlags	  = STARTF_USESTDHANDLES;
	si.hStdInput  = newInput[0];
	si.hStdOutput = newOutput[1];
	si.hStdError  = newError[1];
	if (CreateProcess(path, cmdLine, NULL, NULL, TRUE, 0, NULL, NULL, &si, &pi))
	{
		CloseHandle(pi.hThread);

		// Pass input
		if (redirIn && !PassInput(sqvm,newInput))
		{
			// Child process won't get input, so kill it
			TerminateProcess(pi.hProcess, UINT(-1));
		}

		WaitForSingleObject(pi.hProcess, INFINITE);

		DWORD exitCode;
		res = GetExitCodeProcess(pi.hProcess, &exitCode) ? SQInteger(exitCode) : 0;
		CloseHandle(pi.hProcess);

		// Pass output
		if (redirOut && !PassOutput(sqvm,newOutput, "OUTPUT"))
			goto finish;

		// Pass error
		if (redirOut && !PassOutput(sqvm,newError, "ERROR"))
			goto finish;
	}
	else
		PrintError("ERROR: Failed to run '%s'.\n", path);

finish:
	free(cmdLine);
#else // SHELL_PLATFORM_WINDOWS
	if (redirIn && pipe(newInput))
	{
		PrintError("ERROR: Failed to create input pipe.\n");
		goto finish;
	}

	if (redirOut && pipe(newOutput))
	{
		PrintError("ERROR: Failed to create output pipe.\n");
		goto finish;
	}

	if (redirErr && pipe(newError))
	{
		PrintError("ERROR: Failed to create error pipe.\n");
		goto finish;
	}

	pid_t pid;
	pid = fork();
	if (pid < 0)
		PrintError("ERROR: Failed to fork.\n");
	else if (pid == 0)
	{
		// Duplicate handles and run process
		if (redirIn)
		{
			dup2(newInput[0], STDIN_FILENO);
			CloseSysHandle(newInput[0]);
		}

		if (redirOut)
		{
			dup2(newOutput[1], STDOUT_FILENO);
			CloseSysHandle(newOutput[1]);
		}

		if (redirErr)
		{
			dup2(newError[1], STDERR_FILENO);
			CloseSysHandle(newError[1]);
		}

		execvp(path, args);
		// If we got here, execvp() failed.
		PrintError("ERROR: Failed to run '%s'.\n", path);
		_exit(EXIT_FAILURE);
	}
	else
	{
		// Pass input
		if (redirIn && !PassInput(sqvm,newInput))
		{
			// Child process won't get input, so kill it
			kill(pid, SIGKILL);
		}

		// Get child's exit status
		int ces;
		waitpid(pid, &ces, 0);
		res = ces;

		// Pass output
		if (redirOut && !PassOutput(sqvm,newOutput, "OUTPUT"))
			goto finish;

		// Pass error
		if (redirErr && !PassOutput(sqvm,newError, "ERROR"))
			goto finish;
	}

finish:
#endif // SHELL_PLATFORM_WINDOWS
	for (i = 1; i < n; ++i)
		free(args[i]);

	// Close pipes
	if (redirIn)
	{
		CloseSysHandle(newInput[0]);
		CloseSysHandle(newInput[1]);
	}
	if (redirOut)
	{
		CloseSysHandle(newOutput[0]);
		CloseSysHandle(newOutput[1]);
	}
	if (redirErr)
	{
		CloseSysHandle(newError[0]);
		CloseSysHandle(newError[1]);
	}
	sq_pushinteger(sqvm, res);
	return 1;
}

// chmod(string path, integer mode)
// chmod(string path, string mode)
// Set file's permissions
static SQInteger ChMod (HSQUIRRELVM sqvm)
{
#if !defined(SHELL_PLATFORM_WINDOWS)
	const SQChar* path;
	SQInteger	  mode;
	sq_getstring(sqvm, 2, &path);
	if (sq_gettype(sqvm, 3) == OT_INTEGER)
		sq_getinteger(sqvm, 3, &mode);
	else
	{
		const SQChar* strMode;
		sq_getstring(sqvm, 3, &strMode);
		mode = SQInteger(strtoul(strMode, NULL, 8));
	}
	chmod(ConvPath(path, SQTrue), mode_t(mode));
#endif
	return 0;
}

// chown(string path, string newOwner)
// Set file's owner
static SQInteger ChOwn (HSQUIRRELVM sqvm)
{
#if !defined(SHELL_PLATFORM_WINDOWS)
	const SQChar *path, *newOwner;
	sq_getstring(sqvm, 2, &path);
	sq_getstring(sqvm, 3, &newOwner);
	passwd* pwd = getpwnam(newOwner);
	if (pwd)
		chown(ConvPath(path, SQTrue), pwd->pw_uid, gid_t(-1));
#endif
	return 0;
}

// chgrp(string path, string newGroup)
// Set file's group
static SQInteger ChGrp (HSQUIRRELVM sqvm)
{
#if !defined(SHELL_PLATFORM_WINDOWS)
	const SQChar *path, *newGroup;
	sq_getstring(sqvm, 2, &path);
	sq_getstring(sqvm, 3, &newGroup);
	group* grp = getgrnam(newGroup);
	if (grp)
		chown(ConvPath(path, SQTrue), uid_t(-1), grp->gr_gid);
#endif
	return 0;
}

// string_array readdir(string path)
// Get list of directory entries
static SQInteger ReadDir (HSQUIRRELVM sqvm)
{
	const SQChar* path;
	sq_getstring(sqvm, 2, &path);

	SQChar dirPath[MAX_PATH];
	strncpy_s(dirPath, sizeof(dirPath), ConvPath(path, SQFalse), MAX_PATH);

	// Remove redundant trailing slashes
	size_t pathLen = strlen(dirPath);
	if (pathLen)
	{
		for (SQChar* c = &dirPath[pathLen - 1]; c > dirPath && c[0] == '/' && c[-1] == '/'; --c)
		{
			--pathLen;
			*c = 0;
		}
	}
	else
	{
		sq_pushnull(sqvm);
		return 1;
	}

	// Add trailing slash if necessary
	if (dirPath[pathLen - 1] != '/')
	{
		dirPath[pathLen++] = '/';
		dirPath[pathLen]   = 0;
	}

#if defined(SHELL_PLATFORM_WINDOWS)
	// Treat "/" path as system root
	if (dirPath[0] == '/')
	{
		SQChar winDir[MAX_PATH];
		if (GetWindowsDirectory(winDir, MAX_PATH))
			dirPath[0] = winDir[0];
		else
			dirPath[0] = 'c';
		dirPath[1] = ':';
		dirPath[2] = 0;
	}
#endif

	// Accept only existing directories
	bool isDir = false;
#if defined(SHELL_PLATFORM_WINDOWS)
	DWORD attrs = GetFileAttributes(ConvPath(dirPath, SQTrue));
	isDir = (attrs != INVALID_FILE_ATTRIBUTES) && (attrs & FILE_ATTRIBUTE_DIRECTORY);
#else
	struct stat fileStat;
	if (!stat(dirPath, &fileStat))
		isDir = S_ISDIR(fileStat.st_mode);
#endif
	if (!isDir)
	{
		sq_pushnull(sqvm);
		return 1;
	}

	SQChar tmp[MAX_PATH];
#if defined(SHELL_PLATFORM_WINDOWS)
	// Append wildcard to the end of path
#if (_MSC_VER >= 1400)
	_snprintf_s(tmp, MAX_PATH, MAX_PATH - 1, "%s*", dirPath);
#else
	_snprintf(tmp, MAX_PATH - 1, "%s*", dirPath);
#endif

	// List files
	WIN32_FIND_DATA fd;
	HANDLE			fh = FindFirstFile(ConvPath(tmp, SQTrue), &fd);
	if (fh != INVALID_HANDLE_VALUE)
	{
		// First 2 entries will be "." and ".." - skip them
		FindNextFile(fh, &fd);

		sq_newarray(sqvm, 0);
		while (FindNextFile(fh, &fd))
		{
#if (_MSC_VER >= 1400)
			_snprintf_s(tmp, MAX_PATH, MAX_PATH - 1, "%s%s", dirPath, fd.cFileName);
#else
			_snprintf(tmp, MAX_PATH - 1, "%s%s", dirPath, fd.cFileName);
#endif
			sq_pushstring(sqvm, ConvPath(tmp, SQFalse), -1);
			sq_arrayappend(sqvm, -2);
		}
		FindClose(fh);
	}
	else
		sq_pushnull(sqvm);
#else // SHELL_PLATFORM_WINDOWS
	DIR* dir = opendir(dirPath);
	if (dir)
	{
		// As in Windows, first 2 entries are "." and ".."
		readdir(dir);
		readdir(dir);

		dirent* de;
		sq_newarray(sqvm, 0);
		while ((de = readdir(dir)) != NULL)
		{
			snprintf(tmp, MAX_PATH - 1, "%s%s", dirPath, de->d_name);
			sq_pushstring(sqvm, ConvPath(tmp, SQFalse), -1);
			sq_arrayappend(sqvm, -2);
		}
		closedir(dir);
	}
	else
		sq_pushnull(sqvm);
#endif // SHELL_PLATFORM_WINDOWS
	return 1;
}
static SQInteger GetDirs (HSQUIRRELVM sqvm)
{
	const SQChar* path;
	sq_getstring(sqvm, 2, &path);

	SQChar dirPath[MAX_PATH];
	strncpy_s(dirPath, sizeof(dirPath), ConvPath(path, SQFalse), MAX_PATH);

	// Remove redundant trailing slashes
	size_t pathLen = strlen(dirPath);
	if (pathLen)
	{
		for (SQChar* c = &dirPath[pathLen - 1]; c > dirPath && c[0] == '/' && c[-1] == '/'; --c)
		{
			--pathLen;
			*c = 0;
		}
	}
	else
	{
		sq_pushnull(sqvm);
		return 1;
	}

	// Add trailing slash if necessary
	if (dirPath[pathLen - 1] != '/')
	{
		dirPath[pathLen++] = '/';
		dirPath[pathLen]   = 0;
	}

#if defined(SHELL_PLATFORM_WINDOWS)
	// Treat "/" path as system root
	if (dirPath[0] == '/')
	{
		SQChar winDir[MAX_PATH];
		if (GetWindowsDirectory(winDir, MAX_PATH))
			dirPath[0] = winDir[0];
		else
			dirPath[0] = 'c';
		dirPath[1] = ':';
		dirPath[2] = 0;
	}
#endif

	// Accept only existing directories
	bool isDir = false;
#if defined(SHELL_PLATFORM_WINDOWS)
	DWORD attrs = GetFileAttributes(ConvPath(dirPath, SQTrue));
	isDir = (attrs != INVALID_FILE_ATTRIBUTES) && (attrs & FILE_ATTRIBUTE_DIRECTORY);
#else
	struct stat fileStat;
	if (!stat(dirPath, &fileStat))
		isDir = S_ISDIR(fileStat.st_mode);
#endif
	if (!isDir)
	{
		sq_pushnull(sqvm);
		return 1;
	}

	SQChar tmp[MAX_PATH];
#if defined(SHELL_PLATFORM_WINDOWS)
	// Append wildcard to the end of path
#if (_MSC_VER >= 1400)
	_snprintf_s(tmp, MAX_PATH, MAX_PATH - 1, "%s*", dirPath);
#else
	_snprintf(tmp, MAX_PATH - 1, "%s*", dirPath);
#endif

	// List files
	WIN32_FIND_DATA fd;
	HANDLE			fh = FindFirstFile(ConvPath(tmp, SQTrue), &fd);
	if (fh != INVALID_HANDLE_VALUE)
	{
		// First 2 entries will be "." and ".." - skip them
		FindNextFile(fh, &fd);

		sq_newarray(sqvm, 0);
		while (FindNextFile(fh, &fd))
		{
//#if (_MSC_VER >= 1400)
//			_snprintf_s(tmp, MAX_PATH, MAX_PATH - 1, "%s%s", dirPath, fd.cFileName);
//#else
//			_snprintf(tmp, MAX_PATH - 1, "%s%s", dirPath, fd.cFileName);
//#endif
//			sq_pushstring(sqvm, ConvPath(tmp, SQFalse), -1);
			sq_pushstring(sqvm, fd.cFileName , -1);
			sq_arrayappend(sqvm, -2);
		}
		FindClose(fh);
	}
	else
		sq_pushnull(sqvm);
#else // SHELL_PLATFORM_WINDOWS
	DIR* dir = opendir(dirPath);
	if (dir)
	{
		// As in Windows, first 2 entries are "." and ".."
		readdir(dir);
		readdir(dir);

		dirent* de;
		sq_newarray(sqvm, 0);
		while ((de = readdir(dir)) != NULL)
		{
			//snprintf(tmp, MAX_PATH - 1, "%s%s", dirPath, de->d_name);
			//sq_pushstring(sqvm, ConvPath(tmp, SQFalse), -1);
			sq_pushstring(sqvm, de->d_name , -1);
			sq_arrayappend(sqvm, -2);
		}
		closedir(dir);
	}
	else
		sq_pushnull(sqvm);
#endif // SHELL_PLATFORM_WINDOWS
	return 1;
}
// exit([exitCode])
// Quit the shell
int retCode;
bool silent=true;
//HSQUIRRELVM sqvm;
static SQInteger Exit (HSQUIRRELVM sqvm)
{
	SQInteger code = EXIT_SUCCESS;
	if (sq_gettop(sqvm) == 2)
		sq_getinteger(sqvm, 2, &code);

	retCode = int(code);
	return sq_suspendvm(sqvm);
}

// integer filetime(string path[, integer which = MODIFICATION])
// Get file time
// Supported values for 'which' parameter:
//    CREATION     - time when the file was created;
//    ACCESS       - time when the file was last accessed;
//    MODIFICATION - time when the file contents were modified;
//    CHANGE       - time when the file information (owner, group, etc.) was changed.
// Returned time is UTC
static SQInteger FileTime (HSQUIRRELVM sqvm)
{
	SQInteger	  res = 0;
	const SQChar* path;
	SQInteger	  which = FILETIME_CREATE;
	sq_getstring(sqvm, 2, &path);
	if (sq_gettop(sqvm) == 3)
		sq_getinteger(sqvm, 3, &which);

#if defined(SHELL_PLATFORM_WINDOWS)
	HANDLE file = CreateFile(ConvPath(path, SQTrue), GENERIC_READ, FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, OPEN_EXISTING, 0, NULL);
	if (file != INVALID_HANDLE_VALUE)
	{
		FILETIME cFTime, // Creation time
				 aFTime, // Last access time
				 mFTime; // Last modification time
		GetFileTime(file, &cFTime, &aFTime, &mFTime);
		CloseHandle(file);

		FILETIME* fileTime;
		switch (which)
		{
		case FILETIME_CREATE:
			fileTime = &cFTime;
			break;

		case FILETIME_ACCESS:
			fileTime = &aFTime;
			break;

		default:
			fileTime = &mFTime;
		}

		SYSTEMTIME sysTime;
		FileTimeToSystemTime(fileTime, &sysTime);
		res = TimeToInt(sysTime.wYear, sysTime.wMonth, sysTime.wDay, sysTime.wHour, sysTime.wMinute, sysTime.wSecond);
	}
#else // SHELL_PLATFORM_WINDOWS
	struct stat fileStat;
	if (!stat(ConvPath(path, SQTrue), &fileStat))
	{
		time_t* fileTime;
		switch (which)
		{
		case FILETIME_CREATE:
		case FILETIME_ACCESS:
			fileTime = &fileStat.st_atime;
			break;

		case FILETIME_CHANGE:
			fileTime = &fileStat.st_ctime;
			break;

		default:
			fileTime = &fileStat.st_mtime;
		}
		tm* sysTime = gmtime(fileTime);
		res = TimeToInt(sysTime->tm_year + 1900, sysTime->tm_mon + 1, sysTime->tm_mday,
						sysTime->tm_hour, sysTime->tm_min, sysTime->tm_sec);
	}
#endif // SHELL_PLATFORM_WINDOWS
	sq_pushinteger(sqvm, res);
	return 1;
}

// integer systime()
// Get system time
static SQInteger SysTime (HSQUIRRELVM sqvm)
{
#if defined(SHELL_PLATFORM_WINDOWS)
	SYSTEMTIME sysTime;
	GetSystemTime(&sysTime);
	sq_pushinteger(sqvm, TimeToInt(sysTime.wYear, sysTime.wMonth, sysTime.wDay, sysTime.wHour, sysTime.wMinute, sysTime.wSecond));
#else
	time_t curTime = time(NULL);
	tm*    sysTime = gmtime(&curTime);
	sq_pushinteger(sqvm, TimeToInt(sysTime->tm_year + 1900, sysTime->tm_mon + 1, sysTime->tm_mday,
								   sysTime->tm_hour, sysTime->tm_min, sysTime->tm_sec));
#endif
	return 1;
}

// integer localtime()
// Get system time
static SQInteger LocalTime (HSQUIRRELVM sqvm)
{
#if defined(SHELL_PLATFORM_WINDOWS)
	SYSTEMTIME localTime;
	GetLocalTime(&localTime);
	sq_pushinteger(sqvm, TimeToInt(localTime.wYear, localTime.wMonth, localTime.wDay,
								   localTime.wHour, localTime.wMinute, localTime.wSecond));
#else
	time_t curTime	 = time(NULL);
	tm*    localTime = localtime(&curTime);
	sq_pushinteger(sqvm, TimeToInt(localTime->tm_year + 1900, localTime->tm_mon + 1, localTime->tm_mday,
								   localTime->tm_hour, localTime->tm_min, localTime->tm_sec));
#endif
	return 1;
}

// setfiletime(string path, integer which, integer time)
// Modify file timestamp
static SQInteger SetFileTime2 (HSQUIRRELVM sqvm)
{
	const SQChar* path;
	SQInteger	  which,
				  timeValue;
	sq_getstring(sqvm, 2, &path);
	sq_getinteger(sqvm, 3, &which);
	sq_getinteger(sqvm, 4, &timeValue);

	// Unpack time
	unsigned year,
			 month,
			 day,
			 hour,
			 minute,
			 second;
	IntToTime(timeValue, year, month, day, hour, minute, second);

#if defined(SHELL_PLATFORM_WINDOWS)
	HANDLE file = CreateFile(ConvPath(path, SQTrue), GENERIC_READ | GENERIC_WRITE, FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, OPEN_EXISTING, 0, NULL);
	if (file != INVALID_HANDLE_VALUE)
	{
		FILETIME cFTime,	// Creation time
				 aFTime,	// Last access time
				 mFTime;	// Last modification time
		GetFileTime(file, &cFTime, &aFTime, &mFTime);

		FILETIME   newFTime;
		SYSTEMTIME sysTime = { WORD(year), WORD(month), 0, WORD(day), WORD(hour), WORD(minute), WORD(second), 0 };
		SystemTimeToFileTime(&sysTime, &newFTime);
		switch (which)
		{
		case FILETIME_CREATE:
			SetFileTime(file, &newFTime, &aFTime, &mFTime);
			break;

		case FILETIME_ACCESS:
			SetFileTime(file, &cFTime, &newFTime, &mFTime);
			break;

		case FILETIME_MODIFY:
		case FILETIME_CHANGE:
			SetFileTime(file, &cFTime, &aFTime, &newFTime);
		}
		CloseHandle(file);
	}
#else // SHELL_PLATFORM_WINDOWS
	struct stat	  fileStat;
	const SQChar* filePath = ConvPath(path, SQTrue);
	if (!stat(filePath, &fileStat))
	{
		tm		sysTime  = { int(second), int(minute), int(hour), int(day), int(month - 1), int(year - 1900), 0, 0, 0 };
		time_t	newTime  = mktime(&sysTime);
		utimbuf newFTime = { fileStat.st_atime, fileStat.st_mtime };
		switch (which)
		{
		case FILETIME_CREATE:
		case FILETIME_ACCESS:
			newFTime.actime = newTime;
			break;

		case FILETIME_MODIFY:
		case FILETIME_CHANGE:
			newFTime.modtime = newTime;
			break;

		default:
			return 0;
		}
		utime(filePath, &newFTime);
	}
#endif // SHELL_PLATFORM_WINDOWS
	return 0;
}


void Init_Base (HSQUIRRELVM sqvm)
{
	RegInt(sqvm,"CREATION", FILETIME_CREATE, SQTrue);
	RegInt(sqvm,"ACCESS", FILETIME_ACCESS, SQTrue);
	RegInt(sqvm,"MODIFICATION", FILETIME_MODIFY, SQTrue);
	RegInt(sqvm,"CHANGE", FILETIME_CHANGE, SQTrue);
	RegStr(sqvm,"OUTPUT", "");
	RegStr(sqvm,"ERROR", "");
	RegCFunc(sqvm,"mkdir", MkDir, -2, 3, ". s i|s");
	RegCFunc(sqvm,"getcwd", GetCWD, 1, 0, ".");
	RegCFunc(sqvm,"chdir", ChDir, 2, 0, ". s");
	RegCFunc(sqvm,"exist", Exist, 2, 0, ". s");
	RegCFunc(sqvm,"copy", Copy, -3, 4, ". s s b");
	RegCFunc(sqvm,"move", Move, 3, 0, ". s s");
	RegCFunc(sqvm,"remove", Remove, 2, 0, ". s");
	RegCFunc(sqvm,"rmdir", RmDir, -2, 3, ". s b");
	RegCFunc(sqvm,"getenv", GetEnv, 2, 0, ". s");
	RegCFunc(sqvm,"setenv", SetEnv, 3, 0, ". s s");
	RegCFunc(sqvm,"delenv", DelEnv, 2, 0, ". s");
	RegCFunc(sqvm,"run", Run, -2, 6, ". s a o|s b b");
	RegCFunc(sqvm,"exec", StartProcess, -2, 6, ". s a o|s b b");
	RegCFunc(sqvm,"chmod", ChMod, 3, 0, ". s i|s");
	RegCFunc(sqvm,"chown", ChOwn, 3, 0, ". s s");
	RegCFunc(sqvm,"chgrp", ChGrp, 3, 0, ". s s");
	RegCFunc(sqvm,"readdir", ReadDir, 2, 0, ". s");
	RegCFunc(sqvm,"exit", Exit, -1, 2, ". i");
	RegCFunc(sqvm,"filetime", FileTime, -2, 3, ". s i");
	RegCFunc(sqvm,"systime", SysTime, 1, 0, ".");
	RegCFunc(sqvm,"localtime", LocalTime, 1, 0, ".");
	RegCFunc(sqvm,"setfiletime", SetFileTime2, 4, 0, ". s i i");
	RegCFunc(sqvm,"getdir", GetDirs, 2, 0, ". s");
	//_RPT0(_CRT_WARN, "------ Base library initialized\n");
}

void Shutdown_Base ()
{
	//_RPT0(_CRT_WARN, "------ Base library deinitialized\n");
}
