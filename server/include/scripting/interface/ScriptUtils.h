#ifndef SCRIPTUTILS_H
#define SCRIPTUTILS_H

#include <chrono>
#include <string>

struct ScriptTimeSample
{
	double sample;
	std::chrono::high_resolution_clock::time_point sample_time;
};

class ScriptRunError
{
public:
	std::string getErrorString() const
	{
		std::string error_message; // ("Error compiling: ");
		error_message.append(error);
		error_message.append(" at ");
		error_message.append(error_line);
		return error_message;
	}

	std::string filename;
	std::string error;
	std::string error_line;
	int startcol = 0;
	int endcol = 0;
	int lineno = 0;
};

#endif
