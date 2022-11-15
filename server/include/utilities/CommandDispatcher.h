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
			cmd_map_type& _commands;

			public:
				Builder(cmd_map_type& cmds) : _commands(cmds) {}

				void registerCommand(key_type key, fn_type fn) {
					_commands[key] = fn;
				}
		};

		CommandDispatcher() {}
		CommandDispatcher(std::function<void(Builder)> initfn) {
			initfn(Builder(commands));
		}

		bool execute(key_type key, Ts... args) {
			auto cmd_iter = commands.find(key);
			if (cmd_iter == commands.end()) {
				return false;
			}

			return cmd_iter->second(args...);
		}

	private:
		cmd_map_type commands;
};

#endif
