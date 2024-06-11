#ifndef SCRIPTARGUMENTS_H
#define SCRIPTARGUMENTS_H

#include "ScriptObject.h"
#include <memory>
#include <string>
#include <tuple>

class IScriptFunction;

namespace detail
{
	template<typename T>
	inline void invalidateBinding(T val)
	{
	}

	template<typename T>
	inline void invalidateBinding(IScriptObject<T>* val)
	{
		// Decrease reference for wrapped objects
		val->decreaseReference();
	}

	template<typename T>
	inline void validateBinding(T val)
	{
	}

	template<typename T>
	inline void validateBinding(IScriptObject<T>* val)
	{
		// Increase reference for wrapped objects
		val->increaseReference();
	}
}; // namespace detail

class IScriptArguments
{
public:
	IScriptArguments() = default;

	virtual ~IScriptArguments() = default;

	virtual bool invoke(IScriptFunction* func, bool catchExceptions = false) = 0;
};

template<typename... Ts>
class ScriptArguments : public IScriptArguments
{
public:
	template<typename... Args>
	ScriptArguments(Args&&... An)
		: IScriptArguments(), m_resolved(false), m_tuple(std::forward<Args>(An)...)
	{
		validateArgs(std::index_sequence_for<Ts...>{});
	}

	virtual ~ScriptArguments()
	{
		if (!m_resolved)
		{
			invalidateArgs(std::index_sequence_for<Ts...>{});
		}
	}

	virtual bool invoke(IScriptFunction* func, bool catchExceptions = false) = 0;

	inline size_t count() const
	{
		return m_argc;
	}

	inline const std::tuple<Ts...>& args() const
	{
		return m_tuple;
	}

protected:
	static constexpr int m_argc = (sizeof...(Ts));

	bool m_resolved;
	std::tuple<Ts...> m_tuple;

private:
	template<std::size_t... Is>
	inline void invalidateArgs(std::index_sequence<Is...>)
	{
		if constexpr (sizeof...(Is) > 0)
		{
			int unused[] = { ((detail::invalidateBinding(std::get<Is>(m_tuple))), void(), 0)... };
			static_cast<void>(unused); // Avoid warning for unused variable
		}
	}

	template<std::size_t... Is>
	inline void validateArgs(std::index_sequence<Is...>)
	{
		if constexpr (sizeof...(Is) > 0)
		{
			int unused[] = { ((detail::validateBinding(std::get<Is>(m_tuple))), void(), 0)... };
			static_cast<void>(unused); // Avoid warning for unused variable
		}
	}
};

#endif
