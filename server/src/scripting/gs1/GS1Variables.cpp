#include <chrono>

#include <BabyDI.h>
#include <Server.h>
#include <scripting/gs1/GS1Variables.h>
#include <scripting/ScriptContainers.h>

///////////////////////////////////////////////////////////////////////////////
namespace preagonal::gs1
{
///////////////////////////////////////////////////////////////////////////////

void setReadOnlyGlobalVariables(GameVariableStore& variableStore)
{
	auto* server = BabyDI::Get<Server>();

	variableStore.add(GameVariable{ "timevar", [&server](auto) -> GameValue { return static_cast<double>(server->getNWTime()); }, {} });
	variableStore.add(GameVariable{ "timevar2", [&server](auto) -> GameValue { return static_cast<double>(chrono::high_resolution_clock::now().time_since_epoch().count()); }, {} });
}

///////////////////////////////////////////////////////////////////////////////
} // end namespace preagonal::gs1
