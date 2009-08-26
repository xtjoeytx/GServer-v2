#include "IDebug.h"
#include <stdio.h>
#include <string.h>
#include <stdarg.h>

#ifndef _WIN32
#include <unistd.h>
#endif

#include <boost/thread.hpp>
#include "CLog.h"
#include "CString.h"

static CString getBasePath();

CLog::CLog()
: enabled(false), file(0)
{
	// Get base path.
	homepath = getBasePath();
}

CLog::CLog(const CString& _file, bool _enabled)
: enabled(_enabled), filename(_file), file(0)
{
	// Get base path.
	homepath = getBasePath();

	// Open the file now.
	if (enabled)
		file = fopen((homepath + filename).text(), "a");
	if (0 == file)
		enabled = false;
}

CLog::~CLog()
{
	boost::recursive_mutex::scoped_lock lock(m_write);
	if (file)
	{
		fflush(file);
		fclose(file);
	}
}

void CLog::out(const CString format, ...)
{
	va_list s_format_v;
	va_start(s_format_v, format);

	boost::recursive_mutex::scoped_lock lock(m_write);

	// Log output to file.
	if (true == enabled && 0 != file)
	{
		vfprintf(file, format.text(), s_format_v);
		fflush(file);
	}

	// Display message.
	vprintf(format.text(), s_format_v);

	va_end(s_format_v);
}

void CLog::clear()
{
	boost::recursive_mutex::scoped_lock lock(m_write);
	if (file) fclose(file);

	file = fopen((homepath + filename).text(), "w");
	if (0 == file)
		enabled = false;
}

CString getBasePath()
{
	CString homepath;

#if defined(_WIN32) || defined(_WIN64)
	// Get the path.
	char path[MAX_PATH];
	GetModuleFileNameA(0, path, MAX_PATH);

	// Find the program exe and remove it from the path.
	// Assign the path to homepath.
	homepath = path;
	int pos = homepath.findl('\\');
	if (pos == -1) homepath.clear();
	else if (pos != (homepath.length() - 1))
		homepath.removeI(++pos, homepath.length());
#else
	// Get the path to the program.
	char path[ 260 ];
	memset((void*)path, 0, 260);
	readlink("/proc/self/exe", path, sizeof(path));

	// Assign the path to homepath.
	char* end = strrchr(path, '/');
	if (end != 0)
	{
		end++;
		if (end != 0) *end = '\0';
		homepath = path;
	}
#endif

	return homepath;
}
