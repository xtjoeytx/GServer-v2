#ifndef TIMEOUTGENERATOR_H
#define TIMEOUTGENERATOR_H

#include <chrono>
#include <functional>

#include <utilities/CommonTypes.h>

using namespace std::chrono_literals;

///////////////////////////////////////////////////////////////////////////////
namespace preagonal
{
///////////////////////////////////////////////////////////////////////////////

/// @brief A utility struct for generating periodic timeouts and invoking a callback after a specified interval.
struct TimeoutGenerator
{
	using time_point = precise_clock::time_point;
	using time_delta = std::chrono::milliseconds;

	TimeoutGenerator() = default;

	template<typename Rep, typename Period>
	explicit TimeoutGenerator(std::chrono::duration<Rep, Period> timeout, const bool repeated = false)
		: timeout(std::chrono::duration_cast<time_delta>(timeout)), repeated(repeated) {}

public:
	time_delta timeout = 5ms;
	bool repeated = true;

	/// @brief A callback function to be invoked with the current iteration index.  DO NOT USE IF STORED IN A VECTOR THAT MAY REALLOCATE!
	std::function<void(int)> callbackIterations = nullptr;

	/// @brief A callback function that takes a time duration as its parameter.  DO NOT USE IF STORED IN A VECTOR THAT MAY REALLOCATE!
	std::function<void(time_delta)> callbackDuration = nullptr;

public:
	int update(const time_point now = precise_clock::now())
	{
		if (!m_running)
			return 0;

		const auto duration = now - m_lastTimeout;
		int iterations = static_cast<int>(duration / timeout);
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

	void setLastTimeout(const time_point lastTimeout = precise_clock::now())
	{
		m_lastTimeout = lastTimeout;
	}

	void start()
	{
		m_running = true;
		m_lastTimeout = precise_clock::now();
	}

	template<typename Duration>
	void startFor(Duration timeoutDuration)
	{
		timeout = std::chrono::duration_cast<time_delta>(timeoutDuration);
		start();
	}

	template<typename Duration>
	void runOnceFor(Duration timeoutDuration)
	{
		repeated = false;
		startFor(timeoutDuration);
	}

	void resume()
	{
		m_running = true;
	}

	void stop()
	{
		m_running = false;
	}

	[[nodiscard]] bool isRunning() const
	{
		return m_running;
	}

	[[nodiscard]] clock::duration getRemainingTime(const time_point now = precise_clock::now()) const
	{
		if (!m_running) return clock::duration::zero();
		const auto elapsed = now - m_lastTimeout;
		const auto remaining = timeout - std::chrono::duration_cast<time_delta>(elapsed);
		return std::chrono::duration_cast<clock::duration>(remaining > clock::duration::zero() ? remaining : clock::duration::zero());
	}

	[[nodiscard]] size_t getRemainingTimeIn50msIncrements(const time_point now = precise_clock::now()) const
	{
		if (!m_running) return 0;
		const auto elapsed = now - m_lastTimeout;
		const auto remaining = timeout - std::chrono::duration_cast<time_delta>(elapsed);
		if (remaining > clock::duration::zero())
			return std::chrono::duration_cast<clock::duration>(remaining).count() / 50;
		return 0;
	}

protected:
	bool m_running = false;
	time_point m_lastTimeout = precise_clock::now();
};

///////////////////////////////////////////////////////////////////////////////
} // end namespace preagonal

#endif // TIMEOUTGENERATOR_H
