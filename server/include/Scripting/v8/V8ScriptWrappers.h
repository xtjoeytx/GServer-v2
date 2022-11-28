#pragma once

#ifndef V8SCRIPTWRAPPERS_H
#define V8SCRIPTWRAPPERS_H

#include <cstring>
#include <string>
#include <regex>
#include <iostream>

const std::regex word_regex(R"((public[\s]+function[\s]+){1}(\w+)[\s]*\()");

inline std::string getPublicFunctions(const std::string_view& code)
{
	std::vector<std::string> eventList;

	std::string s(code);

	auto words_begin = std::sregex_iterator(s.begin(), s.end(), word_regex);
	auto words_end = std::sregex_iterator();

	for (std::sregex_iterator i = words_begin; i != words_end; ++i) {
		std::smatch match = *i;

		std::string match_str = match.str();
		if (match.size() == 3)
			eventList.push_back(match[2]);
	}

	std::string varNames = "";
	std::string varNameQuotes = "";
	for (const auto eventName : eventList) {
		varNames += eventName + ",";
		varNameQuotes += "\"" + eventName + "\"" + ",";
	}
	if (!varNames.empty()) {
		varNames.pop_back();
		varNameQuotes.pop_back();
	}

	std::string out;
	if (!varNames.empty())
	{
		out += "var " + varNames + ";\n";
		out += "const __publicNames = [" + varNameQuotes + "];\n";
		out += "const __publicFuncs = [" + varNames + "];\n";
		out += R"(
			for (let i = 0; i < __publicNames.length; ++i) {
				if (__publicFuncs[i]) {
					//print("Found fn ", __publicNames[i]);
					self[__publicNames[i]] = __publicFuncs[i];
				} else {
					//print("Not found fn ", __publicNames[i]);
				}
			}
		)";
	}

	return out;
}

template <typename T>
inline std::string WrapScript(const std::string& code) {
	return code;
}

template <typename T>
inline std::string WrapScript(const std::string_view& code) {
	return code.data();
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

	std::string publicFunctions = getPublicFunctions(code);
	std::string fixedCode = std::regex_replace(std::string(code), word_regex, "function $2(");

	wrappedCode.append(fixedCode);
	wrappedCode.append("\n});");
	return wrappedCode;
}

class TPlayer;
template <>
inline std::string WrapScript<TPlayer>(const std::string_view& code) {
	static const char* prefixString = "(function(player) {" \
		"const self = player;\n";

	std::string wrappedCode = std::string(prefixString);

	std::string publicFunctions = getPublicFunctions(code);
	std::string fixedCode = std::regex_replace(std::string(code), word_regex, "function $2(");

	wrappedCode.append(fixedCode);
	wrappedCode.append(publicFunctions);
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

	std::string publicFunctions = getPublicFunctions(code);
	std::string fixedCode = std::regex_replace(std::string(code), word_regex, "function $2(");

	wrappedCode.append(fixedCode);
	wrappedCode.append("\n});");
	return wrappedCode;
}

#endif
