#ifndef CLOG_H
#define CLOG_H

#include <stdarg.h>
#include <stdio.h>
#include "CString.h"

#ifndef NO_BOOST
namespace boost
{
	class recursive_mutex;
}
#endif

//! Logger class for logging information to a file.
class CLog
{
	public:
		//! Creates a new CLog that outputs to the specific file.
		//! \param file The file to log to.
		//! \param enabled If the class logs by default to the file or not.
		CLog();
		CLog(const CString& _file, bool _enabled = true);

		//! Cleans up and closes files.
		virtual ~CLog();

		//! Outputs a text string to a file.
		/*!
			Outputs text to a file.  Use like a standard printf() command.
			\param format Format string.
		*/
		void out(const CString format, ...);
		void append(const CString format, ...);

		//! Clears the output file.
		void clear();

		//! Closes the file.
		void close();

		//! Opens the file.
		void open();

		//! Gets the enabled state of the class.
		//! \return True if logging is enabled.
		bool getEnabled() const;

		//! Sets the enabled state of the logger class.
		/*!
			Sets the enabled state of the logger class.
			If /a enabled is set to false, it will no longer log to a file.
			\param enabled If true, log to file.  If false, don't log to file.
		*/
		void setEnabled(bool enabled);

		//! Gets the name of the log file.
		//! \return Name of the log file.
		const CString& getFilename() const;

		//! Sets the name of the file to write to.
		//! \param filename Name of the file to write to.
		void setFilename(const CString& filename);

	private:
		//! If the class is enabled or not.
		bool enabled;

		//! Filename to write to.
		CString filename;

		//! Application home path.
		CString homepath;

		//! File handle.
		FILE* file;

#ifndef NO_BOOST
		//! Mutex
		boost::recursive_mutex* m_write;
#endif
};

inline
bool CLog::getEnabled() const
{
	return enabled;
}

inline
const CString& CLog::getFilename() const
{
	return filename;
}

inline
void CLog::setEnabled(bool enabled)
{
	this->enabled = enabled;
}

#endif
