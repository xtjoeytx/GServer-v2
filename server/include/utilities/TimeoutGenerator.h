#ifndef TIMEOUTGENERATOR_H
#define TIMEOUTGENERATOR_H

#include <chrono>
#include <functional>

using namespace std::chrono_literals;

///////////////////////////////////////////////////////////////////////////////
namespace preagonal
{
///////////////////////////////////////////////////////////////////////////////

/// @brief A utility struct for generating periodic timeouts and invoking a callback after a specified interval.
struct TimeoutGenerator
{
	using time_point = std::chrono::high_resolution_clock::time_point;
	using time_delta = std::chrono::milliseconds;

	TimeoutGenerator() = default;

	template<typename Rep, typename Period>
	TimeoutGenerator(std::chrono::duration<Rep, Period> timeout, bool repeated = false)
		: timeout(std::chrono::duration_cast<time_delta>(timeout)), repeated(repeated) {}

public:
	time_delta timeout = 5ms;
	bool repeated = true;
	std::function<void(int)> callbackIterations = nullptr;
	std::function<void(time_delta)> callbackDuration = nullptr;

public:
	int update(time_point now = std::chrono::high_resolution_clock::now())
	{
		if (!m_running)
			return 0;

		auto duration = now - m_lastTimeout;
		int iterations = duration / timeout;
		if (iterations > 0)
		{
			m_lastTimeout = now;
			if (!repeated)
			{
				m_running = false;
				iterations = 1;
			}

			if (callbackIterations)
				callbackIterations(iterations);
			if (callbackDuration)
				callbackDuration(std::chrono::duration_cast<time_delta>(duration));
		}

		return iterations;
	}

	void setLastTimeout(time_point lastTimeout = std::chrono::high_resolution_clock::now())
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
	time_point m_lastTimeout = std::chrono::high_resolution_clock::now();
};

///////////////////////////////////////////////////////////////////////////////
} // end namespace preagonal

#endif // TIMEOUTGENERATOR_H
