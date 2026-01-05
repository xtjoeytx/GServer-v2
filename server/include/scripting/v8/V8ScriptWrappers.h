#ifndef V8SCRIPTWRAPPERS_H
#define V8SCRIPTWRAPPERS_H

#include <cstring>
#include <string>

enum V8ScriptWrapperFlags {
	EMIT_NONE = 0,
	EMIT_SCOPE = 1,
	EMIT_CLASS = 2
};

std::string prepareScript(const std::string& code, uint32_t scriptFlags = EMIT_NONE);

template<typename T>
inline std::string wrapScript(const std::string& code)
{
	return code;
}

template<typename T>
inline std::string wrapScript(const std::string_view& code)
{
	return code.data();
}

class NPC;
template<>
inline std::string wrapScript<NPC>(const std::string_view& code)
{
	// self.onCreated || onCreated, for first declared to take precedence
	// if (onCreated) for latest function to override
	static const char* prefixString = "(function(npc) {" \
									  "const self = npc;\n";

	std::string wrappedCode = std::string(prefixString);
	wrappedCode.append(prepareScript(std::string{ code }, EMIT_SCOPE));
	wrappedCode.append("\n});");
	return wrappedCode;
}

class TScriptClass;
template <>
inline std::string WrapScript<TScriptClass>(const std::string_view& code) {
	static const char* prefixString = "(function(npc) {" \
		"const self = npc;\n";

	std::string wrappedCode = std::string(prefixString);
	wrappedCode.append(prepareScript(std::string{ code }, EMIT_SCOPE | EMIT_CLASS));
	wrappedCode.append("\n});");
	return wrappedCode;
}


class Player;
template<>
inline std::string wrapScript<Player>(const std::string_view& code)
{
	static const char* prefixString = "(function(player) {" \
		"const self = player;\n";

	std::string wrappedCode = std::string(prefixString);
	wrappedCode.append(prepareScript(std::string{ code }, EMIT_CLASS));
	wrappedCode.append("\n});");
	return wrappedCode;
}

class Weapon;
template<>
inline std::string wrapScript<Weapon>(const std::string_view& code)
{
	static const char* prefixString = "(function(weapon) {" \
		"const self = weapon;\n";

	std::string wrappedCode = std::string(prefixString);
	wrappedCode.append(prepareScript(std::string{ code }, EMIT_SCOPE));
	wrappedCode.append("\n});");
	return wrappedCode;
}

#endif
