#ifndef ISCRIPTEDOBJECT_H
#define ISCRIPTEDOBJECT_H

#include <cstdint>
#include <string>
#include <string_view>
#include <vector>

///////////////////////////////////////////////////////////////////////////////

namespace preagonal
{

///////////////////////////////////////////////////////////////////////////////

class IScriptedObject
{
public:
	virtual ~IScriptedObject() = 0;

public:
	//std::vector<uint8_t> getCompiledScript() const;

private:
};

} // end namespace preagonal

#endif // ISCRIPTEDOBJECT_H
