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
	inline void InvalidateBinding(T val)
	{
	}

	template<typename T>
	inline void InvalidateBinding(IScriptObject<T>* val)
	{
		// Decrease reference for wrapped objects
		val->decreaseReference();
	}

	template<typename T>
	inline void ValidateBinding(T val)
	{
	}

	template<typename T>
	inline void ValidateBinding(IScriptObject<T>* val)
	{
		// Increase reference for wrapped objects
		val->increaseReference();
	}
}; // namespace detail

class IScriptArguments
{
public:
	IScriptArguments()          = default;
	virtual ~IScriptArguments() = default;

	virtual bool Invoke(IScriptFunction* func, bool catchExceptions = false) = 0;
};

template<typename... Ts>
class ScriptArguments : public IScriptArguments
{
public:
	template<typename... Args>
	ScriptArguments(Args&&... An)
		: IScriptArguments(), _resolved(false), _tuple(std::forward<Args>(An)...)
	{
		validate_args(std::index_sequence_for<Ts...>{});
	}

	virtual ~ScriptArguments()
	{
		if (!_resolved)
		{
			invalidate_args(std::index_sequence_for<Ts...>{});
		}
	}

	virtual bool Invoke(IScriptFunction* func, bool catchExceptions = false) = 0;

	inline size_t Count() const
	{
		return Argc;
	}

	inline const std::tuple<Ts...>& Args() const
	{
		return _tuple;
	}

protected:
	static constexpr int Argc = (sizeof...(Ts));

	bool _resolved;
	std::tuple<Ts...> _tuple;

private:
	template<std::size_t... Is>
	inline void invalidate_args(std::index_sequence<Is...>)
	{
		if constexpr (sizeof...(Is) > 0)
		{
			int unused[] = {((detail::InvalidateBinding(std::get<Is>(_tuple))), void(), 0)...};
			static_cast<void>(unused); // Avoid warning for unused variable
		}
	}

	template<std::size_t... Is>
	inline void validate_args(std::index_sequence<Is...>)
	{
		if constexpr (sizeof...(Is) > 0)
		{
			int unused[] = {((detail::ValidateBinding(std::get<Is>(_tuple))), void(), 0)...};
			static_cast<void>(unused); // Avoid warning for unused variable
		}
	}
};

#endif
