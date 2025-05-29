#ifndef TIMEOUTGENERATOR_H
#define TIMEOUTGENERATOR_H

#include <chrono>
#include <functional>

using namespace std::chrono_literals;

///////////////////////////////////////////////////////////////////////////////
namespace preagonal
{
///////////////////////////////////////////////////////////////////////////////

struct TimeoutGenerator
{
	std::chrono::milliseconds timeout = 5ms;
	bool repeated = true;
	std::function<void(int)> callback = nullptr;

public:
	int update()
	{
		if (!m_running)
			return 0;

		auto now = std::chrono::high_resolution_clock::now();
		int iterations = (now - m_lastTimeout) / timeout;
		if (iterations > 0)
		{
			m_lastTimeout = now;
			if (!repeated)
				m_running = false;
			if (callback)
				callback(iterations);
		}

		return iterations;
	}

	void setLastTimeout(std::chrono::steady_clock::time_point lastTimeout = std::chrono::high_resolution_clock::now())
	{
		m_lastTimeout = lastTimeout;
	}

	void start()
	{
		m_running = true;
		m_lastTimeout = std::chrono::high_resolution_clock::now();
	}

	void resume()
	{
		m_running = true;
	}

	void stop()
	{
		m_running = false;
	}

protected:
	bool m_running = false;
	std::chrono::high_resolution_clock::time_point m_lastTimeout = std::chrono::high_resolution_clock::now();
};

///////////////////////////////////////////////////////////////////////////////
} // end namespace preagonal

#endif // TIMEOUTGENERATOR_H
