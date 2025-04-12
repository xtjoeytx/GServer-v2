#include <cassert>
#include "utilities/Log.h"

///////////////////////////////////////////////////////////////////////////////

namespace preagonal::log
{

///////////////////////////////////////////////////////////////////////////////

Indent::Indent(Log* log, uint8_t level)
	: m_log(log)
{
	assert(m_log != nullptr);
	m_old_level = m_log->indentLevel;
	m_log->indentLevel += level;
}

Indent::Indent(IndentAbsolute_t is_absolute, Log* log, uint8_t level)
	: m_log(log)
{
	assert(m_log != nullptr);
	m_old_level = m_log->indentLevel;
	m_log->indentLevel = level;
}

Indent::Indent(Indent&& other) noexcept
	: m_log(std::move(other.m_log)), m_old_level(std::move(other.m_old_level))
{
	other.m_log = nullptr;
	other.m_old_level = 0;
}

Indent::~Indent() noexcept
{
	if (m_log != nullptr)
		m_log->indentLevel = m_old_level;
}

///////////////////////////////////////////////////////////////////////////////

Log& Log::reload()
{
	std::lock_guard lock(mutex);
	if (file && file->is_open())
	{
		file->flush();
		file->close();
	}

	file = std::make_unique<std::ofstream>();
	file->open(filename, std::ios::binary | std::ios::out | std::ios::app);
	return *this;
}

Log& Log::close()
{
	std::lock_guard lock(mutex);
	if (file && file->is_open())
	{
		file->flush();
		file->close();
		file.reset();
	}
	return *this;
}

Log& Log::clear()
{
	std::lock_guard lock(mutex);
	if (file && file->is_open())
	{
		file->close();
	}

	file = std::make_unique<std::ofstream>();
	file->open(filename, std::ios::binary | std::ios::out | std::ios::trunc);
	return *this;
}

std::ofstream* Log::getFile()
{
	if (!file || !file->is_open())
		reload();
	return file.get();
}

///////////////////////////////////////////////////////////////////////////////

} // end namespace preagonal::log
