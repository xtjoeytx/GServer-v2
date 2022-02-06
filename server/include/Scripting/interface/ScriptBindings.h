#pragma once

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
#define SCRIPTENV_D(...) do {} while(0)
#endif

#include "ScriptArguments.h"
#include "ScriptEnv.h"
#include "ScriptFunction.h"
#include "ScriptObject.h"
#include "ScriptUtils.h"

#endif
