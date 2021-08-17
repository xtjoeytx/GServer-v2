#pragma once

#ifndef V8SCRIPTWRAPPERS_H
#define V8SCRIPTWRAPPERS_H

#include <cstring>
#include <string>

template <typename T>
inline std::string WrapScript(const std::string& code) {
	return code;
}

template <typename T>
inline std::string WrapScript(const std::string_view& code) {
	return code;
}

class TNPC;
template <>
inline std::string WrapScript<TNPC>(const std::string_view& code) {
	// self.onCreated || onCreated, for first declared to take precedence
	// if (onCreated) for latest function to override
	static const char* prefixString = "(function(npc) {" \
		"var onCreated, onTimeout, onNpcWarped, onPlayerChats, onPlayerEnters, onPlayerLeaves, onPlayerTouchsMe, onPlayerLogin, onPlayerLogout;" \
		"const self = npc;" \
		"if (onCreated) self.onCreated = onCreated;" \
		"if (onTimeout) self.onTimeout = onTimeout;" \
		"if (onNpcWarped) self.onNpcWarped = onNpcWarped;" \
		"if (onPlayerChats) self.onPlayerChats = onPlayerChats;" \
		"if (onPlayerEnters) self.onPlayerEnters = onPlayerEnters;" \
		"if (onPlayerLeaves) self.onPlayerLeaves = onPlayerLeaves;" \
		"if (onPlayerTouchsMe) self.onPlayerTouchsMe = onPlayerTouchsMe;" \
		"if (onPlayerLogin) self.onPlayerLogin = onPlayerLogin;" \
		"if (onPlayerLogout) self.onPlayerLogout = onPlayerLogout;" \
		"\n";

	std::string wrappedCode = std::string(prefixString);
	wrappedCode.append(code);
	wrappedCode.append("\n});");
	return wrappedCode;
}

class TPlayer;
template <>
inline std::string WrapScript<TPlayer>(const std::string& code) {
	static const char* prefixString = "(function(player) {" \
		"const self = player;\n";

	std::string wrappedCode = std::string(prefixString);
	wrappedCode.append(code);
	wrappedCode.append("\n});");
	return wrappedCode;
}

class TWeapon;
template <>
inline std::string WrapScript<TWeapon>(const std::string_view& code) {
	static const char* prefixString = "(function(weapon) {" \
		"var onCreated, onActionServerSide;" \
		"const self = weapon;" \
		"self.onCreated = onCreated;" \
		"self.onActionServerSide = onActionServerSide;\n";

	std::string wrappedCode = std::string(prefixString);
	wrappedCode.append(code);
	wrappedCode.append("\n});");
	return wrappedCode;
}

#endif
