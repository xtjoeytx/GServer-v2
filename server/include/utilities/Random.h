#ifndef RANDOM_H
#define RANDOM_H

#include <cmath>
#include <cstdint>

///////////////////////////////////////////////////////////////////////////////
namespace preagonal
{
///////////////////////////////////////////////////////////////////////////////

class DelphiRandomDeviceIntegral
{
public:
	using result_type = uint32_t;

public:
	DelphiRandomDeviceIntegral() : m_seed(1) {}
	explicit DelphiRandomDeviceIntegral(const result_type seed) : m_seed(seed) {}

	static constexpr result_type min()
	{
		return m_min;
	}

	static constexpr result_type max()
	{
		return m_max;
	}

public:
	result_type operator()()
	{
		m_seed = (m_seed * 0x8088405 + 1) & 0xFFFFFFFF;
		return m_min + (m_seed % (m_max - m_min));
	}

private:
	static constexpr result_type m_min = 0;
	static constexpr result_type m_max = 0xFFFFFFFF;
	result_type m_seed;
};

class DelphiRandomDeviceReal
{
public:
	using result_type = long double;

public:
	DelphiRandomDeviceReal() : m_seed(1) {}
	explicit DelphiRandomDeviceReal(const result_type seed) : m_seed(static_cast<uint32_t>(seed)) {}

	static constexpr result_type min()
	{
		return m_min;
	}

	static constexpr result_type max()
	{
		return m_max;
	}

public:
	result_type operator()()
	{
		m_seed = (m_seed * 0x8088405 + 1) & 0xFFFFFFFF;
		return std::ldexp(static_cast<long double>(m_seed), -32);
	}

private:
	static constexpr result_type m_min = 0.0;
	static constexpr result_type m_max = 1.0;
	uint32_t m_seed;
};

///////////////////////////////////////////////////////////////////////////////
} // end namespace preagonal

#endif // RANDOM_H
