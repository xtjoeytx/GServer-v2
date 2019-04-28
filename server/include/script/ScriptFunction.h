#pragma once

class IScriptArguments;

class IScriptFunction
{
public:
	IScriptFunction() = default;
	
	virtual ~IScriptFunction() = 0;
};

inline IScriptFunction::~IScriptFunction() = default;

