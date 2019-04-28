#pragma once

#include <string>

class ScriptRunError
{
public:
	ScriptRunError() = default;
	~ScriptRunError() = default;
	
	// TODO(joey): Remove this
	void DebugPrint()
	{
		printf("Error compiling: %s\n", error.c_str());
		printf("\tLine: %s\n", error_line.c_str());
		printf("\tFilename: %s\n", filename.c_str());
		printf("\tLine Number: %d\n", lineno);
		printf("\tSpecifically: %s\n", error_line.substr(startcol, endcol - startcol).c_str());
	}
	
	std::string filename;
	std::string error;
	std::string error_line;
	int startcol, endcol;
	int lineno;
};
