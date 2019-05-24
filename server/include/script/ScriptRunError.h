#pragma once

#include <string>

class ScriptRunError
{
public:
	ScriptRunError() : lineno(0), startcol(0), endcol(0) {
	}

	~ScriptRunError() = default;
	
	std::string getErrorString() const {
		std::string error_message; // ("Error compiling: ");
		error_message.append(error);
		error_message.append(" at ");
		error_message.append(error_line);
		return error_message;
	}
	
	std::string filename;
	std::string error;
	std::string error_line;
	int startcol, endcol;
	int lineno;
};
