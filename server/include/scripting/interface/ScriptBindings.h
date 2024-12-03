#ifndef SCRIPTBINDINGS_H
#define SCRIPTBINDINGS_H

#ifdef NDEBUG
	#ifdef ENABLE_SCRIPTENV_DEBUG
		#define _SCRIPTENV_DEBUG
	#endif
#endif

#ifdef _SCRIPTENV_DEBUG
	#define SCRIPTENV_D(...) printf(__VA_ARGS__)
#else
	#define SCRIPTENV_D(...) \
		do {                 \
		}                    \
		while (0)
#endif

#include "scripting/interface/ScriptArguments.h"
#include "scripting/interface/ScriptEnv.h"
#include "scripting/interface/ScriptFunction.h"
#include "scripting/interface/ScriptObject.h"
#include "scripting/interface/ScriptUtils.h"

#endif
