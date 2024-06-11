#ifndef GS2EMU_COMMANDDISPATCHER_H
#define GS2EMU_COMMANDDISPATCHER_H

#include <functional>
#include <string>
#include <unordered_map>

template<typename key_type, typename... Ts>
class CommandDispatcher
{
	using fn_type = std::function<bool(Ts...)>;
	using cmd_map_type = std::unordered_map<key_type, fn_type>;

public:
	class Builder
	{
		cmd_map_type& m_builderCommands;

	public:
		Builder(cmd_map_type& cmds) : m_builderCommands(cmds) {}

		void registerCommand(key_type key, fn_type fn)
		{
			m_builderCommands[key] = fn;
		}
	};

	CommandDispatcher() {}

	CommandDispatcher(std::function<void(Builder)> initfn)
	{
		initfn(Builder(m_commands));
	}

	bool execute(key_type key, Ts... args)
	{
		auto cmd_iter = m_commands.find(key);
		if (cmd_iter == m_commands.end())
		{
			return false;
		}

		return cmd_iter->second(args...);
	}

private:
	cmd_map_type m_commands;
};

#endif
